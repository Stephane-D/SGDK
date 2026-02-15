use crate::assembler::{Assembler, ErrorKind, Ending, OutputMode};

impl Assembler {
    // ============================================================
    // Directive dispatch
    // ============================================================

    pub fn parse_directive(&mut self) -> bool {
        let olp = self.lp;
        self.bp = self.lp;

        if let Some(n) = self.getinstr() {
            let nl = n.to_ascii_lowercase();
            // Check if directive name matches (strip leading '.')
            let key = if nl.starts_with('.') { &nl[1..] } else { &nl };
            if let Some(&func) = self.dir_tab.get(key) {
                func(self);
                return true;
            }
        } else {
            // Check for ## alignment shorthand
            if self.cur_char() == Some('#') && self.peek_char(1) == Some('#') {
                self.lp += 2;
                let old_synerr = self.synerr;
                self.synerr = false;
                let val = self.parse_expression().unwrap_or(4);
                self.synerr = old_synerr;
                self.mapadr += (!self.mapadr + 1) & (val - 1);
                return true;
            }
        }
        self.lp = olp;
        false
    }

    // ============================================================
    // Directive implementations
    // ============================================================

    pub fn dir_byte(&mut self) {
        let e = self.get_bytes(0, false);
        if e.is_empty() {
            self.error(".byte with no arguments", None, ErrorKind::Pass2);
            return;
        }
        self.emit_bytes_array(&e);
    }

    pub fn dir_dc(&mut self) {
        let e = self.get_bytes(0, true);
        if e.is_empty() {
            self.error(".byte with no arguments", None, ErrorKind::Pass2);
            return;
        }
        self.emit_bytes_array(&e);
    }

    pub fn dir_dz(&mut self) {
        let mut e = self.get_bytes(0, false);
        if e.is_empty() {
            self.error(".byte with no arguments", None, ErrorKind::Pass2);
            return;
        }
        e.push(0);
        self.emit_bytes_array(&e);
    }

    pub fn dir_abyte(&mut self) {
        if let Some(add) = self.parse_expression() {
            self.check8(add);
            let e = self.get_bytes((add & 255) as i32, false);
            if e.is_empty() {
                self.error(".abyte with no arguments", None, ErrorKind::Pass2);
                return;
            }
            self.emit_bytes_array(&e);
        } else {
            self.error("Expression expected", None, ErrorKind::Pass2);
        }
    }

    pub fn dir_abytec(&mut self) {
        if let Some(add) = self.parse_expression() {
            self.check8(add);
            let e = self.get_bytes((add & 255) as i32, true);
            if e.is_empty() {
                self.error(".abyte with no arguments", None, ErrorKind::Pass2);
                return;
            }
            self.emit_bytes_array(&e);
        } else {
            self.error("Expression expected", None, ErrorKind::Pass2);
        }
    }

    pub fn dir_abytez(&mut self) {
        if let Some(add) = self.parse_expression() {
            self.check8(add);
            let mut e = self.get_bytes((add & 255) as i32, false);
            if e.is_empty() {
                self.error(".abyte with no arguments", None, ErrorKind::Pass2);
                return;
            }
            e.push(0);
            self.emit_bytes_array(&e);
        } else {
            self.error("Expression expected", None, ErrorKind::Pass2);
        }
    }

    pub fn dir_word(&mut self) {
        let mut e: Vec<i32> = Vec::new();
        self.skip_blanks();
        while self.cur_char().is_some() {
            if let Some(val) = self.parse_expression() {
                self.check16(val);
                if e.len() > 127 {
                    self.error("Over 128 values in .word", None, ErrorKind::Fatal);
                }
                e.push((val & 65535) as i32);
            } else {
                let rem = self.remaining().to_string();
                self.error("Syntax error", Some(&rem), ErrorKind::CatchAll);
                return;
            }
            self.skip_blanks();
            if self.cur_char() != Some(',') { break; }
            self.lp += 1;
            self.skip_blanks();
        }
        if e.is_empty() {
            self.error(".word with no arguments", None, ErrorKind::Pass2);
            return;
        }
        self.emit_words(&e);
    }

