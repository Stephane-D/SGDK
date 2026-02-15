use crate::vgm_command::VGMCommand;

/// XGM command type constants
pub const XGM_FRAME: u8 = 0x00;
pub const XGM_PSG: u8 = 0x10;
pub const XGM_YM2612_PORT0: u8 = 0x20;
pub const XGM_YM2612_PORT1: u8 = 0x30;
pub const XGM_YM2612_REGKEY: u8 = 0x40;
pub const XGM_PCM: u8 = 0x50;
pub const XGM_LOOP: u8 = 0x7E;
pub const XGM_END: u8 = 0x7F;

/// Represents a single XGM command
#[derive(Clone, Debug)]
pub struct XGMCommand {
    pub data: Vec<u8>,
    pub offset: i32,
    pub command: u8,
    pub size: usize,
}

impl XGMCommand {
    pub fn new(data: Vec<u8>) -> Self {
        let command = data[0];
        let size = data.len();
        XGMCommand { data, offset: -1, command, size }
    }

    pub fn new_ex(command: u8, data: Vec<u8>) -> Self {
        let size = data.len();
        XGMCommand { data, offset: -1, command, size }
    }

    pub fn create_loop_command(offset: i32) -> Self {
        let data = vec![
            XGM_LOOP,
            offset as u8,
            (offset >> 8) as u8,
            (offset >> 16) as u8,
        ];
        XGMCommand::new(data)
    }

    pub fn create_frame_command() -> Self {
        XGMCommand::new(vec![XGM_FRAME])
    }

    pub fn create_end_command() -> Self {
        XGMCommand::new(vec![XGM_END])
    }

    pub fn create_from_data(data: &[u8]) -> Self {
        let command = data[0];
        let size_low = command & 0x0F;
        let com_size = match command & 0xF0 {
            0x10 | 0x40 => 1 + size_low as usize + 1, // PSG, YM_REGKEY
            0x20 | 0x30 => 1 + (size_low as usize + 1) * 2, // YM_PORT0, YM_PORT1
            0x50 => 2, // PCM
            0x70 => {
                if size_low == 0x0E { 4 } // LOOP
                else { 1 }
            }
            _ => 1,
        };
        XGMCommand::new(data[..com_size].to_vec())
    }

    pub fn get_type(&self) -> u8 {
        match self.command {
            XGM_FRAME => XGM_FRAME,
            XGM_LOOP => XGM_LOOP,
            XGM_END => XGM_END,
            _ => self.command & 0xF0,
        }
    }

    pub fn get_size(&self) -> usize {
        let size_low = self.command & 0x0F;
        match self.command & 0xF0 {
            0x10 | 0x40 => 1 + size_low as usize + 1,
            0x20 | 0x30 => 1 + (size_low as usize + 1) * 2,
            0x50 => 2,
            0x70 => if size_low == 0x0E { 4 } else { 1 },
            _ => 1,
        }
    }

    pub fn is_frame(&self) -> bool { self.command == XGM_FRAME }
    pub fn is_loop(&self) -> bool { self.command == XGM_LOOP }
    pub fn get_loop_offset(&self) -> i32 { crate::get_u24(&self.data, 1) as i32 }
    pub fn is_end(&self) -> bool { self.command == XGM_END }

    pub fn is_pcm(&self) -> bool { (self.command & 0xF0) == XGM_PCM }
    pub fn get_pcm_id(&self) -> i32 {
        if self.is_pcm() { self.data[1] as i32 } else { -1 }
    }
    pub fn get_pcm_channel(&self) -> i32 {
        if self.is_pcm() { (self.data[0] & 3) as i32 } else { -1 }
    }
    pub fn get_pcm_prio(&self) -> i32 {
        if self.is_pcm() { ((self.data[0] >> 2) & 3) as i32 } else { -1 }
    }

