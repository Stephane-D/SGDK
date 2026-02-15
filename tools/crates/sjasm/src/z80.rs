use crate::assembler::{Assembler, ErrorKind};

/// Z80 register identifiers
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(i32)]
pub enum Z80Reg {
    B = 0, C = 1, D = 2, E = 3, H = 4, L = 5, A = 7,
    I = 8, R = 9, F = 10,
    BC = 0x10, DE = 0x20, HL = 0x30,
    IXH = 0x31, IXL = 0x32, IYH = 0x33, IYL = 0x34,
    SP = 0x40, AF = 0x50,
    IX = 0xdd, IY = 0xfd,
    UNK = -1,
}

/// Z80 condition codes
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Z80Cond {
    C, M, NC, NZ, P, PE, PO, Z, UNK,
}

impl Assembler {
    // ============================================================
    // Z80 instruction dispatch
    // ============================================================

    pub fn pi_z80(&mut self) {
        let _olp = self.lp;
        self.bp = self.lp;
        if let Some(n) = self.getinstr() {
            let nl = n.to_ascii_lowercase();
            if let Some(&func) = self.z80_tab.get(&nl) {
                func(self);
                return;
            }
            self.error("Unrecognized instruction", Some(&self.line[self.bp..].to_string()), ErrorKind::Pass2);
            self.lp = self.line.len();
        } else {
            // ## alignment
            if self.cur_char() == Some('#') && self.peek_char(1) == Some('#') {
                self.lp += 2;
                let old_synerr = self.synerr;
                self.synerr = false;
                let val = self.parse_expression().unwrap_or(4);
                self.synerr = old_synerr;
                self.mapadr += (!self.mapadr + 1) & (val - 1);
                return;
            }
            let rem = self.remaining().to_string();
            self.error("Unrecognized instruction", Some(&rem), ErrorKind::Pass2);
        }
    }

    // ============================================================
    // Helpers
    // ============================================================

    pub fn z80getbyte(&mut self) -> i32 {
        match self.parse_expression() {
            Some(val) => { self.check8(val); (val & 255) as i32 }
            None => { self.error("Operand expected", None, ErrorKind::Pass2); 0 }
        }
    }

    pub fn z80getword(&mut self) -> i32 {
        match self.parse_expression() {
            Some(val) => { self.check16(val); (val & 65535) as i32 }
            None => { self.error("Operand expected", None, ErrorKind::Pass2); 0 }
        }
    }

    pub fn z80getidxoffset(&mut self) -> i32 {
        self.skip_blanks();
        if self.cur_char() == Some(')') || self.cur_char() == Some(']') {
            return 0;
        }
        match self.parse_expression() {
            Some(val) => { self.check8o(val); (val & 255) as i32 }
            None => { self.error("Operand expected", None, ErrorKind::Pass2); 0 }
        }
    }

    pub fn z80getadres(&mut self) -> Option<i64> {
        if let Some(val) = self.get_local_label_value() {
            return Some(val);
        }
        if let Some(val) = self.parse_expression() {
            return Some(val);
        }
        self.error("Operand expected", None, ErrorKind::CatchAll);
        None
    }

    pub fn getz80cond(&mut self) -> Z80Cond {
        let pp = self.lp;
        self.skip_blanks();
        let c1 = match self.cur_char() {
            Some(c) => c,
            None => { self.lp = pp; return Z80Cond::UNK; }
        };
        self.lp += 1;
        match c1 {
            'n' | 'N' => {
                let c2 = self.cur_char().unwrap_or('\0');
                self.lp += 1;
                let upper = c1 == 'N';
                match if upper { c2 } else { c2 } {
                    c if (c == 'z' || c == 'Z') && !self.is_labchar() => return Z80Cond::NZ,
                    c if (c == 'c' || c == 'C') && !self.is_labchar() => return Z80Cond::NC,
                    c if (c == 's' || c == 'S') && !self.is_labchar() => return Z80Cond::P,
                    _ => {}
                }
            }
            'z' | 'Z' => {
                if !self.is_labchar() { return Z80Cond::Z; }
            }
            'c' | 'C' => {
                if !self.is_labchar() { return Z80Cond::C; }
            }
            'm' | 'M' | 's' | 'S' => {
                if !self.is_labchar() { return Z80Cond::M; }
            }
            'p' | 'P' => {
                if !self.is_labchar() { return Z80Cond::P; }
                let c2 = self.cur_char().unwrap_or('\0');
                self.lp += 1;
                match c2.to_ascii_lowercase() {
                    'e' if !self.is_labchar() => return Z80Cond::PE,
                    'o' if !self.is_labchar() => return Z80Cond::PO,
                    _ => {}
                }
            }
            _ => {}
        }
        self.lp = pp;
        Z80Cond::UNK
    }

    pub fn getz80reg(&mut self) -> Z80Reg {
        let pp = self.lp;
        self.skip_blanks();
        let c1 = match self.cur_char() {
            Some(c) => c,
            None => { self.lp = pp; return Z80Reg::UNK; }
        };
        self.lp += 1;
        match c1 {
            'a' | 'A' => {
                if !self.is_labchar() { return Z80Reg::A; }
                if self.matches_char_no_lab(if c1 == 'a' { 'f' } else { 'F' }) { return Z80Reg::AF; }
            }
            'b' | 'B' => {
                if !self.is_labchar() { return Z80Reg::B; }
                if self.matches_char_no_lab(if c1 == 'b' { 'c' } else { 'C' }) { return Z80Reg::BC; }
            }
            'c' | 'C' => {
                if !self.is_labchar() { return Z80Reg::C; }
            }
            'd' | 'D' => {
                if !self.is_labchar() { return Z80Reg::D; }
                if self.matches_char_no_lab(if c1 == 'd' { 'e' } else { 'E' }) { return Z80Reg::DE; }
            }
            'e' | 'E' => {
                if !self.is_labchar() { return Z80Reg::E; }
            }
            'f' | 'F' => {
                if !self.is_labchar() { return Z80Reg::F; }
            }
            'h' | 'H' => {
                if !self.is_labchar() { return Z80Reg::H; }
                if self.matches_char_no_lab(if c1 == 'h' { 'l' } else { 'L' }) { return Z80Reg::HL; }
            }
            'i' | 'I' => {
                let x_char = if c1 == 'i' { 'x' } else { 'X' };
                let y_char = if c1 == 'i' { 'y' } else { 'Y' };
                let h_char = if c1 == 'i' { 'h' } else { 'H' };
                let l_char = if c1 == 'i' { 'l' } else { 'L' };
                if self.cur_char() == Some(x_char) {
                    if !self.is_labchar_at(1) { self.lp += 1; return Z80Reg::IX; }
                    if self.peek_char(1) == Some(h_char) && !self.is_labchar_at(2) { self.lp += 2; return Z80Reg::IXH; }
                    if self.peek_char(1) == Some(l_char) && !self.is_labchar_at(2) { self.lp += 2; return Z80Reg::IXL; }
                }
                if self.cur_char() == Some(y_char) {
                    if !self.is_labchar_at(1) { self.lp += 1; return Z80Reg::IY; }
                    if self.peek_char(1) == Some(h_char) && !self.is_labchar_at(2) { self.lp += 2; return Z80Reg::IYH; }
                    if self.peek_char(1) == Some(l_char) && !self.is_labchar_at(2) { self.lp += 2; return Z80Reg::IYL; }
                }
                if !self.is_labchar() { return Z80Reg::I; }
            }
            'l' | 'L' => {
                if !self.is_labchar() { return Z80Reg::L; }
            }
            'r' | 'R' => {
                if !self.is_labchar() { return Z80Reg::R; }
            }
            's' | 'S' => {
                if self.matches_char_no_lab(if c1 == 's' { 'p' } else { 'P' }) { return Z80Reg::SP; }
            }
            _ => {}
        }
        self.lp = pp;
        Z80Reg::UNK
    }

    /// Check if current char is a valid label char
    fn is_labchar(&self) -> bool {
        self.cur_char().map(|c| crate::tables::is_valid_id_char(c)).unwrap_or(false)
    }

    /// Check if char at offset from lp is a label char
    fn is_labchar_at(&self, offset: usize) -> bool {
        self.peek_char(offset).map(|c| crate::tables::is_valid_id_char(c)).unwrap_or(false)
    }

    /// Match current char and advance if next is not labchar
    fn matches_char_no_lab(&mut self, c: char) -> bool {
        if self.cur_char() == Some(c) && !self.is_labchar_at(1) {
            self.lp += 1;
            true
        } else {
            false
        }
    }

    /// Check for matching paren - returns lp position after closing paren
    fn getparen(&self, start: usize) -> usize {
        let bytes = self.line.as_bytes();
        let mut i = start;
        // skip leading blanks
        while i < bytes.len() && (bytes[i] == b' ' || bytes[i] == b'\t') {
            i += 1;
        }
        let mut depth = 0;
        while i < bytes.len() {
            match bytes[i] as char {
                '(' => depth += 1,
                ')' => {
                    depth -= 1;
                    if depth == 0 {
                        i += 1;
                        // skip trailing blanks (matches C++ behavior)
                        while i < bytes.len() && (bytes[i] == b' ' || bytes[i] == b'\t') {
                            i += 1;
                        }
                        return i;
                    }
                }
                _ => {}
            }
            i += 1;
        }
        i
    }

    /// needcomma - expect and consume comma
    fn needcomma(&mut self) -> bool {
        self.skip_blanks();
        if self.cur_char() != Some(',') {
            self.error("Comma expected", None, ErrorKind::Pass2);
            return false;
        }
        self.lp += 1;
        true
    }

    // ============================================================
    // ALU instructions (common pattern: A,reg / A,imm / A,(HL) / A,(IX+d) / A,(IY+d))
    // ============================================================

