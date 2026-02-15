/// VGM command constants
pub const VGM_DATA_BLOCK: u8 = 0x67;
pub const VGM_END: u8 = 0x66;
pub const VGM_SEEK: u8 = 0xE0;
pub const VGM_WRITE_SN76489: u8 = 0x50;
pub const VGM_WRITE_YM2612_PORT0: u8 = 0x52;
pub const VGM_WRITE_YM2612_PORT1: u8 = 0x53;
pub const VGM_WAIT_NTSC_FRAME: u8 = 0x62;
pub const VGM_WAIT_PAL_FRAME: u8 = 0x63;
pub const VGM_STREAM_CONTROL: u8 = 0x90;
pub const VGM_STREAM_DATA: u8 = 0x91;
pub const VGM_STREAM_FREQUENCY: u8 = 0x92;
pub const VGM_STREAM_START_LONG: u8 = 0x93;
pub const VGM_STREAM_STOP: u8 = 0x94;
pub const VGM_STREAM_START: u8 = 0x95;
pub const VGM_LOOP_START: u8 = 0x30;
pub const VGM_LOOP_END: u8 = 0x31;
pub const VGM_WRITE_RF5C68: u8 = 0xB0;
pub const VGM_WRITE_RF5C164: u8 = 0xB1;

/// Represents a single VGM file command
#[derive(Clone, Debug)]
pub struct VGMCommand {
    pub data: Vec<u8>,
    pub offset: usize,
    pub command: u8,
    pub size: usize,
    pub time: i32,
}

impl VGMCommand {
    /// Create a simple command (no data, just command byte)
    pub fn new(command: u8, time: i32) -> Self {
        VGMCommand {
            data: vec![command],
            offset: 0,
            command,
            size: 1,
            time,
        }
    }

    /// Create command from data at offset
    pub fn from_data(data: &[u8], offset: usize, time: i32) -> Self {
        let command = data[offset];
        let mut cmd = VGMCommand {
            data: Vec::new(),
            offset: 0,
            command,
            size: 0,
            time,
        };
        cmd.size = cmd.compute_size_from(data, offset);
        // Copy data for this command
        cmd.data = data[offset..offset + cmd.size].to_vec();
        cmd.offset = 0;
        cmd
    }

    /// Create command from owned data
    pub fn from_owned(data: Vec<u8>, time: i32) -> Self {
        let command = data[0];
        let mut cmd = VGMCommand {
            data: Vec::new(),
            offset: 0,
            command,
            size: 0,
            time,
        };
        cmd.size = cmd.compute_size_from_owned(&data);
        cmd.data = data;
        cmd
    }

    fn compute_size_from(&self, data: &[u8], offset: usize) -> usize {
        match self.command {
            0x4F | 0x50 => 2,
            0x51..=0x5F => 3,
            0x61 => 3,
            0x62 | 0x63 => 1,
            0x66 => 1,
            0x67 => 7 + crate::get_u32(data, offset + 3) as usize,
            0x68 => 12 + crate::get_u24(data, offset + 9) as usize,
            0x90 | 0x91 => 5,
            0x92 => 6,
            0x93 => 11,
            0x94 => 2,
            0x95 => 5,
            _ => match self.command >> 4 {
                0x3 => 2,
                0x4 => 3,
                0x7 => 1,
                0x8 => 1,
                0xA | 0xB => 3,
                0xC | 0xD => 4,
                0xE | 0xF => 5,
                _ => 1,
            },
        }
    }

    fn compute_size_from_owned(&self, data: &[u8]) -> usize {
        self.compute_size_from(data, 0)
    }

    pub fn is_data_block(&self) -> bool { self.command == VGM_DATA_BLOCK }
    pub fn get_data_bank_id(&self) -> u8 { self.data[self.offset + 2] }
    pub fn get_data_block_len(&self) -> u32 { crate::get_u32(&self.data, self.offset + 3) }

    pub fn is_seek(&self) -> bool { self.command == VGM_SEEK }
    pub fn get_seek_address(&self) -> i32 {
        if self.is_seek() { crate::get_u32(&self.data, self.offset + 1) as i32 } else { -1 }
    }

    pub fn is_end(&self) -> bool { self.command == VGM_END }
    pub fn is_loop_start(&self) -> bool { self.command == VGM_LOOP_START }
    pub fn is_loop_end(&self) -> bool { self.command == VGM_LOOP_END }

