use crate::assembler::{Assembler, ErrorKind, StructMemb};
use crate::tables::is_valid_id_char;

impl Assembler {
    /// Skip blanks (whitespace <= ' ')
    pub fn skip_blanks(&mut self) -> bool {
        while let Some(c) = self.cur_char() {
            if c > ' ' { break; }
            self.lp += 1;
        }
        self.cur_char().is_none()
    }

    /// Skip blanks in a string slice, return new offset
    pub fn skip_blanks_in(s: &str, mut pos: usize) -> usize {
        let bytes = s.as_bytes();
        while pos < bytes.len() && bytes[pos] <= b' ' {
            pos += 1;
        }
        pos
    }

    /// Check if current position is whitespace
    pub fn white(&self) -> bool {
        match self.cur_char() {
            Some(c) => c <= ' ',
            None => true,
        }
    }

    /// Check for '=' or 'equ' keyword after blanks
    pub fn need_equ(&mut self) -> bool {
        let olp = self.lp;
        self.skip_blanks();
        if self.cur_char() == Some('=') {
            self.lp += 1;
            return true;
        }
        if self.cur_char() == Some('.') {
            self.lp += 1;
        }
        if self.cmphstr("equ") {
            return true;
        }
        self.lp = olp;
        false
    }

    /// Check for '#' or 'field' keyword
    pub fn need_field(&mut self) -> bool {
        let olp = self.lp;
        self.skip_blanks();
        if self.cur_char() == Some('#') {
            self.lp += 1;
            return true;
        }
        if self.cur_char() == Some('.') {
            self.lp += 1;
        }
        if self.cmphstr("field") {
            return true;
        }
        self.lp = olp;
        false
    }

    /// Compare string at current position (case-sensitive if first char is uppercase,
    /// case-insensitive otherwise), advance lp if match. Returns true on match.
    pub fn cmphstr(&mut self, s: &str) -> bool {
        let bytes = self.line.as_bytes();
        let sbytes = s.as_bytes();
        if self.lp + sbytes.len() > bytes.len() {
            return false;
        }

        let first = bytes.get(self.lp).copied().unwrap_or(0);
        let is_upper = first.is_ascii_uppercase();

        for (i, &sb) in sbytes.iter().enumerate() {
            let lb = bytes[self.lp + i];
            if is_upper {
                if lb != sb.to_ascii_uppercase() {
                    return false;
                }
            } else if lb != sb {
                return false;
            }
        }

        // Next char must be <= ' '  (not a continuation of identifier)
        let next_pos = self.lp + sbytes.len();
        if next_pos < bytes.len() && bytes[next_pos] > b' ' {
            return false;
        }

        self.lp = next_pos;
        true
    }

    /// Check for comma
    pub fn comma(&mut self) -> bool {
        self.skip_blanks();
        if self.cur_char() == Some(',') {
            self.lp += 1;
            true
        } else {
            false
        }
    }

    /// Check for opening paren/bracket
    pub fn oparen(&mut self, c: char) -> bool {
        self.skip_blanks();
        if self.cur_char() == Some(c) {
            self.lp += 1;
            true
        } else {
            false
        }
    }

    /// Check for closing paren, uses the matching close char
    pub fn cparen(&mut self, open: char) -> bool {
        let close = match open {
            '(' => ')',
            '[' => ']',
            '{' => '}',
            _ => ')',
        };
        self.skip_blanks();
        if self.cur_char() == Some(close) {
            self.lp += 1;
            true
        } else {
            false
        }
    }

    /// Get identifier at current position. Returns None if not starting with alpha/underscore.
    pub fn getid(&mut self) -> Option<String> {
        self.skip_blanks();
        let first = self.cur_char()?;
        if !first.is_ascii_alphabetic() && first != '_' {
            return None;
        }
        let start = self.lp;
        while let Some(c) = self.cur_char() {
            if !is_valid_id_char(c) { break; }
            self.lp += 1;
        }
        Some(self.line[start..self.lp].to_string())
    }

