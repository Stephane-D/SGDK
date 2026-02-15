use crate::assembler::{Assembler, ErrorKind, Ending, OutputMode};
use std::fs::File;
use std::io::{Read, Seek, SeekFrom, Write};
use std::path::{Path, PathBuf};

/// Output buffer size
const DEST_BUF_LEN: usize = 8192;

impl Assembler {
    // ============================================================
    // Emit functions
    // ============================================================

    fn emit_raw(&mut self, byte: u8) {
        self.eb.push(byte as i32);
        if self.pass == 2 {
            self.dest_buf.push(byte);
            if self.dest_buf.len() >= DEST_BUF_LEN {
                self.write_dest();
            }
        }
        self.adres += 1;
    }

    pub fn emit_byte(&mut self, byte: i32) {
        self.eadres = self.adres;
        self.emit_raw(byte as u8);
    }

    pub fn emit_bytes_array(&mut self, bytes: &[i32]) {
        if bytes.is_empty() { return; }
        if bytes[0] == -1 {
            let rem = self.remaining().to_string();
            self.error("Illegal instruction", Some(&rem), ErrorKind::CatchAll);
            self.lp = self.line.len();
            return;
        }
        self.eadres = self.adres;
        for &b in bytes {
            if b == -1 { break; }
            self.emit_raw(b as u8);
        }
    }

    pub fn emit_words(&mut self, words: &[i32]) {
        self.eadres = self.adres;
        for &w in words {
            if w == -1 { break; }
            self.emit_raw((w % 256) as u8);
            self.emit_raw((w / 256) as u8);
        }
    }

    pub fn emit_block(&mut self, byte: i64, len: i64) {
        self.eadres = self.adres;
        let b = byte as u8;
        if len > 0 {
            self.eb.push(b as i32);
        }
        for _ in 0..len {
            if self.pass == 2 {
                self.dest_buf.push(b);
                if self.dest_buf.len() >= DEST_BUF_LEN {
                    self.write_dest();
                }
            }
            self.adres += 1;
        }
    }

    // ============================================================
    // Dest output management
    // ============================================================

    fn write_dest(&mut self) {
        if let Some(ref mut f) = self.output {
            let _ = f.write_all(&self.dest_buf);
        }
        self.dest_len += self.dest_buf.len() as i64;
        self.dest_buf.clear();
    }

    pub fn open_dest(&mut self, mode: OutputMode) {
        self.dest_len = 0;
        self.dest_buf.clear();

        let f = match mode {
            OutputMode::Truncate => File::create(&self.destfilename),
            OutputMode::Rewind | OutputMode::Append => {
                let exists = Path::new(&self.destfilename).exists();
                if !exists {
                    File::create(&self.destfilename)
                } else {
                    std::fs::OpenOptions::new()
                        .read(true)
                        .write(true)
                        .open(&self.destfilename)
                }
            }
        };

        match f {
            Ok(mut file) => {
                match mode {
                    OutputMode::Rewind => { let _ = file.seek(SeekFrom::Start(0)); }
                    OutputMode::Append => { let _ = file.seek(SeekFrom::End(0)); }
                    _ => {}
                }
                self.output = Some(file);
            }
            Err(_e) => {
                self.error(&format!("Error opening file: {}", self.destfilename), None, ErrorKind::Fatal);
            }
        }
    }

    pub fn close_dest(&mut self) {
        self.write_dest();
        if self.size != -1 {
            if self.dest_len > self.size {
                self.error("File exceeds 'size'", None, ErrorKind::Pass2);
            } else {
                let pad = self.size - self.dest_len;
                for _ in 0..pad {
                    self.dest_buf.push(0);
                    if self.dest_buf.len() >= 256 {
                        self.write_dest();
                    }
                }
                self.write_dest();
            }
        }
        self.output = None;
    }

    pub fn new_dest(&mut self, filename: &str, mode: OutputMode) {
        self.close_dest();
        self.destfilename = filename.to_string();
        self.open_dest(mode);
    }

    pub fn seek_dest(&mut self, offset: i64, from_start: bool) {
        self.write_dest();
        if let Some(ref mut f) = self.output {
            let seek = if from_start { SeekFrom::Start(offset as u64) } else { SeekFrom::Current(offset) };
            if f.seek(seek).is_err() {
                self.error("File seek error (FORG)", None, ErrorKind::Fatal);
            }
        }
    }

    // ============================================================
    // Listing
    // ============================================================

    pub fn list_file(&mut self) {
        // Simplified listing - just reset state
        self.epadres = self.adres;
        self.eadres = -1;
        self.eb.clear();
        self.listdata = false;
        self.donotlist = false;
    }

    pub fn list_file_skip(&mut self) {
        self.epadres = self.adres;
        self.eadres = -1;
        self.eb.clear();
        self.listdata = false;
        self.donotlist = false;
    }

    pub fn open_list(&mut self) {
        if self.listfile {
            match File::create(&self.listfilename) {
                Ok(f) => self.list_fp = Some(f),
                Err(_) => {
                    self.error(&format!("Error opening file: {}", self.listfilename), None, ErrorKind::Fatal);
                }
            }
        }
    }