    pub fn is_pcm(&self) -> bool { (self.command & 0xF0) == 0x80 }

    pub fn is_wait(&self) -> bool {
        self.is_short_wait() || (self.command >= 0x61 && self.command <= 0x63)
    }
    pub fn is_wait_ntsc(&self) -> bool { self.command == 0x62 }
    pub fn is_wait_pal(&self) -> bool { self.command == 0x63 }
    pub fn is_short_wait(&self) -> bool { (self.command & 0xF0) == 0x70 }

    pub fn get_wait_value(&self) -> i32 {
        if self.is_short_wait() {
            return (self.command & 0x0F) as i32 + 1;
        }
        if self.is_pcm() {
            return (self.command & 0x0F) as i32;
        }
        match self.command {
            0x61 => crate::get_u16(&self.data, self.offset + 1) as i32,
            0x62 => 0x2DF,
            0x63 => 0x372,
            _ => 0,
        }
    }

    pub fn as_byte_array(&self) -> Vec<u8> {
        self.data[self.offset..self.offset + self.size].to_vec()
    }

    // PSG methods
    pub fn is_psg_write(&self) -> bool { self.command == VGM_WRITE_SN76489 }
    pub fn is_psg_env_write(&self) -> bool {
        self.command == VGM_WRITE_SN76489 && (self.get_psg_value() & 0x91) == 0x91
    }
    pub fn is_psg_tone_write(&self) -> bool {
        self.is_psg_write() && !self.is_psg_env_write()
    }
    pub fn get_psg_value(&self) -> i32 {
        if self.is_psg_write() { self.data[self.offset + 1] as i32 & 0xFF } else { -1 }
    }

    // YM2612 methods
    pub fn is_ym2612_port0_write(&self) -> bool { self.command == VGM_WRITE_YM2612_PORT0 }
    pub fn is_ym2612_port1_write(&self) -> bool { self.command == VGM_WRITE_YM2612_PORT1 }
    pub fn is_ym2612_write(&self) -> bool {
        self.is_ym2612_port0_write() || self.is_ym2612_port1_write()
    }
    pub fn get_ym2612_port(&self) -> i32 {
        if self.is_ym2612_port0_write() { 0 }
        else if self.is_ym2612_port1_write() { 1 }
        else { -1 }
    }
    pub fn get_ym2612_channel(&self) -> i32 {
        if self.is_ym2612_port0_write() {
            (self.get_ym2612_register() & 3) as i32
        } else if self.is_ym2612_port1_write() {
            (self.get_ym2612_register() & 3) as i32 + 3
        } else {
            -1
        }
    }
    pub fn get_ym2612_register(&self) -> i32 {
        if self.is_ym2612_write() { self.data[self.offset + 1] as i32 & 0xFF } else { -1 }
    }
    pub fn get_ym2612_value(&self) -> i32 {
        if self.is_ym2612_write() { self.data[self.offset + 2] as i32 & 0xFF } else { -1 }
    }

    pub fn is_ym2612_key_write(&self) -> bool {
        self.is_ym2612_port0_write() && self.get_ym2612_register() == 0x28
    }
    pub fn is_ym2612_key_off_write(&self) -> bool {
        self.is_ym2612_key_write() && (self.get_ym2612_value() & 0xF0) == 0x00
    }
    pub fn is_ym2612_key_on_write(&self) -> bool {
        self.is_ym2612_key_write() && (self.get_ym2612_value() & 0xF0) != 0x00
    }
    pub fn get_ym2612_key_channel(&self) -> i32 {
        if self.is_ym2612_key_write() {
            let reg = self.get_ym2612_value() & 0x7;
            if reg == 3 || reg == 7 { return -1; }
            let mut r = reg;
            if r >= 4 { r -= 1; }
            r
        } else {
            -1
        }
    }

    pub fn is_ym2612_0x2x_write(&self) -> bool {
        self.is_ym2612_port0_write() && (self.get_ym2612_register() & 0xF0) == 0x20
    }
    pub fn is_ym2612_timers_write(&self) -> bool {
        self.is_ym2612_port0_write() && self.get_ym2612_register() == 0x27
    }
    pub fn is_ym2612_timers_no_special_no_csm_write(&self) -> bool {
        self.is_ym2612_timers_write() && (self.get_ym2612_value() & 0xC0) == 0x00
    }
    pub fn is_dac_enabled(&self) -> bool {
        self.is_ym2612_port0_write() && self.get_ym2612_register() == 0x2B
    }
    pub fn is_dac_enabled_on(&self) -> bool {
        self.is_dac_enabled() && (self.get_ym2612_value() & 0x80) == 0x80
    }
    pub fn is_dac_enabled_off(&self) -> bool {
        self.is_dac_enabled() && (self.get_ym2612_value() & 0x80) == 0x00
    }