    fn alu_op(&mut self, base_reg: i32, base_ixh: i32, base_mem: i32, base_imm: i32) {
        let mut e = [-1i32; 4];
        let mut reg = self.getz80reg();
        match reg {
            Z80Reg::A => {
                if !self.comma() {
                    e[0] = base_reg + Z80Reg::A as i32;
                    self.emit_bytes_array(&e);
                    return;
                }
                reg = self.getz80reg();
            }
            _ => {}
        }
        match reg {
            Z80Reg::IXH => { e[0] = 0xdd; e[1] = base_ixh; }
            Z80Reg::IXL => { e[0] = 0xdd; e[1] = base_ixh + 1; }
            Z80Reg::IYH => { e[0] = 0xfd; e[1] = base_ixh; }
            Z80Reg::IYL => { e[0] = 0xfd; e[1] = base_ixh + 1; }
            Z80Reg::B | Z80Reg::C | Z80Reg::D | Z80Reg::E |
            Z80Reg::H | Z80Reg::L | Z80Reg::A => {
                e[0] = base_reg + reg as i32;
            }
            Z80Reg::F | Z80Reg::I | Z80Reg::R |
            Z80Reg::AF | Z80Reg::BC | Z80Reg::DE | Z80Reg::HL | Z80Reg::SP |
            Z80Reg::IX | Z80Reg::IY => {}
            _ => {
                reg = Z80Reg::UNK;
                if self.oparen('[') {
                    reg = self.getz80reg();
                    if reg == Z80Reg::UNK { self.emit_bytes_array(&e); return; }
                } else if self.oparen('(') {
                    reg = self.getz80reg();
                    if reg == Z80Reg::UNK { self.lp -= 1; }
                }
                match reg {
                    Z80Reg::HL => {
                        if self.cparen('(') || self.cparen('[') { e[0] = base_mem; }
                    }
                    Z80Reg::IX | Z80Reg::IY => {
                        e[1] = base_mem;
                        e[2] = self.z80getidxoffset();
                        if self.cparen('(') || self.cparen('[') { e[0] = reg as i32; }
                    }
                    _ => {
                        e[0] = base_imm;
                        e[1] = self.z80getbyte();
                    }
                }
            }
        }
        self.emit_bytes_array(&e);
    }

    pub fn piz_adc(&mut self) {
        let mut e = [-1i32; 4];
        let reg = self.getz80reg();
        match reg {
            Z80Reg::HL => {
                if !self.comma() { self.error("Comma expected", None, ErrorKind::Pass2); return; }
                match self.getz80reg() {
                    Z80Reg::BC => { e[0] = 0xed; e[1] = 0x4a; }
                    Z80Reg::DE => { e[0] = 0xed; e[1] = 0x5a; }
                    Z80Reg::HL => { e[0] = 0xed; e[1] = 0x6a; }
                    Z80Reg::SP => { e[0] = 0xed; e[1] = 0x7a; }
                    _ => {}
                }
                self.emit_bytes_array(&e);
            }
            _ => {
                // Reset and use ALU pattern
                if reg == Z80Reg::A {
                    if !self.comma() {
                        self.emit_byte(0x8f);
                        return;
                    }
                    let r = self.getz80reg();
                    self.alu_op_from_reg(r, 0x88, 0x8c, 0x8e, 0xce);
                } else {
                    self.alu_op_from_reg(reg, 0x88, 0x8c, 0x8e, 0xce);
                }
            }
        }
    }

    /// ALU op starting from an already-parsed register
    fn alu_op_from_reg(&mut self, reg: Z80Reg, base_reg: i32, base_ixh: i32, base_mem: i32, base_imm: i32) {
        let mut e = [-1i32; 4];
        match reg {
            Z80Reg::IXH => { e[0] = 0xdd; e[1] = base_ixh; }
            Z80Reg::IXL => { e[0] = 0xdd; e[1] = base_ixh + 1; }
            Z80Reg::IYH => { e[0] = 0xfd; e[1] = base_ixh; }
            Z80Reg::IYL => { e[0] = 0xfd; e[1] = base_ixh + 1; }
            Z80Reg::B | Z80Reg::C | Z80Reg::D | Z80Reg::E |
            Z80Reg::H | Z80Reg::L | Z80Reg::A => {
                e[0] = base_reg + reg as i32;
            }
            Z80Reg::F | Z80Reg::I | Z80Reg::R |
            Z80Reg::AF | Z80Reg::BC | Z80Reg::DE | Z80Reg::HL | Z80Reg::SP |
            Z80Reg::IX | Z80Reg::IY => {}
            _ => {
                let mut reg2 = Z80Reg::UNK;
                if self.oparen('[') {
                    reg2 = self.getz80reg();
                    if reg2 == Z80Reg::UNK { self.emit_bytes_array(&e); return; }
                } else if self.oparen('(') {
                    reg2 = self.getz80reg();
                    if reg2 == Z80Reg::UNK { self.lp -= 1; }
                }
                match reg2 {
                    Z80Reg::HL => {
                        if self.cparen('(') || self.cparen('[') { e[0] = base_mem; }
                    }
                    Z80Reg::IX | Z80Reg::IY => {
                        e[1] = base_mem;
                        e[2] = self.z80getidxoffset();
                        if self.cparen('(') || self.cparen('[') { e[0] = reg2 as i32; }
                    }
                    _ => {
                        e[0] = base_imm;
                        e[1] = self.z80getbyte();
                    }
                }
            }
        }
        self.emit_bytes_array(&e);
    }

    pub fn piz_add(&mut self) {
        let mut e = [-1i32; 4];
        let reg = self.getz80reg();
        match reg {
            Z80Reg::HL => {
                if !self.comma() { self.error("Comma expected", None, ErrorKind::Pass2); return; }
                match self.getz80reg() {
                    Z80Reg::BC => { e[0] = 0x09; }
                    Z80Reg::DE => { e[0] = 0x19; }
                    Z80Reg::HL => { e[0] = 0x29; }
                    Z80Reg::SP => { e[0] = 0x39; }
                    _ => {}
                }
                self.emit_bytes_array(&e);
            }
            Z80Reg::IX => {
                if !self.comma() { self.error("Comma expected", None, ErrorKind::Pass2); return; }
                match self.getz80reg() {
                    Z80Reg::BC => { e[0] = 0xdd; e[1] = 0x09; }
                    Z80Reg::DE => { e[0] = 0xdd; e[1] = 0x19; }
                    Z80Reg::IX => { e[0] = 0xdd; e[1] = 0x29; }
                    Z80Reg::SP => { e[0] = 0xdd; e[1] = 0x39; }
                    _ => {}
                }
                self.emit_bytes_array(&e);
            }
            Z80Reg::IY => {
                if !self.comma() { self.error("Comma expected", None, ErrorKind::Pass2); return; }
                match self.getz80reg() {
                    Z80Reg::BC => { e[0] = 0xfd; e[1] = 0x09; }
                    Z80Reg::DE => { e[0] = 0xfd; e[1] = 0x19; }
                    Z80Reg::IY => { e[0] = 0xfd; e[1] = 0x29; }
                    Z80Reg::SP => { e[0] = 0xfd; e[1] = 0x39; }
                    _ => {}
                }
                self.emit_bytes_array(&e);
            }
            Z80Reg::A => {
                if !self.comma() {
                    self.emit_byte(0x87);
                    return;
                }
                let r = self.getz80reg();
                self.alu_op_from_reg(r, 0x80, 0x84, 0x86, 0xc6);
            }
            _ => {
                self.alu_op_from_reg(reg, 0x80, 0x84, 0x86, 0xc6);
            }
        }
    }

    pub fn piz_and(&mut self) { self.alu_op(0xa0, 0xa4, 0xa6, 0xe6); }
    pub fn piz_cp(&mut self)  { self.alu_op(0xb8, 0xbc, 0xbe, 0xfe); }
    pub fn piz_or(&mut self)  { self.alu_op(0xb0, 0xb4, 0xb6, 0xf6); }
    pub fn piz_xor(&mut self) { self.alu_op(0xa8, 0xac, 0xae, 0xee); }

    pub fn piz_sub(&mut self) {
        let mut e = [-1i32; 4];
        let reg = self.getz80reg();
        match reg {
            Z80Reg::HL => {
                if !self.needcomma() { self.emit_bytes_array(&e); return; }
                match self.getz80reg() {
                    Z80Reg::BC => { e[0] = 0xb7; e[1] = 0xed; e[2] = 0x42; }
                    Z80Reg::DE => { e[0] = 0xb7; e[1] = 0xed; e[2] = 0x52; }
                    Z80Reg::HL => { e[0] = 0xb7; e[1] = 0xed; e[2] = 0x62; }
                    Z80Reg::SP => { e[0] = 0xb7; e[1] = 0xed; e[2] = 0x72; }
                    _ => {}
                }
                self.emit_bytes_array(&e);
            }
            Z80Reg::A => {
                if !self.comma() {
                    self.emit_byte(0x97);
                    return;
                }
                let r = self.getz80reg();
                self.alu_op_from_reg(r, 0x90, 0x94, 0x96, 0xd6);
            }
            _ => {
                self.alu_op_from_reg(reg, 0x90, 0x94, 0x96, 0xd6);
            }
        }
    }

    pub fn piz_sbc(&mut self) {
        let mut e = [-1i32; 4];
        let reg = self.getz80reg();
        match reg {
            Z80Reg::HL => {
                if !self.comma() { self.error("Comma expected", None, ErrorKind::Pass2); return; }
                match self.getz80reg() {
                    Z80Reg::BC => { e[0] = 0xed; e[1] = 0x42; }
                    Z80Reg::DE => { e[0] = 0xed; e[1] = 0x52; }
                    Z80Reg::HL => { e[0] = 0xed; e[1] = 0x62; }
                    Z80Reg::SP => { e[0] = 0xed; e[1] = 0x72; }
                    _ => {}
                }
                self.emit_bytes_array(&e);
            }
            Z80Reg::A => {
                if !self.comma() {
                    self.emit_byte(0x9f);
                    return;
                }
                let r = self.getz80reg();
                self.alu_op_from_reg(r, 0x98, 0x9c, 0x9e, 0xde);
            }
            _ => {
                self.alu_op_from_reg(reg, 0x98, 0x9c, 0x9e, 0xde);
            }
        }
    }

    // ============================================================
    // BIT/RES/SET
    // ============================================================