    pub fn close(&mut self) {
        self.close_dest();
        self.exp_fp = None;
        self.list_fp = None;
    }

    // ============================================================
    // Include file
    // ============================================================

    pub fn get_path(&self, fname: &str) -> PathBuf {
        // Try relative to current file directory
        let current_dir = Path::new(&self.filename).parent().unwrap_or(Path::new("."));
        let f = fname.trim_start_matches('<');
        let candidate = current_dir.join(f);
        if candidate.exists() {
            return candidate;
        }
        // Try include directories
        for dir in &self.include_dirs {
            let candidate = Path::new(dir).join(f);
            if candidate.exists() {
                return candidate;
            }
        }
        // Return the filename as-is
        PathBuf::from(f)
    }

    pub fn bin_inc_file(&mut self, fname: &str, offset: i32, length: i32) {
        let path = self.get_path(fname);
        let fname_display = fname.trim_start_matches('<');

        let mut file = match File::open(&path) {
            Ok(f) => f,
            Err(_) => {
                self.error(&format!("Error opening file: {}", fname_display), None, ErrorKind::Fatal);
                return;
            }
        };

        if offset > 0 {
            let mut skip_buf = vec![0u8; offset as usize];
            match file.read_exact(&mut skip_buf) {
                Ok(_) => {}
                Err(_) => {
                    self.error("Offset beyond filelength", Some(fname_display), ErrorKind::Fatal);
                    return;
                }
            }
        }

        if length > 0 {
            let mut buf = vec![0u8; length as usize];
            match file.read_exact(&mut buf) {
                Ok(_) => {}
                Err(_) => {
                    self.error("Unexpected end of file", Some(fname_display), ErrorKind::Fatal);
                    return;
                }
            }
            for &b in &buf {
                if self.pass == 2 {
                    self.dest_buf.push(b);
                    if self.dest_buf.len() >= DEST_BUF_LEN {
                        self.write_dest();
                    }
                }
                self.adres += 1;
            }
        } else {
            // Read entire file
            if self.pass == 2 {
                self.write_dest();
            }
            let mut buf = [0u8; DEST_BUF_LEN];
            loop {
                match file.read(&mut buf) {
                    Ok(0) => break,
                    Ok(n) => {
                        if self.pass == 2 {
                            self.dest_buf.extend_from_slice(&buf[..n]);
                            self.write_dest();
                        }
                        self.adres += n as i64;
                    }
                    Err(_) => {
                        self.error("Read error", Some(fname_display), ErrorKind::Fatal);
                        break;
                    }
                }
            }
        }
    }

    pub fn open_file(&mut self, nfilename: &str) {
        let old_filename = self.filename.clone();
        let old_lcurlin = self.lcurlin;
        let old_input_lines = std::mem::take(&mut self.input_lines);
        let old_input_idx = self.input_idx;
        self.lcurlin = 0;
        self.input_idx = 0;

        if self.include > 20 {
            self.error("Over 20 files nested", None, ErrorKind::Fatal);
            return;
        }
        self.include += 1;

        let path = self.get_path(nfilename);
        let fname = nfilename.trim_start_matches('<');
        self.filename = fname.to_string();

        let content = match std::fs::read(&path) {
            Ok(bytes) => String::from_utf8_lossy(&bytes).into_owned(),
            Err(_) => {
                self.error(&format!("Error opening file: {}", fname), None, ErrorKind::Fatal);
                self.include -= 1;
                self.filename = old_filename;
                self.input_lines = old_input_lines;
                self.input_idx = old_input_idx;
                self.lcurlin = old_lcurlin;
                return;
            }
        };

        self.input_lines = content.lines().map(|l| l.to_string()).collect();
        self.input_idx = 0;

        while self.input_idx < self.input_lines.len() {
            if !self.running { break; }
            let line = self.input_lines[self.input_idx].clone();
            self.input_idx += 1;
            self.lcurlin += 1;
            self.curlin += 1;
            self.line = line;
            self.lp = 0;
            if self.line.len() >= crate::assembler::LINEMAX - 1 {
                self.error("Line too long", None, ErrorKind::Fatal);
            }
            self.parse_line();
        }

        self.include -= 1;
        self.filename = old_filename;
        if self.lcurlin > self.maxlin {
            self.maxlin = self.lcurlin;
        }
        self.lcurlin = old_lcurlin;
        self.input_lines = old_input_lines;
        self.input_idx = old_input_idx;
    }

    // ============================================================
    // ReadFile / SkipFile (for IF/ELSE/ENDIF)
    // ============================================================

