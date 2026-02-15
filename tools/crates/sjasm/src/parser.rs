use crate::assembler::{Assembler, ErrorKind, StructMemb};
use crate::tables::{StructMembI, make_label_name};

impl Assembler {
    // ============================================================
    // Expression parser with operator precedence
    // ============================================================

    pub fn parse_expression(&mut self) -> Option<i64> {
        self.parse_exp_dot()
    }

    fn parse_exp_dot(&mut self) -> Option<i64> {
        let mut left = self.parse_exp_log_or()?;
        while self.need_char(':') {
            let right = self.parse_exp_log_or()?;
            left = left * 256 + right;
        }
        Some(left)
    }

    fn parse_exp_log_or(&mut self) -> Option<i64> {
        let mut left = self.parse_exp_log_and()?;
        while self.need_op("||") != 0 {
            let right = self.parse_exp_log_and()?;
            left = -((left != 0 || right != 0) as i64);
        }
        Some(left)
    }

    fn parse_exp_log_and(&mut self) -> Option<i64> {
        let mut left = self.parse_exp_bit_or()?;
        while self.need_op("&&") != 0 {
            let right = self.parse_exp_bit_or()?;
            left = -((left != 0 && right != 0) as i64);
        }
        Some(left)
    }

    fn parse_exp_bit_or(&mut self) -> Option<i64> {
        let mut left = self.parse_exp_bit_xor()?;
        loop {
            if self.need_op("|_") != 0 || self.needa("or", '|' as i32, None, None) != 0 {
                let right = self.parse_exp_bit_xor()?;
                left |= right;
            } else {
                break;
            }
        }
        Some(left)
    }

    fn parse_exp_bit_xor(&mut self) -> Option<i64> {
        let mut left = self.parse_exp_bit_and()?;
        loop {
            if self.need_op("^ ") != 0 || self.needa("xor", '^' as i32, None, None) != 0 {
                let right = self.parse_exp_bit_and()?;
                left ^= right;
            } else {
                break;
            }
        }
        Some(left)
    }

    fn parse_exp_bit_and(&mut self) -> Option<i64> {
        let mut left = self.parse_exp_equ()?;
        loop {
            if self.need_op("&_") != 0 || self.needa("and", '&' as i32, None, None) != 0 {
                let right = self.parse_exp_equ()?;
                left &= right;
            } else {
                break;
            }
        }
        Some(left)
    }

    fn parse_exp_equ(&mut self) -> Option<i64> {
        let mut left = self.parse_exp_cmp()?;
        loop {
            let op = self.need_op("=_==!=");
            if op == 0 { break; }
            let right = self.parse_exp_cmp()?;
            left = match op {
                x if x == '=' as i32 || x == '=' as i32 + '=' as i32 => -((left == right) as i64),
                x if x == '!' as i32 + '=' as i32 => -((left != right) as i64),
                _ => { self.error("Parser error", None, ErrorKind::Pass2); left }
            };
        }
        Some(left)
    }

    fn parse_exp_cmp(&mut self) -> Option<i64> {
        let mut left = self.parse_exp_min_max()?;
        loop {
            let op = self.need_op("<=>=< > ");
            if op == 0 { break; }
            let right = self.parse_exp_min_max()?;
            left = match op {
                x if x == '<' as i32 => -((left < right) as i64),
                x if x == '>' as i32 => -((left > right) as i64),
                x if x == '<' as i32 + '=' as i32 => -((left <= right) as i64),
                x if x == '>' as i32 + '=' as i32 => -((left >= right) as i64),
                _ => { self.error("Parser error", None, ErrorKind::Pass2); left }
            };
        }
        Some(left)
    }

    fn parse_exp_min_max(&mut self) -> Option<i64> {
        let mut left = self.parse_exp_shift()?;
        loop {
            let op = self.need_op("<?>?");
            if op == 0 { break; }
            let right = self.parse_exp_shift()?;
            left = match op {
                x if x == '<' as i32 + '?' as i32 => left.min(right),
                x if x == '>' as i32 + '?' as i32 => left.max(right),
                _ => { self.error("Parser error", None, ErrorKind::Pass2); left }
            };
        }
        Some(left)
    }