    fn bit_res_set(&mut self, base: i32) {
        let mut e = [-1i32; 5];
        let bit = self.z80getbyte();
        let valid_bit = if !self.comma() { -1 } else { bit };
        let reg = self.getz80reg();
        match reg {
            Z80Reg::B | Z80Reg::C | Z80Reg::D | Z80Reg::E |
            Z80Reg::H | Z80Reg::L | Z80Reg::A => {
                e[0] = 0xcb; e[1] = 8 * valid_bit + base + reg as i32;
            }
            _ => {
                if !self.oparen('[') && !self.oparen('(') {
                    self.emit_bytes_array(&e);
                    return;
                }
                let reg2 = self.getz80reg();
                match reg2 {
                    Z80Reg::HL => {
                        if self.cparen('(') || self.cparen('[') { e[0] = 0xcb; }
                        e[1] = 8 * valid_bit + base + 6;
                    }
                    Z80Reg::IX | Z80Reg::IY => {
                        e[1] = 0xcb;
                        e[2] = self.z80getidxoffset();
                        e[3] = 8 * valid_bit + base + 6;
                        if self.cparen('(') || self.cparen('[') { e[0] = reg2 as i32; }
                        // RES/SET can have an extra register target
                        if base != 0x40 && self.comma() {
                            let extra = self.getz80reg();
                            match extra {
                                Z80Reg::B | Z80Reg::C | Z80Reg::D | Z80Reg::E |
                                Z80Reg::H | Z80Reg::L | Z80Reg::A => {
                                    e[3] = 8 * valid_bit + base + extra as i32;
                                }
                                _ => {
                                    let rem = self.remaining().to_string();
                                    self.error("Illegal operand", Some(&rem), ErrorKind::Suppres);
                                }
                            }
                        }
                    }
                    _ => {}
                }
            }
        }
        if valid_bit < 0 || valid_bit > 7 { e[0] = -1; }
        self.emit_bytes_array(&e);
    }

    pub fn piz_bit(&mut self) { self.bit_res_set(0x40); }
    pub fn piz_res(&mut self) { self.bit_res_set(0x80); }
    pub fn piz_set(&mut self) { self.bit_res_set(0xc0); }

    // ============================================================
    // Shift/rotate instructions (common pattern)
    // ============================================================

    fn shift_op(&mut self, base: i32, has_16bit: bool, hl_special: Option<i32>) {
        let mut e = [-1i32; 5];
        let reg = self.getz80reg();
        match reg {
            Z80Reg::B | Z80Reg::C | Z80Reg::D | Z80Reg::E |
            Z80Reg::H | Z80Reg::L | Z80Reg::A => {
                e[0] = 0xcb; e[1] = base + reg as i32;
            }
            Z80Reg::BC if has_16bit => {
                e[0] = 0xcb; e[2] = 0xcb;
                e[1] = base + 1; e[3] = base + 0x10 - base + if base >= 0x18 { 1 } else { -0x10 + 0x10 };
                // BC: low byte first, then high with carry
                // For SLA: CB 21, CB 10 (SLA C, RL B)
                // For SRL: CB 38, CB 19 (SRL B, RR C)  
                // For SRA: CB 28, CB 19 (SRA B, RR C)
                // For SLL: CB 31, CB 10 (SLL C(?), RL B)
                // For RL: CB 11, CB 10 (RL C, RL B)
                // For RR: CB 18, CB 19 (RR B, RR C)
                // This is complex. Let me match the C++ more precisely.
                match base {
                    0x20 => { e[1] = 0x21; e[3] = 0x10; } // SLA C, RL B
                    0x28 => { e[1] = 0x28; e[3] = 0x19; } // SRA B, RR C
                    0x30 => { e[1] = 0x31; e[3] = 0x10; } // SLL C, RL B
                    0x38 => { e[1] = 0x38; e[3] = 0x19; } // SRL B, RR C
                    0x10 => { e[1] = 0x11; e[3] = 0x10; } // RL C, RL B
                    0x18 => { e[1] = 0x18; e[3] = 0x19; } // RR B, RR C
                    _ => { e[0] = -1; }
                }
            }
            Z80Reg::DE if has_16bit => {
                e[0] = 0xcb; e[2] = 0xcb;
                match base {
                    0x20 => { e[1] = 0x23; e[3] = 0x12; }
                    0x28 => { e[1] = 0x2a; e[3] = 0x1b; }
                    0x30 => { e[1] = 0x33; e[3] = 0x12; }
                    0x38 => { e[1] = 0x3a; e[3] = 0x1b; }
                    0x10 => { e[1] = 0x13; e[3] = 0x12; }
                    0x18 => { e[1] = 0x1a; e[3] = 0x1b; }
                    _ => { e[0] = -1; }
                }
            }
            Z80Reg::HL if has_16bit => {
                // Some ops have special HL handling
                if let Some(special) = hl_special {
                    e[0] = special;
                } else {
                    e[0] = 0xcb; e[2] = 0xcb;
                    match base {
                        0x20 => { e[1] = 0x25; e[3] = 0x14; }
                        0x28 => { e[1] = 0x2c; e[3] = 0x1d; }
                        0x30 => { e[1] = 0x35; e[3] = 0x14; }
                        0x38 => { e[1] = 0x3c; e[3] = 0x1d; }
                        0x10 => { e[1] = 0x15; e[3] = 0x14; }
                        0x18 => { e[1] = 0x1c; e[3] = 0x1d; }
                        _ => { e[0] = -1; }
                    }
                }
            }
            _ => {
                if !self.oparen('[') && !self.oparen('(') {
                    self.emit_bytes_array(&e);
                    return;
                }
                let reg2 = self.getz80reg();
                match reg2 {
                    Z80Reg::HL => {
                        if self.cparen('(') || self.cparen('[') { e[0] = 0xcb; }
                        e[1] = base + 6;
                    }
                    Z80Reg::IX | Z80Reg::IY => {
                        e[1] = 0xcb;
                        e[2] = self.z80getidxoffset();
                        e[3] = base + 6;
                        if self.cparen('(') || self.cparen('[') { e[0] = reg2 as i32; }
                        if self.comma() {
                            let extra = self.getz80reg();
                            match extra {
                                Z80Reg::B | Z80Reg::C | Z80Reg::D | Z80Reg::E |
                                Z80Reg::H | Z80Reg::L | Z80Reg::A => {
                                    e[3] = base + extra as i32;
                                }
                                _ => {
                                    let rem = self.remaining().to_string();
                                    self.error("Illegal operand", Some(&rem), ErrorKind::Suppres);
                                }
                            }
                        }
                    }
                    _ => {}
                }
            }
        }
        self.emit_bytes_array(&e);
    }

    pub fn piz_rl(&mut self)  { self.shift_op(0x10, true, None); }
    pub fn piz_rlc(&mut self) { self.shift_op(0x00, false, None); }
    pub fn piz_rr(&mut self)  { self.shift_op(0x18, true, None); }
    pub fn piz_rrc(&mut self) { self.shift_op(0x08, false, None); }
    pub fn piz_sla(&mut self) { self.shift_op(0x20, true, Some(0x29)); } // SLA HL = ADD HL,HL
    pub fn piz_sll(&mut self) { self.shift_op(0x30, true, None); }
    pub fn piz_sra(&mut self) { self.shift_op(0x28, true, None); }
    pub fn piz_srl(&mut self) { self.shift_op(0x38, true, None); }

    // ============================================================
    // INC/DEC
    // ============================================================

    pub fn piz_inc(&mut self) {
        let mut e = [-1i32; 4];
        match self.getz80reg() {
            Z80Reg::A  => e[0] = 0x3c,
            Z80Reg::B  => e[0] = 0x04,
            Z80Reg::BC => e[0] = 0x03,
            Z80Reg::C  => e[0] = 0x0c,
            Z80Reg::D  => e[0] = 0x14,
            Z80Reg::DE => e[0] = 0x13,
            Z80Reg::E  => e[0] = 0x1c,
            Z80Reg::H  => e[0] = 0x24,
            Z80Reg::HL => e[0] = 0x23,
            Z80Reg::IX => { e[0] = 0xdd; e[1] = 0x23; }
            Z80Reg::IY => { e[0] = 0xfd; e[1] = 0x23; }
            Z80Reg::L  => e[0] = 0x2c,
            Z80Reg::SP => e[0] = 0x33,
            Z80Reg::IXH => { e[0] = 0xdd; e[1] = 0x24; }
            Z80Reg::IXL => { e[0] = 0xdd; e[1] = 0x2c; }
            Z80Reg::IYH => { e[0] = 0xfd; e[1] = 0x24; }
            Z80Reg::IYL => { e[0] = 0xfd; e[1] = 0x2c; }
            _ => {
                if !self.oparen('[') && !self.oparen('(') { self.emit_bytes_array(&e); return; }
                let reg = self.getz80reg();
                match reg {
                    Z80Reg::HL => { if self.cparen('(') || self.cparen('[') { e[0] = 0x34; } }
                    Z80Reg::IX | Z80Reg::IY => {
                        e[1] = 0x34; e[2] = self.z80getidxoffset();
                        if self.cparen('(') || self.cparen('[') { e[0] = reg as i32; }
                    }
                    _ => {}
                }
            }
        }
        self.emit_bytes_array(&e);
    }

    pub fn piz_dec(&mut self) {
        let mut e = [-1i32; 4];
        match self.getz80reg() {
            Z80Reg::A  => e[0] = 0x3d,
            Z80Reg::B  => e[0] = 0x05,
            Z80Reg::BC => e[0] = 0x0b,
            Z80Reg::C  => e[0] = 0x0d,
            Z80Reg::D  => e[0] = 0x15,
            Z80Reg::DE => e[0] = 0x1b,
            Z80Reg::E  => e[0] = 0x1d,
            Z80Reg::H  => e[0] = 0x25,
            Z80Reg::HL => e[0] = 0x2b,
            Z80Reg::IX => { e[0] = 0xdd; e[1] = 0x2b; }
            Z80Reg::IY => { e[0] = 0xfd; e[1] = 0x2b; }
            Z80Reg::L  => e[0] = 0x2d,
            Z80Reg::SP => e[0] = 0x3b,
            Z80Reg::IXH => { e[0] = 0xdd; e[1] = 0x25; }
            Z80Reg::IXL => { e[0] = 0xdd; e[1] = 0x2d; }
            Z80Reg::IYH => { e[0] = 0xfd; e[1] = 0x25; }
            Z80Reg::IYL => { e[0] = 0xfd; e[1] = 0x2d; }
            _ => {
                if !self.oparen('[') && !self.oparen('(') { self.emit_bytes_array(&e); return; }
                let reg = self.getz80reg();
                match reg {
                    Z80Reg::HL => { if self.cparen('(') || self.cparen('[') { e[0] = 0x35; } }
                    Z80Reg::IX | Z80Reg::IY => {
                        e[1] = 0x35; e[2] = self.z80getidxoffset();
                        if self.cparen('(') || self.cparen('[') { e[0] = reg as i32; }
                    }
                    _ => {}
                }
            }
        }
        self.emit_bytes_array(&e);
    }

