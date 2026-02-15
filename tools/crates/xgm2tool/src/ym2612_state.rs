use crate::util;
use crate::vgm_command::VGMCommand;

/// Dual register pairs (must be written together: high first, then low)
const DUALS: [[usize; 2]; 7] = [
    [0x24, 0x25],
    [0xA4, 0xA0],
    [0xA5, 0xA1],
    [0xA6, 0xA2],
    [0xAC, 0xA8],
    [0xAD, 0xA9],
    [0xAE, 0xAA],
];

/// YM2612 state tracker
#[derive(Clone, Debug)]
pub struct YM2612State {
    pub registers: [[i32; 256]; 2],
    pub init: [[bool; 256]; 2],
}

impl YM2612State {
    pub fn new() -> Self {
        YM2612State {
            registers: [[0; 256]; 2],
            init: [[false; 256]; 2],
        }
    }

    pub fn from_state(source: &YM2612State) -> Self {
        YM2612State {
            registers: source.registers,
            init: source.init,
        }
    }

    pub fn clear(&mut self) {
        for i in 0..256 {
            self.registers[0][i] = 0;
            self.registers[1][i] = 0;
            self.init[0][i] = false;
            self.init[1][i] = false;
        }
    }

    pub fn get(&self, port: usize, reg: usize) -> i32 {
        if Self::can_ignore(port, reg) {
            return 0;
        }
        self.registers[port][reg]
    }

    pub fn set(&mut self, port: usize, reg: usize, value: i32) -> bool {
        if Self::can_ignore(port, reg) {
            return false;
        }

        let mut new_value = value;

        if port == 0 {
            // special case of KEY ON/OFF register
            if reg == 0x28 {
                // invalid channel number ? --> ignore
                if (value & 3) == 3 {
                    return false;
                }

                let ch = (value & 7) as usize;
                let old_value = self.registers[port][ch];
                new_value = value & 0xF0;

                if old_value != new_value {
                    self.registers[port][ch] = new_value;
                    // always write when key state change
                    return true;
                }
                return false;
            }

            // special case of Timer register --> ignore useless bits
            if reg == 0x27 {
                new_value = value & 0xC0;
            }
        }

        if !self.init[port][reg] || self.registers[port][reg] != new_value {
            self.registers[port][reg] = value;
            self.init[port][reg] = true;
            return true;
        }

        false
    }

    pub fn is_diff(&self, state: &YM2612State, port: usize, reg: usize) -> bool {
        if !state.init[port][reg] {
            return false;
        }
        if Self::can_ignore(port, reg) {
            return false;
        }
        if !self.init[port][reg] {
            return true;
        }
        self.get(port, reg) != state.get(port, reg)
    }

    /// Return true if write can be ignored
    pub fn can_ignore(port: usize, reg: usize) -> bool {
        match reg {
            0x22 | 0x24 | 0x25 | 0x26 | 0x27 | 0x28 | 0x2B => {
                return port == 1;
            }
            _ => {}
        }

        if reg >= 0x30 && reg < 0xB8 {
            return (reg & 3) == 3;
        }

        true
    }

    /// Return the dual entry for this reg (or None if no dual entry)
    pub fn get_dual_reg(reg: usize) -> Option<[usize; 2]> {
        for dual in &DUALS {
            if dual[0] == reg || dual[1] == reg {
                return Some(*dual);
            }
        }
        None
    }

