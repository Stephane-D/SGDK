use crate::command::CommandTrait;
use crate::util;

pub const DATA_BLOCK: u8 = 0x67;
pub const END: u8 = 0x66;
pub const SEEK: u8 = 0xE0;
pub const WRITE_SN76489: u8 = 0x50;
pub const WRITE_YM2612_PORT0: u8 = 0x52;
pub const WRITE_YM2612_PORT1: u8 = 0x53;
pub const WAIT_NTSC_FRAME: u8 = 0x62;
pub const WAIT_PAL_FRAME: u8 = 0x63;
pub const STREAM_CONTROL: u8 = 0x90;
pub const STREAM_DATA: u8 = 0x91;
pub const STREAM_FREQUENCY: u8 = 0x92;
pub const STREAM_START_LONG: u8 = 0x93;
pub const STREAM_STOP: u8 = 0x94;
pub const STREAM_START: u8 = 0x95;
pub const LOOP_START: u8 = 0x30;
pub const LOOP_END: u8 = 0x31;

/// Compute size of a VGM command from data at offset
pub fn compute_size(data: &[u8], offset: usize) -> usize {
    if data.is_empty() { return 0; }
    let command = util::get_u8(data, offset);
    match command {
        0x4F | WRITE_SN76489 => 2,
        0x51 | WRITE_YM2612_PORT0 | WRITE_YM2612_PORT1 |
        0x54..=0x5F => 3,
        0x61 => 3,
        WAIT_NTSC_FRAME | WAIT_PAL_FRAME => 1,
        0x66 => 1,
        DATA_BLOCK => 7 + util::get_u32(data, offset + 0x03) as usize,
        0x68 => 12 + util::get_u24(data, offset + 0x09) as usize,
        STREAM_CONTROL => 5,
        STREAM_DATA => 5,
        STREAM_FREQUENCY => 6,
        STREAM_START_LONG => 11,
        STREAM_STOP => 2,
        STREAM_START => 5,
        _ => match command >> 4 {
            0x3 => 2,
            0x4 => 3,
            0x7 => 1,
            0x8 => 1,
            0xA | 0xB => 3,
            0xC | 0xD => 4,
            0xE | 0xF => 5,
            _ => 1,
        }
    }
}

#[derive(Clone, Debug)]
pub struct VGMCommand {
    pub data: Vec<u8>,
    pub time: i32,
    pub size: usize,
    pub origin_offset: i32,
    pub is_loop_start: bool,
}

impl VGMCommand {
    /// Create from data at offset (copies the range)
    pub fn from_data(data: &[u8], offset: usize) -> Self {
        let size = compute_size(data, offset);
        let d = data[offset..offset + size].to_vec();
        VGMCommand { data: d, time: 0, size, origin_offset: 0, is_loop_start: false }
    }

    /// Create from owned data
    pub fn new(data: Vec<u8>) -> Self {
        let size = data.len();
        VGMCommand { data, time: 0, size, origin_offset: 0, is_loop_start: false }
    }

    /// Create from byte slice
    pub fn from_slice(data: &[u8]) -> Self {
        Self::new(data.to_vec())
    }

    /// Create single-byte command
    pub fn from_command(command: u8) -> Self {
        VGMCommand { data: vec![command], time: 0, size: 1, origin_offset: 0, is_loop_start: false }
    }

    /// Create loop start marker (empty command)
    pub fn new_loop_start() -> Self {
        VGMCommand { data: vec![], time: 0, size: 0, origin_offset: 0, is_loop_start: true }
    }

    pub fn get_command(&self) -> u8 {
        if self.is_loop_start { LOOP_START }
        else if self.data.is_empty() { 0 }
        else { self.data[0] }
    }

    // --- Query methods ---
    pub fn is_data_block(&self) -> bool { self.get_command() == DATA_BLOCK }
    pub fn get_data_bank_id(&self) -> u8 { util::get_u8(&self.data, 2) }
    pub fn get_data_block_len(&self) -> usize { util::get_u32(&self.data, 3) as usize }