    pub fn read_file(&mut self) -> Ending {
        loop {
            if !self.running { return Ending::End; }

            let line_result = if self.lijst {
                if self.lijst_idx >= self.lijstp.len() {
                    return Ending::End;
                }
                let line = self.lijstp[self.lijst_idx].clone();
                self.lijst_idx += 1;
                Some(line)
            } else {
                if self.input_idx >= self.input_lines.len() {
                    return Ending::End;
                }
                let line = self.input_lines[self.input_idx].clone();
                self.input_idx += 1;
                self.lcurlin += 1;
                self.curlin += 1;
                Some(line)
            };

            if let Some(line) = line_result {
                self.line = line;
                self.lp = 0;
                self.skip_blanks();
                if self.cur_char() == Some('.') { self.lp += 1; }
                let olp = self.lp;
                if self.cmphstr("endif") { return Ending::EndIf; }
                self.lp = olp;
                if self.cmphstr("else") { self.list_file(); return Ending::Else; }
                self.lp = olp;
                if self.cmphstr("endt") || self.cmphstr("dephase") { return Ending::EndTextArea; }
                self.lp = olp;
                if self.compass_compat && self.cmphstr("endc") { return Ending::EndIf; }
                self.lp = 0;
                self.parse_line();
            }
        }
    }

    pub fn skip_file(&mut self) -> Ending {
        let mut iflevel = 0;
        loop {
            if !self.running { return Ending::End; }

            let line_result = if self.lijst {
                if self.lijst_idx >= self.lijstp.len() {
                    return Ending::End;
                }
                let line = self.lijstp[self.lijst_idx].clone();
                self.lijst_idx += 1;
                Some(line)
            } else {
                if self.input_idx >= self.input_lines.len() {
                    return Ending::End;
                }
                let line = self.input_lines[self.input_idx].clone();
                self.input_idx += 1;
                self.lcurlin += 1;
                self.curlin += 1;
                Some(line)
            };

            if let Some(line) = line_result {
                self.line = line;
                self.lp = 0;
                self.skip_blanks();
                if self.cur_char() == Some('.') { self.lp += 1; }

                let olp = self.lp;
                if self.cmphstr("if") { iflevel += 1; }
                self.lp = olp;
                if self.cmphstr("ifexist") { iflevel += 1; }
                self.lp = olp;
                if self.cmphstr("ifnexist") { iflevel += 1; }
                self.lp = olp;
                if self.cmphstr("ifdef") { iflevel += 1; }
                self.lp = olp;
                if self.cmphstr("ifndef") { iflevel += 1; }
                self.lp = olp;
                if self.cmphstr("endif") {
                    if iflevel > 0 { iflevel -= 1; } else { return Ending::EndIf; }
                }
                self.lp = olp;
                if self.cmphstr("else") {
                    if iflevel == 0 { self.list_file(); return Ending::Else; }
                }
                self.list_file_skip();
            }
        }
    }

    /// Read lines from file input until 'end' keyword, storing into Vec
    pub fn read_file_to_string_list(&mut self, end_keyword: &str) -> Vec<String> {
        let mut result = Vec::new();
        loop {
            if !self.running { break; }
            let line_opt = if self.lijst {
                if self.lijst_idx >= self.lijstp.len() { None }
                else {
                    let l = self.lijstp[self.lijst_idx].clone();
                    self.lijst_idx += 1;
                    Some(l)
                }
            } else {
                if self.input_idx >= self.input_lines.len() { None }
                else {
                    let l = self.input_lines[self.input_idx].clone();
                    self.input_idx += 1;
                    self.lcurlin += 1;
                    self.curlin += 1;
                    Some(l)
                }
            };
            let line = match line_opt {
                Some(l) => l,
                None => {
                    self.error("Unexpected end of file", None, ErrorKind::Fatal);
                    break;
                }
            };

            if self.inside_compass_macro_def {
                // Compass-style: replace @ in labels to _
            }

            // Check for end keyword
            let trimmed = line.trim_start();
            if !trimmed.is_empty() && trimmed.as_bytes()[0] <= b' ' || trimmed.is_empty() {
                // blank line, just store
            } else {
                // check if leading whitespace line has end directive
            }
            // The C++ code checks: if first non-blank char and it starts with whitespace
            let line_bytes = line.as_bytes();
            let mut p = 0;
            while p < line_bytes.len() && (line_bytes[p] == b' ' || line_bytes[p] == b'\t') { p += 1; }
            if p > 0 || line_bytes.get(0) == Some(&b' ') || line_bytes.get(0) == Some(&b'\t') || p == 0 {
                let rest = &line[p..];
                let check = if rest.starts_with('.') { &rest[1..] } else { rest };
                if check.len() >= end_keyword.len() {
                    let candidate = &check[..end_keyword.len()];
                    if candidate.eq_ignore_ascii_case(end_keyword) {
                        let after = &check[end_keyword.len()..];
                        if after.is_empty() || after.starts_with(|c: char| c <= ' ' || c == ';') {
                            return result;
                        }
                    }
                }
            }

            result.push(line);
            self.list_file_skip();
        }
        result
    }

    // ============================================================
    // Export file
    // ============================================================

    pub fn write_exp(&mut self, name: &str, value: i64) {
        if self.exp_fp.is_none() {
            match File::create(&self.expfilename) {
                Ok(f) => self.exp_fp = Some(f),
                Err(_) => {
                    self.error(&format!("Error opening file: {}", self.expfilename), None, ErrorKind::Fatal);
                    return;
                }
            }
        }
        if let Some(ref mut f) = self.exp_fp {
            let _ = writeln!(f, "{}: EQU {:08X}h", name, value as u32);
        }
    }
}