    /// Get instruction name (alphanumeric, starting with alpha or '.')). 
    pub fn getinstr(&mut self) -> Option<String> {
        self.skip_blanks();
        let first = self.cur_char()?;
        if !first.is_ascii_alphabetic() && first != '.' {
            return None;
        }
        let start = self.lp;
        self.lp += 1;
        while let Some(c) = self.cur_char() {
            if !c.is_ascii_alphanumeric() { break; }
            self.lp += 1;
        }
        Some(self.line[start..self.lp].to_string())
    }

    /// Check that value fits in 8 bits
    pub fn check8(&mut self, val: i64) -> bool {
        let uval = val as u64;
        if uval != (uval & 0xFF) && (!val as u64) > 127 {
            self.error("Bytes lost", None, ErrorKind::Pass2);
            return false;
        }
        true
    }

    /// Check 8-bit offset range
    pub fn check8o(&mut self, val: i64) -> bool {
        if val < -128 || val > 127 {
            self.error("Offset out of range", None, ErrorKind::Pass2);
            return false;
        }
        true
    }

    /// Check that value fits in 16 bits
    pub fn check16(&mut self, val: i64) -> bool {
        let uval = val as u64;
        if uval != (uval & 0xFFFF) && (!val as u64) > 32767 {
            self.error("Bytes lost", None, ErrorKind::Pass2);
            return false;
        }
        true
    }

    /// Check that value fits in 24 bits
    pub fn check24(&mut self, val: i64) -> bool {
        let uval = val as u64;
        if uval != (uval & 0xFFFFFF) && (!val as u64) > 8388607 {
            self.error("Bytes lost", None, ErrorKind::Pass2);
            return false;
        }
        true
    }

    /// Check for a specific character  
    pub fn need_char(&mut self, c: char) -> bool {
        self.skip_blanks();
        if self.cur_char() == Some(c) {
            self.lp += 1;
            true
        } else {
            false
        }
    }

    /// Check for operator chars. The string encodes pairs: char + modifier.
    /// Returns the matched operator value or 0.
    pub fn need_op(&mut self, ops: &str) -> i32 {
        self.skip_blanks();
        let bytes = ops.as_bytes();
        let line_bytes = self.line.as_bytes();
        let mut i = 0;
        while i < bytes.len() {
            let c1 = bytes[i] as char;
            i += 1;
            if i >= bytes.len() { break; }
            let c2 = bytes[i] as char;
            i += 1;

            if self.lp >= line_bytes.len() { continue; }
            let lc = line_bytes[self.lp] as char;

            if lc != c1 { continue; }
            if c2 == ' ' {
                // Single char match, but next char must not be the same
                self.lp += 1;
                return c1 as i32;
            }
            if c2 == '_' {
                // Single char match only if next is NOT same char
                if self.lp + 1 < line_bytes.len() && line_bytes[self.lp + 1] == c1 as u8 {
                    continue;
                }
                self.lp += 1;
                return c1 as i32;
            }
            // Two-char match
            if self.lp + 1 < line_bytes.len() && line_bytes[self.lp + 1] == c2 as u8 {
                self.lp += 2;
                return c1 as i32 + c2 as i32;
            }
        }
        0
    }

    /// Check for keyword alternatives (like "not"→'!', "low"→'l', etc.)
    pub fn needa(&mut self, c1: &str, r1: i32, c2: Option<(&str, i32)>, c3: Option<(&str, i32)>) -> i32 {
        if !self.cur_char().map(|c| c.is_ascii_alphabetic()).unwrap_or(false) {
            return 0;
        }
        let olp = self.lp;
        if self.cmphstr(c1) { return r1; }
        if let Some((s2, r2)) = c2 {
            self.lp = olp;
            if self.cmphstr(s2) { return r2; }
        }
        if let Some((s3, r3)) = c3 {
            self.lp = olp;
            if self.cmphstr(s3) { return r3; }
        }
        self.lp = olp;
        0
    }