    fn parse_exp_shift(&mut self) -> Option<i64> {
        let mut left = self.parse_exp_add()?;
        loop {
            let mut op = self.need_op("<<>>");
            if op == 0 {
                op = self.needa("shl", '<' as i32 + '<' as i32, Some(("shr", '>' as i32)), None);
                if op == 0 { break; }
            }
            // Check for >>> (unsigned right shift)
            if op == '>' as i32 + '>' as i32 && self.cur_char() == Some('>') {
                self.lp += 1;
                let right = self.parse_exp_add()?;
                left = ((left as u64) >> (right as u32)) as i64;
                continue;
            }
            let right = self.parse_exp_add()?;
            left = match op {
                x if x == '<' as i32 + '<' as i32 => left << right,
                x if x == '>' as i32 + '>' as i32 || x == '>' as i32 => left >> right,
                _ => { self.error("Parser error", None, ErrorKind::Pass2); left }
            };
        }
        Some(left)
    }

    fn parse_exp_add(&mut self) -> Option<i64> {
        let mut left = self.parse_exp_mul()?;
        loop {
            let op = self.need_op("+ - ");
            if op == 0 { break; }
            let right = self.parse_exp_mul()?;
            left = match op as u8 as char {
                '+' => left.wrapping_add(right),
                '-' => left.wrapping_sub(right),
                _ => { self.error("Parser error", None, ErrorKind::Pass2); left }
            };
        }
        Some(left)
    }

    fn parse_exp_mul(&mut self) -> Option<i64> {
        let mut left = self.parse_exp_unary()?;
        loop {
            let mut op = self.need_op("* / % ");
            if op == 0 {
                op = self.needa("mod", '%' as i32, None, None);
                if op == 0 { break; }
            }
            let right = self.parse_exp_unary()?;
            left = match op as u8 as char {
                '*' => left.wrapping_mul(right),
                '/' => if right != 0 { left / right } else { self.error("Division by zero", None, ErrorKind::Pass2); 0 },
                '%' => if right != 0 { left % right } else { self.error("Division by zero", None, ErrorKind::Pass2); 0 },
                _ => { self.error("Parser error", None, ErrorKind::Pass2); left }
            };
        }
        Some(left)
    }

    fn parse_exp_unary(&mut self) -> Option<i64> {
        let op = self.need_op("! ~ + - ");
        if op != 0 {
            let right = self.parse_exp_unary()?;
            return Some(match op as u8 as char {
                '!' => -((!right != 0) as i64),
                '~' => !right,
                '+' => right,
                '-' => right.wrapping_neg(),
                _ => { self.error("Parser error", None, ErrorKind::Pass2); 0 }
            });
        }
        let kw = self.needa("not", '!' as i32, Some(("low", 'l' as i32)), Some(("high", 'h' as i32)));
        if kw != 0 {
            let right = self.parse_exp_unary()?;
            return Some(match kw as u8 as char {
                '!' => -((!right != 0) as i64),
                'l' => right & 255,
                'h' => (right >> 8) & 255,
                _ => { self.error("Parser error", None, ErrorKind::Pass2); 0 }
            });
        }
        self.parse_exp_prim()
    }

    fn parse_exp_prim(&mut self) -> Option<i64> {
        self.skip_blanks();
        let c = self.cur_char()?;

        // Parenthesized expression
        if c == '(' {
            self.lp += 1;
            let val = self.parse_expression()?;
            if !self.need_char(')') {
                self.error("')' expected", None, ErrorKind::Pass2);
            }
            return Some(val);
        }

        // Numeric constants
        if c.is_ascii_digit()
            || (c == '#' && self.peek_char(1).map(|c2| c2.is_ascii_alphanumeric()).unwrap_or(false))
            || (c == '$' && self.peek_char(1).map(|c2| c2.is_ascii_alphanumeric()).unwrap_or(false))
            || c == '%'
            || (c == '&' && self.peek_char(1).map(|c2| c2.is_ascii_alphanumeric()).unwrap_or(false))
        {
            return self.get_constant();
        }

        // Label/identifier
        if c.is_ascii_alphabetic() || c == '_' || c == '.' || c == '@' {
            return self.get_label_value();
        }

        // Bare $ = current address
        if c == '$' {
            self.lp += 1;
            return Some(self.adres);
        }

        // Character constant
        if let Some(val) = self.get_char_const() {
            return Some(val);
        }

        if self.synerr {
            let rem = self.remaining().to_string();
            self.error("Syntax error", Some(&rem), ErrorKind::CatchAll);
        }
        None
    }

    // ============================================================
    // Label value resolution
    // ============================================================