    // ============================================================
    // CALL / JP / JR / DJNZ / RET / RST
    // ============================================================

    pub fn piz_call(&mut self) {
        let mut e = [-1i32; 4];
        match self.getz80cond() {
            Z80Cond::C  => { if self.comma() { e[0] = 0xdc; } }
            Z80Cond::M  => { if self.comma() { e[0] = 0xfc; } }
            Z80Cond::NC => { if self.comma() { e[0] = 0xd4; } }
            Z80Cond::NZ => { if self.comma() { e[0] = 0xc4; } }
            Z80Cond::P  => { if self.comma() { e[0] = 0xf4; } }
            Z80Cond::PE => { if self.comma() { e[0] = 0xec; } }
            Z80Cond::PO => { if self.comma() { e[0] = 0xe4; } }
            Z80Cond::Z  => { if self.comma() { e[0] = 0xcc; } }
            Z80Cond::UNK => { e[0] = 0xcd; }
        }
        let callad = self.z80getadres().unwrap_or(0);
        e[1] = (callad & 255) as i32;
        e[2] = ((callad >> 8) & 255) as i32;
        if callad > 65535 { self.error("Bytes lost", None, ErrorKind::Pass2); }
        self.emit_bytes_array(&e);
    }

    pub fn piz_jp(&mut self) {
        let mut e = [-1i32; 4];
        let mut k = false;
        match self.getz80cond() {
            Z80Cond::C  => { if self.comma() { e[0] = 0xda; } }
            Z80Cond::M  => { if self.comma() { e[0] = 0xfa; } }
            Z80Cond::NC => { if self.comma() { e[0] = 0xd2; } }
            Z80Cond::NZ => { if self.comma() { e[0] = 0xc2; } }
            Z80Cond::P  => { if self.comma() { e[0] = 0xf2; } }
            Z80Cond::PE => { if self.comma() { e[0] = 0xea; } }
            Z80Cond::PO => { if self.comma() { e[0] = 0xe2; } }
            Z80Cond::Z  => { if self.comma() { e[0] = 0xca; } }
            Z80Cond::UNK => {
                let mut reg = Z80Reg::UNK;
                let mut haakjes = false;
                if self.oparen('[') {
                    reg = self.getz80reg();
                    if reg == Z80Reg::UNK { self.emit_bytes_array(&e); return; }
                    haakjes = true;
                } else if self.oparen('(') {
                    reg = self.getz80reg();
                    if reg == Z80Reg::UNK { self.lp -= 1; } else { haakjes = true; }
                }
                if reg == Z80Reg::UNK { reg = self.getz80reg(); }
                match reg {
                    Z80Reg::HL => {
                        if haakjes && !(self.cparen('(') || self.cparen('[')) { self.emit_bytes_array(&e); return; }
                        e[0] = 0xe9; k = true;
                    }
                    Z80Reg::IX | Z80Reg::IY => {
                        e[1] = 0xe9;
                        if haakjes && !(self.cparen('(') || self.cparen('[')) { self.emit_bytes_array(&e); return; }
                        e[0] = reg as i32; k = true;
                    }
                    _ => { e[0] = 0xc3; }
                }
            }
        }
        if !k {
            let jpad = self.z80getadres().unwrap_or(0);
            e[1] = (jpad & 255) as i32;
            e[2] = ((jpad >> 8) & 255) as i32;
            if jpad > 65535 { self.error("Bytes lost", None, ErrorKind::Pass2); }
        }
        self.emit_bytes_array(&e);
    }

    pub fn piz_jr(&mut self) {
        let mut e = [-1i32; 4];
        match self.getz80cond() {
            Z80Cond::C  => { if self.comma() { e[0] = 0x38; } }
            Z80Cond::NC => { if self.comma() { e[0] = 0x30; } }
            Z80Cond::NZ => { if self.comma() { e[0] = 0x20; } }
            Z80Cond::Z  => { if self.comma() { e[0] = 0x28; } }
            Z80Cond::M | Z80Cond::P | Z80Cond::PE | Z80Cond::PO => {
                self.error("Illegal condition", None, ErrorKind::Pass2);
            }
            Z80Cond::UNK => { e[0] = 0x18; }
        }
        let adres = self.adres;
        let jrad = self.z80getadres().unwrap_or(adres + 2);
        let jmp = (jrad - adres - 2) as i32;
        if jmp < -128 || jmp > 127 {
            self.error(&format!("Target out of range ({})", jmp), None, ErrorKind::Pass2);
        }
        e[1] = if jmp < 0 { 256 + jmp } else { jmp };
        self.emit_bytes_array(&e);
    }

    pub fn piz_djnz(&mut self) {
        let mut e = [-1i32; 3];
        let adres = self.adres;
        let nad = self.z80getadres().unwrap_or(adres + 2);
        let jmp = (nad - adres - 2) as i32;
        if jmp < -128 || jmp > 127 {
            self.error(&format!("Target out of range ({})", jmp), None, ErrorKind::Pass2);
        }
        e[0] = 0x10;
        e[1] = if jmp < 0 { 256 + jmp } else { jmp };
        self.emit_bytes_array(&e);
    }

    pub fn piz_ret(&mut self) {
        let e = match self.getz80cond() {
            Z80Cond::C  => 0xd8,
            Z80Cond::M  => 0xf8,
            Z80Cond::NC => 0xd0,
            Z80Cond::NZ => 0xc0,
            Z80Cond::P  => 0xf0,
            Z80Cond::PE => 0xe8,
            Z80Cond::PO => 0xe0,
            Z80Cond::Z  => 0xc8,
            Z80Cond::UNK => 0xc9,
        };
        self.emit_byte(e);
    }

    pub fn piz_rst(&mut self) {
        let e = match self.z80getbyte() {
            0x00 => 0xc7, 0x08 => 0xcf, 0x10 => 0xd7, 0x18 => 0xdf,
            0x20 => 0xe7, 0x28 => 0xef, 0x30 => 0xf7, 0x38 => 0xff,
            _ => {
                let line = self.line.clone();
                self.error("Illegal operand", Some(&line), ErrorKind::Pass2);
                self.lp = self.line.len();
                return;
            }
        };
        self.emit_byte(e);
    }

    // ============================================================
    // Simple instructions
    // ============================================================

    pub fn piz_ccf(&mut self)  { self.emit_byte(0x3f); }
    pub fn piz_cpl(&mut self)  { self.emit_byte(0x2f); }
    pub fn piz_daa(&mut self)  { self.emit_byte(0x27); }
    pub fn piz_di(&mut self)   { self.emit_byte(0xf3); }
    pub fn piz_ei(&mut self)   { self.emit_byte(0xfb); }
    pub fn piz_exx(&mut self)  { self.emit_byte(0xd9); }
    pub fn piz_halt(&mut self) { self.emit_byte(0x76); }
    pub fn piz_nop(&mut self)  { self.emit_byte(0x00); }
    pub fn piz_rla(&mut self)  { self.emit_byte(0x17); }
    pub fn piz_rlca(&mut self) { self.emit_byte(0x07); }
    pub fn piz_rra(&mut self)  { self.emit_byte(0x1f); }
    pub fn piz_rrca(&mut self) { self.emit_byte(0x0f); }
    pub fn piz_scf(&mut self)  { self.emit_byte(0x37); }

    // ED-prefixed simple instructions
    fn emit_ed(&mut self, byte: i32) {
        let e = [0xed, byte, -1];
        self.emit_bytes_array(&e);
    }

    pub fn piz_cpd(&mut self)  { self.emit_ed(0xa9); }
    pub fn piz_cpdr(&mut self) { self.emit_ed(0xb9); }
    pub fn piz_cpi(&mut self)  { self.emit_ed(0xa1); }
    pub fn piz_cpir(&mut self) { self.emit_ed(0xb1); }
    pub fn piz_ind(&mut self)  { self.emit_ed(0xaa); }
    pub fn piz_indr(&mut self) { self.emit_ed(0xba); }
    pub fn piz_ini(&mut self)  { self.emit_ed(0xa2); }
    pub fn piz_inir(&mut self) { self.emit_ed(0xb2); }
    pub fn piz_ldir(&mut self) { self.emit_ed(0xb0); }
    pub fn piz_lddr(&mut self) { self.emit_ed(0xb8); }
    pub fn piz_neg(&mut self)  { self.emit_ed(0x44); }
    pub fn piz_otdr(&mut self) { self.emit_ed(0xbb); }
    pub fn piz_otir(&mut self) { self.emit_ed(0xb3); }
    pub fn piz_outd(&mut self) { self.emit_ed(0xab); }
    pub fn piz_outi(&mut self) { self.emit_ed(0xa3); }
    pub fn piz_reti(&mut self) { self.emit_ed(0x4d); }
    pub fn piz_retn(&mut self) { self.emit_ed(0x45); }
    pub fn piz_rld(&mut self)  { self.emit_ed(0x6f); }
    pub fn piz_rrd(&mut self)  { self.emit_ed(0x67); }

    // ============================================================
    // IM
    // ============================================================

    pub fn piz_im(&mut self) {
        let mut e = [0xed, -1i32, -1];
        match self.z80getbyte() {
            0 => e[1] = 0x46,
            1 => e[1] = 0x56,
            2 => e[1] = 0x5e,
            _ => e[0] = -1,
        }
        self.emit_bytes_array(&e);
    }

    // ============================================================
    // EX
    // ============================================================

    pub fn piz_ex(&mut self) {
        let mut e = [-1i32; 4];
        match self.getz80reg() {
            Z80Reg::AF => {
                if self.comma() {
                    if self.getz80reg() == Z80Reg::AF {
                        if self.cur_char() == Some('\'') { self.lp += 1; }
                    }
                }
                e[0] = 0x08;
            }
            Z80Reg::DE => {
                if !self.comma() { self.error("Comma expected", None, ErrorKind::Pass2); self.emit_bytes_array(&e); return; }
                if self.getz80reg() != Z80Reg::HL { self.emit_bytes_array(&e); return; }
                e[0] = 0xeb;
            }
            _ => {
                if !self.oparen('[') && !self.oparen('(') { self.emit_bytes_array(&e); return; }
                if self.getz80reg() != Z80Reg::SP { self.emit_bytes_array(&e); return; }
                if !(self.cparen('(') || self.cparen('[')) { self.emit_bytes_array(&e); return; }
                if !self.comma() { self.error("Comma expected", None, ErrorKind::Pass2); self.emit_bytes_array(&e); return; }
                let reg = self.getz80reg();
                match reg {
                    Z80Reg::HL => e[0] = 0xe3,
                    Z80Reg::IX | Z80Reg::IY => { e[0] = reg as i32; e[1] = 0xe3; }
                    _ => {}
                }
            }
        }
        self.emit_bytes_array(&e);
    }