    /// Get a numeric constant (hex, bin, decimal, octal with various prefix/suffix notations)
    pub fn get_constant(&mut self) -> Option<i64> {
        self.skip_blanks();
        let cur = self.cur_char()?;

        // &H prefix (hex) or &B prefix (binary)
        if cur == '&' {
            self.lp += 1;
            let prefix = self.cur_char()?.to_ascii_lowercase();
            if prefix == 'h' {
                // Switch to # prefix handling
                self.lp += 1;
                return self.parse_hex_digits();
            } else if prefix == 'b' {
                self.lp += 1;
                return self.parse_bin_digits();
            }
            self.error("Syntax error", None, ErrorKind::CatchAll);
            return None;
        }

        match cur {
            '#' | '$' => {
                self.lp += 1;
                // Check if next is alphanumeric (for $ as hex prefix)
                if !self.cur_char().map(|c| c.is_ascii_alphanumeric()).unwrap_or(false) {
                    if cur == '$' {
                        // Bare $ means current address
                        return None; // handled elsewhere
                    }
                    self.error("Syntax error", None, ErrorKind::CatchAll);
                    return None;
                }
                self.parse_hex_digits()
            }
            '%' => {
                self.lp += 1;
                self.parse_bin_digits()
            }
            '0' => {
                if self.peek_char(1).map(|c| c == 'x' || c == 'X').unwrap_or(false) {
                    self.lp += 2;
                    self.parse_hex_digits()
                } else {
                    self.parse_suffixed_constant()
                }
            }
            _ if cur.is_ascii_digit() => {
                self.parse_suffixed_constant()
            }
            _ => None,
        }
    }

    fn parse_hex_digits(&mut self) -> Option<i64> {
        let start = self.lp;
        let mut val: i64 = 0;
        while let Some(c) = self.cur_char() {
            if self.compass_compat && (c == ' ' || c == '\t') {
                self.lp += 1;
                continue;
            }
            if let Some(v) = char_digit_val(c) {
                if v >= 16 {
                    self.error("Digit not in base", None, ErrorKind::Pass2);
                    return None;
                }
                val = val.wrapping_mul(16).wrapping_add(v as i64);
                self.lp += 1;
            } else {
                break;
            }
        }
        if self.lp == start {
            self.error("Syntax error", None, ErrorKind::CatchAll);
            return None;
        }
        Some(val)
    }

    fn parse_bin_digits(&mut self) -> Option<i64> {
        let start = self.lp;
        let mut val: i64 = 0;
        while let Some(c) = self.cur_char() {
            if self.compass_compat && (c == ' ' || c == '\t') {
                self.lp += 1;
                continue;
            }
            if c == '0' || c == '1' {
                val = val.wrapping_mul(2).wrapping_add((c as i64) - ('0' as i64));
                self.lp += 1;
            } else if c.is_ascii_digit() {
                self.error("Digit not in base", None, ErrorKind::Pass2);
                return None;
            } else {
                break;
            }
        }
        if self.lp == start {
            self.error("Syntax error", None, ErrorKind::CatchAll);
            return None;
        }
        Some(val)
    }

    /// Parse constant with suffix (h, b, o, q, d) or plain decimal
    fn parse_suffixed_constant(&mut self) -> Option<i64> {
        let start = self.lp;
        // Scan to end of alphanumeric sequence
        let mut end = self.lp;
        while end < self.line.len() {
            let c = self.line.as_bytes()[end] as char;
            if c.is_ascii_alphanumeric() || (self.compass_compat && (c == ' ' || c == '\t')) {
                end += 1;
            } else {
                break;
            }
        }
        // Find last non-space char
        let mut last = end - 1;
        while last > start && {
            let c = self.line.as_bytes()[last] as char;
            self.compass_compat && (c == ' ' || c == '\t')
        } {
            last -= 1;
        }

        let last_char = self.line.as_bytes()[last] as char;
        let (base, _digit_end) = if last_char.is_ascii_digit() {
            (10u64, end)
        } else {
            let b = match last_char.to_ascii_lowercase() {
                'b' => 2u64,
                'h' => 16,
                'o' | 'q' => 8,
                'd' => 10,
                _ => return None,
            };
            (b, last) // digits end before the suffix
        };

        // Parse digits from start to digit_end in reverse with positional values
        let mut val: i64 = 0;
        let mut pb: i64 = 1;
        let _pos = if base != 10 || !last_char.is_ascii_digit() { last - 1 } else { end - 1 };
        // Actually let's do it forward like the C++ code does in reverse
        // The C++ code: do { v=getval(*p); val+=v*pb; pb*=base; } while(p--!=p3);
        // where it goes from position p (last digit position) back to p3 (start)
        let mut p = if base == 10 && last_char.is_ascii_digit() { end - 1 } else { last - 1 };
        loop {
            let c = self.line.as_bytes()[p] as char;
            if self.compass_compat && (c == ' ' || c == '\t') {
                if p == start { break; }
                p -= 1;
                continue;
            }
            if let Some(v) = char_digit_val(c) {
                if v as u64 >= base {
                    self.error("Digit not in base", None, ErrorKind::Pass2);
                    return None;
                }
                val = val.wrapping_add((v as i64).wrapping_mul(pb));
                pb = pb.wrapping_mul(base as i64);
            } else {
                return None;
            }
            if p == start { break; }
            p -= 1;
        }
        self.lp = end;
        Some(val)
    }