    pub fn get_label_value(&mut self) -> Option<i64> {
        // Try local numeric labels first (e.g., 1b, 2f)
        if let Some(val) = self.get_local_label_value() {
            return Some(val);
        }

        let olp = self.lp;
        let old_lnf = self.labelnotfound;

        // Macro label prefix handling
        let has_mlp = self.macrolabp.is_some();
        if has_mlp {
            let mlp_clone = self.macrolabp.clone();
            let mut use_mlp = true;

            if self.cur_char() == Some('@') {
                self.lp += 1;
                use_mlp = false;
            }

            if use_mlp && self.cur_char() == Some('.') {
                // Local label within macro scope
                self.lp += 1;
                let mut prefix = mlp_clone.unwrap_or_default();
                prefix.push('>');

                let name = self.scan_label_chars();
                if name.is_empty() {
                    self.error("Invalid labelname", None, ErrorKind::Pass2);
                    return None;
                }

                // Try progressively shorter prefixes
                let full = format!("{}{}", prefix, name);
                if let Some(val) = self.labtab.lookup(&full, self.pass) {
                    return Some(val);
                }
                self.labelnotfound = old_lnf;

                // Try with dots
                let mut remaining_prefix = &prefix[..];
                loop {
                    if let Some(pos) = remaining_prefix.find('.') {
                        remaining_prefix = &remaining_prefix[pos + 1..];
                        let try_name = format!("{}{}", remaining_prefix, name);
                        if let Some(val) = self.labtab.lookup(&try_name, self.pass) {
                            return Some(val);
                        }
                        self.labelnotfound = old_lnf;
                    } else if let Some(pos) = remaining_prefix.find('>') {
                        let _ = &remaining_prefix[pos + 1..];
                        break;
                    } else {
                        break;
                    }
                }
            }
        }

        // Reset and try normal label resolution
        self.lp = olp;
        let mut is_global = false;
        let mut is_local = false;

        if self.cur_char() == Some('@') {
            is_global = true;
            self.lp += 1;
        } else if self.cur_char() == Some('.') {
            is_local = true;
            self.lp += 1;
        }

        let mut prefix = String::new();
        if !is_global {
            if let Some(ref mlp) = self.modlabp {
                prefix.push_str(mlp);
                prefix.push('.');
            }
        }
        if is_local {
            prefix.push_str(&self.vorlabp.clone());
            prefix.push('.');
        }

        let name = self.scan_label_chars();
        if name.is_empty() {
            self.error("Invalid labelname", None, ErrorKind::Pass2);
            return None;
        }

        let full = format!("{}{}", prefix, name);
        if let Some(val) = self.labtab.lookup(&full, self.pass) {
            return Some(val);
        }
        self.labelnotfound = old_lnf;

        // Try without module prefix
        if !is_local && !is_global {
            if let Some(val) = self.labtab.lookup(&name, self.pass) {
                return Some(val);
            }
        }

        if self.pass == 2 {
            self.error("Label not found", Some(&full), ErrorKind::Pass2);
        }
        self.labelnotfound = true;
        Some(0)
    }

    /// Try to parse a local numeric label (like 1b, 2f)
    pub fn get_local_label_value(&mut self) -> Option<i64> {
        self.skip_blanks();
        if !self.cur_char().map(|c| c.is_ascii_digit()).unwrap_or(false) {
            return None;
        }

        let start = self.lp;
        let mut num_str = String::new();
        while let Some(c) = self.cur_char() {
            if !c.is_ascii_digit() { break; }
            num_str.push(c);
            self.lp += 1;
        }

        let dir = self.cur_char();
        if dir.is_none() { self.lp = start; return None; }
        let dir = dir.unwrap();

        // Next must be 'b'/'B' or 'f'/'F', and not followed by alphanumeric
        if dir != 'b' && dir != 'B' && dir != 'f' && dir != 'F'  {
            self.lp = start;
            return None;
        }
        self.lp += 1;
        if self.cur_char().map(|c| c.is_ascii_alphanumeric()).unwrap_or(false) {
            self.lp = start;
            return None;
        }

        let nummer: i64 = num_str.parse().unwrap_or(0);
        let gcurlin = self.gcurlin;

        let nval = match dir.to_ascii_lowercase() {
            'b' => self.loklabtab.zoekb(nummer, gcurlin),
            'f' => self.loklabtab.zoekf(nummer, gcurlin),
            _ => None,
        };

        match nval {
            Some(v) => Some(v),
            None => {
                if self.pass == 2 {
                    self.error("Label not found", Some(&num_str), ErrorKind::Suppres);
                }
                Some(0)
            }
        }
    }

