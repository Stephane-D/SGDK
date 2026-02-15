use crate::command::CommandTrait;
use crate::util;
use crate::vgm_command::VGMCommand;
use crate::ym2612_state::YM2612State;

/// XGM FM Command for XGM2 format
#[derive(Clone, Debug)]
pub struct XGMFMCommand {
    pub data: Vec<u8>,
    pub time: i32,
    pub size: usize,
    pub origin_offset: i32,
    pub is_loop_start: bool,
    pub dummy: bool,
}

// Command type constants
impl XGMFMCommand {
    pub const WAIT_SHORT: u8 = 0x00;
    pub const WAIT_LONG: u8 = 0x0F;
    pub const PCM: u8 = 0x10;
    pub const FM_LOAD_INST: u8 = 0x20;
    pub const FM_FREQ: u8 = 0x30;
    pub const FM_KEY: u8 = 0x40;
    pub const FM_KEY_SEQ: u8 = 0x50;
    pub const FM0_PAN: u8 = 0x60;
    pub const FM1_PAN: u8 = 0x70;
    pub const FM_FREQ_WAIT: u8 = 0x80;
    pub const FM_TL: u8 = 0x90;
    pub const FM_FREQ_DELTA: u8 = 0xA0;
    pub const FM_FREQ_DELTA_WAIT: u8 = 0xB0;
    pub const FM_TL_DELTA: u8 = 0xC0;
    pub const FM_TL_DELTA_WAIT: u8 = 0xD0;
    pub const FM_WRITE: u8 = 0xE0;
    pub const FRAME_DELAY: u8 = 0xF0;
    pub const FM_KEY_ADV: u8 = 0xF8;
    pub const FM_LFO: u8 = 0xF9;
    pub const FM_CH3_SPECIAL_ON: u8 = 0xFA;
    pub const FM_CH3_SPECIAL_OFF: u8 = 0xFB;
    pub const FM_DAC_ON: u8 = 0xFC;
    pub const FM_DAC_OFF: u8 = 0xFD;
    pub const LOOP: u8 = 0xFF;
    pub const DUMMY: u8 = 0xF7;
    pub const LOOP_START: u8 = 0xF6;
}

impl XGMFMCommand {
    fn compute_size(data: &[u8], offset: usize) -> usize {
        let command = util::get_u8(data, offset);

        match command {
            Self::WAIT_LONG => 2,
            Self::FM_KEY_ADV => 2,
            Self::FM_LFO => 2,
            Self::FM_CH3_SPECIAL_ON | Self::FM_CH3_SPECIAL_OFF
            | Self::FM_DAC_ON | Self::FM_DAC_OFF => 1,
            Self::FRAME_DELAY => 1,
            Self::LOOP => 4,
            _ => match command & 0xF0 {
                0x00 => 1, // WAIT_SHORT
                0x10 => 2, // PCM
                0x20 => 31, // FM_LOAD_INST
                0x30 | 0x80 => 3, // FM_FREQ / FM_FREQ_WAIT
                0x40 => 1, // FM_KEY
                0x50 => 1, // FM_KEY_SEQ
                0x90 | 0xC0 | 0xD0 => 2, // FM_TL / FM_TL_DELTA / FM_TL_DELTA_WAIT
                0x60 | 0x70 => 1, // FM0_PAN / FM1_PAN
                0xE0 => 1 + (2 * (((command & 0x07) + 1) as usize)), // FM_WRITE
                0xA0 | 0xB0 => 2, // FM_FREQ_DELTA / FM_FREQ_DELTA_WAIT
                _ => 1,
            },
        }
    }

    pub fn from_data(data: &[u8], offset: usize) -> Self {
        let size = Self::compute_size(data, offset);
        let cmd_data = data[offset..offset + size].to_vec();
        XGMFMCommand {
            data: cmd_data,
            time: 0,
            size,
            origin_offset: 0,
            is_loop_start: false,
            dummy: false,
        }
    }

    pub fn new(data: &[u8]) -> Self {
        let size = data.len();
        XGMFMCommand {
            data: data.to_vec(),
            time: 0,
            size,
            origin_offset: 0,
            is_loop_start: false,
            dummy: false,
        }
    }

    pub fn from_data_with_time(data: &[u8], offset: usize, time: i32) -> Self {
        let mut cmd = Self::from_data(data, offset);
        cmd.time = time;
        cmd
    }

    fn from_command(command: u8) -> Self {
        Self::new(&[command])
    }

    pub fn new_loop_start() -> Self {
        XGMFMCommand {
            data: Vec::new(),
            time: 0,
            size: 0,
            origin_offset: 0,
            is_loop_start: true,
            dummy: false,
        }
    }