    pub fn is_seek(&self) -> bool { self.get_command() == SEEK }
    pub fn get_seek_address(&self) -> i32 {
        if self.is_seek() { util::get_i32(&self.data, 1) } else { -1 }
    }

    pub fn is_end(&self) -> bool { self.get_command() == END }
    pub fn is_pcm(&self) -> bool { (self.get_command() & 0xF0) == 0x80 }
    pub fn is_wait(&self) -> bool { self.is_short_wait() || (self.get_command() >= 0x61 && self.get_command() <= 0x63) }
    pub fn is_short_wait(&self) -> bool { (self.get_command() & 0xF0) == 0x70 }

    pub fn get_wait_value(&self) -> i32 {
        if self.is_short_wait() { return (self.get_command() & 0x0F) as i32 + 1; }
        if self.is_pcm() { return (self.get_command() & 0x0F) as i32; }
        match self.get_command() {
            0x61 => util::get_u16(&self.data, 1) as i32,
            WAIT_NTSC_FRAME => 0x2DF,
            WAIT_PAL_FRAME => 0x372,
            _ => 0,
        }
    }

    // --- PSG methods ---
    pub fn is_psg_write(&self) -> bool { self.get_command() == WRITE_SN76489 }
    pub fn is_psg_low_byte_write(&self) -> bool { self.is_psg_write() && (self.get_psg_value() & 0x80) == 0x80 }
    pub fn is_psg_high_byte_write(&self) -> bool { self.is_psg_write() && (self.get_psg_value() & 0x80) == 0x00 }
    pub fn is_psg_env_write(&self) -> bool { self.is_psg_low_byte_write() && (self.get_psg_value() & 0x10) == 0x10 }
    pub fn is_psg_tone_write(&self) -> bool { self.is_psg_write() && !self.is_psg_env_write() }
    pub fn is_psg_tone_low_write(&self) -> bool { self.is_psg_tone_write() && self.is_psg_low_byte_write() }
    pub fn is_psg_tone_high_write(&self) -> bool { self.is_psg_tone_write() && self.is_psg_high_byte_write() }

    pub fn get_psg_value(&self) -> u8 {
        if self.is_psg_write() { util::get_u8(&self.data, 1) } else { 0xFF }
    }

    pub fn get_psg_channel(&self) -> i32 {
        if self.is_psg_write() {
            if self.is_psg_low_byte_write() {
                ((self.get_psg_value() >> 5) & 3) as i32
            } else {
                -1 // needs tracking of last low byte channel
            }
        } else { -1 }
    }

    pub fn get_psg_frequence(&self) -> i32 {
        if self.is_psg_tone_high_write() { return ((self.get_psg_value() & 0x3F) as i32) << 4; }
        if self.is_psg_tone_low_write() { return (self.get_psg_value() & 0xF) as i32; }
        -1
    }

    pub fn get_psg_env(&self) -> i32 {
        if self.is_psg_env_write() { (self.get_psg_value() & 0xF) as i32 } else { -1 }
    }

    // --- YM2612 methods ---
    pub fn is_ym2612_port0_write(&self) -> bool { self.get_command() == WRITE_YM2612_PORT0 }
    pub fn is_ym2612_port1_write(&self) -> bool { self.get_command() == WRITE_YM2612_PORT1 }
    pub fn is_ym2612_write(&self) -> bool { self.is_ym2612_port0_write() || self.is_ym2612_port1_write() }

    pub fn get_ym2612_port(&self) -> i32 {
        if self.is_ym2612_port0_write() { 0 }
        else if self.is_ym2612_port1_write() { 1 }
        else { -1 }
    }

    pub fn get_ym2612_register(&self) -> i32 {
        if self.is_ym2612_write() { util::get_u8(&self.data, 1) as i32 } else { -1 }
    }

    pub fn get_ym2612_value(&self) -> i32 {
        if self.is_ym2612_write() { util::get_u8(&self.data, 2) as i32 } else { -1 }
    }