    pub fn is_psg_write(&self) -> bool { (self.command & 0xF0) == XGM_PSG }
    pub fn get_psg_write_count(&self) -> i32 {
        if self.is_psg_write() { (self.data[0] & 0x0F) as i32 + 1 } else { -1 }
    }

    pub fn is_ym2612_port0_write(&self) -> bool { (self.command & 0xF0) == XGM_YM2612_PORT0 }
    pub fn is_ym2612_port1_write(&self) -> bool { (self.command & 0xF0) == XGM_YM2612_PORT1 }
    pub fn is_ym2612_write(&self) -> bool {
        self.is_ym2612_port0_write() || self.is_ym2612_port1_write()
    }
    pub fn get_ym2612_port(&self) -> i32 {
        if self.is_ym2612_port0_write() { 0 }
        else if self.is_ym2612_port1_write() { 1 }
        else { -1 }
    }
    pub fn get_ym2612_write_count(&self) -> i32 {
        if self.is_ym2612_write() || self.is_ym2612_reg_key_write() {
            (self.data[0] & 0x0F) as i32 + 1
        } else { -1 }
    }
    pub fn is_ym2612_reg_key_write(&self) -> bool { (self.command & 0xF0) == XGM_YM2612_REGKEY }

    pub fn set_offset(&mut self, value: i32) { self.offset = value; }

    /// Remove a specific YM2612 register write from this command
    /// Returns false if the command should be removed entirely
    pub fn remove_ym2612_reg_write(&mut self, port: i32, reg: u8) -> bool {
        match port {
            -1 => if !self.is_ym2612_write() { return true; },
            0 => if !self.is_ym2612_port0_write() { return true; },
            1 => if !self.is_ym2612_port1_write() { return true; },
            _ => return true,
        }

        let size = self.get_ym2612_write_count() as usize;
        let mut new_data = Vec::with_capacity(size * 2 + 1);
        new_data.push(0); // placeholder for command byte

        for i in 0..size {
            let r = self.data[i * 2 + 1];
            let v = self.data[i * 2 + 2];
            if r != reg {
                new_data.push(r);
                new_data.push(v);
            }
        }

        let new_size = (new_data.len() - 1) / 2;
        if new_size != size {
            if new_size == 0 { return false; }
            new_data[0] = (self.data[0] & 0xF0) | ((new_size - 1) as u8);
            self.data = new_data;
            self.size = new_size * 2 + 1;
        }
        true
    }

    // Factory methods for creating XGM commands from VGM command groups

    /// Create a YM key command from a slice of VGM key commands, consuming up to `max` entries.
    /// Returns (command, number_consumed)
    pub fn create_ym_key_command(commands: &[VGMCommand], max: usize) -> (Self, usize) {
        let size = max.min(commands.len());
        let mut data = vec![XGM_YM2612_REGKEY | ((size - 1) as u8)];
        for i in 0..size {
            data.push(commands[i].get_ym2612_value() as u8);
        }
        (XGMCommand::new(data), size)
    }

    pub fn create_ym_key_commands(commands: &[VGMCommand]) -> Vec<XGMCommand> {
        let mut result = Vec::new();
        let mut offset = 0;
        while offset < commands.len() {
            let (cmd, consumed) = Self::create_ym_key_command(&commands[offset..], 16);
            result.push(cmd);
            offset += consumed;
        }
        result
    }

    pub fn create_ym_port0_commands(commands: &[VGMCommand]) -> Vec<XGMCommand> {
        let mut result = Vec::new();
        let mut offset = 0;
        while offset < commands.len() {
            let remaining = &commands[offset..];
            let size = 16.min(remaining.len());
            let mut data = vec![XGM_YM2612_PORT0 | ((size - 1) as u8)];
            for i in 0..size {
                data.push(remaining[i].get_ym2612_register() as u8);
                data.push(remaining[i].get_ym2612_value() as u8);
            }
            result.push(XGMCommand::new(data));
            offset += size;
        }
        result
    }