    pub fn get_command(&self) -> u8 {
        if self.data.is_empty() { 0 } else { self.data[0] }
    }

    pub fn get_type(&self) -> u8 {
        if self.is_loop_start {
            return Self::LOOP_START;
        }

        let com = self.get_command();

        match com {
            Self::WAIT_LONG | Self::FM_KEY_ADV | Self::FM_LFO
            | Self::FM_CH3_SPECIAL_ON | Self::FM_CH3_SPECIAL_OFF
            | Self::FM_DAC_ON | Self::FM_DAC_OFF
            | Self::FRAME_DELAY | Self::LOOP | Self::DUMMY => com,
            _ => com & 0xF0,
        }
    }

    // --- Wait queries ---

    pub fn is_wait_short(&self) -> bool { self.get_type() == Self::WAIT_SHORT }
    pub fn is_wait_long(&self) -> bool { self.get_type() == Self::WAIT_LONG }

    pub fn is_wait(&self, real_wait_only: bool) -> bool {
        if real_wait_only {
            self.is_wait_short() || self.is_wait_long()
        } else {
            self.is_wait_short() || self.is_wait_long()
                || self.is_ym_freq_write_wait() || self.is_ym_freq_delta_write_wait()
                || self.is_ym_tl_delta_wait() || self.is_frame_delay()
        }
    }

    pub fn get_wait_frame(&self) -> i32 {
        if self.is_wait_long() {
            util::get_u8(&self.data, 1) as i32 + 16
        } else if self.is_wait_short() {
            (util::get_u8(&self.data, 0) as i32 & 0xF) + 1
        } else if self.is_wait(false) {
            1
        } else {
            0
        }
    }

    pub fn is_dummy(&self) -> bool { self.get_type() == Self::DUMMY }
    pub fn is_frame_delay(&self) -> bool { self.get_type() == Self::FRAME_DELAY }
    pub fn is_loop(&self) -> bool { self.get_type() == Self::LOOP }
    pub fn is_pcm(&self) -> bool { self.get_type() == Self::PCM }
    pub fn is_ym_load_inst(&self) -> bool { self.get_type() == Self::FM_LOAD_INST }
    pub fn is_ym_write(&self) -> bool { self.get_type() == Self::FM_WRITE }

    // --- Freq queries ---

    pub fn is_ym_freq_write_no_wait(&self) -> bool { self.get_type() == Self::FM_FREQ }
    pub fn is_ym_freq_write_wait(&self) -> bool { self.get_type() == Self::FM_FREQ_WAIT }
    pub fn is_ym_freq_write(&self) -> bool { self.is_ym_freq_write_no_wait() || self.is_ym_freq_write_wait() }

    pub fn get_ym_freq_value(&self) -> i32 {
        if self.is_ym_freq_write() {
            (((self.data[1] as i32) & 0x3F) << 8) | (self.data[2] as i32 & 0xFF)
        } else { -1 }
    }

    pub fn is_ym_freq_special_write(&self) -> bool {
        self.is_ym_freq_write() && (self.get_command() & 8) != 0
    }
    pub fn is_ym_freq_with_key_on(&self) -> bool {
        self.is_ym_freq_write() && (self.data[1] & 0x80) != 0
    }
    pub fn is_ym_freq_with_key_off(&self) -> bool {
        self.is_ym_freq_write() && (self.data[1] & 0x40) != 0
    }
    pub fn is_ym_freq_with_key_write(&self) -> bool {
        self.is_ym_freq_write() && (self.data[1] & 0xC0) != 0
    }
    pub fn set_ym_freq_key_on(&mut self) {
        if self.is_ym_freq_write() { self.data[1] |= 0x80; }
    }
    pub fn set_ym_freq_key_off(&mut self) {
        if self.is_ym_freq_write() { self.data[1] |= 0x40; }
    }

    // --- Freq delta queries ---

    pub fn is_ym_freq_delta_write_no_wait(&self) -> bool { self.get_type() == Self::FM_FREQ_DELTA }
    pub fn is_ym_freq_delta_write_wait(&self) -> bool { self.get_type() == Self::FM_FREQ_DELTA_WAIT }
    pub fn is_ym_freq_delta_write(&self) -> bool { self.is_ym_freq_delta_write_no_wait() || self.is_ym_freq_delta_write_wait() }
    pub fn is_ym_freq_delta_special_write(&self) -> bool {
        self.is_ym_freq_delta_write() && (self.get_command() & 8) != 0
    }