    // ============================================================
    // IN / OUT
    // ============================================================

    pub fn piz_in(&mut self) {
        let mut e = [-1i32; 3];
        let reg = self.getz80reg();
        match reg {
            Z80Reg::A => {
                if !self.comma() { self.emit_bytes_array(&e); return; }
                if !self.oparen('[') && !self.oparen('(') { self.emit_bytes_array(&e); return; }
                if self.getz80reg() == Z80Reg::C {
                    e[1] = 0x78;
                    if self.cparen('(') || self.cparen('[') { e[0] = 0xed; }
                } else {
                    e[1] = self.z80getbyte();
                    if self.cparen('(') || self.cparen('[') { e[0] = 0xdb; }
                }
            }
            Z80Reg::B | Z80Reg::C | Z80Reg::D | Z80Reg::E |
            Z80Reg::H | Z80Reg::L | Z80Reg::F => {
                if !self.comma() { self.emit_bytes_array(&e); return; }
                if !self.oparen('[') && !self.oparen('(') { self.emit_bytes_array(&e); return; }
                if self.getz80reg() != Z80Reg::C { self.emit_bytes_array(&e); return; }
                if self.cparen('(') || self.cparen('[') { e[0] = 0xed; }
                e[1] = match reg {
                    Z80Reg::B => 0x40, Z80Reg::C => 0x48, Z80Reg::D => 0x50,
                    Z80Reg::E => 0x58, Z80Reg::H => 0x60, Z80Reg::L => 0x68,
                    Z80Reg::F => 0x70, _ => -1,
                };
            }
            _ => {
                if !self.oparen('[') && !self.oparen('(') { self.emit_bytes_array(&e); return; }
                if self.getz80reg() != Z80Reg::C { self.emit_bytes_array(&e); return; }
                if self.cparen('(') || self.cparen('[') { e[0] = 0xed; }
                e[1] = 0x70;
            }
        }
        self.emit_bytes_array(&e);
    }

    pub fn piz_out(&mut self) {
        let mut e = [-1i32; 3];
        if self.oparen('[') || self.oparen('(') {
            if self.getz80reg() == Z80Reg::C {
                if (self.cparen('(') || self.cparen('[')) && self.comma() {
                    let reg = self.getz80reg();
                    match reg {
                        Z80Reg::A => { e[0] = 0xed; e[1] = 0x79; }
                        Z80Reg::B => { e[0] = 0xed; e[1] = 0x41; }
                        Z80Reg::C => { e[0] = 0xed; e[1] = 0x49; }
                        Z80Reg::D => { e[0] = 0xed; e[1] = 0x51; }
                        Z80Reg::E => { e[0] = 0xed; e[1] = 0x59; }
                        Z80Reg::H => { e[0] = 0xed; e[1] = 0x61; }
                        Z80Reg::L => { e[0] = 0xed; e[1] = 0x69; }
                        _ => {
                            self.z80getbyte(); // consume but discard
                            e[0] = 0xed; e[1] = 0x71;
                        }
                    }
                }
            } else {
                e[1] = self.z80getbyte();
                if (self.cparen('(') || self.cparen('[')) && self.comma() {
                    if self.getz80reg() == Z80Reg::A { e[0] = 0xd3; }
                }
            }
        }
        self.emit_bytes_array(&e);
    }

    // ============================================================
    // PUSH / POP
    // ============================================================

    pub fn piz_push(&mut self) {
        let mut e = Vec::with_capacity(30);
        let mut ok = true;
        while ok {
            match self.getz80reg() {
                Z80Reg::AF => e.push(0xf5),
                Z80Reg::BC => e.push(0xc5),
                Z80Reg::DE => e.push(0xd5),
                Z80Reg::HL => e.push(0xe5),
                Z80Reg::IX => { e.push(0xdd); e.push(0xe5); }
                Z80Reg::IY => { e.push(0xfd); e.push(0xe5); }
                _ => { ok = false; }
            }
            if !self.comma() || e.len() > 27 { ok = false; }
        }
        e.push(-1);
        self.emit_bytes_array(&e);
    }

    pub fn piz_pop(&mut self) {
        if self.compass_compat {
            self.piz_pop_reversed();
        } else {
            self.piz_pop_original();
        }
    }

    fn piz_pop_original(&mut self) {
        // Original: emits in reverse order (last parsed first)
        let mut stack = Vec::new();
        let mut ok = true;
        while ok {
            match self.getz80reg() {
                Z80Reg::AF => stack.push(vec![0xf1]),
                Z80Reg::BC => stack.push(vec![0xc1]),
                Z80Reg::DE => stack.push(vec![0xd1]),
                Z80Reg::HL => stack.push(vec![0xe1]),
                Z80Reg::IX => stack.push(vec![0xdd, 0xe1]),
                Z80Reg::IY => stack.push(vec![0xfd, 0xe1]),
                _ => { ok = false; }
            }
            if !self.comma() || stack.len() > 14 { ok = false; }
        }
        let mut e = Vec::new();
        for entry in stack.into_iter().rev() {
            e.extend(entry);
        }
        e.push(-1);
        self.emit_bytes_array(&e);
    }

    fn piz_pop_reversed(&mut self) {
        let mut e = Vec::with_capacity(30);
        let mut ok = true;
        while ok {
            match self.getz80reg() {
                Z80Reg::AF => e.push(0xf1),
                Z80Reg::BC => e.push(0xc1),
                Z80Reg::DE => e.push(0xd1),
                Z80Reg::HL => e.push(0xe1),
                Z80Reg::IX => { e.push(0xdd); e.push(0xe1); }
                Z80Reg::IY => { e.push(0xfd); e.push(0xe1); }
                _ => { ok = false; }
            }
            if !self.comma() || e.len() > 27 { ok = false; }
        }
        e.push(-1);
        self.emit_bytes_array(&e);
    }

    // ============================================================
    // MULUB / MULUW
    // ============================================================

    pub fn piz_mulub(&mut self) {
        let mut e = [-1i32; 3];
        let mut reg = self.getz80reg();
        if reg == Z80Reg::A && self.comma() { reg = self.getz80reg(); }
        match reg {
            Z80Reg::B | Z80Reg::C | Z80Reg::D | Z80Reg::E |
            Z80Reg::H | Z80Reg::L | Z80Reg::A => {
                e[0] = 0xed; e[1] = 0xc1 + (reg as i32) * 8;
            }
            _ => {}
        }
        self.emit_bytes_array(&e);
    }

    pub fn piz_muluw(&mut self) {
        let mut e = [-1i32; 3];
        let mut reg = self.getz80reg();
        if reg == Z80Reg::HL && self.comma() { reg = self.getz80reg(); }
        match reg {
            Z80Reg::BC | Z80Reg::SP => {
                e[0] = 0xed; e[1] = 0xb3 + reg as i32;
            }
            _ => {}
        }
        self.emit_bytes_array(&e);
    }

    // ============================================================
    // LD (the big one)
    // ============================================================

    pub fn piz_ld(&mut self) {
        let mut e = [-1i32; 7];
        match self.getz80reg() {
            Z80Reg::F | Z80Reg::AF => {
                self.emit_bytes_array(&e);
                return;
            }
            Z80Reg::A => self.ld_a(&mut e),
            Z80Reg::B => self.ld_simple_reg(&mut e, 0x40, 0x06, 0x44, 0x45, 0x46),
            Z80Reg::C => self.ld_simple_reg(&mut e, 0x48, 0x0e, 0x4c, 0x4d, 0x4e),
            Z80Reg::D => self.ld_simple_reg(&mut e, 0x50, 0x16, 0x54, 0x55, 0x56),
            Z80Reg::E => self.ld_simple_reg(&mut e, 0x58, 0x1e, 0x5c, 0x5d, 0x5e),
            Z80Reg::H => self.ld_h(&mut e),
            Z80Reg::L => self.ld_l(&mut e),
            Z80Reg::I => self.ld_i(&mut e),
            Z80Reg::R => self.ld_r(&mut e),
            Z80Reg::IXL => self.ld_ixl(&mut e),
            Z80Reg::IXH => self.ld_ixh(&mut e),
            Z80Reg::IYL => self.ld_iyl(&mut e),
            Z80Reg::IYH => self.ld_iyh(&mut e),
            Z80Reg::BC => self.ld_rp(&mut e, 0xed, 0x4b, 0x01),
            Z80Reg::DE => self.ld_rp(&mut e, 0xed, 0x5b, 0x11),
            Z80Reg::HL => self.ld_hl(&mut e),
            Z80Reg::SP => self.ld_sp(&mut e),
            Z80Reg::IX => self.ld_ix(&mut e),
            Z80Reg::IY => self.ld_iy(&mut e),
            _ => self.ld_indirect(&mut e),
        }
        self.emit_bytes_array(&e);
    }