    pub fn create_ym_port1_commands(commands: &[VGMCommand]) -> Vec<XGMCommand> {
        let mut result = Vec::new();
        let mut offset = 0;
        while offset < commands.len() {
            let remaining = &commands[offset..];
            let size = 16.min(remaining.len());
            let mut data = vec![XGM_YM2612_PORT1 | ((size - 1) as u8)];
            for i in 0..size {
                data.push(remaining[i].get_ym2612_register() as u8);
                data.push(remaining[i].get_ym2612_value() as u8);
            }
            result.push(XGMCommand::new(data));
            offset += size;
        }
        result
    }

    pub fn create_psg_commands(commands: &[VGMCommand]) -> Vec<XGMCommand> {
        let mut result = Vec::new();
        let mut offset = 0;
        while offset < commands.len() {
            let remaining = &commands[offset..];
            let size = 16.min(remaining.len());
            let mut data = vec![XGM_PSG | ((size - 1) as u8)];
            for i in 0..size {
                data.push(remaining[i].get_psg_value() as u8);
            }
            result.push(XGMCommand::new(data));
            offset += size;
        }
        result
    }

    pub fn create_pcm_command(
        xgm_samples: &[crate::xgm_sample::XGMSample],
        vgm: &crate::vgm::VGM,
        command: &VGMCommand,
        channel: i32,
    ) -> XGMCommand {
        let mut data = vec![XGM_PCM, 0];
        let mut prio: u8 = 0;

        if command.is_stream_start_long() {
            let address = command.get_stream_sample_address();
            prio = 3 - (command.get_stream_id() as u8 & 0x3);
            let xgm_sample = xgm_samples.iter().find(|s| s.origin_addr == address);
            let _vgm_sample = vgm.get_sample(address);

            if let Some(sample) = xgm_sample {
                data[1] = sample.index as u8;
            } else {
                if !crate::is_silent() {
                    println!("Warning: no corresponding sample found for VGM command at offset {:06X} in XGM", command.offset);
                }
                data[1] = 0;
            }
        } else if command.is_stream_start() {
            prio = 3 - (command.get_stream_id() as u8 & 0x3);
            let block_id = command.get_stream_block_id() as usize + 1;
            let xgm_sample = xgm_samples.iter().find(|s| s.index == block_id)
                .or_else(|| xgm_samples.iter().find(|s| s.index == prio as usize + 1));
            if let Some(sample) = xgm_sample {
                data[1] = sample.index as u8;
            } else {
                data[1] = 0;
            }
        } else if command.is_stream_stop() {
            prio = 3 - (command.get_stream_id() as u8 & 0x3);
            data[1] = 0;
            if channel == -1 {
                data[0] |= 3 - prio;
            } else {
                data[0] |= channel as u8 & 0x3;
            }
            data[0] |= prio << 2;
            return XGMCommand::new(data);
        } else {
            prio = 3;
            data[1] = 0;
            if channel == -1 {
                data[0] |= 3 - prio;
            } else {
                data[0] |= channel as u8 & 0x3;
            }
            data[0] |= prio << 2;
            return XGMCommand::new(data);
        }

        if channel == -1 {
            data[0] |= 3 - prio;
        } else {
            data[0] |= channel as u8 & 0x3;
        }
        data[0] |= prio << 2;

        XGMCommand::new(data)
    }

    pub fn create_pcm_commands(
        xgm_samples: &[crate::xgm_sample::XGMSample],
        vgm: &crate::vgm::VGM,
        commands: &[VGMCommand],
    ) -> Vec<XGMCommand> {
        let mut result = Vec::new();
        for cmd in commands {
            if cmd.is_stream_start_long() || cmd.is_stream_start() || cmd.is_stream_stop() {
                result.push(Self::create_pcm_command(xgm_samples, vgm, cmd, -1));
            }
        }
        result
    }
}