    pub fn get_ym_freq_delta_value(&self) -> i32 {
        if self.is_ym_freq_delta_write() {
            let delta = ((self.data[1] >> 1) & 0x7F) as i32 + 1;
            if (self.data[1] & 1) != 0 { -delta } else { delta }
        } else { -1 }
    }

    pub fn to_freq_delta(&mut self, delta: i32) -> bool {
        if self.is_ym_freq_write() {
            let cmd = if self.is_ym_freq_write_wait() { Self::FM_FREQ_DELTA_WAIT } else { Self::FM_FREQ_DELTA };
            let deltav = if delta < 0 { -(delta + 1) } else { delta - 1 };
            self.data = vec![
                cmd | (self.data[0] & 0xF),
                (if delta < 0 { 1u8 } else { 0u8 }) | ((deltav as u8) << 1),
            ];
            self.size = 2;
            true
        } else { false }
    }

    // --- Key queries ---

    pub fn is_ym_key_fast_write(&self) -> bool { self.get_type() == Self::FM_KEY }
    pub fn is_ym_key_on_write(&self) -> bool { self.is_ym_key_fast_write() && (self.data[0] & 8) != 0 }
    pub fn is_ym_key_off_write(&self) -> bool { self.is_ym_key_fast_write() && (self.data[0] & 8) == 0 }
    pub fn is_ym_key_sequence(&self) -> bool { self.get_type() == Self::FM_KEY_SEQ }
    pub fn is_ym_key_sequence_on_off(&self) -> bool { self.is_ym_key_sequence() && (self.data[0] & 8) != 0 }
    pub fn is_ym_key_sequence_off_on(&self) -> bool { self.is_ym_key_sequence() && (self.data[0] & 8) == 0 }
    pub fn is_ym_key_adv_write(&self) -> bool { self.get_type() == Self::FM_KEY_ADV }
    pub fn is_ym_key_write(&self) -> bool { self.is_ym_key_fast_write() || self.is_ym_key_sequence() || self.is_ym_key_adv_write() }

    pub fn to_key_seq(&mut self, on_off: bool) -> bool {
        if self.is_ym_key_fast_write() {
            self.data = vec![Self::FM_KEY_SEQ | (self.data[0] & 0x7) | if on_off { 8 } else { 0 }];
            self.size = 1;
            true
        } else { false }
    }

    // --- TL queries ---

    pub fn is_ym_set_tl(&self) -> bool { self.get_type() == Self::FM_TL }
    pub fn get_ym_tl_value(&self) -> i32 {
        if self.is_ym_set_tl() { ((self.data[1] >> 1) & 0x7F) as i32 } else { -1 }
    }
    pub fn is_ym_tl_delta_no_wait(&self) -> bool { self.get_type() == Self::FM_TL_DELTA }
    pub fn is_ym_tl_delta_wait(&self) -> bool { self.get_type() == Self::FM_TL_DELTA_WAIT }
    pub fn is_ym_tl_delta(&self) -> bool { self.is_ym_tl_delta_no_wait() || self.is_ym_tl_delta_wait() }

    pub fn get_ym_tl_delta(&self) -> i32 {
        if self.is_ym_tl_delta() {
            let delta = ((self.data[1] >> 2) & 0x3F) as i32 + 1;
            if (self.data[1] & 2) != 0 { -delta } else { delta }
        } else { -1 }
    }

    pub fn to_tl_delta(&mut self, delta: i32) -> bool {
        if self.is_ym_set_tl() {
            let deltav = if delta < 0 { -(delta + 1) } else { delta - 1 };
            self.data = vec![
                Self::FM_TL_DELTA | (self.data[0] & 0xF),
                (self.data[1] & 1) | (if delta < 0 { 2u8 } else { 0u8 }) | ((deltav as u8) << 2),
            ];
            self.size = 2;
            true
        } else { false }
    }

    // --- Misc queries ---

    pub fn is_ym_pan(&self) -> bool { self.get_type() == Self::FM0_PAN || self.get_type() == Self::FM1_PAN }
    pub fn is_ym_ch3_special_mode(&self) -> bool { self.get_type() == Self::FM_CH3_SPECIAL_ON || self.get_type() == Self::FM_CH3_SPECIAL_OFF }
    pub fn is_ym_dac_mode(&self) -> bool { self.get_type() == Self::FM_DAC_ON || self.get_type() == Self::FM_DAC_OFF }
    pub fn is_ym_lfo(&self) -> bool { self.get_type() == Self::FM_LFO }
    pub fn is_ym_setting(&self) -> bool { self.is_ym_ch3_special_mode() || self.is_ym_dac_mode() || self.is_ym_lfo() }

    pub fn support_wait(&self) -> bool {
        self.is_ym_freq_write() || self.is_ym_freq_delta_write() || self.is_ym_tl_delta()
    }