    pub fn is_ym2612_key_write(&self) -> bool {
        self.is_ym2612_port0_write() && self.get_ym2612_register() == 0x28
    }
    pub fn is_ym2612_key_on_write(&self) -> bool {
        self.is_ym2612_key_write() && (self.get_ym2612_value() & 0xF0) == 0xF0
    }
    pub fn is_ym2612_key_off_write(&self) -> bool {
        self.is_ym2612_key_write() && (self.get_ym2612_value() & 0xF0) == 0x00
    }
    pub fn is_ym2612_0x2x_write(&self) -> bool {
        self.is_ym2612_port0_write() && (self.get_ym2612_register() & 0xF0) == 0x20
    }
    pub fn is_ym2612_dac_on(&self) -> bool {
        self.is_ym2612_port0_write() && self.get_ym2612_register() == 0x2B && (self.get_ym2612_value() & 0x80) != 0
    }
    pub fn is_ym2612_dac_off(&self) -> bool {
        self.is_ym2612_port0_write() && self.get_ym2612_register() == 0x2B && (self.get_ym2612_value() & 0x80) == 0
    }
    pub fn is_ym2612_dac_write(&self) -> bool {
        self.is_ym2612_port0_write() && self.get_ym2612_register() == 0x2A
    }
    pub fn is_ym2612_can_ignore_write(&self) -> bool {
        if self.is_ym2612_port0_write() {
            self.get_ym2612_register() != 0x28 && self.get_ym2612_register() != 0x22 && self.get_ym2612_register() < 0x30
        } else if self.is_ym2612_port1_write() {
            self.get_ym2612_register() < 0x30
        } else { false }
    }
    pub fn is_ym2612_timers_write(&self) -> bool {
        self.is_ym2612_port0_write() && self.get_ym2612_register() == 0x27
    }
    pub fn is_ym2612_timers_no_special_no_csm_write(&self) -> bool {
        self.is_ym2612_timers_write() && (self.get_ym2612_value() & 0xC0) == 0x00
    }
    pub fn is_ym2612_general_write(&self) -> bool {
        self.is_ym2612_write() && self.get_ym2612_register() < 0x30
    }
    pub fn is_ym2612_channel_set(&self) -> bool {
        self.is_ym2612_write() && self.get_ym2612_register() >= 0x30
    }
    pub fn is_ym2612_freq_write(&self) -> bool {
        let port = self.get_ym2612_port();
        if port != -1 {
            let reg = self.get_ym2612_register();
            return reg >= 0xA0 && reg < 0xB0;
        }
        false
    }

    pub fn get_ym2612_channel(&self) -> i32 {
        let port = self.get_ym2612_port();
        if port == -1 { return -1; }
        let reg = self.get_ym2612_register();

        if port == 0 && reg == 0x28 {
            let value = self.get_ym2612_value();
            let ch = value & 3;
            if ch == 3 { return -1; }
            return ch + if (value & 4) != 0 { 3 } else { 0 };
        }
        if reg >= 0xA8 && reg < 0xB0 {
            return 2 + port * 3;
        }
        if reg >= 0x30 && reg < 0xB8 {
            let ch = reg & 3;
            if ch == 3 { return -1; }
            return ch + port * 3;
        }
        -1
    }

    pub fn get_channel(&self) -> i32 {
        if self.is_ym2612_write() { return self.get_ym2612_channel(); }
        if self.is_psg_write() { return self.get_psg_channel(); }
        -1
    }

    pub fn get_ym2612_port_channel(&self) -> i32 {
        let ch = self.get_ym2612_channel();
        if ch == -1 { return -1; }
        if ch >= 3 { ch - 3 } else { ch }
    }