    /// Parse character constant ('abc' or "abc")
    pub fn get_char_const(&mut self) -> Option<i64> {
        let q = self.cur_char()?;
        if q != '\'' && q != '"' {
            return None;
        }

        // Compass compat: empty string
        if self.compass_compat {
            if self.peek_char(1) == Some(q) {
                self.lp += 2;
                return Some(0);
            }
            // Compass: single backslash
            if self.peek_char(1) == Some('\\') && self.peek_char(2) == Some(q) {
                self.lp += 3;
                return Some('\\' as i64);
            }
        }

        let olp = self.lp;
        self.lp += 1; // skip opening quote

        let mut val: i64 = 0;
        let mut shift = 24i32;
        let mut count = 0;

        loop {
            let c = self.cur_char();
            if c.is_none() || c == Some(q) {
                if count == 0 {
                    self.lp = olp;
                    return None;
                }
                break;
            }
            let r = self.get_char_const_char();
            val += (r as i64) << shift;
            shift -= 8;
            count += 1;
        }

        if count > 4 {
            self.error("Overflow", None, ErrorKind::Suppres);
        }
        val >>= shift + 8;
        if self.cur_char() == Some(q) {
            self.lp += 1;
        }
        Some(val)
    }

    /// Parse a single character (possibly with escape)
    pub fn get_char_const_char(&mut self) -> u8 {
        let c = self.cur_char().unwrap_or(0 as char);
        self.lp += 1;
        if c != '\\' {
            return c as u8;
        }
        let esc = self.cur_char().unwrap_or(0 as char);
        self.lp += 1;
        match esc {
            '\\' | '\'' | '"' | '?' => esc as u8,
            'n' | 'N' => 10,
            't' | 'T' => 9,
            'v' | 'V' => 11,
            'b' | 'B' => 8,
            'r' | 'R' => 13,
            'f' | 'F' => 12,
            'a' | 'A' => 7,
            'e' | 'E' => 27,
            'd' | 'D' => 127,
            _ => {
                self.lp -= 1;
                self.error("Unknown escape", None, ErrorKind::Pass2);
                b'\\'
            }
        }
    }

    /// Get bytes from a byte-list expression (for .byte/.db etc.)
    pub fn get_bytes(&mut self, add: i32, dc: bool) -> Vec<i32> {
        let mut result = Vec::new();
        loop {
            self.skip_blanks();
            if self.cur_char().is_none() {
                self.error("Expression expected", None, ErrorKind::Suppres);
                break;
            }
            if result.len() >= 128 {
                self.error("Too many arguments", None, ErrorKind::Suppres);
                break;
            }
            if self.cur_char() == Some('"') {
                self.lp += 1;
                loop {
                    let c = self.cur_char();
                    if c.is_none() || c == Some('"') {
                        if c.is_none() {
                            self.error("Syntax error", None, ErrorKind::Suppres);
                        }
                        break;
                    }
                    if result.len() >= 128 {
                        self.error("Too many arguments", None, ErrorKind::Suppres);
                        break;
                    }
                    if self.compass_compat && self.cur_char() == Some('\\') {
                        result.push((92 + add) & 255);
                        self.lp += 1;
                    } else {
                        let val = self.get_char_const_char() as i64;
                        self.check8(val);
                        result.push(((val + add as i64) & 255) as i32);
                    }
                }
                if self.cur_char() == Some('"') {
                    self.lp += 1;
                }
                if dc && !result.is_empty() {
                    let last = result.len() - 1;
                    result[last] |= 128;
                }
            } else {
                if let Some(val) = self.parse_expression() {
                    self.check8(val);
                    result.push(((val + add as i64) & 255) as i32);
                } else {
                    self.error("Syntax error", None, ErrorKind::Suppres);
                    break;
                }
            }
            self.skip_blanks();
            if self.cur_char() != Some(',') { break; }
            self.lp += 1;
        }
        result
    }