    pub fn add_wait(&mut self) {
        if self.support_wait() {
            if self.is_ym_freq_write_no_wait() {
                self.data[0] = self.data[0].wrapping_add(Self::FM_FREQ_WAIT - Self::FM_FREQ);
            } else if self.is_ym_freq_delta_write_no_wait() {
                self.data[0] = self.data[0].wrapping_add(Self::FM_FREQ_DELTA_WAIT - Self::FM_FREQ_DELTA);
            } else if self.is_ym_tl_delta_no_wait() {
                self.data[0] = self.data[0].wrapping_add(Self::FM_TL_DELTA_WAIT - Self::FM_TL_DELTA);
            }
        }
    }

    // --- PCM queries ---

    pub fn get_pcm_id(&self) -> i32 {
        if self.get_type() == Self::PCM { util::get_u8(&self.data, 1) as i32 } else { -1 }
    }
    pub fn set_pcm_id(&mut self, value: i32) {
        if self.get_type() == Self::PCM { util::set_u8(&mut self.data, 1, value as u8); }
    }
    pub fn get_pcm_channel(&self) -> i32 {
        if self.get_type() == Self::PCM { (self.data[0] & 3) as i32 } else { 0 }
    }
    pub fn get_pcm_half_rate(&self) -> bool {
        self.get_type() == Self::PCM && (self.data[0] & 4) != 0
    }

    // --- Port/Channel/Slot queries ---

    pub fn get_ym_port(&self) -> i32 {
        match self.get_type() {
            Self::FM0_PAN => 0,
            Self::FM1_PAN => 1,
            Self::FM_LOAD_INST | Self::FM_FREQ | Self::FM_FREQ_WAIT
            | Self::FM_FREQ_DELTA | Self::FM_FREQ_DELTA_WAIT
            | Self::FM_KEY | Self::FM_KEY_SEQ => (self.data[0] as i32 >> 2) & 1,
            Self::FM_WRITE => (self.data[0] as i32 >> 3) & 1,
            Self::FM_TL | Self::FM_TL_DELTA | Self::FM_TL_DELTA_WAIT => (self.data[1] as i32) & 1,
            Self::FM_KEY_ADV => (self.data[1] as i32 >> 2) & 1,
            _ => 0,
        }
    }

    pub fn get_ym_channel(&self) -> i32 {
        match self.get_type() {
            Self::FM_FREQ | Self::FM_FREQ_WAIT | Self::FM_FREQ_DELTA | Self::FM_FREQ_DELTA_WAIT => {
                if (self.data[0] & 8) != 0 {
                    2
                } else {
                    (self.data[0] & 3) as i32
                }
            }
            Self::FM_LOAD_INST | Self::FM_KEY | Self::FM_KEY_SEQ
            | Self::FM_TL | Self::FM_TL_DELTA | Self::FM_TL_DELTA_WAIT
            | Self::FM0_PAN | Self::FM1_PAN => (self.data[0] & 3) as i32,
            Self::FM_WRITE => {
                if (self.data[1] & 0xF8) == 0xA8 {
                    2
                } else {
                    (self.data[1] & 3) as i32
                }
            }
            Self::FM_KEY_ADV => (self.data[1] & 3) as i32,
            _ => -1,
        }
    }

    pub fn get_ym_global_channel(&self) -> i32 {
        (self.get_ym_port() * 3) + self.get_ym_channel()
    }

    pub fn get_ym_slot(&self) -> i32 {
        match self.get_type() {
            Self::FM_FREQ | Self::FM_FREQ_WAIT | Self::FM_FREQ_DELTA | Self::FM_FREQ_DELTA_WAIT => {
                if (self.data[0] & 8) != 0 { ((self.data[0] & 3) as i32) + 1 } else { -1 }
            }
            Self::FM_TL | Self::FM_TL_DELTA | Self::FM_TL_DELTA_WAIT => {
                ((self.data[0] >> 2) & 3) as i32
            }
            _ => -1,
        }
    }

    pub fn get_ym_num_write(&self) -> i32 {
        if self.is_ym_write() { (self.data[0] & 7) as i32 + 1 } else { -1 }
    }

    pub fn get_loop_addr(&self) -> i32 {
        if self.is_loop() { util::get_u24(&self.data, 1) as i32 } else { -1 }
    }

    pub fn set_loop_addr(&mut self, address: i32) {
        if self.is_loop() { util::set_u24(&mut self.data, 1, address as u32); }
    }