    // --- Stream methods ---
    pub fn is_stream(&self) -> bool {
        self.is_stream_control() || self.is_stream_data() || self.is_stream_frequency()
            || self.is_stream_start() || self.is_stream_start_long() || self.is_stream_stop()
    }
    pub fn is_stream_control(&self) -> bool { self.get_command() == STREAM_CONTROL }
    pub fn is_stream_data(&self) -> bool { self.get_command() == STREAM_DATA }
    pub fn is_stream_frequency(&self) -> bool { self.get_command() == STREAM_FREQUENCY }
    pub fn is_stream_start(&self) -> bool { self.get_command() == STREAM_START }
    pub fn is_stream_start_long(&self) -> bool { self.get_command() == STREAM_START_LONG }
    pub fn is_stream_stop(&self) -> bool { self.get_command() == STREAM_STOP }

    pub fn get_stream_id(&self) -> i32 {
        if self.is_stream() { util::get_u8(&self.data, 1) as i32 } else { -1 }
    }
    pub fn get_stream_bank_id(&self) -> i32 {
        if self.is_stream_data() { util::get_u8(&self.data, 2) as i32 } else { -1 }
    }
    pub fn get_stream_block_id(&self) -> i32 {
        if self.is_stream_start() { util::get_u8(&self.data, 2) as i32 } else { -1 }
    }
    pub fn set_stream_block_id(&mut self, value: u8) {
        if self.is_stream_start() { util::set_u8(&mut self.data, 2, value); }
    }
    pub fn get_stream_frequency(&self) -> i32 {
        if self.is_stream_frequency() { util::get_i32(&self.data, 2) } else { -1 }
    }
    pub fn get_stream_sample_address(&self) -> i32 {
        if self.is_stream_start_long() { util::get_i32(&self.data, 2) } else { -1 }
    }
    pub fn set_stream_sample_address(&mut self, value: i32) {
        if self.is_stream_start_long() { util::set_i32(&mut self.data, 2, value); }
    }
    pub fn get_stream_sample_size(&self) -> i32 {
        if self.is_stream_start_long() { util::get_i32(&self.data, 7) } else { -1 }
    }

    // --- Static constructors ---
    pub fn create_ym_command(port: i32, reg: i32, value: i32) -> Self {
        if port == 0 {
            VGMCommand::from_slice(&[WRITE_YM2612_PORT0, reg as u8, value as u8])
        } else {
            VGMCommand::from_slice(&[WRITE_YM2612_PORT1, reg as u8, value as u8])
        }
    }

    pub fn create_ym_commands(port: i32, base_reg: i32, value: i32) -> Vec<Self> {
        let mut result = Vec::new();
        for ch in 0..3 {
            for operator in 0..4 {
                result.push(Self::create_ym_command(
                    port,
                    base_reg + ((operator & 3) << 2) + (ch & 3),
                    value,
                ));
            }
        }
        result
    }

    pub fn create_wait_commands(wait: i32) -> Vec<Self> {
        let mut result = Vec::new();
        if wait == 0x2DF {
            result.push(VGMCommand::from_command(WAIT_NTSC_FRAME));
        } else if wait == 0x372 {
            result.push(VGMCommand::from_command(WAIT_PAL_FRAME));
        } else {
            let mut remaining = wait;
            while remaining > 0 {
                let w = remaining.min(65535) as u16;
                let mut data = vec![0x61, 0, 0];
                util::set_u16(&mut data, 1, w);
                result.push(VGMCommand::new(data));
                remaining -= w as i32;
            }
        }
        result
    }

    pub fn get_key_command(commands: &[VGMCommand], channel: i32) -> Option<&VGMCommand> {
        commands.iter().find(|c| c.is_ym2612_key_write() && c.get_ym2612_channel() == channel)
    }

    pub fn get_key_off_command(commands: &[VGMCommand], channel: i32) -> Option<&VGMCommand> {
        commands.iter().find(|c| c.is_ym2612_key_off_write() && c.get_ym2612_channel() == channel)
    }