    /// Get filename from current position (handles "...", <...>, or bare name)
    pub fn getfilename(&mut self) -> String {
        self.skip_blanks();
        let mut result = String::new();
        match self.cur_char() {
            Some('"') => {
                self.lp += 1;
                while let Some(c) = self.cur_char() {
                    if c == '"' { self.lp += 1; break; }
                    result.push(c);
                    self.lp += 1;
                }
            }
            Some('<') => {
                while let Some(c) = self.cur_char() {
                    if c == '>' { self.lp += 1; break; }
                    result.push(c);
                    self.lp += 1;
                }
            }
            _ => {
                while let Some(c) = self.cur_char() {
                    if c <= ' ' || c == ',' { break; }
                    result.push(c);
                    self.lp += 1;
                }
            }
        }
        // Normalize path separators
        #[cfg(windows)]
        { result = result.replace('/', "\\"); }
        #[cfg(not(windows))]
        { result = result.replace('\\', "/"); }
        result
    }

    /// Need comma with error
    pub fn need_comma(&mut self) -> bool {
        self.skip_blanks();
        if self.cur_char() != Some(',') {
            self.error("Comma expected", None, ErrorKind::Pass2);
        }
        if self.cur_char() == Some(',') {
            self.lp += 1;
            true
        } else {
            false
        }
    }

    /// Check for struct member identifier
    pub fn get_struct_member_id(&mut self) -> StructMemb {
        if self.cur_char() == Some('#') {
            self.lp += 1;
            if self.cur_char() == Some('#') {
                self.lp += 1;
                return StructMemb::Align;
            }
            return StructMemb::Block;
        }
        let olp = self.lp;
        if self.cmphstr("byte") { return StructMemb::Byte; }
        self.lp = olp;
        if self.cmphstr("word") { return StructMemb::Word; }
        self.lp = olp;
        if self.cmphstr("block") { return StructMemb::Block; }
        self.lp = olp;
        if self.cmphstr("db") { return StructMemb::Byte; }
        self.lp = olp;
        if self.cmphstr("dw") { return StructMemb::Word; }
        self.lp = olp;
        if self.cmphstr("dword") { return StructMemb::DWord; }
        self.lp = olp;
        if self.cmphstr("ds") { return StructMemb::Block; }
        self.lp = olp;
        if self.cmphstr("dd") { return StructMemb::DWord; }
        self.lp = olp;
        if self.cmphstr("align") { return StructMemb::Align; }
        self.lp = olp;
        if self.cmphstr("defs") { return StructMemb::Block; }
        self.lp = olp;
        if self.cmphstr("defb") { return StructMemb::Byte; }
        self.lp = olp;
        if self.cmphstr("defw") { return StructMemb::Word; }
        self.lp = olp;
        if self.cmphstr("defd") { return StructMemb::DWord; }
        self.lp = olp;
        if self.cmphstr("d24") { return StructMemb::D24; }
        self.lp = olp;
        StructMemb::Unknown
    }
}

/// Get numeric value of a hex/decimal digit
fn char_digit_val(c: char) -> Option<u32> {
    if c.is_ascii_digit() {
        Some(c as u32 - '0' as u32)
    } else if c.is_ascii_uppercase() {
        Some(c as u32 - 'A' as u32 + 10)
    } else if c.is_ascii_lowercase() {
        Some(c as u32 - 'a' as u32 + 10)
    } else {
        None
    }
}