    pub fn dir_dword(&mut self) {
        let mut e: Vec<i32> = Vec::new();
        self.skip_blanks();
        while self.cur_char().is_some() {
            if let Some(val) = self.parse_expression() {
                if e.len() > 127 {
                    self.error("Over 128 values in .dword", None, ErrorKind::Fatal);
                }
                e.push((val & 65535) as i32);
                e.push((val >> 16) as i32);
            } else {
                let rem = self.remaining().to_string();
                self.error("Syntax error", Some(&rem), ErrorKind::CatchAll);
                return;
            }
            self.skip_blanks();
            if self.cur_char() != Some(',') { break; }
            self.lp += 1;
            self.skip_blanks();
        }
        if e.is_empty() {
            self.error(".dword with no arguments", None, ErrorKind::Pass2);
            return;
        }
        self.emit_words(&e);
    }

    pub fn dir_d24(&mut self) {
        let mut e: Vec<i32> = Vec::new();
        self.skip_blanks();
        while self.cur_char().is_some() {
            if let Some(val) = self.parse_expression() {
                self.check24(val);
                if e.len() / 3 > 127 {
                    self.error("Over 128 values in .d24", None, ErrorKind::Fatal);
                }
                e.push((val & 255) as i32);
                e.push(((val >> 8) & 255) as i32);
                e.push(((val >> 16) & 255) as i32);
            } else {
                let rem = self.remaining().to_string();
                self.error("Syntax error", Some(&rem), ErrorKind::CatchAll);
                return;
            }
            self.skip_blanks();
            if self.cur_char() != Some(',') { break; }
            self.lp += 1;
            self.skip_blanks();
        }
        if e.is_empty() {
            self.error(".d24 with no arguments", None, ErrorKind::Pass2);
            return;
        }
        self.emit_bytes_array(&e);
    }

    pub fn dir_block(&mut self) {
        if let Some(teller) = self.parse_expression() {
            let teller = if teller < 0 {
                self.error("block with negative size", None, ErrorKind::Pass2);
                0
            } else { teller };
            let val = if self.comma() {
                self.parse_expression().unwrap_or(0)
            } else { 0 };
            self.emit_block(val, teller);
        } else {
            let rem = self.remaining().to_string();
            self.error("Syntax Error", Some(&rem), ErrorKind::CatchAll);
        }
    }

    pub fn dir_org(&mut self) {
        if let Some(val) = self.parse_expression() {
            self.adres = val;
        } else {
            self.error("Syntax error", None, ErrorKind::CatchAll);
        }
    }

    pub fn dir_map(&mut self) {
        self.maplstp.push(self.mapadr);
        self.labelnotfound = false;
        if let Some(val) = self.parse_expression() {
            self.mapadr = val;
        } else {
            self.error("Syntax error", None, ErrorKind::CatchAll);
        }
        if self.labelnotfound {
            self.error("Forward reference", None, ErrorKind::All);
        }
    }

    pub fn dir_endmap(&mut self) {
        if let Some(val) = self.maplstp.pop() {
            self.mapadr = val;
        } else {
            self.error(".endmodule without module", None, ErrorKind::Pass2);
        }
    }

    pub fn dir_align(&mut self) {
        let val = self.parse_expression().unwrap_or(4);
        match val {
            1 => {}
            2 | 4 | 8 | 16 | 32 | 64 | 128 | 256 |
            512 | 1024 | 2048 | 4096 | 8192 | 16384 | 32768 => {
                let pad = (!self.adres + 1) & (val - 1);
                self.emit_block(0, pad);
            }
            _ => {
                self.error("Illegal align", None, ErrorKind::Pass2);
            }
        }
    }

    pub fn dir_module(&mut self) {
        if let Some(ref mlp) = self.modlabp {
            self.modlstp.push(mlp.clone());
        } else {
            self.modlstp.push(String::new());
        }
        self.skip_blanks();
        if self.cur_char().is_none() || self.cur_char() == Some(';') {
            self.modlabp = None;
            return;
        }
        if let Some(n) = self.getid() {
            self.modlabp = Some(n);
        } else {
            self.error("Syntax error", None, ErrorKind::CatchAll);
        }
    }

    pub fn dir_endmodule(&mut self) {
        if let Some(val) = self.modlstp.pop() {
            self.modlabp = if val.is_empty() { None } else { Some(val) };
        } else {
            self.error(".endmodule without module", None, ErrorKind::Pass2);
        }
    }