    /// Scan label characters at current position
    fn scan_label_chars(&mut self) -> String {
        let start = self.lp;
        while let Some(c) = self.cur_char() {
            if !crate::tables::is_valid_id_char(c) { break; }
            self.lp += 1;
        }
        self.line[start..self.lp].to_string()
    }

    // ============================================================
    // Line parsing
    // ============================================================

    /// Replace defines in the current line
    pub fn replace_defines(&mut self) -> String {
        let mut result = String::new();
        let line = self.line.clone();
        let bytes = line.as_bytes();
        let mut i = 0;
        let mut in_comment = self.comlin > 0;
        let mut comnxtlin = 0i32;

        while i < bytes.len() {
            let c = bytes[i] as char;

            // Block comment handling
            if in_comment || comnxtlin > 0 {
                if c == '*' && i + 1 < bytes.len() && bytes[i + 1] == b'/' {
                    result.push(' ');
                    i += 2;
                    if comnxtlin > 0 { comnxtlin -= 1; } else if in_comment { in_comment = false; }
                    continue;
                }
            }

            // Line comment
            if c == ';' && !in_comment && comnxtlin == 0 {
                break;
            }
            if c == '/' && i + 1 < bytes.len() && bytes[i + 1] == b'/' && !in_comment && comnxtlin == 0 {
                break;
            }
            if c == '/' && i + 1 < bytes.len() && bytes[i + 1] == b'*' {
                i += 2;
                comnxtlin += 1;
                continue;
            }

            // String literals
            if (c == '"' || (!self.compass_compat && c == '\'')) && !in_comment && comnxtlin == 0 {
                // Special case: AF' is not a string literal — it's the alternate register
                // C++ checks: (a!='\'' || (*(lp-2)!='f' || *(lp-3)!='a') && (*(lp-2)!='F' && *(lp-3)!='A'))
                if c == '\'' && i >= 2 {
                    let prev1 = (bytes[i - 1] as char).to_ascii_lowercase();
                    let prev2 = (bytes[i - 2] as char).to_ascii_lowercase();
                    if prev1 == 'f' && prev2 == 'a' {
                        // AF' — treat as normal character, not string delimiter
                        result.push(c);
                        i += 1;
                        continue;
                    }
                }
                result.push(c);
                i += 1;
                while i < bytes.len() {
                    let sc = bytes[i] as char;
                    result.push(sc);
                    if sc == c { i += 1; break; }
                    if sc == '\\' && !self.compass_compat {
                        i += 1;
                        if i < bytes.len() {
                            result.push(bytes[i] as char);
                        }
                    }
                    i += 1;
                }
                continue;
            }

            if in_comment || comnxtlin > 0 {
                if bytes[i] == 0 { break; }
                i += 1;
                continue;
            }

            // Identifier → try define replacement
            if c.is_ascii_alphabetic() || c == '_' {
                let start = i;
                while i < bytes.len() && crate::tables::is_valid_id_char(bytes[i] as char) {
                    i += 1;
                }
                let id = &line[start..i];

                // Try define table
                if let Some(repl) = self.definetab.get(id) {
                    result.push_str(repl);
                    continue;
                }
                // Try macro define table
                if self.macrolabp.is_some() {
                    if let Some(repl) = self.macdeftab.get(id) {
                        result.push_str(repl);
                        continue;
                    }
                }
                result.push_str(id);
                continue;
            }

            result.push(c);
            i += 1;
        }

        self.comnxtlin = comnxtlin;
        result
    }

    /// Parse a line of assembly
    pub fn parse_line(&mut self) {
        if self.compass_compat {
            self.reformat_compass_macro();
        }
        if self.inside_compass_macro_def {
            self.replace_at_to_underscore();
        }

        self.gcurlin += 1;
        self.replace_define_counter = 0;
        self.comnxtlin = 0;

        let replaced = self.replace_defines();
        self.line = replaced;
        self.lp = 0;

        if self.comlin > 0 {
            self.comlin += self.comnxtlin;
            self.list_file_skip();
            return;
        }
        self.comlin += self.comnxtlin;

        if self.remaining().is_empty() {
            self.list_file();
            return;
        }

        self.parse_label();
        if self.skip_blanks() { self.list_file(); return; }
        self.parse_macro();
        if self.skip_blanks() { self.list_file(); return; }
        self.parse_instruction();
        if self.skip_blanks() { self.list_file(); return; }

        if self.cur_char().is_some() {
            let rem = self.remaining().to_string();
            self.error("Unexpected", Some(&rem), ErrorKind::Pass2);
        }
        self.list_file();
    }