    pub fn set_dummy(&mut self) {
        let mut new_data = vec![0u8; self.data.len() + 1];
        new_data[0] = Self::DUMMY;
        new_data[1..].copy_from_slice(&self.data);
        self.data = new_data;
        self.size = self.data.len();
    }

    fn set_ym_write(&mut self, header: u8, data_list: &[i32]) {
        if data_list.is_empty() {
            self.set_dummy();
            return;
        }

        let mut new_data = vec![0u8; data_list.len() + 1];
        new_data[0] = header & 0xF8;
        new_data[0] |= ((data_list.len() / 2) - 1) as u8;

        for i in (0..data_list.len()).step_by(2) {
            new_data[i + 1] = data_list[i] as u8;
            new_data[i + 2] = data_list[i + 1] as u8;
        }

        self.data = new_data;
        self.size = self.data.len();
    }

    // ---- Static factory methods ----

    pub fn create_frame_command() -> Self {
        Self::from_command(Self::WAIT_SHORT)
    }

    pub fn create_wait_short(wait: i32) -> Self {
        Self::new(&[Self::WAIT_SHORT | ((wait - 1) as u8 & 0xF)])
    }

    pub fn create_wait_long(wait: i32) -> Self {
        Self::new(&[Self::WAIT_LONG, (wait - 16) as u8])
    }

    pub fn create_wait_commands(wait: i32) -> Vec<Self> {
        let mut result = Vec::new();
        let mut remain = wait;

        while remain > 271 {
            result.push(Self::create_wait_long(271));
            remain -= 271;
        }
        if remain > 15 {
            result.push(Self::create_wait_long(remain));
        } else {
            result.push(Self::create_wait_short(remain));
        }
        result
    }

    pub fn create_frame_delay() -> Self {
        Self::new(&[Self::FRAME_DELAY])
    }

    pub fn create_loop(offset: i32) -> Self {
        Self::new(&[
            Self::LOOP,
            (offset & 0xFF) as u8,
            ((offset >> 8) & 0xFF) as u8,
            ((offset >> 16) & 0xFF) as u8,
        ])
    }

    pub fn create_end() -> Self {
        Self::create_loop(-1)
    }

    pub fn create_pcm_stop(channel: i32) -> Self {
        Self::new(&[Self::PCM | (channel as u8 & 0x3), 0])
    }

    pub fn create_ym_key_command(command: &VGMCommand) -> Self {
        let value = command.get_ym2612_value();
        let key_write = value & 0xF0;
        let ch = value & 0x07;

        if key_write == 0x00 || key_write == 0xF0 {
            Self::new(&[Self::FM_KEY | (if key_write == 0 { 0x00 } else { 0x08 }) | ch as u8])
        } else {
            Self::new(&[Self::FM_KEY_ADV, value as u8])
        }
    }

    pub fn create_ym_key_commands(commands: &[VGMCommand]) -> Vec<Self> {
        commands.iter().map(|c| Self::create_ym_key_command(c)).collect()
    }

    pub fn create_ym_ch_command(commands: &mut Vec<VGMCommand>, channel: i32) -> Self {
        let size = 8.min(commands.len());
        let mut data = vec![0u8; (size * 2) + 1];

        data[0] = Self::FM_WRITE;
        data[0] |= if channel >= 3 { 8 } else { 0 };
        data[0] |= size as u8 - 1;

        let mut off = 1;
        for i in 0..size {
            let command = &commands[i];
            data[off] = command.get_ym2612_register() as u8;
            off += 1;
            data[off] = command.get_ym2612_value() as u8;
            off += 1;
        }

        commands.drain(0..size);
        Self::new(&data)
    }

    pub fn create_ym_ch_commands(commands: &[VGMCommand], channel: i32) -> Vec<Self> {
        let mut result = Vec::new();
        let mut remaining: Vec<VGMCommand> = commands.to_vec();

        while !remaining.is_empty() {
            result.push(Self::create_ym_ch_command(&mut remaining, channel));
        }
        result
    }

    pub fn create_ym_misc_command(command: &VGMCommand) -> Option<Self> {
        match command.get_ym2612_register() {
            0x22 => {
                // LFO
                Some(Self::new(&[Self::FM_LFO, command.get_ym2612_value() as u8]))
            }
            0x27 => {
                // CH2 special mode
                if (command.get_ym2612_value() & 0x40) != 0 {
                    Some(Self::new(&[Self::FM_CH3_SPECIAL_ON]))
                } else {
                    Some(Self::new(&[Self::FM_CH3_SPECIAL_OFF]))
                }
            }
            0x2B => {
                // DAC enable
                if (command.get_ym2612_value() & 0x80) != 0 {
                    Some(Self::new(&[Self::FM_DAC_ON]))
                } else {
                    Some(Self::new(&[Self::FM_DAC_OFF]))
                }
            }
            reg => {
                if !crate::is_silent() {
                    println!("VGM --> XGM conversion: write to register {:02X} ignored", reg);
                }
                None
            }
        }
    }