    pub fn dir_z80(&mut self) {
        // Already in Z80 mode, nothing to do
    }

    pub fn dir_end(&mut self) {
        self.running = false;
    }

    pub fn dir_size(&mut self) {
        let _bp_saved = self.bp;
        if let Some(val) = self.parse_expression() {
            if self.pass == 2 { return; }
            if self.size != -1 {
                self.error("Multiple sizes?", None, ErrorKind::Pass2);
                return;
            }
            self.size = val;
        } else {
            self.error("Syntax error", None, ErrorKind::CatchAll);
        }
    }

    pub fn dir_incbin(&mut self) {
        let fnaam = self.getfilename();
        let mut offset = -1i32;
        let mut length = -1i32;
        if self.comma() {
            if !self.comma() {
                if let Some(val) = self.parse_expression() {
                    if val < 0 {
                        self.error("Negative values are not allowed", None, ErrorKind::Pass2);
                        return;
                    }
                    offset = val as i32;
                } else {
                    self.error("Syntax error", None, ErrorKind::CatchAll);
                    return;
                }
            }
            if self.comma() {
                if let Some(val) = self.parse_expression() {
                    if val < 0 {
                        self.error("Negative values are not allowed", None, ErrorKind::Pass2);
                        return;
                    }
                    length = val as i32;
                } else {
                    self.error("Syntax error", None, ErrorKind::CatchAll);
                    return;
                }
            }
        }
        self.bin_inc_file(&fnaam, offset, length);
    }

    pub fn dir_textarea(&mut self) {
        let oadres = self.adres;
        self.labelnotfound = false;
        if let Some(val) = self.parse_expression() {
            if self.labelnotfound {
                self.error("Forward reference", None, ErrorKind::All);
            }
            self.list_file();
            self.adres = val;
            if self.read_file() != Ending::EndTextArea {
                self.error("No end of textarea", None, ErrorKind::Pass2);
            }
            self.adres = oadres + self.adres - val;
        } else {
            self.error("No adress given", None, ErrorKind::Pass2);
        }
    }

    fn dir_if_cond(&mut self, error_msg: &str) {
        self.labelnotfound = false;
        if let Some(val) = self.parse_expression() {
            if self.labelnotfound {
                self.error("Forward reference", None, ErrorKind::All);
            }
            if val != 0 {
                self.list_file();
                match self.read_file() {
                    Ending::Else => {
                        if self.skip_file() != Ending::EndIf {
                            self.error(error_msg, None, ErrorKind::Pass2);
                        }
                    }
                    Ending::EndIf => {}
                    _ => {
                        self.error(error_msg, None, ErrorKind::Pass2);
                    }
                }
            } else {
                self.list_file();
                match self.skip_file() {
                    Ending::Else => {
                        if self.read_file() != Ending::EndIf {
                            self.error(error_msg, None, ErrorKind::Pass2);
                        }
                    }
                    Ending::EndIf => {}
                    _ => {
                        self.error(error_msg, None, ErrorKind::Pass2);
                    }
                }
            }
            self.lp = self.line.len();
        } else {
            self.error("Syntax error", None, ErrorKind::CatchAll);
        }
    }

    pub fn dir_cond(&mut self) {
        self.dir_if_cond("No endc");
    }

    pub fn dir_if(&mut self) {
        self.dir_if_cond("No endif");
    }

    pub fn dir_else(&mut self) {
        self.error("Else without if", None, ErrorKind::Pass2);
    }

    pub fn dir_endc(&mut self) {
        self.error("Endc without cond", None, ErrorKind::Pass2);
    }

    pub fn dir_endif(&mut self) {
        self.error("Endif without if", None, ErrorKind::Pass2);
    }

    pub fn dir_end_textarea(&mut self) {
        self.error("Endt without textarea", None, ErrorKind::Pass2);
    }

    pub fn dir_noop(&mut self) {
        self.lp = self.line.len();
    }

    pub fn dir_include(&mut self) {
        let fnaam = self.getfilename();
        self.list_file();
        self.open_file(&fnaam);
        self.donotlist = true;
    }