    /// Returns commands list to update to the specified YM2612 state
    pub fn get_delta(&self, state: &YM2612State, ignore_timer_writes: bool) -> Vec<VGMCommand> {
        let mut result = Vec::new();

        // do global registers first
        for port in 0..2 {
            for reg in 0..0x30 {
                // can ignore or special case of KEY ON/OFF register
                if Self::can_ignore(port, reg) || (port == 0 && reg == 0x28) {
                    continue;
                }
                // ignore timer write
                if ignore_timer_writes && port == 0 && (reg == 0x24 || reg == 0x25 || reg == 0x26) {
                    continue;
                }
                // ignore timer write (special case of 0x27 register)
                if ignore_timer_writes && port == 0 && reg == 0x27 {
                    if !state.init[port][reg] {
                        continue;
                    }
                    if self.init[port][reg]
                        && (self.registers[0][0x27] & 0xC0) == (state.registers[0][0x27] & 0xC0)
                    {
                        continue;
                    }
                }
                // ignore dual reg
                if Self::get_dual_reg(reg).is_some() {
                    continue;
                }

                // value is different --> add command
                if self.is_diff(state, port, reg) {
                    result.push(VGMCommand::create_ym_command(
                        port as i32,
                        reg as i32,
                        state.get(port, reg),
                    ));
                }
            }
        }

        // then do dual registers
        for dual in &DUALS {
            let reg0 = dual[0];
            let reg1 = dual[1];

            // value is different
            if self.is_diff(state, 0, reg0) || self.is_diff(state, 0, reg1) {
                result.push(VGMCommand::create_ym_command(0, reg0 as i32, state.get(0, reg0)));
                result.push(VGMCommand::create_ym_command(0, reg1 as i32, state.get(0, reg1)));
            }
            // port 1 too ?
            if dual[0] > 0x30 {
                if self.is_diff(state, 1, reg0) || self.is_diff(state, 1, reg1) {
                    result.push(VGMCommand::create_ym_command(1, reg0 as i32, state.get(1, reg0)));
                    result.push(VGMCommand::create_ym_command(1, reg1 as i32, state.get(1, reg1)));
                }
            }
        }

        for port in 0..2 {
            for reg_base in (0x30..0xA0).step_by(0x10) {
                for ch in 0..3 {
                    for sl in (0..0x10).step_by(4) {
                        let r = reg_base + sl + ch;
                        if Self::can_ignore(port, r) {
                            continue;
                        }
                        if Self::get_dual_reg(r).is_some() {
                            continue;
                        }
                        if self.is_diff(state, port, r) {
                            result.push(VGMCommand::create_ym_command(
                                port as i32,
                                r as i32,
                                state.get(port, r),
                            ));
                        }
                    }
                }
            }

            for reg_base in (0xA0..0xB8).step_by(4) {
                for ch in 0..3 {
                    let r = reg_base + ch;
                    if Self::can_ignore(port, r) {
                        continue;
                    }
                    if Self::get_dual_reg(r).is_some() {
                        continue;
                    }
                    if self.is_diff(state, port, r) {
                        result.push(VGMCommand::create_ym_command(
                            port as i32,
                            r as i32,
                            state.get(port, r),
                        ));
                    }
                }
            }
        }

        result
    }