    pub fn create_ym_misc_commands(commands: &[VGMCommand]) -> Vec<Self> {
        commands.iter().filter_map(|c| Self::create_ym_misc_command(c)).collect()
    }

    fn create_ym_freq_command_from_vgm(commands: &[VGMCommand], key_off_before: bool, key_on_after: bool) -> Self {
        let com_freq_high = &commands[0];
        let com_freq_low = &commands[1];

        let port = com_freq_high.get_ym2612_port();
        let ch = com_freq_high.get_ym2612_port_channel();
        let reg = com_freq_high.get_ym2612_register();
        let spe = if reg >= 0xA8 && reg < 0xB0 { 1 } else { 0 };

        Self::new(&[
            Self::FM_FREQ | (port << 3) as u8 | (spe << 2) as u8 | ch as u8,
            (com_freq_high.get_ym2612_value() as u8 & 0x3F)
                | (if key_off_before { 0x40 } else { 0x00 })
                | (if key_on_after { 0x80 } else { 0x00 }),
            com_freq_low.get_ym2612_value() as u8,
        ])
    }

    pub fn create_ym_freq_commands(commands: &[VGMCommand]) -> Vec<Self> {
        let mut result = Vec::new();
        let mut i = 0;
        while i + 1 < commands.len() {
            result.push(Self::create_ym_freq_command_from_vgm(&commands[i..i+2], false, false));
            i += 2;
        }
        result
    }

    pub fn create_ym_freq_command(channel: i32, special: bool, freq: i32, key_off_before: bool, key_on_after: bool) -> Self {
        let port = if channel >= 3 { 1 } else { 0 };
        let ch = if port == 1 { (channel + 1) & 3 } else { channel & 3 };

        Self::new(&[
            Self::FM_FREQ | (port << 2) as u8 | (if special { 8 } else { 0 }) | ch as u8,
            (((freq >> 8) & 0x3F) as u8)
                | (if key_off_before { 0x40 } else { 0x00 })
                | (if key_on_after { 0x80 } else { 0x00 }),
            (freq & 0xFF) as u8,
        ])
    }

    pub fn create_pcm_command(xgm: &crate::xgm::XGM, command: &VGMCommand, channel: i32) -> Option<Self> {
        let base = Self::PCM | (channel as u8 & 0x3);

        if command.is_stream_start_long() {
            let sample = xgm.get_sample_by_origin_address(command.get_stream_sample_address());
            match sample {
                Some(s) => {
                    let mut d0 = base;
                    if s.half_rate { d0 |= 4; }
                    Some(Self::new(&[d0, s.id as u8]))
                }
                None => {
                    eprintln!("Warning: no corresponding sample found in XGM (origin sample addr={:06X})", command.get_stream_sample_address());
                    None
                }
            }
        } else if command.is_stream_start() {
            let sample = xgm.get_sample_by_origin_id(command.get_stream_block_id() + 1);
            match sample {
                Some(s) => {
                    let mut d0 = base;
                    if s.half_rate { d0 |= 4; }
                    Some(Self::new(&[d0, s.id as u8]))
                }
                None => {
                    eprintln!("Warning: no corresponding sample found in XGM (origin sample id={:02X})", command.get_stream_block_id() + 1);
                    None
                }
            }
        } else {
            // stop command
            Some(Self::new(&[base, 0]))
        }
    }

    pub fn create_pcm_commands(xgm: &crate::xgm::XGM, commands: &[VGMCommand]) -> Vec<Self> {
        let mut result = Vec::new();
        for command in commands {
            if command.is_stream_start_long() || command.is_stream_start() || command.is_stream_stop() {
                if let Some(xgm_cmd) = Self::create_pcm_command(xgm, command, 0) {
                    result.push(xgm_cmd);
                }
            }
        }
        result
    }