    /// Parse a label at the beginning of a line
    fn parse_label(&mut self) {
        if self.white() { return; }

        let mut temp = String::new();
        while let Some(c) = self.cur_char() {
            if c <= ' ' || c == ':' || c == '=' { break; }
            temp.push(c);
            self.lp += 1;
        }
        if self.cur_char() == Some(':') { self.lp += 1; }

        self.skip_blanks();

        self.labelnotfound = false;

        if temp.starts_with(|c: char| c.is_ascii_digit()) {
            // Numeric label
            if self.need_equ() || self.need_field() {
                self.error("Numberlabels only allowed as adresslabels", None, ErrorKind::Pass2);
                return;
            }
            let val: i64 = temp.parse().unwrap_or(0);
            if self.pass == 1 {
                let gcurlin = self.gcurlin;
                self.loklabtab.insert(val, self.adres, gcurlin);
            }
        } else {
            let val;
            if self.need_equ() {
                match self.parse_expression() {
                    Some(v) => val = v,
                    None => {
                        self.error("Expression error", None, ErrorKind::Pass2);
                        return;
                    }
                }
                if self.labelnotfound {
                    self.error("Forward reference", None, ErrorKind::Pass1);
                }
            } else if self.need_field() {
                val = self.mapadr;
                let old_synerr = self.synerr;
                self.synerr = false;
                if let Some(nv) = self.parse_expression() {
                    self.mapadr += nv;
                }
                self.synerr = old_synerr;
                if self.labelnotfound {
                    self.error("Forward reference", None, ErrorKind::Pass1);
                }
            } else {
                // Check for struct instantiation
                let save_lp = self.lp;
                self.skip_blanks();
                let mut gl = false;
                if self.cur_char() == Some('@') {
                    self.lp += 1;
                    gl = true;
                }
                if let Some(n) = self.getid() {
                    let modlabp = self.modlabp.clone();
                    if let Some(st) = self.structtab.lookup(&n, gl, &modlabp) {
                        // Struct emit - emit labels and members
                        let st_clone = st.clone();
                        self.emit_struct_instance(&temp, &st_clone);
                        return;
                    }
                }
                self.lp = save_lp;
                val = self.adres;
            }

            let modlabp = self.modlabp.clone();
            let vorlabp = self.vorlabp.clone();
            let macrolabp = self.macrolabp.clone();
            match make_label_name(&temp, &modlabp, &vorlabp, &macrolabp) {
                Ok((label, new_vorlabp)) => {
                    self.vorlabp = new_vorlabp;
                    if self.pass == 2 {
                        // Verify label has same value
                        if let Some(oval) = self.labtab.lookup(&label, self.pass) {
                            if val != oval {
                                self.error("Label has different value in pass 2", Some(&temp), ErrorKind::Pass2);
                            }
                        } else {
                            self.error("Internal error. ParseLabel()", None, ErrorKind::Fatal);
                        }
                    } else {
                        if !self.labtab.insert(&label, val) {
                            self.error("Duplicate label", Some(&label), ErrorKind::Pass1);
                        }
                    }
                }
                Err(e) => {
                    self.error(&e, None, ErrorKind::Pass2);
                }
            }
        }
    }

    /// Parse macro invocation
    fn parse_macro(&mut self) {
        let save_lp = self.lp;
        let mut gl = false;
        self.skip_blanks();
        if self.cur_char() == Some('@') {
            gl = true;
            self.lp += 1;
        }
        if let Some(n) = self.getid() {
            // Check struct emit
            let modlabp = self.modlabp.clone();
            if let Some(st) = self.structtab.lookup(&n, gl, &modlabp) {
                let st_clone = st.clone();
                self.emit_struct_instance_no_label(&st_clone);
                self.lp = self.line.len(); // consume entire line
                return;
            }
            // Check macro emit
            if !gl {
                if self.macrotab.exists(&n) {
                    self.emit_macro(&n);
                    self.lp = self.line.len();
                    return;
                }
            }
        }
        self.lp = save_lp;
    }

    /// Parse instruction or directive
    fn parse_instruction(&mut self) {
        if self.parse_directive() { return; }
        self.pi_z80();
    }