    fn ld_a(&mut self, e: &mut [i32; 7]) {
        if !self.comma() { return; }
        let reg = self.getz80reg();
        match reg {
            Z80Reg::A | Z80Reg::B | Z80Reg::C | Z80Reg::D | Z80Reg::E | Z80Reg::H | Z80Reg::L => {
                e[0] = 0x78 + reg as i32;
            }
            Z80Reg::I => { e[0] = 0xed; e[1] = 0x57; }
            Z80Reg::R => { e[0] = 0xed; e[1] = 0x5f; }
            Z80Reg::IXL => { e[0] = 0xdd; e[1] = 0x7d; }
            Z80Reg::IXH => { e[0] = 0xdd; e[1] = 0x7c; }
            Z80Reg::IYL => { e[0] = 0xfd; e[1] = 0x7d; }
            Z80Reg::IYH => { e[0] = 0xfd; e[1] = 0x7c; }
            _ => {
                if self.oparen('[') {
                    let reg2 = self.getz80reg();
                    if reg2 == Z80Reg::UNK {
                        let b = self.z80getword();
                        e[1] = b & 255; e[2] = (b >> 8) & 255;
                        if self.cparen('[') { e[0] = 0x3a; }
                        return;
                    }
                    match reg2 {
                        Z80Reg::BC => { if self.cparen('[') { e[0] = 0x0a; } }
                        Z80Reg::DE => { if self.cparen('[') { e[0] = 0x1a; } }
                        Z80Reg::HL => { if self.cparen('[') { e[0] = 0x7e; } }
                        Z80Reg::IX | Z80Reg::IY => {
                            e[1] = 0x7e; e[2] = self.z80getidxoffset();
                            if self.cparen('[') { e[0] = reg2 as i32; }
                        }
                        _ => {}
                    }
                } else if self.oparen('(') {
                    let reg2 = self.getz80reg();
                    if reg2 == Z80Reg::UNK {
                        self.lp -= 1;
                        let olp = self.lp;
                        if let Some(b) = self.parse_expression() {
                            let paren_end = self.getparen(olp);
                            if paren_end == self.lp {
                                self.check16(b); e[0] = 0x3a; e[1] = (b & 255) as i32; e[2] = ((b >> 8) & 255) as i32;
                            } else {
                                self.check8(b); e[0] = 0x3e; e[1] = (b & 255) as i32;
                            }
                        }
                        return;
                    }
                    match reg2 {
                        Z80Reg::BC => { if self.cparen('(') { e[0] = 0x0a; } }
                        Z80Reg::DE => { if self.cparen('(') { e[0] = 0x1a; } }
                        Z80Reg::HL => { if self.cparen('(') { e[0] = 0x7e; } }
                        Z80Reg::IX | Z80Reg::IY => {
                            e[1] = 0x7e; e[2] = self.z80getidxoffset();
                            if self.cparen('(') { e[0] = reg2 as i32; }
                        }
                        _ => {}
                    }
                } else {
                    e[0] = 0x3e; e[1] = self.z80getbyte();
                }
            }
        }
    }

    fn ld_simple_reg(&mut self, e: &mut [i32; 7], base: i32, imm: i32, ixh_op: i32, ixl_op: i32, mem_op: i32) {
        if !self.comma() { return; }
        let reg = self.getz80reg();
        match reg {
            Z80Reg::A | Z80Reg::B | Z80Reg::C | Z80Reg::D | Z80Reg::E | Z80Reg::H | Z80Reg::L => {
                e[0] = base + reg as i32;
            }
            Z80Reg::IXH => { e[0] = 0xdd; e[1] = ixh_op; }
            Z80Reg::IXL => { e[0] = 0xdd; e[1] = ixl_op; }
            Z80Reg::IYH => { e[0] = 0xfd; e[1] = ixh_op; }
            Z80Reg::IYL => { e[0] = 0xfd; e[1] = ixl_op; }
            _ => {
                if self.oparen('[') {
                    let r2 = self.getz80reg();
                    if r2 == Z80Reg::UNK { return; }
                    match r2 {
                        Z80Reg::HL => { if self.cparen('[') { e[0] = mem_op; } }
                        Z80Reg::IX | Z80Reg::IY => {
                            e[1] = mem_op; e[2] = self.z80getidxoffset();
                            if self.cparen('[') { e[0] = r2 as i32; }
                        }
                        _ => {}
                    }
                } else if self.oparen('(') {
                    let r2 = self.getz80reg();
                    if r2 == Z80Reg::UNK { self.lp -= 1; e[0] = imm; e[1] = self.z80getbyte(); return; }
                    match r2 {
                        Z80Reg::HL => { if self.cparen('(') { e[0] = mem_op; } }
                        Z80Reg::IX | Z80Reg::IY => {
                            e[1] = mem_op; e[2] = self.z80getidxoffset();
                            if self.cparen('(') { e[0] = r2 as i32; }
                        }
                        _ => {}
                    }
                } else {
                    e[0] = imm; e[1] = self.z80getbyte();
                }
            }
        }
    }

    fn ld_h(&mut self, e: &mut [i32; 7]) {
        if !self.comma() { return; }
        let reg = self.getz80reg();
        match reg {
            Z80Reg::A | Z80Reg::B | Z80Reg::C | Z80Reg::D | Z80Reg::E | Z80Reg::H | Z80Reg::L => {
                e[0] = 0x60 + reg as i32;
            }
            _ => {
                if self.oparen('[') {
                    let r2 = self.getz80reg();
                    if r2 == Z80Reg::UNK { return; }
                    match r2 {
                        Z80Reg::HL => { if self.cparen('[') { e[0] = 0x66; } }
                        Z80Reg::IX | Z80Reg::IY => {
                            e[1] = 0x66; e[2] = self.z80getidxoffset();
                            if self.cparen('[') { e[0] = r2 as i32; }
                        }
                        _ => {}
                    }
                } else if self.oparen('(') {
                    let r2 = self.getz80reg();
                    if r2 == Z80Reg::UNK { self.lp -= 1; e[0] = 0x26; e[1] = self.z80getbyte(); return; }
                    match r2 {
                        Z80Reg::HL => { if self.cparen('(') { e[0] = 0x66; } }
                        Z80Reg::IX | Z80Reg::IY => {
                            e[1] = 0x66; e[2] = self.z80getidxoffset();
                            if self.cparen('(') { e[0] = r2 as i32; }
                        }
                        _ => {}
                    }
                } else {
                    e[0] = 0x26; e[1] = self.z80getbyte();
                }
            }
        }
    }

    fn ld_l(&mut self, e: &mut [i32; 7]) {
        if !self.comma() { return; }
        let reg = self.getz80reg();
        match reg {
            Z80Reg::A | Z80Reg::B | Z80Reg::C | Z80Reg::D | Z80Reg::E | Z80Reg::H | Z80Reg::L => {
                e[0] = 0x68 + reg as i32;
            }
            _ => {
                if self.oparen('[') {
                    let r2 = self.getz80reg();
                    if r2 == Z80Reg::UNK { return; }
                    match r2 {
                        Z80Reg::HL => { if self.cparen('[') { e[0] = 0x6e; } }
                        Z80Reg::IX | Z80Reg::IY => {
                            e[1] = 0x6e; e[2] = self.z80getidxoffset();
                            if self.cparen('[') { e[0] = r2 as i32; }
                        }
                        _ => {}
                    }
                } else if self.oparen('(') {
                    let r2 = self.getz80reg();
                    if r2 == Z80Reg::UNK { self.lp -= 1; e[0] = 0x2e; e[1] = self.z80getbyte(); return; }
                    match r2 {
                        Z80Reg::HL => { if self.cparen('(') { e[0] = 0x6e; } }
                        Z80Reg::IX | Z80Reg::IY => {
                            e[1] = 0x6e; e[2] = self.z80getidxoffset();
                            if self.cparen('(') { e[0] = r2 as i32; }
                        }
                        _ => {}
                    }
                } else {
                    e[0] = 0x2e; e[1] = self.z80getbyte();
                }
            }
        }
    }

    fn ld_i(&mut self, e: &mut [i32; 7]) {
        if !self.comma() { return; }
        if self.getz80reg() == Z80Reg::A { e[0] = 0xed; }
        e[1] = 0x47;
    }

    fn ld_r(&mut self, e: &mut [i32; 7]) {
        if !self.comma() { return; }
        if self.getz80reg() == Z80Reg::A { e[0] = 0xed; }
        e[1] = 0x4f;
    }

    fn ld_ixl(&mut self, e: &mut [i32; 7]) {
        if !self.comma() { return; }
        let reg = self.getz80reg();
        match reg {
            Z80Reg::A | Z80Reg::B | Z80Reg::C | Z80Reg::D | Z80Reg::E => {
                e[0] = 0xdd; e[1] = 0x68 + reg as i32;
            }
            Z80Reg::IXL => { e[0] = 0xdd; e[1] = 0x6d; }
            Z80Reg::IXH => { e[0] = 0xdd; e[1] = 0x6c; }
            _ => { e[0] = 0xdd; e[1] = 0x2e; e[2] = self.z80getbyte(); }
        }
    }

    fn ld_ixh(&mut self, e: &mut [i32; 7]) {
        if !self.comma() { return; }
        let reg = self.getz80reg();
        match reg {
            Z80Reg::A | Z80Reg::B | Z80Reg::C | Z80Reg::D | Z80Reg::E => {
                e[0] = 0xdd; e[1] = 0x60 + reg as i32;
            }
            Z80Reg::IXL => { e[0] = 0xdd; e[1] = 0x65; }
            Z80Reg::IXH => { e[0] = 0xdd; e[1] = 0x64; }
            _ => { e[0] = 0xdd; e[1] = 0x26; e[2] = self.z80getbyte(); }
        }
    }

    fn ld_iyl(&mut self, e: &mut [i32; 7]) {
        if !self.comma() { return; }
        let reg = self.getz80reg();
        match reg {
            Z80Reg::A | Z80Reg::B | Z80Reg::C | Z80Reg::D | Z80Reg::E => {
                e[0] = 0xfd; e[1] = 0x68 + reg as i32;
            }
            Z80Reg::IYL => { e[0] = 0xfd; e[1] = 0x6d; }
            Z80Reg::IYH => { e[0] = 0xfd; e[1] = 0x6c; }
            _ => { e[0] = 0xfd; e[1] = 0x2e; e[2] = self.z80getbyte(); }
        }
    }

    fn ld_iyh(&mut self, e: &mut [i32; 7]) {
        if !self.comma() { return; }
        let reg = self.getz80reg();
        match reg {
            Z80Reg::A | Z80Reg::B | Z80Reg::C | Z80Reg::D | Z80Reg::E => {
                e[0] = 0xfd; e[1] = 0x60 + reg as i32;
            }
            Z80Reg::IYL => { e[0] = 0xfd; e[1] = 0x65; }
            Z80Reg::IYH => { e[0] = 0xfd; e[1] = 0x64; }
            _ => { e[0] = 0xfd; e[1] = 0x26; e[2] = self.z80getbyte(); }
        }
    }