    pub fn dir_output(&mut self) {
        let mut fnaam = self.getfilename();
        if fnaam.starts_with('<') {
            fnaam = fnaam[1..].to_string();
        }
        let mut mode = OutputMode::Truncate;
        if self.comma() {
            if let Some(mc) = self.cur_char() {
                self.lp += 1;
                match mc.to_ascii_lowercase() {
                    't' => mode = OutputMode::Truncate,
                    'r' => mode = OutputMode::Rewind,
                    'a' => mode = OutputMode::Append,
                    _ => {
                        self.error("Syntax error", None, ErrorKind::CatchAll);
                    }
                }
            }
        }
        if self.pass == 2 {
            self.new_dest(&fnaam, mode);
        }
    }

    pub fn dir_define(&mut self) {
        // Find 'define' in the line to get position after the keyword
        let line_lower = self.line.to_ascii_lowercase();
        if let Some(pos) = line_lower.find("define") {
            let p = pos + 6; // skip "define"
            let after = &self.line[p..];
            // Parse id from after
            let trimmed = after.trim_start();
            let id_start = p + (after.len() - trimmed.len());
            // Get identifier
            let mut id_end = id_start;
            let line_bytes = self.line.as_bytes();
            if id_end < line_bytes.len() && (line_bytes[id_end].is_ascii_alphabetic() || line_bytes[id_end] == b'_') {
                id_end += 1;
                while id_end < line_bytes.len() && crate::tables::is_valid_id_char(line_bytes[id_end] as char) {
                    id_end += 1;
                }
            }
            if id_end <= id_start {
                self.error("illegal define", None, ErrorKind::Pass2);
                return;
            }
            let id = self.line[id_start..id_end].to_string();
            let replacement = self.line[id_end..].trim_start().to_string();
            self.definetab.add(&id, &replacement);
            self.lp = self.line.len();
        } else {
            self.error("define error", None, ErrorKind::Fatal);
        }
    }

    pub fn dir_ifdef(&mut self) {
        // Find 'ifdef' in line
        let line_lower = self.line.to_ascii_lowercase();
        if let Some(pos) = line_lower.find("ifdef") {
            let p = pos + 5;
            let after = self.line[p..].trim_start();
            // Parse id
            let mut chars = after.chars();
            let first = chars.next();
            if !matches!(first, Some(c) if c.is_ascii_alphabetic() || c == '_') {
                self.error("Illegal identifier", None, ErrorKind::Pass1);
                return;
            }
            let mut id = String::new();
            id.push(first.unwrap());
            for c in chars {
                if !crate::tables::is_valid_id_char(c) { break; }
                id.push(c);
            }

            if self.definetab.exists(&id) {
                self.list_file();
                match self.read_file() {
                    Ending::Else => {
                        if self.skip_file() != Ending::EndIf {
                            self.error("No endif", None, ErrorKind::Pass2);
                        }
                    }
                    Ending::EndIf => {}
                    _ => {
                        self.error("No endif!", None, ErrorKind::Pass2);
                    }
                }
            } else {
                self.list_file();
                match self.skip_file() {
                    Ending::Else => {
                        if self.read_file() != Ending::EndIf {
                            self.error("No endif", None, ErrorKind::Pass2);
                        }
                    }
                    Ending::EndIf => {}
                    _ => {
                        self.error("No endif!", None, ErrorKind::Pass2);
                    }
                }
            }
            self.lp = self.line.len();
        } else {
            self.error("ifdef error", None, ErrorKind::Fatal);
        }
    }

    pub fn dir_ifndef(&mut self) {
        // Find 'ifndef' in line
        let line_lower = self.line.to_ascii_lowercase();
        if let Some(pos) = line_lower.find("ifndef") {
            let p = pos + 6;
            let after = self.line[p..].trim_start();
            let mut chars = after.chars();
            let first = chars.next();
            if !matches!(first, Some(c) if c.is_ascii_alphabetic() || c == '_') {
                self.error("Illegal identifier", None, ErrorKind::Pass1);
                return;
            }
            let mut id = String::new();
            id.push(first.unwrap());
            for c in chars {
                if !crate::tables::is_valid_id_char(c) { break; }
                id.push(c);
            }

            if !self.definetab.exists(&id) {
                self.list_file();
                match self.read_file() {
                    Ending::Else => {
                        if self.skip_file() != Ending::EndIf {
                            self.error("No endif", None, ErrorKind::Pass2);
                        }
                    }
                    Ending::EndIf => {}
                    _ => {
                        self.error("No endif!", None, ErrorKind::Pass2);
                    }
                }
            } else {
                self.list_file();
                match self.skip_file() {
                    Ending::Else => {
                        if self.read_file() != Ending::EndIf {
                            self.error("No endif", None, ErrorKind::Pass2);
                        }
                    }
                    Ending::EndIf => {}
                    _ => {
                        self.error("No endif!", None, ErrorKind::Pass2);
                    }
                }
            }
            self.lp = self.line.len();
        } else {
            self.error("ifndef error", None, ErrorKind::Fatal);
        }
    }