    // ============================================================
    // Struct emission helpers
    // ============================================================

    fn emit_struct_instance(&mut self, label: &str, st: &crate::tables::StructDef) {
        // Define labels for struct fields
        let modlabp = self.modlabp.clone();
        let vorlabp = self.vorlabp.clone();
        let macrolabp = self.macrolabp.clone();
        let adres = self.adres;
        let pass = self.pass;

        // Main label
        if let Ok((lab, new_vorlabp)) = make_label_name(label, &modlabp, &vorlabp, &macrolabp) {
            self.vorlabp = new_vorlabp;
            if pass == 2 {
                if let Some(oval) = self.labtab.lookup(&lab, pass) {
                    if adres != oval {
                        self.error("Label has different value in pass 2", None, ErrorKind::Pass2);
                    }
                }
            } else {
                if !self.labtab.insert(&lab, adres) {
                    self.error("Duplicate label", Some(&lab), ErrorKind::Pass1);
                }
            }
            // Member labels
            let prefix = format!("{}.", lab);
            for mn in &st.member_names {
                let full = format!("{}{}", prefix, mn.name);
                if pass == 2 {
                    if let Some(oval) = self.labtab.lookup(&full, pass) {
                        if mn.offset + adres != oval {
                            self.error("Label has different value in pass 2", None, ErrorKind::Pass2);
                        }
                    }
                } else {
                    if !self.labtab.insert(&full, mn.offset + adres) {
                        self.error("Duplicate label", Some(&full), ErrorKind::Pass1);
                    }
                }
            }
        }

        // Emit member bytes
        self.emit_struct_members(st);
    }

    fn emit_struct_instance_no_label(&mut self, st: &crate::tables::StructDef) {
        self.emit_struct_members(st);
    }

    fn emit_struct_members(&mut self, st: &crate::tables::StructDef) {
        let mut bytes: Vec<i32> = Vec::new();
        let mut haakjes = 0;

        self.skip_blanks();
        if self.cur_char() == Some('{') {
            haakjes += 1;
            self.lp += 1;
        }

        for item in &st.member_items {
            match item.kind {
                StructMemb::Block => {
                    for _ in 0..item.len {
                        bytes.push(item.def as i32);
                    }
                }
                StructMemb::Byte => {
                    let old_synerr = self.synerr;
                    self.synerr = false;
                    let val = self.parse_expression().unwrap_or(item.def);
                    self.synerr = old_synerr;
                    bytes.push((val % 256) as i32);
                    self.comma();
                }
                StructMemb::Word => {
                    let old_synerr = self.synerr;
                    self.synerr = false;
                    let val = self.parse_expression().unwrap_or(item.def);
                    self.synerr = old_synerr;
                    bytes.push((val % 256) as i32);
                    bytes.push(((val >> 8) % 256) as i32);
                    self.comma();
                }
                StructMemb::D24 => {
                    let old_synerr = self.synerr;
                    self.synerr = false;
                    let val = self.parse_expression().unwrap_or(item.def);
                    self.synerr = old_synerr;
                    bytes.push((val % 256) as i32);
                    bytes.push(((val >> 8) % 256) as i32);
                    bytes.push(((val >> 16) % 256) as i32);
                    self.comma();
                }
                StructMemb::DWord => {
                    let old_synerr = self.synerr;
                    self.synerr = false;
                    let val = self.parse_expression().unwrap_or(item.def);
                    self.synerr = old_synerr;
                    bytes.push((val % 256) as i32);
                    bytes.push(((val >> 8) % 256) as i32);
                    bytes.push(((val >> 16) % 256) as i32);
                    bytes.push(((val >> 24) % 256) as i32);
                    self.comma();
                }
                StructMemb::ParenOpen => {
                    self.skip_blanks();
                    if self.cur_char() == Some('{') {
                        haakjes += 1;
                        self.lp += 1;
                    }
                }
                StructMemb::ParenClose => {
                    self.skip_blanks();
                    if haakjes > 0 && self.cur_char() == Some('}') {
                        haakjes -= 1;
                        self.lp += 1;
                        self.comma();
                    }
                }
                _ => {}
            }
        }
        while haakjes > 0 {
            if !self.need_char('}') {
                self.error("closing } missing", None, ErrorKind::Pass2);
            }
            haakjes -= 1;
        }
        self.emit_bytes_array(&bytes);
    }

