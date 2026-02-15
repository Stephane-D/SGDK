use crate::vgm_command::VGMCommand;

/// PSG (SN76489) state tracker
#[derive(Clone, Debug)]
pub struct PSGState {
    registers: [[i32; 2]; 4],
    init: [[bool; 2]; 4],
    index: i32,
    reg_type: i32,
}

impl PSGState {
    pub fn new() -> Self {
        PSGState {
            registers: [[0; 2]; 4],
            init: [[false; 2]; 4],
            index: -1,
            reg_type: -1,
        }
    }

    pub fn from_state(state: &PSGState) -> Self {
        PSGState {
            registers: state.registers,
            init: state.init,
            index: -1,
            reg_type: -1,
        }
    }

    pub fn clear(&mut self) {
        for i in 0..4 {
            self.registers[i][0] = 0;
            self.registers[i][1] = 0;
            self.init[i][0] = false;
            self.init[i][1] = false;
        }
    }

    pub fn get(&self, ind: usize, typ: usize) -> i32 {
        match typ {
            0 => {
                // tone / noise
                if ind == 3 {
                    self.registers[ind][typ] & 0x7
                } else {
                    self.registers[ind][typ] & 0x3FF
                }
            }
            1 => {
                // volume
                self.registers[ind][typ] & 0xF
            }
            _ => 0,
        }
    }

    pub fn write(&mut self, value: i32) {
        if (value & 0x80) != 0 {
            self.write_low(value & 0x7F);
        } else {
            self.write_high(value & 0x7F);
        }
    }

    fn write_low(&mut self, value: i32) {
        self.index = (value >> 5) & 0x03;
        self.reg_type = (value >> 4) & 0x01;
        let ind = self.index as usize;
        let typ = self.reg_type as usize;

        if typ == 0 && ind == 3 {
            self.registers[ind][typ] &= !0x7;
            self.registers[ind][typ] |= value & 0x7;
        } else {
            self.registers[ind][typ] &= !0xF;
            self.registers[ind][typ] |= value & 0xF;
        }

        self.init[ind][typ] = true;
    }

    fn write_high(&mut self, value: i32) {
        if self.index < 0 || self.reg_type < 0 {
            return;
        }

        let ind = self.index as usize;
        let typ = self.reg_type as usize;

        if typ == 0 && ind == 3 {
            self.registers[ind][typ] &= !0x7;
            self.registers[ind][typ] |= value & 0x7;
        } else if typ == 1 {
            self.registers[ind][typ] &= !0xF;
            self.registers[ind][typ] |= value & 0xF;
        } else {
            self.registers[ind][typ] &= !0x3F0;
            self.registers[ind][typ] |= (value & 0x3F) << 4;
        }

        self.init[ind][typ] = true;
    }

    pub fn is_same(&self, state: &PSGState, ind: usize, typ: usize) -> bool {
        // consider same if 'state' is not initialised here
        if !state.init[ind][typ] {
            return true;
        }
        // consider different if we are not initialised here while 'state' is
        if !self.init[ind][typ] {
            return false;
        }
        state.get(ind, typ) == self.get(ind, typ)
    }

    pub fn is_low_same(&self, state: &PSGState, ind: usize, typ: usize) -> bool {
        if !state.init[ind][typ] {
            return true;
        }
        if !self.init[ind][typ] {
            return false;
        }
        (state.get(ind, typ) & 0xF) == (self.get(ind, typ) & 0xF)
    }

    pub fn is_high_same(&self, state: &PSGState, ind: usize, typ: usize) -> bool {
        if !state.init[ind][typ] {
            return true;
        }
        if !self.init[ind][typ] {
            return false;
        }
        (state.get(ind, typ) & 0x3F0) == (self.get(ind, typ) & 0x3F0)
    }

    pub fn is_diff(&self, state: &PSGState, ind: usize, typ: usize) -> bool {
        !self.is_same(state, ind, typ)
    }

    pub fn is_low_diff_only(&self, state: &PSGState, ind: usize, typ: usize) -> bool {
        !self.is_low_same(state, ind, typ) && self.is_high_same(state, ind, typ)
    }

    fn create_low_write_command(&self, ind: usize, typ: usize, value: i32) -> VGMCommand {
        VGMCommand::from_slice(&[
            crate::vgm_command::WRITE_SN76489,
            (0x80 | ((ind as u8) << 5) | ((typ as u8) << 4) | (value as u8 & 0xF)),
        ])
    }

    fn create_write_commands(ind: usize, typ: usize, value: i32) -> Vec<VGMCommand> {
        let mut result = Vec::new();

        result.push(VGMCommand::from_slice(&[
            crate::vgm_command::WRITE_SN76489,
            (0x80 | ((ind as u8) << 5) | ((typ as u8) << 4) | (value as u8 & 0xF)),
        ]));

        if typ == 0 && ind != 3 {
            result.push(VGMCommand::from_slice(&[
                crate::vgm_command::WRITE_SN76489,
                ((value >> 4) as u8 & 0x3F),
            ]));
        }

        result
    }

    /// Returns commands list to update to the specified PSG state
    pub fn get_delta(&self, state: &PSGState) -> Vec<VGMCommand> {
        let mut result = Vec::new();

        for ind in 0..4 {
            for typ in 0..2 {
                if typ == 0 {
                    // value different on low bits only --> add single command
                    if self.is_low_diff_only(state, ind, typ) {
                        result.push(self.create_low_write_command(ind, typ, state.get(ind, typ)));
                    }
                    // value is different --> add commands
                    else if self.is_diff(state, ind, typ) {
                        result.extend(Self::create_write_commands(ind, typ, state.get(ind, typ)));
                    }
                } else {
                    // value is different --> add commands
                    if self.is_diff(state, ind, typ) {
                        result.extend(Self::create_write_commands(ind, typ, state.get(ind, typ)));
                    }
                }
            }
        }

        result
    }
}