    // Stream methods
    pub fn is_stream(&self) -> bool {
        self.is_stream_control() || self.is_stream_data() || self.is_stream_frequency()
            || self.is_stream_start() || self.is_stream_start_long() || self.is_stream_stop()
    }
    pub fn is_stream_control(&self) -> bool { self.command == VGM_STREAM_CONTROL }
    pub fn is_stream_data(&self) -> bool { self.command == VGM_STREAM_DATA }
    pub fn is_stream_frequency(&self) -> bool { self.command == VGM_STREAM_FREQUENCY }
    pub fn is_stream_start(&self) -> bool { self.command == VGM_STREAM_START }
    pub fn is_stream_start_long(&self) -> bool { self.command == VGM_STREAM_START_LONG }
    pub fn is_stream_stop(&self) -> bool { self.command == VGM_STREAM_STOP }

    pub fn get_stream_id(&self) -> i32 {
        if self.is_stream() { self.data[self.offset + 1] as i32 & 0xFF } else { -1 }
    }
    pub fn get_stream_bank_id(&self) -> i32 {
        if self.is_stream_data() { self.data[self.offset + 2] as i32 & 0xFF } else { -1 }
    }
    pub fn get_stream_block_id(&self) -> i32 {
        if self.is_stream_start() { self.data[self.offset + 2] as i32 & 0xFF } else { -1 }
    }
    pub fn get_stream_frequency(&self) -> i32 {
        if self.is_stream_frequency() { crate::get_u32(&self.data, self.offset + 2) as i32 } else { -1 }
    }
    pub fn get_stream_sample_address(&self) -> i32 {
        if self.is_stream_start_long() { crate::get_u32(&self.data, self.offset + 2) as i32 } else { -1 }
    }
    pub fn get_stream_sample_size(&self) -> i32 {
        if self.is_stream_start_long() { crate::get_u32(&self.data, self.offset + 7) as i32 } else { -1 }
    }

    pub fn is_same(&self, other: &VGMCommand) -> bool {
        if self.size != other.size { return false; }
        self.data[self.offset..self.offset + self.size]
            == other.data[other.offset..other.offset + other.size]
    }

    pub fn is_rf5c68_control(&self) -> bool {
        self.command == VGM_WRITE_RF5C68 || self.command == VGM_WRITE_RF5C164
    }

    /// Create a YM2612 write command
    pub fn create_ym_command(port: i32, reg: u8, value: u8) -> Self {
        let cmd_byte = if port == 0 { VGM_WRITE_YM2612_PORT0 } else { VGM_WRITE_YM2612_PORT1 };
        VGMCommand {
            data: vec![cmd_byte, reg, value],
            offset: 0,
            command: cmd_byte,
            size: 3,
            time: -1,
        }
    }

    /// Create YM commands for all channels/operators  
    pub fn create_ym_commands(port: i32, base_reg: u8, value: u8) -> Vec<VGMCommand> {
        let mut result = Vec::new();
        for ch in 0..3u8 {
            for op in 0..4u8 {
                result.push(VGMCommand::create_ym_command(
                    port,
                    base_reg.wrapping_add((op & 3) << 2).wrapping_add(ch & 3),
                    value,
                ));
            }
        }
        result
    }
}

// Helper functions for command lists

pub fn contains_command(commands: &[VGMCommand], command: &VGMCommand) -> bool {
    commands.iter().any(|c| c.is_same(command))
}

pub fn get_key_on_command(commands: &[VGMCommand], channel: i32) -> Option<usize> {
    commands.iter().position(|c| c.is_ym2612_key_on_write() && c.get_ym2612_key_channel() == channel)
}

pub fn get_key_off_command(commands: &[VGMCommand], channel: i32) -> Option<usize> {
    commands.iter().position(|c| c.is_ym2612_key_off_write() && c.get_ym2612_key_channel() == channel)
}

pub fn get_key_command(commands: &[VGMCommand], channel: i32) -> Option<usize> {
    commands.iter().position(|c| c.is_ym2612_key_write() && c.get_ym2612_key_channel() == channel)
}