    /// Emit macro
    fn emit_macro(&mut self, name: &str) {
        let macro_entry = match self.macrotab.get(name) {
            Some(m) => m.clone(),
            None => return,
        };

        let old_macrolabp = self.macrolabp.clone();
        let labnr = format!("{}", self.macronummer);
        self.macronummer += 1;
        self.macrolabp = Some(if let Some(ref old) = old_macrolabp {
            format!("{}.{}", labnr, old)
        } else {
            self.macdeftab.init();
            labnr
        });

        let old_defs_len = self.macdeftab.save_snapshot();

        // Parse arguments
        let mut args_iter = macro_entry.args.iter();
        while let Some(arg_name) = args_iter.next() {
            self.skip_blanks();
            if self.cur_char().is_none() {
                self.error("Not enough arguments", None, ErrorKind::Pass2);
                break;
            }
            let mut arg_val = String::new();
            if self.cur_char() == Some('<') {
                self.lp += 1;
                while let Some(c) = self.cur_char() {
                    if c == '>' { self.lp += 1; break; }
                    if c == '!' {
                        self.lp += 1;
                        if let Some(c2) = self.cur_char() {
                            arg_val.push(c2);
                            self.lp += 1;
                        }
                    } else {
                        arg_val.push(c);
                        self.lp += 1;
                    }
                }
            } else {
                while let Some(c) = self.cur_char() {
                    if c == ',' { break; }
                    arg_val.push(c);
                    self.lp += 1;
                }
                // Trim trailing whitespace (C++ stores full value but expression parser ignores trailing spaces)
                let trimmed = arg_val.trim_end().to_string();
                arg_val = trimmed;
            }
            self.macdeftab.add(arg_name, &arg_val);
            self.skip_blanks();
            if args_iter.len() > 0 {
                if self.cur_char() != Some(',') {
                    self.error("Not enough arguments", None, ErrorKind::Pass2);
                    break;
                }
                self.lp += 1;
            }
        }

        self.list_file();

        let old_listmacro = self.listmacro;
        self.listmacro = true;
        let old_lijst = self.lijst;
        let old_lijstp = std::mem::take(&mut self.lijstp);
        let old_lijst_idx = self.lijst_idx;

        let old_line = self.line.clone();

        // Set up body as lijst so directives (IF/ELSE/ENDIF etc.) can consume lines from it
        self.lijst = true;
        self.lijstp = macro_entry.body.clone();
        self.lijst_idx = 0;

        while self.lijst_idx < self.lijstp.len() {
            let body_line = self.lijstp[self.lijst_idx].clone();
            self.lijst_idx += 1;
            self.line = body_line;
            self.lp = 0;
            self.parse_line();
        }

        self.line = old_line;
        self.lijst = old_lijst;
        self.lijstp = old_lijstp;
        self.lijst_idx = old_lijst_idx;
        self.macdeftab.restore_snapshot(old_defs_len);
        self.macrolabp = old_macrolabp;
        self.listmacro = old_listmacro;
        self.donotlist = true;
    }

    // ============================================================
    // Compass compatibility helpers
    // ============================================================

    pub fn replace_at_to_underscore(&mut self) {
        self.line = self.line.replace('@', "_");
    }

    fn reformat_compass_macro(&mut self) {
        // Detect Compass-style macro: "LABEL MACRO params"
        let line_bytes = self.line.as_bytes();
        if line_bytes.is_empty() || line_bytes[0] <= b' ' || line_bytes[0] == b';' {
            return;
        }

        let mut i = 0;
        // Skip label
        while i < line_bytes.len() && line_bytes[i] > b' ' && line_bytes[i] != b':' && line_bytes[i] != b';' {
            i += 1;
        }
        if i >= line_bytes.len() || line_bytes[i] == b';' { return; }
        let label_end = i;

        // Skip colons and whitespace
        while i < line_bytes.len() && (line_bytes[i] == b':' || line_bytes[i] == b' ' || line_bytes[i] == b'\t') {
            i += 1;
        }
        if i + 5 > line_bytes.len() { return; }

        // Check for "MACRO"
        let keyword = &self.line[i..].to_ascii_lowercase();
        if !keyword.starts_with("macro") { return; }
        if i + 5 < line_bytes.len() && line_bytes[i + 5] > b' ' { return; }

        let label = self.line[..label_end].to_string();
        let params_start = i + 5;
        let params = self.line[params_start..].to_string();

        self.line = format!(" macro {}{}", label, params);
        self.inside_compass_macro_def = true;
    }