    pub fn convert_to_set_panning_commands(
        port: i32, ch: i32, com: &mut XGMFMCommand, ym_state: &YM2612State,
    ) -> Vec<XGMFMCommand> {
        let mut result = Vec::new();

        if !com.is_ym_write() {
            return result;
        }

        let mut ignored: Vec<i32> = Vec::new();
        let num = (com.data[0] & 0x07) as usize + 1;

        for i in 0..num {
            let reg = ((util::get_u8(&com.data, (i * 2) + 1) as i32) & 0xFC) | ch;
            let value = util::get_u8(&com.data, (i * 2) + 2) as i32;
            let pan = (value >> 6) & 3;

            if reg >= 0xB4 && reg < 0xB8
                && (ym_state.get(port as usize, reg as usize) & 0x3F) == (value & 0x3F)
            {
                let mut data = vec![0u8; 1];
                data[0] = if port == 0 { Self::FM0_PAN } else { Self::FM1_PAN };
                data[0] |= ch as u8;
                data[0] |= (pan as u8) << 2;

                let mut cmd = Self::new(&data);
                cmd.time = com.time;
                result.push(cmd);
            } else {
                ignored.push(reg);
                ignored.push(value);
            }
        }

        com.set_ym_write(com.data[0], &ignored);
        result
    }

    pub fn convert_to_set_tl_commands(
        _port: i32, ch: i32, com: &mut XGMFMCommand,
    ) -> Vec<XGMFMCommand> {
        let mut result = Vec::new();

        if !com.is_ym_write() {
            return result;
        }

        let mut ignored: Vec<i32> = Vec::new();
        let num = (com.data[0] & 0x07) as usize + 1;

        for i in 0..num {
            let reg = util::get_u8(&com.data, (i * 2) + 1) as i32;
            let value = util::get_u8(&com.data, (i * 2) + 2) as i32;

            if reg >= 0x40 && reg < 0x50 {
                let s = (reg >> 2) & 3;
                let mut data = vec![0u8; 2];
                data[0] = Self::FM_TL + ((s as u8) << 2) + ch as u8;
                data[1] = ((value & 0x7F) << 1) as u8;
                data[1] |= (_port & 1) as u8;

                let mut cmd = Self::new(&data);
                cmd.time = com.time;
                result.push(cmd);
            } else {
                ignored.push(reg);
                ignored.push(value);
            }
        }

        com.set_ym_write(com.data[0], &ignored);
        result
    }

    pub fn convert_to_load_inst_command(
        port: i32, ch: i32, fm_write_commands: &mut [XGMFMCommand], ym_state: &YM2612State,
    ) -> Self {
        let mut data = vec![0u8; 31];
        let mut ignored: Vec<i32> = Vec::new();

        data[0] = Self::FM_LOAD_INST | ((port as u8 & 1) << 2) | (ch as u8 & 3);

        // initialize from YM state
        let mut d = 1;
        for r in 0..7 {
            for s in 0..4 {
                data[d] = ym_state.get(port as usize, 0x30 + (r << 4) + (s << 2) + ch as usize) as u8;
                d += 1;
            }
        }
        data[d] = ym_state.get(port as usize, 0xB0 + ch as usize) as u8;
        d += 1;
        data[d] = ym_state.get(port as usize, 0xB4 + ch as usize) as u8;

        let mut time = 0;

        for com in fm_write_commands.iter_mut() {
            if !com.is_ym_write() {
                eprintln!("Warning: convertToLoadInstCommand - unexpected command #{:02X}", com.get_type());
                continue;
            }

            time = com.time;
            ignored.clear();
            let num = com.get_ym_num_write();

            for i in 0..num as usize {
                let reg = util::get_u8(&com.data, (i * 2) + 1) as i32;
                let value = util::get_u8(&com.data, (i * 2) + 2) as i32;

                if reg >= 0x30 && reg < 0xA0 {
                    let r = (reg >> 4) & 0xF;
                    let s = (reg >> 2) & 3;
                    let index = ((r - 3) * 4 + s) as usize;
                    data[index + 1] = value as u8;
                } else if reg >= 0xB0 && reg <= 0xB8 {
                    data[29 + if reg >= 0xB4 { 1 } else { 0 }] = value as u8;
                } else {
                    ignored.push(reg);
                    ignored.push(value);
                }
            }

            com.set_ym_write(com.data[0], &ignored);
        }

        let mut cmd = Self::new(&data);
        cmd.time = time;
        cmd
    }

    // ---- Filter methods ----

    pub fn filter_channel(commands: &[XGMFMCommand], channel: i32, get_wait: bool, get_loop_start: bool) -> Vec<XGMFMCommand> {
        commands.iter()
            .filter(|c| c.get_channel() == channel || (get_wait && c.is_wait(true)) || (get_loop_start && c.is_loop_start))
            .cloned()
            .collect()
    }

    pub fn filter_ym_write(commands: &[XGMFMCommand]) -> Vec<XGMFMCommand> {
        commands.iter().filter(|c| c.is_ym_write()).cloned().collect()
    }

    pub fn filter_ym_write_range(commands: &[XGMFMCommand], start: usize, end: usize) -> Vec<XGMFMCommand> {
        commands[start..end].iter().filter(|c| c.is_ym_write()).cloned().collect()
    }