    pub fn dir_export(&mut self) {
        if self.pass == 1 { return; }
        if let Some(n) = self.getid() {
            self.labelnotfound = false;
            if let Some(val) = self.get_label_value() {
                if self.labelnotfound {
                    self.error("Label not found", Some(&n), ErrorKind::Suppres);
                    return;
                }
                let name = n.clone();
                self.write_exp(&name, val);
            }
        } else {
            let rem = self.remaining().to_string();
            self.error("Syntax error", Some(&rem), ErrorKind::CatchAll);
        }
    }

    pub fn dir_macro(&mut self) {
        if self.lijst {
            self.error("No macro definitions allowed here", None, ErrorKind::Fatal);
        }
        if let Some(n) = self.getid() {
            // Parse argument names
            self.skip_blanks();
            let mut args = Vec::new();
            while let Some(arg) = self.getid() {
                args.push(arg);
                self.skip_blanks();
                if self.cur_char() == Some(',') { self.lp += 1; } else { break; }
            }
            let remaining = self.remaining().trim().to_string();
            if !remaining.is_empty() && !remaining.starts_with(';') {
                self.error("Unexpected", Some(&remaining), ErrorKind::Pass1);
            }
            self.list_file();
            // Read body lines until "endm"
            let body = self.read_file_to_string_list("endm");
            if body.is_empty() && self.pass == 1 {
                // Only warn, don't fatal - empty macros are valid
            }
            self.macrotab.add(&n, args, body);
            self.inside_compass_macro_def = false;
        } else {
            self.error("Illegal macroname", None, ErrorKind::Pass1);
        }
    }

    pub fn dir_endm(&mut self) {
        self.inside_compass_macro_def = false;
        self.error("End macro without macro", None, ErrorKind::Pass2);
    }

    pub fn dir_ends(&mut self) {
        self.error("End structre without structure", None, ErrorKind::Pass2);
    }

    pub fn dir_assert(&mut self) {
        let p = self.remaining().to_string();
        if let Some(val) = self.parse_expression() {
            if self.pass == 2 && val == 0 {
                self.error("Assertion failed", Some(&p), ErrorKind::Pass2);
            }
            self.lp = self.line.len();
        } else {
            self.error("Syntax error", None, ErrorKind::CatchAll);
        }
    }

    pub fn dir_struct(&mut self) {
        let mut global = false;
        let mut offset: i64 = 0;
        self.skip_blanks();
        if self.cur_char() == Some('@') {
            self.lp += 1;
            global = true;
        }
        let naam = match self.getid() {
            Some(n) => n,
            None => {
                self.error("Illegal structurename", None, ErrorKind::Pass1);
                return;
            }
        };
        if self.comma() {
            self.labelnotfound = false;
            if let Some(val) = self.parse_expression() {
                offset = val;
            } else {
                self.error("Syntax error", None, ErrorKind::CatchAll);
                return;
            }
            if self.labelnotfound {
                self.error("Forward reference", None, ErrorKind::All);
            }
        }

        let modlabp = self.modlabp.clone();
        let _st_key = self.structtab.add(&naam, offset, 0, global, &modlabp);
        self.list_file();

        // Read struct body
        // We need to read lines from the input until "ends"
        // This is handled via the file reading mechanism
        // For now, read lines until .ends
        loop {
            if !self.running { break; }
            // In the C++ code, ReadLine() fetches next line
            // Here we rely on the caller feeding lines to us
            // For include-based reading, we need to handle this differently
            // The struct parsing reads from the list or file input
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
            match line_opt {
                Some(l) => {
                    self.line = l;
                    self.lp = 0;
                }
                None => {
                    self.error("Unexpected end of structure", None, ErrorKind::Pass1);
                    break;
                }
            }

            let trimmed = self.line.trim_start();
            if trimmed.is_empty() || trimmed.starts_with(';') {
                self.list_file_skip();
                continue;
            }
            // Check for .ends
            self.lp = 0;
            if self.white() {
                self.skip_blanks();
                if self.cur_char() == Some('.') { self.lp += 1; }
                if self.cmphstr("ends") {
                    break;
                }
            }
            self.lp = 0;
            let naam_clone = naam.clone();
            self.parse_struct_line(&naam_clone);
            self.list_file_skip();
        }

        // Define labels for struct members
        let modlabp = self.modlabp.clone();
        if let Some(st) = self.structtab.lookup(&naam, global, &modlabp) {
            let st_clone = st.clone();
            let pass = self.pass;
            // Define struct member labels
            for mn in &st_clone.member_names {
                let full = format!("{}.{}", naam, mn.name);
                if pass == 2 {
                    // verify
                } else {
                    self.labtab.insert(&full, mn.offset);
                }
            }
        }
    }