    // ============================================================
    // Struct line parsing (for inside .struct definitions)
    // ============================================================

    pub fn parse_struct_line(&mut self, st_key: &str) {
        self.replace_define_counter = 0;
        self.comnxtlin = 0;

        let replaced = self.replace_defines();
        self.line = replaced;
        self.lp = 0;

        if self.comlin > 0 {
            self.comlin += self.comnxtlin;
            return;
        }
        self.comlin += self.comnxtlin;
        if self.remaining().is_empty() { return; }

        // Parse struct label
        self.prevlab = None;
        if !self.white() {
            if self.cur_char() == Some('.') { self.lp += 1; }
            let mut name = String::new();
            while let Some(c) = self.cur_char() {
                if !crate::tables::is_valid_id_char(c) { break; }
                name.push(c);
                self.lp += 1;
            }
            if self.cur_char() == Some(':') { self.lp += 1; }
            self.skip_blanks();
            if name.starts_with(|c: char| c.is_ascii_digit()) {
                self.error("Numberlabels not allowed within structs", None, ErrorKind::Pass2);
                return;
            }
            self.prevlab = Some(name.clone());

            // Add label to struct
            let modlabp = self.modlabp.clone();
            if let Some(st) = self.structtab.lookup_mut(st_key, false, &modlabp) {
                st.add_label(&name);
            }
        }
        if self.skip_blanks() { return; }

        // Parse struct member instruction
        self.bp = self.lp;
        let memb_kind = self.get_struct_member_id();

        let modlabp = self.modlabp.clone();
        match memb_kind {
            StructMemb::Block => {
                let len = self.parse_expression().unwrap_or(1);
                let val = if self.comma() {
                    self.parse_expression().unwrap_or(0)
                } else { 0 };
                self.check8(val);
                if let Some(st) = self.structtab.lookup_mut(st_key, false, &modlabp) {
                    st.add_memb(StructMembI { offset: st.noffset, len, def: val & 255, kind: StructMemb::Block });
                }
            }
            StructMemb::Byte => {
                let val = self.parse_expression().unwrap_or(0);
                self.check8(val);
                if let Some(st) = self.structtab.lookup_mut(st_key, false, &modlabp) {
                    st.add_memb(StructMembI { offset: st.noffset, len: 1, def: val, kind: StructMemb::Byte });
                }
            }
            StructMemb::Word => {
                let val = self.parse_expression().unwrap_or(0);
                self.check16(val);
                if let Some(st) = self.structtab.lookup_mut(st_key, false, &modlabp) {
                    st.add_memb(StructMembI { offset: st.noffset, len: 2, def: val, kind: StructMemb::Word });
                }
            }
            StructMemb::D24 => {
                let val = self.parse_expression().unwrap_or(0);
                self.check24(val);
                if let Some(st) = self.structtab.lookup_mut(st_key, false, &modlabp) {
                    st.add_memb(StructMembI { offset: st.noffset, len: 3, def: val, kind: StructMemb::D24 });
                }
            }
            StructMemb::DWord => {
                let val = self.parse_expression().unwrap_or(0);
                if let Some(st) = self.structtab.lookup_mut(st_key, false, &modlabp) {
                    st.add_memb(StructMembI { offset: st.noffset, len: 4, def: val, kind: StructMemb::DWord });
                }
            }
            StructMemb::Align => {
                let val = self.parse_expression().unwrap_or(4);
                if let Some(st) = self.structtab.lookup_mut(st_key, false, &modlabp) {
                    st.noffset += (!st.noffset + 1) & (val - 1);
                }
            }
            StructMemb::Unknown => {
                // May be a nested struct
                let save_lp = self.lp;
                let mut gl = false;
                self.skip_blanks();
                if self.cur_char() == Some('@') { self.lp += 1; gl = true; }
                if let Some(n) = self.getid() {
                    let modlabp2 = self.modlabp.clone();
                    if let Some(nested) = self.structtab.lookup(&n, gl, &modlabp2) {
                        let nested_clone = nested.clone();
                        if let Some(st) = self.structtab.lookup_mut(st_key, false, &modlabp) {
                            if let Some(ref prev) = self.prevlab {
                                st.copy_labels_from(&nested_clone, prev);
                            }
                            // Copy members
                            for item in &nested_clone.member_items {
                                st.copy_memb(item, item.def);
                            }
                        }
                        return;
                    }
                }
                self.lp = save_lp;
            }
            _ => {}
        }
    }
}