    pub fn get_command_desc(&self) -> String {
        if self.is_stream_control() { return "Stream ctrl".to_string(); }
        if self.is_stream_data() { return "Stream data".to_string(); }
        if self.is_stream_frequency() { return "Stream freq".to_string(); }
        if self.is_stream_start() { return "Stream start sh".to_string(); }
        if self.is_stream_start_long() { return "Stream start".to_string(); }
        if self.is_stream_stop() { return "Stream stop".to_string(); }
        if self.is_data_block() { return "Data block".to_string(); }
        if self.is_pcm() { return "PCM".to_string(); }
        if self.is_seek() { return "PCM seek".to_string(); }
        if self.is_loop_start { return "Loop start".to_string(); }
        if self.is_end() { return "End".to_string(); }
        if self.is_psg_env_write() { return format!("PSG env #{:X}", self.get_psg_env()); }
        if self.is_psg_tone_high_write() { return format!("PSG tone high #{:03X}", self.get_psg_frequence()); }
        if self.is_psg_tone_low_write() { return format!("PSG tone low #{:X}", self.get_psg_frequence()); }
        if self.is_short_wait() { return "Short wait".to_string(); }
        if self.is_wait() { return "Wait".to_string(); }
        if self.is_ym2612_write() {
            return util::get_ym_command_desc(
                self.get_ym2612_port(), self.get_ym2612_register(), self.get_ym2612_value()
            );
        }
        "Others".to_string()
    }
}

impl CommandTrait for VGMCommand {
    fn data(&self) -> &[u8] { &self.data }
    fn data_mut(&mut self) -> &mut Vec<u8> { &mut self.data }
    fn time(&self) -> i32 { self.time }
    fn set_time(&mut self, t: i32) { self.time = t; }
    fn size(&self) -> usize { self.size }
    fn origin_offset(&self) -> i32 { self.origin_offset }
    fn set_origin_offset(&mut self, offset: i32) { self.origin_offset = offset; }
    fn get_channel(&self) -> i32 { self.get_ym2612_channel() }
}

impl std::fmt::Display for VGMCommand {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}", self.get_command_desc())
    }
}

/// Channel sorter
pub fn sort_by_channel(commands: &mut [VGMCommand]) {
    commands.sort_by_key(|c| c.get_ym2612_channel());
}

/// Channel + register sorter
pub fn sort_by_channel_reg(commands: &mut [VGMCommand]) {
    commands.sort_by(|a, b| {
        let ch_cmp = a.get_ym2612_channel().cmp(&b.get_ym2612_channel());
        if ch_cmp != std::cmp::Ordering::Equal { return ch_cmp; }
        let ar = a.get_ym2612_register();
        let br = b.get_ym2612_register();
        // freq registers: high then low
        if (ar & 0xF4) == 0xA4 && (br & 0xF4) == 0xA0 { return std::cmp::Ordering::Less; }
        if (ar & 0xF4) == 0xA0 && (br & 0xF4) == 0xA4 { return std::cmp::Ordering::Greater; }
        ar.cmp(&br)
    });
}

/// Level OFF / normal / Level ON sorter
pub fn sort_level_off_com_level_on(commands: &mut [VGMCommand]) {
    commands.sort_by(|a, b| {
        let ra = a.get_ym2612_register();
        let va = a.get_ym2612_value();
        // TL off first
        if (ra & 0xF0) == 0x40 {
            return if (va & 0x7F) == 0x7F { std::cmp::Ordering::Less } else { std::cmp::Ordering::Greater };
        }
        if (ra & 0xF0) == 0x80 {
            return if (va & 0xF0) == 0xF0 { std::cmp::Ordering::Less } else { std::cmp::Ordering::Greater };
        }
        let rb = b.get_ym2612_register();
        let vb = b.get_ym2612_value();
        if (rb & 0xF0) == 0x40 {
            return if (vb & 0x7F) == 0x7F { std::cmp::Ordering::Greater } else { std::cmp::Ordering::Less };
        }
        if (rb & 0xF0) == 0x80 {
            return if (vb & 0xF0) == 0xF0 { std::cmp::Ordering::Greater } else { std::cmp::Ordering::Less };
        }
        std::cmp::Ordering::Equal
    });
}