    pub fn dir_forg(&mut self) {
        self.skip_blanks();
        let from_start = !(self.cur_char() == Some('+') || self.cur_char() == Some('-'));
        if let Some(val) = self.parse_expression() {
            if self.pass == 2 {
                self.seek_dest(val, from_start);
            }
        } else {
            self.error("Syntax error", None, ErrorKind::CatchAll);
        }
    }

    pub fn dir_rept(&mut self) {
        self.labelnotfound = false;
        let val = match self.parse_expression() {
            Some(v) => v,
            None => {
                self.error("Syntax error", None, ErrorKind::CatchAll);
                return;
            }
        };
        if self.labelnotfound {
            self.error("Forward reference", None, ErrorKind::All);
        }
        if val < 0 {
            self.error("Illegal repeat value", None, ErrorKind::CatchAll);
            return;
        }
        self.list_file();

        // Collect lines until endm
        let mut body: Vec<String> = Vec::new();
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
                    self.error("Unexpected end of repeat block", None, ErrorKind::Pass1);
                    break;
                }
            };

            let trimmed = line.trim_start();
            let check = if trimmed.starts_with('.') { &trimmed[1..] } else { trimmed };
            if check.len() >= 4 {
                let candidate = &check[..4];
                if candidate.eq_ignore_ascii_case("endm") {
                    let after = &check[4..];
                    if after.is_empty() || after.starts_with(|c: char| c <= ' ' || c == ';') {
                        break;
                    }
                }
            }
            body.push(line);
        }

        self.inside_compass_macro_def = false;
        self.list_file();

        let old_listmacro = self.listmacro;
        self.listmacro = true;
        let old_lijst = self.lijst;
        let old_lijstp = std::mem::take(&mut self.lijstp);
        let old_lijst_idx = self.lijst_idx;
        let saved_line = self.line.clone();

        for _ in 0..val {
            // Set up body as lijst so directives can consume lines properly
            self.lijst = true;
            self.lijstp = body.clone();
            self.lijst_idx = 0;

            while self.lijst_idx < self.lijstp.len() {
                let body_line = self.lijstp[self.lijst_idx].clone();
                self.lijst_idx += 1;
                self.line = body_line;
                self.lp = 0;
                self.parse_line();
            }
        }

        self.line = saved_line;
        self.lijst = old_lijst;
        self.lijstp = old_lijstp;
        self.lijst_idx = old_lijst_idx;
        self.listmacro = old_listmacro;
        self.donotlist = true;
    }

    pub fn dir_arm(&mut self) {
        self.error("No ARM support in this version", None, ErrorKind::Fatal);
    }

    pub fn dir_thumb(&mut self) {
        self.error("No ARM support in this version", None, ErrorKind::Fatal);
    }

    // ============================================================
    // Register all directives
    // ============================================================

    pub fn insert_directives(&mut self) {
        self.dir_tab.insert("assert".into(), Assembler::dir_assert);
        self.dir_tab.insert("byte".into(), Assembler::dir_byte);
        self.dir_tab.insert("abyte".into(), Assembler::dir_abyte);
        self.dir_tab.insert("abytec".into(), Assembler::dir_abytec);
        self.dir_tab.insert("abytez".into(), Assembler::dir_abytez);
        self.dir_tab.insert("word".into(), Assembler::dir_word);
        self.dir_tab.insert("block".into(), Assembler::dir_block);
        self.dir_tab.insert("dword".into(), Assembler::dir_dword);
        self.dir_tab.insert("d24".into(), Assembler::dir_d24);
        self.dir_tab.insert("org".into(), Assembler::dir_org);
        self.dir_tab.insert("map".into(), Assembler::dir_map);
        self.dir_tab.insert("align".into(), Assembler::dir_align);
        self.dir_tab.insert("module".into(), Assembler::dir_module);
        self.dir_tab.insert("z80".into(), Assembler::dir_z80);
        self.dir_tab.insert("arm".into(), Assembler::dir_arm);
        self.dir_tab.insert("thumb".into(), Assembler::dir_thumb);
        self.dir_tab.insert("size".into(), Assembler::dir_size);
        self.dir_tab.insert("textarea".into(), Assembler::dir_textarea);
        self.dir_tab.insert("phase".into(), Assembler::dir_textarea);
        self.dir_tab.insert("msx".into(), Assembler::dir_z80);
        self.dir_tab.insert("else".into(), Assembler::dir_else);
        self.dir_tab.insert("export".into(), Assembler::dir_export);
        self.dir_tab.insert("end".into(), Assembler::dir_end);
        self.dir_tab.insert("include".into(), Assembler::dir_include);
        self.dir_tab.insert("incbin".into(), Assembler::dir_incbin);
        self.dir_tab.insert("if".into(), Assembler::dir_if);
        self.dir_tab.insert("output".into(), Assembler::dir_output);
        self.dir_tab.insert("define".into(), Assembler::dir_define);
        self.dir_tab.insert("ifdef".into(), Assembler::dir_ifdef);
        self.dir_tab.insert("ifndef".into(), Assembler::dir_ifndef);
        self.dir_tab.insert("macro".into(), Assembler::dir_macro);
        self.dir_tab.insert("struct".into(), Assembler::dir_struct);
        self.dir_tab.insert("dc".into(), Assembler::dir_dc);
        self.dir_tab.insert("dz".into(), Assembler::dir_dz);
        self.dir_tab.insert("db".into(), Assembler::dir_byte);
        self.dir_tab.insert("dw".into(), Assembler::dir_word);
        self.dir_tab.insert("ds".into(), Assembler::dir_block);
        self.dir_tab.insert("dd".into(), Assembler::dir_dword);
        self.dir_tab.insert("dm".into(), Assembler::dir_byte);
        self.dir_tab.insert("defb".into(), Assembler::dir_byte);
        self.dir_tab.insert("defw".into(), Assembler::dir_word);
        self.dir_tab.insert("defs".into(), Assembler::dir_block);
        self.dir_tab.insert("defd".into(), Assembler::dir_dword);
        self.dir_tab.insert("defm".into(), Assembler::dir_byte);
        self.dir_tab.insert("endmod".into(), Assembler::dir_endmodule);
        self.dir_tab.insert("endmodule".into(), Assembler::dir_endmodule);
        self.dir_tab.insert("endmap".into(), Assembler::dir_endmap);
        self.dir_tab.insert("rept".into(), Assembler::dir_rept);
        self.dir_tab.insert("fpos".into(), Assembler::dir_forg);
        self.dir_tab.insert("endif".into(), Assembler::dir_endif);
        self.dir_tab.insert("endt".into(), Assembler::dir_end_textarea);
        self.dir_tab.insert("dephase".into(), Assembler::dir_end_textarea);
        self.dir_tab.insert("endm".into(), Assembler::dir_endm);
        self.dir_tab.insert("ends".into(), Assembler::dir_ends);

        if self.compass_compat {
            self.dir_tab.insert("cond".into(), Assembler::dir_cond);
            self.dir_tab.insert("endc".into(), Assembler::dir_endc);
            self.dir_tab.insert(".label".into(), Assembler::dir_noop);
            self.dir_tab.insert(".upper".into(), Assembler::dir_noop);
            self.dir_tab.insert("tsrhooks".into(), Assembler::dir_noop);
            self.dir_tab.insert("breakp".into(), Assembler::dir_noop);
        }
    }
}