    pub fn filter_ym_freq(commands: &[XGMFMCommand], want_freq_low: bool) -> Vec<XGMFMCommand> {
        commands.iter()
            .filter(|c| c.is_ym_freq_write() || (want_freq_low && c.is_ym_freq_delta_write()))
            .cloned()
            .collect()
    }

    pub fn find_next_ym_key_command(commands: &[XGMFMCommand], start_ind: usize) -> usize {
        for i in start_ind..commands.len() {
            if commands[i].is_ym_key_write() {
                return i;
            }
        }
        commands.len()
    }
}

impl CommandTrait for XGMFMCommand {
    fn data(&self) -> &[u8] { &self.data }
    fn data_mut(&mut self) -> &mut Vec<u8> { &mut self.data }
    fn time(&self) -> i32 { self.time }
    fn set_time(&mut self, t: i32) { self.time = t; }
    fn size(&self) -> usize { self.size }
    fn origin_offset(&self) -> i32 { self.origin_offset }
    fn set_origin_offset(&mut self, offset: i32) { self.origin_offset = offset; }
    fn get_channel(&self) -> i32 { self.get_ym_global_channel() }
}

impl std::fmt::Display for XGMFMCommand {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        if self.is_wait_short() {
            write!(f, "FM WAIT S #{}", self.get_wait_frame())
        } else if self.is_wait_long() {
            write!(f, "FM WAIT L #{}", self.get_wait_frame())
        } else if self.is_pcm() {
            write!(f, "PCM #{}", self.get_pcm_id())
        } else if self.is_ym_load_inst() {
            write!(f, "FM LOADINST")
        } else if self.is_ym_freq_special_write() {
            write!(f, "FM FREQ{} S{} {}{}",
                if self.is_ym_freq_write_wait() { " W" } else { "" },
                self.get_ym_slot(),
                if (self.data[1] & 0x40) != 0 { "x" } else { "" },
                if (self.data[1] & 0x80) != 0 { "o" } else { "" })
        } else if self.is_ym_freq_write() {
            write!(f, "FM FREQ{} {}{}",
                if self.is_ym_freq_write_wait() { " W" } else { "" },
                if (self.data[1] & 0x40) != 0 { "x" } else { "" },
                if (self.data[1] & 0x80) != 0 { "o" } else { "" })
        } else if self.is_ym_freq_delta_special_write() {
            write!(f, "FM FREQD {}{} S{}",
                self.get_ym_freq_delta_value(),
                if self.is_ym_freq_delta_write_wait() { " W" } else { "" },
                self.get_ym_slot())
        } else if self.is_ym_freq_delta_write() {
            write!(f, "FM FREQD {}{}",
                self.get_ym_freq_delta_value(),
                if self.is_ym_freq_delta_write_wait() { " W" } else { "" })
        } else if self.is_ym_key_on_write() {
            write!(f, "FM KEY ON")
        } else if self.is_ym_key_off_write() {
            write!(f, "FM KEY OFF")
        } else if self.is_ym_key_adv_write() {
            write!(f, "FM KEY ADV")
        } else if self.is_ym_key_sequence() {
            write!(f, "FM KEY SEQ")
        } else if self.is_ym_key_write() {
            write!(f, "FM KEY")
        } else if self.is_ym_set_tl() {
            write!(f, "FM TL S{}", self.get_ym_slot())
        } else if self.is_ym_tl_delta() {
            write!(f, "FM TLD {}{} S{}",
                self.get_ym_tl_delta(),
                if self.is_ym_tl_delta_wait() { " W." } else { "" },
                self.get_ym_slot())
        } else if self.is_ym_pan() {
            write!(f, "FM PAN")
        } else if self.is_ym_dac_mode() {
            if self.get_type() == Self::FM_DAC_ON {
                write!(f, "DAC ON")
            } else {
                write!(f, "DAC OFF")
            }
        } else if self.is_ym_lfo() {
            write!(f, "LFO")
        } else if self.is_ym_ch3_special_mode() {
            write!(f, "CH2 spe")
        } else if self.is_ym_write() {
            write!(f, "FM WRITE")
        } else if self.is_frame_delay() {
            write!(f, "FRAME DELAY")
        } else if self.is_loop_start {
            write!(f, "FM LOOP St")
        } else if self.is_loop() {
            write!(f, "FM LOOP #{:06X}", self.get_loop_addr())
        } else if self.is_dummy() {
            write!(f, "FM DUMMY ({})", Self::from_data(&self.data, 1))
        } else {
            write!(f, "FM ???")
        }
    }
}
