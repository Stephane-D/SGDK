use crate::vgm_command::VGMCommand;

const DUALS_SIZE: usize = 7;
const DUALS: [[u8; 2]; DUALS_SIZE] = [
    [0x24, 0x25], [0xA4, 0xA0], [0xA5, 0xA1], [0xA6, 0xA2],
    [0xAC, 0xA8], [0xAD, 0xA9], [0xAE, 0xAA],
];

/// YM2612 FM synthesizer state tracker
#[derive(Clone, Debug)]
pub struct YM2612 {
    pub registers: [[i32; 256]; 2],  // [port][reg]
    pub init: [[bool; 256]; 2],
}

impl YM2612 {
    pub fn new() -> Self {
        let mut ym = YM2612 {
            registers: [[-1; 256]; 2],
            init: [[false; 256]; 2],
        };
        ym.clear();
        ym
    }

    pub fn clear(&mut self) {
        for i in 0..256 {
            self.registers[0][i] = -1;
            self.registers[1][i] = -1;
            self.init[0][i] = false;
            self.init[1][i] = false;
        }
    }

    pub fn initialize(&mut self) {
        for i in 0..0x20 {
            self.registers[0][i] = 0;
            self.registers[1][i] = 0;
            self.init[0][i] = true;
            self.init[1][i] = true;
        }
        for i in 0x20..256 {
            self.registers[0][i] = 0xFF;
            self.registers[1][i] = 0xFF;
            self.init[0][i] = true;
            self.init[1][i] = true;
        }
    }

    pub fn get(&self, port: usize, reg: usize) -> i32 {
        if Self::can_ignore(port as i32, reg as i32) { return 0; }
        self.registers[port][reg]
    }

    /// Set register value. Returns true if this is a KEY write that changed state.
    pub fn set(&mut self, port: usize, reg: usize, value: i32) -> bool {
        if Self::can_ignore(port as i32, reg as i32) { return false; }

        let mut new_value = value;

        if port == 0 {
            // KEY ON/OFF register
            if reg == 0x28 {
                let key_reg = (value & 7) as usize;
                let old_value = self.registers[0][key_reg];
                new_value = value & 0xF0;
                if old_value != new_value {
                    self.registers[0][key_reg] = new_value;
                    return true;
                }
                return false;
            }
            // Timer register
            if reg == 0x27 {
                new_value = value & 0xC0;
            }
        }

        self.registers[port][reg] = new_value;
        self.init[port][reg] = true;
        false
    }

    pub fn is_same(&self, other: &YM2612, port: usize, reg: usize) -> bool {
        if !self.init[port][reg] && !other.init[port][reg] { return true; }
        (self.init[port][reg] || Self::can_ignore(port as i32, reg as i32))
            && self.get(port, reg) == other.get(port, reg)
    }

    pub fn is_diff(&self, other: &YM2612, port: usize, reg: usize) -> bool {
        !self.is_same(other, port, reg)
    }

    /// Generate VGM commands to transition from self state to `target` state
    pub fn get_delta(&self, target: &YM2612) -> Vec<VGMCommand> {
        let mut result = Vec::new();

        // Dual registers first
        for dual in &DUALS {
            let reg0 = dual[0] as usize;
            let reg1 = dual[1] as usize;

            if self.is_diff(target, 0, reg0) || self.is_diff(target, 0, reg1) {
                result.push(VGMCommand::create_ym_command(0, dual[0], target.get(0, reg0) as u8));
                result.push(VGMCommand::create_ym_command(0, dual[1], target.get(0, reg1) as u8));
            }
            if dual[0] > 0x30 {
                if self.is_diff(target, 1, reg0) || self.is_diff(target, 1, reg1) {
                    result.push(VGMCommand::create_ym_command(1, dual[0], target.get(1, reg0) as u8));
                    result.push(VGMCommand::create_ym_command(1, dual[1], target.get(1, reg1) as u8));
                }
            }
        }

        // Other registers
        for port in 0..2 {
            for reg in 0..256 {
                if Self::can_ignore(port as i32, reg as i32) { continue; }
                if port == 0 && reg == 0x28 { continue; } // KEY register
                if Self::get_dual_reg(reg as u8).is_some() { continue; }

                if self.is_diff(target, port, reg) {
                    result.push(VGMCommand::create_ym_command(
                        port as i32, reg as u8, target.get(port, reg) as u8,
                    ));
                }
            }
        }

        result
    }

    /// Returns true if writes to this register can be ignored
    pub fn can_ignore(port: i32, reg: i32) -> bool {
        match reg {
            0x22 | 0x24 | 0x25 | 0x26 | 0x27 | 0x28 | 0x2B => port == 1,
            _ => {
                if reg >= 0x30 && reg < 0xB8 {
                    (reg & 3) == 3
                } else {
                    true
                }
            }
        }
    }

    /// Return the dual register pair for this reg, if any
    pub fn get_dual_reg(reg: u8) -> Option<[u8; 2]> {
        for dual in &DUALS {
            if dual[0] == reg || dual[1] == reg {
                return Some(*dual);
            }
        }
        None
    }
}