    /// Update state from XGMFMCommand list
    pub fn update_state(&mut self, commands: &[crate::xgm_fm_command::XGMFMCommand]) {
        use crate::xgm_fm_command::XGMFMCommand;

        for com in commands {
            match com.get_type() {
                XGMFMCommand::FM_LFO => {
                    self.set(0, 0x22, com.data[1] as i32);
                }
                XGMFMCommand::FM_CH3_SPECIAL_ON => {
                    let v = self.get(0, 0x27);
                    self.set(0, 0x27, (v & 0x3F) | 0x40);
                }
                XGMFMCommand::FM_CH3_SPECIAL_OFF => {
                    let v = self.get(0, 0x27);
                    self.set(0, 0x27, v & 0x3F);
                }
                XGMFMCommand::FM_DAC_ON => {
                    let v = self.get(0, 0x2B);
                    self.set(0, 0x2B, (v & 0x7F) | 0x80);
                }
                XGMFMCommand::FM_DAC_OFF => {
                    let v = self.get(0, 0x2B);
                    self.set(0, 0x2B, v & 0x7F);
                }
                XGMFMCommand::FM0_PAN | XGMFMCommand::FM1_PAN => {
                    let port = if com.get_type() == XGMFMCommand::FM0_PAN { 0 } else { 1 };
                    let ch = (com.data[0] as usize) & 0x03;
                    let v = ((com.data[0] as i32) & 0x0C) << 4;
                    let cur = self.get(port, 0xB4 + ch);
                    self.set(port, 0xB4 + ch, (cur & 0x3F) | v);
                }
                XGMFMCommand::FM_WRITE => {
                    let port = ((com.data[0] >> 3) & 1) as usize;
                    let num = (com.data[0] & 0x07) as usize + 1;
                    for i in 0..num {
                        self.set(
                            port,
                            util::get_u8(&com.data, (i * 2) + 1) as usize,
                            util::get_u8(&com.data, (i * 2) + 2) as i32,
                        );
                    }
                }
                XGMFMCommand::FM_LOAD_INST => {
                    let port = ((com.data[0] >> 2) & 1) as usize;
                    let ch = (com.data[0] & 0x03) as usize;
                    let mut off = 1;
                    // slot writes
                    for r in 0..7 {
                        for sl in 0..4 {
                            self.set(
                                port,
                                0x30 + (r << 4) + (sl << 2) + ch,
                                util::get_u8(&com.data, off) as i32,
                            );
                            off += 1;
                        }
                    }
                    // ch writes
                    self.set(port, 0xB0 + ch, util::get_u8(&com.data, off) as i32);
                    off += 1;
                    self.set(port, 0xB4 + ch, util::get_u8(&com.data, off) as i32);
                }
                XGMFMCommand::FM_TL => {
                    let port = (com.data[1] & 1) as usize;
                    let ch = (com.data[0] & 3) as usize;
                    let sl = ((com.data[0] >> 2) & 3) as usize;
                    let reg = 0x40 + (sl << 2) + ch;
                    let v = ((com.data[1] >> 1) & 0x7F) as i32;
                    self.set(port, reg, v);
                }
                XGMFMCommand::FM_TL_DELTA | XGMFMCommand::FM_TL_DELTA_WAIT => {
                    let port = (com.data[1] & 1) as usize;
                    let ch = (com.data[0] & 3) as usize;
                    let sl = ((com.data[0] >> 2) & 3) as usize;
                    let reg = 0x40 + (sl << 2) + ch;
                    let mut v = ((com.data[1] >> 2) & 0x3F) as i32;
                    if (com.data[1] & 2) != 0 {
                        v = -v;
                    }
                    let cur = self.get(port, reg);
                    self.set(port, reg, cur + v);
                }
                XGMFMCommand::FM_FREQ | XGMFMCommand::FM_FREQ_WAIT => {
                    let port = ((com.data[0] >> 2) & 1) as usize;
                    let ch = (com.data[0] & 3) as usize;
                    if (com.data[0] & 8) != 0 {
                        // special mode
                        self.set(port, 0xA6 + ch, (com.data[2] & 0x3F) as i32);
                        self.set(port, 0xA2 + ch, (com.data[1] & 0xFF) as i32);
                    } else {
                        self.set(port, 0xA4 + ch, (com.data[2] & 0x3F) as i32);
                        self.set(port, 0xA0 + ch, (com.data[1] & 0xFF) as i32);
                    }
                }
                XGMFMCommand::FM_FREQ_DELTA | XGMFMCommand::FM_FREQ_DELTA_WAIT => {
                    let port = ((com.data[0] >> 2) & 1) as usize;
                    let ch = (com.data[0] & 3) as usize;
                    let base_reg = if (com.data[0] & 8) != 0 { 0x40 } else { 0x42 };
                    let reg = base_reg + ch;
                    let mut delta = ((com.data[1] >> 1) & 0x7F) as i32;
                    if (com.data[1] & 1) != 0 {
                        delta = -delta;
                    }
                    let v = (((self.get(port, reg + 4) & 0x3F) << 8)
                        | (self.get(port, reg) & 0xFF))
                        + delta;
                    self.set(port, reg + 4, (v >> 8) & 0x3F);
                    self.set(port, reg, v & 0xFF);
                }
                XGMFMCommand::FM_KEY => {
                    let ch = (com.data[0] & 7) as i32;
                    let key_val = if (com.data[0] & 8) != 0 { 0xF0 } else { 0x00 };
                    self.set(0, 0x28, key_val + ch);
                }
                XGMFMCommand::FM_KEY_SEQ => {
                    let ch = (com.data[0] & 7) as i32;
                    let key_val = if (com.data[0] & 8) != 0 { 0x00 } else { 0xF0 };
                    self.set(0, 0x28, key_val + ch);
                }
                XGMFMCommand::FM_KEY_ADV => {
                    self.set(0, 0x28, com.data[1] as i32);
                }
                _ => {}
            }
        }
    }
}