    fn ld_rp(&mut self, e: &mut [i32; 7], ed_prefix: i32, ed_reg: i32, imm_op: i32) {
        // Determine the high/low register opcodes for this register pair
        // BC: B=0x40 base, C=0x48 base → LD B,x = 0x40+src, LD C,x = 0x48+src
        // DE: D=0x50 base, E=0x58 base → LD D,x = 0x50+src, LD E,x = 0x58+src
        let (hi_base, lo_base) = if imm_op == 0x01 { (0x40, 0x48) } else { (0x50, 0x58) };
        let (ix_hi, ix_lo) = if imm_op == 0x01 { (0x44, 0x4d) } else { (0x54, 0x5d) };

        if !self.comma() { return; }
        let reg = self.getz80reg();
        match reg {
            Z80Reg::BC => { e[0] = hi_base + 0; e[1] = lo_base + 1; } // LD H,B / LD L,C
            Z80Reg::DE => { e[0] = hi_base + 2; e[1] = lo_base + 3; } // LD H,D / LD L,E
            Z80Reg::HL => { e[0] = hi_base + 4; e[1] = lo_base + 5; } // LD H,H / LD L,L
            Z80Reg::IX => { e[0] = 0xdd; e[1] = ix_hi; e[2] = 0xdd; e[3] = ix_lo; }
            Z80Reg::IY => { e[0] = 0xfd; e[1] = ix_hi; e[2] = 0xfd; e[3] = ix_lo; }
            Z80Reg::UNK => {
                if self.oparen('[') {
                    let r2 = self.getz80reg();
                    if r2 == Z80Reg::UNK {
                        let b = self.z80getword();
                        e[1] = ed_reg; e[2] = b & 255; e[3] = (b >> 8) & 255;
                        if self.cparen('[') { e[0] = ed_prefix; }
                        return;
                    }
                    // (HL) (IX+d) (IY+d) for 16-bit load
                    match r2 {
                        Z80Reg::HL => {
                            if self.cparen('[') { e[0] = lo_base + 6; }
                            e[1] = 0x23; e[2] = hi_base + 6; e[3] = 0x2b;
                        }
                        Z80Reg::IX | Z80Reg::IY => {
                            let b = self.z80getidxoffset();
                            if b == 127 { self.error("Offset out of range", None, ErrorKind::Pass2); }
                            if self.cparen('[') { e[0] = r2 as i32; e[3] = r2 as i32; }
                            e[1] = lo_base + 6; e[4] = hi_base + 6; e[2] = b; e[5] = b + 1;
                        }
                        _ => {}
                    }
                } else if self.oparen('(') {
                    let r2 = self.getz80reg();
                    if r2 == Z80Reg::UNK {
                        self.lp -= 1;
                        let olp = self.lp;
                        let b = self.z80getword();
                        let paren_end = self.getparen(olp);
                        if paren_end == self.lp {
                            e[0] = ed_prefix; e[1] = ed_reg; e[2] = b & 255; e[3] = (b >> 8) & 255;
                        } else {
                            e[0] = imm_op; e[1] = b & 255; e[2] = (b >> 8) & 255;
                        }
                        return;
                    }
                    match r2 {
                        Z80Reg::HL => {
                            if self.cparen('(') { e[0] = lo_base + 6; }
                            e[1] = 0x23; e[2] = hi_base + 6; e[3] = 0x2b;
                        }
                        Z80Reg::IX | Z80Reg::IY => {
                            let b = self.z80getidxoffset();
                            if b == 127 { self.error("Offset out of range", None, ErrorKind::Pass2); }
                            if self.cparen('(') { e[0] = r2 as i32; e[3] = r2 as i32; }
                            e[1] = lo_base + 6; e[4] = hi_base + 6; e[2] = b; e[5] = b + 1;
                        }
                        _ => {}
                    }
                } else {
                    e[0] = imm_op; let b = self.z80getword(); e[1] = b & 255; e[2] = (b >> 8) & 255;
                    return;
                }
            }
            _ => {} // other register pairs not handled
        }
    }

    fn ld_hl(&mut self, e: &mut [i32; 7]) {
        if !self.comma() { return; }
        let reg = self.getz80reg();
        match reg {
            Z80Reg::BC => { e[0] = 0x60; e[1] = 0x69; }
            Z80Reg::DE => { e[0] = 0x62; e[1] = 0x6b; }
            Z80Reg::HL => { e[0] = 0x64; e[1] = 0x6d; }
            Z80Reg::IX => { e[0] = 0xdd; e[1] = 0xe5; e[2] = 0xe1; }
            Z80Reg::IY => { e[0] = 0xfd; e[1] = 0xe5; e[2] = 0xe1; }
            _ => {
                if self.oparen('[') {
                    let r2 = self.getz80reg();
                    if r2 == Z80Reg::UNK {
                        let b = self.z80getword();
                        e[1] = b & 255; e[2] = (b >> 8) & 255;
                        if self.cparen('[') { e[0] = 0x2a; }
                        return;
                    }
                    match r2 {
                        Z80Reg::IX | Z80Reg::IY => {
                            let b = self.z80getidxoffset();
                            if b == 127 { self.error("Offset out of range", None, ErrorKind::Pass2); }
                            if self.cparen('[') { e[0] = r2 as i32; e[3] = r2 as i32; }
                            e[1] = 0x6e; e[4] = 0x66; e[2] = b; e[5] = b + 1;
                        }
                        _ => {}
                    }
                } else if self.oparen('(') {
                    let r2 = self.getz80reg();
                    if r2 == Z80Reg::UNK {
                        self.lp -= 1;
                        let olp = self.lp;
                        let b = self.z80getword();
                        let paren_end = self.getparen(olp);
                        if paren_end == self.lp {
                            e[0] = 0x2a; e[1] = b & 255; e[2] = (b >> 8) & 255;
                        } else {
                            e[0] = 0x21; e[1] = b & 255; e[2] = (b >> 8) & 255;
                        }
                        return;
                    }
                    match r2 {
                        Z80Reg::IX | Z80Reg::IY => {
                            let b = self.z80getidxoffset();
                            if b == 127 { self.error("Offset out of range", None, ErrorKind::Pass2); }
                            if self.cparen('(') { e[0] = r2 as i32; e[3] = r2 as i32; }
                            e[1] = 0x6e; e[4] = 0x66; e[2] = b; e[5] = b + 1;
                        }
                        _ => {}
                    }
                } else {
                    e[0] = 0x21; let b = self.z80getword(); e[1] = b & 255; e[2] = (b >> 8) & 255;
                }
            }
        }
    }

    fn ld_sp(&mut self, e: &mut [i32; 7]) {
        if !self.comma() { return; }
        let reg = self.getz80reg();
        match reg {
            Z80Reg::HL => { e[0] = 0xf9; }
            Z80Reg::IX | Z80Reg::IY => { e[0] = reg as i32; e[1] = 0xf9; }
            _ => {
                if self.oparen('(') || self.oparen('[') {
                    let b = self.z80getword();
                    e[1] = 0x7b; e[2] = b & 255; e[3] = (b >> 8) & 255;
                    if self.cparen('(') || self.cparen('[') { e[0] = 0xed; }
                } else {
                    let b = self.z80getword();
                    e[0] = 0x31; e[1] = b & 255; e[2] = (b >> 8) & 255;
                }
            }
        }
    }

    fn ld_ix(&mut self, e: &mut [i32; 7]) {
        if !self.comma() { return; }
        let reg = self.getz80reg();
        match reg {
            Z80Reg::BC => { e[0] = 0xdd; e[2] = 0xdd; e[1] = 0x69; e[3] = 0x60; }
            Z80Reg::DE => { e[0] = 0xdd; e[2] = 0xdd; e[1] = 0x6b; e[3] = 0x62; }
            Z80Reg::HL => { e[0] = 0xe5; e[1] = 0xdd; e[2] = 0xe1; }
            Z80Reg::IX => { e[0] = 0xdd; e[2] = 0xdd; e[1] = 0x6d; e[3] = 0x64; }
            Z80Reg::IY => { e[0] = 0xfd; e[1] = 0xe5; e[2] = 0xdd; e[3] = 0xe1; }
            _ => {
                if self.oparen('[') {
                    let b = self.z80getword();
                    e[1] = 0x2a; e[2] = b & 255; e[3] = (b >> 8) & 255;
                    if self.cparen('[') { e[0] = 0xdd; }
                    return;
                }
                let beginhaakje = self.oparen('(');
                if beginhaakje { self.lp -= 1; }
                let olp = self.lp;
                let b = self.z80getword();
                if beginhaakje && self.getparen(olp) == self.lp {
                    e[0] = 0xdd; e[1] = 0x2a; e[2] = b & 255; e[3] = (b >> 8) & 255;
                } else {
                    e[0] = 0xdd; e[1] = 0x21; e[2] = b & 255; e[3] = (b >> 8) & 255;
                }
            }
        }
    }

    fn ld_iy(&mut self, e: &mut [i32; 7]) {
        if !self.comma() { return; }
        let reg = self.getz80reg();
        match reg {
            Z80Reg::BC => { e[0] = 0xfd; e[2] = 0xfd; e[1] = 0x69; e[3] = 0x60; }
            Z80Reg::DE => { e[0] = 0xfd; e[2] = 0xfd; e[1] = 0x6b; e[3] = 0x62; }
            Z80Reg::HL => { e[0] = 0xe5; e[1] = 0xfd; e[2] = 0xe1; }
            Z80Reg::IX => { e[0] = 0xdd; e[1] = 0xe5; e[2] = 0xfd; e[3] = 0xe1; }
            Z80Reg::IY => { e[0] = 0xfd; e[2] = 0xfd; e[1] = 0x6d; e[3] = 0x64; }
            _ => {
                if self.oparen('[') {
                    let b = self.z80getword();
                    e[1] = 0x2a; e[2] = b & 255; e[3] = (b >> 8) & 255;
                    if self.cparen('[') { e[0] = 0xfd; }
                    return;
                }
                let beginhaakje = self.oparen('(');
                if beginhaakje { self.lp -= 1; }
                let olp = self.lp;
                let b = self.z80getword();
                if beginhaakje && self.getparen(olp) == self.lp {
                    e[0] = 0xfd; e[1] = 0x2a; e[2] = b & 255; e[3] = (b >> 8) & 255;
                } else {
                    e[0] = 0xfd; e[1] = 0x21; e[2] = b & 255; e[3] = (b >> 8) & 255;
                }
            }
        }
    }

