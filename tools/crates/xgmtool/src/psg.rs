use crate::vgm_command::VGMCommand;

/// PSG (SN76489) state tracker
#[derive(Clone, Debug)]
pub struct PSG {
    /// registers[channel][type]: type 0 = tone/noise, type 1 = volume
    pub registers: [[i32; 2]; 4],
    pub init: [[bool; 2]; 4],
    pub index: i32,
    pub typ: i32,
}

impl PSG {
    pub fn new() -> Self {
        let mut psg = PSG {
            registers: [[-1; 2]; 4],
            init: [[false; 2]; 4],
            index: -1,
            typ: -1,
        };
        psg.clear();
        psg
    }

    pub fn clear(&mut self) {
        for i in 0..4 {
            self.registers[i][0] = -1;
            self.registers[i][1] = -1;
            self.init[i][0] = false;
            self.init[i][1] = false;
        }
    }

    pub fn get(&self, ind: usize, typ: usize) -> i32 {
        match typ {
            0 => {
                if ind == 3 { self.registers[ind][typ] & 0x7 }
                else { self.registers[ind][typ] & 0x3FF }
            }
            1 => self.registers[ind][typ] & 0xF,
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
        self.typ = (value >> 4) & 0x01;
        let ind = self.index as usize;
        let typ = self.typ as usize;

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
        let ind = self.index as usize;
        let typ = self.typ as usize;

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

    pub fn is_same(&self, target: &PSG, ind: usize, typ: usize) -> bool {
        if !self.init[ind][typ] && !target.init[ind][typ] { return true; }
        self.init[ind][typ] && (self.get(ind, typ) == target.get(ind, typ))
    }

    pub fn is_low_same(&self, target: &PSG, ind: usize, typ: usize) -> bool {
        if !self.init[ind][typ] && !target.init[ind][typ] { return true; }
        self.init[ind][typ] && ((self.get(ind, typ) & 0xF) == (target.get(ind, typ) & 0xF))
    }

    pub fn is_high_same(&self, target: &PSG, ind: usize, typ: usize) -> bool {
        if !self.init[ind][typ] && !target.init[ind][typ] { return true; }
        self.init[ind][typ] && ((self.get(ind, typ) & 0x3F0) == (target.get(ind, typ) & 0x3F0))
    }

    pub fn is_diff(&self, target: &PSG, ind: usize, typ: usize) -> bool {
        !self.is_same(target, ind, typ)
    }

    pub fn is_low_diff_only(&self, target: &PSG, ind: usize, typ: usize) -> bool {
        !self.is_low_same(target, ind, typ) && self.is_high_same(target, ind, typ)
    }

    fn create_low_write_command(ind: usize, typ: usize, value: i32) -> VGMCommand {
        let data = vec![0x50, 0x80u8 | ((ind as u8) << 5) | ((typ as u8) << 4) | (value as u8 & 0xF)];
        VGMCommand::from_owned(data, -1)
    }

    fn create_write_commands(ind: usize, typ: usize, value: i32) -> Vec<VGMCommand> {
        let mut result = Vec::new();
        let low_byte = 0x80u8 | ((ind as u8) << 5) | ((typ as u8) << 4) | (value as u8 & 0xF);
        result.push(VGMCommand::from_owned(vec![0x50, low_byte], -1));

        if typ == 0 && ind != 3 {
            let high_byte = ((value >> 4) & 0x3F) as u8;
            result.push(VGMCommand::from_owned(vec![0x50, high_byte], -1));
        }

        result
    }

    /// Generate VGM commands to transition from self state to `target` state
    pub fn get_delta(&self, target: &PSG) -> Vec<VGMCommand> {
        let mut result = Vec::new();

        for ind in 0..4 {
            for typ in 0..2 {
                if typ == 0 {
                    if self.is_low_diff_only(target, ind, typ) {
                        result.push(Self::create_low_write_command(ind, typ, target.get(ind, typ)));
                    } else if self.is_diff(target, ind, typ) {
                        result.extend(Self::create_write_commands(ind, typ, target.get(ind, typ)));
                    }
                } else if self.is_diff(target, ind, typ) {
                    result.extend(Self::create_write_commands(ind, typ, target.get(ind, typ)));
                }
            }
        }

        result
    }
}