    fn ld_indirect(&mut self, e: &mut [i32; 7]) {
        if !self.oparen('(') && !self.oparen('[') { return; }
        let reg = self.getz80reg();
        match reg {
            Z80Reg::BC => {
                if !(self.cparen('(') || self.cparen('[')) { return; }
                if !self.comma() { return; }
                if self.getz80reg() != Z80Reg::A { return; }
                e[0] = 0x02;
            }
            Z80Reg::DE => {
                if !(self.cparen('(') || self.cparen('[')) { return; }
                if !self.comma() { return; }
                if self.getz80reg() != Z80Reg::A { return; }
                e[0] = 0x12;
            }
            Z80Reg::HL => {
                if !(self.cparen('(') || self.cparen('[')) { return; }
                if !self.comma() { return; }
                let src = self.getz80reg();
                match src {
                    Z80Reg::A | Z80Reg::B | Z80Reg::C | Z80Reg::D | Z80Reg::E | Z80Reg::H | Z80Reg::L => {
                        e[0] = 0x70 + src as i32;
                    }
                    Z80Reg::BC => { e[0] = 0x71; e[1] = 0x23; e[2] = 0x70; e[3] = 0x2b; }
                    Z80Reg::DE => { e[0] = 0x73; e[1] = 0x23; e[2] = 0x72; e[3] = 0x2b; }
                    _ => {
                        if src == Z80Reg::UNK {
                            e[0] = 0x36; e[1] = self.z80getbyte();
                        }
                    }
                }
            }
            Z80Reg::IX | Z80Reg::IY => {
                e[2] = self.z80getidxoffset();
                if !(self.cparen('(') || self.cparen('[')) { return; }
                if !self.comma() { return; }
                let src = self.getz80reg();
                match src {
                    Z80Reg::A | Z80Reg::B | Z80Reg::C | Z80Reg::D | Z80Reg::E | Z80Reg::H | Z80Reg::L => {
                        e[0] = reg as i32; e[1] = 0x70 + src as i32;
                    }
                    Z80Reg::BC => {
                        if e[2] == 127 { self.error("Offset out of range", None, ErrorKind::Pass2); }
                        e[0] = reg as i32; e[3] = reg as i32; e[1] = 0x71; e[4] = 0x70; e[5] = e[2] + 1;
                    }
                    Z80Reg::DE => {
                        if e[2] == 127 { self.error("Offset out of range", None, ErrorKind::Pass2); }
                        e[0] = reg as i32; e[3] = reg as i32; e[1] = 0x73; e[4] = 0x72; e[5] = e[2] + 1;
                    }
                    Z80Reg::HL => {
                        if e[2] == 127 { self.error("Offset out of range", None, ErrorKind::Pass2); }
                        e[0] = reg as i32; e[3] = reg as i32; e[1] = 0x75; e[4] = 0x74; e[5] = e[2] + 1;
                    }
                    _ => {
                        if src == Z80Reg::UNK {
                            e[0] = reg as i32; e[1] = 0x36; e[3] = self.z80getbyte();
                        }
                    }
                }
            }
            _ => {
                // (addr), reg
                let b = self.z80getword();
                if !(self.cparen('(') || self.cparen('[')) { return; }
                if !self.comma() { return; }
                match self.getz80reg() {
                    Z80Reg::A  => { e[0] = 0x32; e[1] = b & 255; e[2] = (b >> 8) & 255; }
                    Z80Reg::BC => { e[0] = 0xed; e[1] = 0x43; e[2] = b & 255; e[3] = (b >> 8) & 255; }
                    Z80Reg::DE => { e[0] = 0xed; e[1] = 0x53; e[2] = b & 255; e[3] = (b >> 8) & 255; }
                    Z80Reg::HL => { e[0] = 0x22; e[1] = b & 255; e[2] = (b >> 8) & 255; }
                    Z80Reg::IX => { e[0] = 0xdd; e[1] = 0x22; e[2] = b & 255; e[3] = (b >> 8) & 255; }
                    Z80Reg::IY => { e[0] = 0xfd; e[1] = 0x22; e[2] = b & 255; e[3] = (b >> 8) & 255; }
                    Z80Reg::SP => { e[0] = 0xed; e[1] = 0x73; e[2] = b & 255; e[3] = (b >> 8) & 255; }
                    _ => {}
                }
            }
        }
    }

    // ============================================================
    // LDD / LDI (extended)
    // ============================================================

    pub fn piz_ldd(&mut self) {
        // Simple case: LDD with no operands = ED A8
        let pp = self.lp;
        let reg = self.getz80reg();
        if reg == Z80Reg::UNK {
            // Check if we have indirect
            if self.oparen('[') || self.oparen('(') {
                // LDD (reg),reg with auto-decrement
                self.lp = pp;
                // Fall through to complex case
            } else {
                self.lp = pp;
                self.emit_ed(0xa8);
                return;
            }
        }
        self.lp = pp;
        // Complex form - just emit simple ED A8 for now
        self.emit_ed(0xa8);
    }

    pub fn piz_ldi(&mut self) {
        let pp = self.lp;
        let reg = self.getz80reg();
        if reg == Z80Reg::UNK {
            if self.oparen('[') || self.oparen('(') {
                self.lp = pp;
            } else {
                self.lp = pp;
                self.emit_ed(0xa0);
                return;
            }
        }
        self.lp = pp;
        self.emit_ed(0xa0);
    }

    // ============================================================
    // Register all Z80 instructions
    // ============================================================

    pub fn init_z80(&mut self) {
        self.z80_tab.insert("adc".into(), Assembler::piz_adc);
        self.z80_tab.insert("add".into(), Assembler::piz_add);
        self.z80_tab.insert("and".into(), Assembler::piz_and);
        self.z80_tab.insert("bit".into(), Assembler::piz_bit);
        self.z80_tab.insert("call".into(), Assembler::piz_call);
        self.z80_tab.insert("ccf".into(), Assembler::piz_ccf);
        self.z80_tab.insert("cp".into(), Assembler::piz_cp);
        self.z80_tab.insert("cpd".into(), Assembler::piz_cpd);
        self.z80_tab.insert("cpdr".into(), Assembler::piz_cpdr);
        self.z80_tab.insert("cpi".into(), Assembler::piz_cpi);
        self.z80_tab.insert("cpir".into(), Assembler::piz_cpir);
        self.z80_tab.insert("cpl".into(), Assembler::piz_cpl);
        self.z80_tab.insert("daa".into(), Assembler::piz_daa);
        self.z80_tab.insert("dec".into(), Assembler::piz_dec);
        self.z80_tab.insert("di".into(), Assembler::piz_di);
        self.z80_tab.insert("djnz".into(), Assembler::piz_djnz);
        self.z80_tab.insert("ei".into(), Assembler::piz_ei);
        self.z80_tab.insert("ex".into(), Assembler::piz_ex);
        self.z80_tab.insert("exx".into(), Assembler::piz_exx);
        self.z80_tab.insert("halt".into(), Assembler::piz_halt);
        self.z80_tab.insert("im".into(), Assembler::piz_im);
        self.z80_tab.insert("in".into(), Assembler::piz_in);
        self.z80_tab.insert("inc".into(), Assembler::piz_inc);
        self.z80_tab.insert("ind".into(), Assembler::piz_ind);
        self.z80_tab.insert("indr".into(), Assembler::piz_indr);
        self.z80_tab.insert("ini".into(), Assembler::piz_ini);
        self.z80_tab.insert("inir".into(), Assembler::piz_inir);
        self.z80_tab.insert("jp".into(), Assembler::piz_jp);
        self.z80_tab.insert("jr".into(), Assembler::piz_jr);
        self.z80_tab.insert("ld".into(), Assembler::piz_ld);
        self.z80_tab.insert("ldd".into(), Assembler::piz_ldd);
        self.z80_tab.insert("lddr".into(), Assembler::piz_lddr);
        self.z80_tab.insert("ldi".into(), Assembler::piz_ldi);
        self.z80_tab.insert("ldir".into(), Assembler::piz_ldir);
        self.z80_tab.insert("mulub".into(), Assembler::piz_mulub);
        self.z80_tab.insert("muluw".into(), Assembler::piz_muluw);
        self.z80_tab.insert("neg".into(), Assembler::piz_neg);
        self.z80_tab.insert("nop".into(), Assembler::piz_nop);
        self.z80_tab.insert("or".into(), Assembler::piz_or);
        self.z80_tab.insert("otdr".into(), Assembler::piz_otdr);
        self.z80_tab.insert("otir".into(), Assembler::piz_otir);
        self.z80_tab.insert("out".into(), Assembler::piz_out);
        self.z80_tab.insert("outd".into(), Assembler::piz_outd);
        self.z80_tab.insert("outi".into(), Assembler::piz_outi);
        self.z80_tab.insert("pop".into(), Assembler::piz_pop);
        self.z80_tab.insert("push".into(), Assembler::piz_push);
        self.z80_tab.insert("res".into(), Assembler::piz_res);
        self.z80_tab.insert("ret".into(), Assembler::piz_ret);
        self.z80_tab.insert("reti".into(), Assembler::piz_reti);
        self.z80_tab.insert("retn".into(), Assembler::piz_retn);
        self.z80_tab.insert("rl".into(), Assembler::piz_rl);
        self.z80_tab.insert("rla".into(), Assembler::piz_rla);
        self.z80_tab.insert("rlc".into(), Assembler::piz_rlc);
        self.z80_tab.insert("rlca".into(), Assembler::piz_rlca);
        self.z80_tab.insert("rld".into(), Assembler::piz_rld);
        self.z80_tab.insert("rr".into(), Assembler::piz_rr);
        self.z80_tab.insert("rra".into(), Assembler::piz_rra);
        self.z80_tab.insert("rrc".into(), Assembler::piz_rrc);
        self.z80_tab.insert("rrca".into(), Assembler::piz_rrca);
        self.z80_tab.insert("rrd".into(), Assembler::piz_rrd);
        self.z80_tab.insert("rst".into(), Assembler::piz_rst);
        self.z80_tab.insert("sbc".into(), Assembler::piz_sbc);
        self.z80_tab.insert("scf".into(), Assembler::piz_scf);
        self.z80_tab.insert("set".into(), Assembler::piz_set);
        self.z80_tab.insert("sla".into(), Assembler::piz_sla);
        self.z80_tab.insert("sli".into(), Assembler::piz_sll);
        self.z80_tab.insert("sll".into(), Assembler::piz_sll);
        self.z80_tab.insert("sra".into(), Assembler::piz_sra);
        self.z80_tab.insert("srl".into(), Assembler::piz_srl);
        self.z80_tab.insert("sub".into(), Assembler::piz_sub);
        self.z80_tab.insert("xor".into(), Assembler::piz_xor);
    }
}
