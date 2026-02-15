use crate::command::CommandTrait;
use crate::util;
use crate::vgm_command::VGMCommand;

/// XGM PSG Command for XGM2 format
#[derive(Clone, Debug)]
pub struct XGMPSGCommand {
    pub data: Vec<u8>,
    pub time: i32,
    pub size: usize,
    pub origin_offset: i32,
    pub is_loop_start: bool,
    pub dummy: bool,
}

impl XGMPSGCommand {
    pub const WAIT_SHORT: u8 = 0x00;
    pub const WAIT_LONG: u8 = 0x0E;
    pub const LOOP: u8 = 0x0F;
    pub const FREQ_LOW: u8 = 0x10;
    pub const FREQ: u8 = 0x20;
    pub const FREQ_WAIT: u8 = 0x30;
    pub const FREQ0_DELTA: u8 = 0x40;
    pub const FREQ1_DELTA: u8 = 0x50;
    pub const FREQ2_DELTA: u8 = 0x60;
    pub const FREQ3_DELTA: u8 = 0x70;
    pub const ENV0: u8 = 0x80;
    pub const ENV1: u8 = 0x90;
    pub const ENV2: u8 = 0xA0;
    pub const ENV3: u8 = 0xB0;
    pub const ENV0_DELTA: u8 = 0xC0;
    pub const ENV1_DELTA: u8 = 0xD0;
    pub const ENV2_DELTA: u8 = 0xE0;
    pub const ENV3_DELTA: u8 = 0xF0;
    pub const DUMMY: u8 = 0xFF;
    pub const LOOP_START: u8 = 0xFE;
}

impl XGMPSGCommand {
    fn compute_size(data: &[u8], offset: usize) -> usize {
        let command = util::get_u8(data, offset);

        match command {
            Self::WAIT_LONG => 2,
            Self::LOOP => 4,
            _ => match command & 0xF0 {
                0x00 => 1, // WAIT_SHORT
                0x20 | 0x30 | 0x10 => 2, // FREQ / FREQ_WAIT / FREQ_LOW
                0x40 | 0x50 | 0x60 | 0x70 => 1, // FREQ_DELTA
                0x80 | 0x90 | 0xA0 | 0xB0 => 1, // ENV
                0xC0 | 0xD0 | 0xE0 | 0xF0 => 1, // ENV_DELTA
                _ => 1,
            },
        }
    }

    pub fn from_data(data: &[u8], offset: usize) -> Self {
        let size = Self::compute_size(data, offset);
        let cmd_data = data[offset..offset + size].to_vec();
        XGMPSGCommand {
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
        XGMPSGCommand {
            data: data.to_vec(),
            time: 0,
            size,
            origin_offset: 0,
            is_loop_start: false,
            dummy: false,
        }
    }

    fn _from_command(command: u8) -> Self {
        Self::new(&[command])
    }

    pub fn new_loop_start() -> Self {
        XGMPSGCommand {
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
        if self.dummy {
            return Self::DUMMY;
        }

        let com = self.get_command();

        match com {
            Self::WAIT_LONG => Self::WAIT_LONG,
            Self::LOOP => Self::LOOP,
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
                || self.is_freq_wait() || self.is_freq_low_wait()
                || self.is_freq_delta_wait() || self.is_env_delta_wait()
        }
    }

    pub fn get_wait_frame(&self) -> i32 {
        if self.is_wait_long() {
            util::get_u8(&self.data, 1) as i32 + 15
        } else if self.is_wait_short() {
            (util::get_u8(&self.data, 0) as i32 & 0xF) + 1
        } else if self.is_wait(false) {
            1
        } else {
            0
        }
    }

    pub fn is_loop(&self) -> bool { self.get_type() == Self::LOOP }

    // --- Freq queries ---

    pub fn is_freq_wait(&self) -> bool { self.get_type() == Self::FREQ_WAIT }
    pub fn is_freq_no_wait(&self) -> bool { self.get_type() == Self::FREQ }
    pub fn is_freq(&self) -> bool { self.is_freq_no_wait() || self.is_freq_wait() }

    pub fn is_freq_low(&self) -> bool { self.get_type() == Self::FREQ_LOW }
    pub fn is_freq_low_no_wait(&self) -> bool { self.is_freq_low() && (self.data[0] & 1) == 0 }
    pub fn is_freq_low_wait(&self) -> bool { self.is_freq_low() && (self.data[0] & 1) != 0 }

    pub fn is_freq_delta(&self) -> bool {
        let t = self.get_type();
        t == Self::FREQ0_DELTA || t == Self::FREQ1_DELTA || t == Self::FREQ2_DELTA
    }
    pub fn is_freq_delta_no_wait(&self) -> bool { self.is_freq_delta() && (self.data[0] & 8) == 0 }
    pub fn is_freq_delta_wait(&self) -> bool { self.is_freq_delta() && (self.data[0] & 8) != 0 }

    // --- Env queries ---

    pub fn is_env(&self) -> bool {
        let t = self.get_type();
        t == Self::ENV0 || t == Self::ENV1 || t == Self::ENV2 || t == Self::ENV3
    }

    pub fn is_env_delta(&self) -> bool {
        let t = self.get_type();
        t == Self::ENV0_DELTA || t == Self::ENV1_DELTA || t == Self::ENV2_DELTA || t == Self::ENV3_DELTA
    }
    pub fn is_env_delta_no_wait(&self) -> bool { self.is_env_delta() && (self.data[0] & 8) == 0 }
    pub fn is_env_delta_wait(&self) -> bool { self.is_env_delta() && (self.data[0] & 8) != 0 }

    pub fn support_wait(&self) -> bool {
        self.is_env_delta() || self.is_freq() || self.is_freq_low() || self.is_freq_delta()
    }

    pub fn add_wait(&mut self) -> bool {
        if self.support_wait() {
            if self.is_env_delta_no_wait() {
                self.data[0] |= 8;
            } else if self.is_freq_no_wait() {
                self.data[0] = self.data[0].wrapping_add(Self::FREQ_WAIT - Self::FREQ);
            } else if self.is_freq_low_no_wait() {
                self.data[0] |= 1;
            } else if self.is_freq_delta_no_wait() {
                self.data[0] |= 8;
            } else {
                return false;
            }
        }
        true
    }

    // --- Getters/Setters ---

    pub fn get_freq(&self) -> i32 {
        if self.is_freq() {
            ((self.data[0] as i32 & 0x3) << 8) | (self.data[1] as i32 & 0xFF)
        } else { -1 }
    }

    pub fn set_freq(&mut self, freq: i32) {
        if self.is_freq() {
            self.data[0] = (self.data[0] & 0xFC) | ((freq >> 8) as u8 & 0x3);
            self.data[1] = (freq & 0xFF) as u8;
        }
    }

    pub fn get_freq_low(&self) -> i32 {
        if self.is_freq_low() { (self.data[1] & 0xF) as i32 } else { -1 }
    }

    pub fn set_freq_low(&mut self, freq_low: i32) {
        if self.is_freq_low() {
            self.data[1] = (self.data[1] & 0xF0) | (freq_low as u8 & 0x0F);
        }
    }

    pub fn get_freq_delta(&self) -> i32 {
        if self.is_freq_delta() {
            let delta = ((self.data[0] & 3) as i32) + 1;
            if (self.data[0] & 4) != 0 { -delta } else { delta }
        } else { 0 }
    }

    pub fn set_freq_delta(&mut self, delta: i32) {
        if self.is_freq_delta() {
            self.data[0] = (self.data[0] & 0xF8)
                | if delta < 0 { (4 | (-delta)) as u8 } else { delta as u8 };
        }
    }

    pub fn to_freq_delta(&mut self, delta: i32) {
        if self.is_freq() || self.is_freq_low() {
            let deltav = if delta < 0 { -(delta + 1) } else { delta - 1 };
            let v = (if self.is_freq_wait() || self.is_freq_low_wait() { 8 } else { 0 })
                | (if delta < 0 { 4 } else { 0 })
                | deltav;

            let ch = self.get_channel();
            let base = match ch {
                0 => Self::FREQ0_DELTA,
                1 => Self::FREQ1_DELTA,
                2 => Self::FREQ2_DELTA,
                3 => Self::FREQ3_DELTA,
                _ => return,
            };
            self.data = vec![base | v as u8];
            self.size = 1;
        }
    }

    pub fn to_freq_low(&mut self, freq_low: i32) {
        if self.is_freq() {
            let wait = self.is_freq_wait();
            let ch = self.get_channel();
            self.data[0] = Self::FREQ_LOW | if wait { 1 } else { 0 };
            self.data[1] = 0x80 | ((ch as u8) << 5) | (freq_low as u8);
        }
    }

    pub fn get_env(&self) -> i32 {
        if self.is_env() { (self.data[0] & 0xF) as i32 } else { -1 }
    }

    pub fn get_env_delta(&self) -> i32 {
        if self.is_env_delta() {
            let delta = ((self.data[0] & 3) as i32) + 1;
            if (self.data[0] & 4) != 0 { -delta } else { delta }
        } else { 0 }
    }

    pub fn to_env_delta(&mut self, delta: i32) {
        if self.is_env() {
            let deltav = if delta < 0 { -(delta + 1) } else { delta - 1 };
            let v = (if delta < 0 { 4 } else { 0 }) | deltav;

            let ch = self.get_channel();
            let base = match ch {
                0 => Self::ENV0_DELTA,
                1 => Self::ENV1_DELTA,
                2 => Self::ENV2_DELTA,
                3 => Self::ENV3_DELTA,
                _ => return,
            };
            self.data = vec![base | v as u8];
            self.size = 1;
        }
    }

    pub fn is_dummy(&self) -> bool { self.dummy }
    pub fn set_dummy(&mut self) { self.dummy = true; }
    pub fn clear_dummy(&mut self) { self.dummy = false; }

    pub fn get_loop_addr(&self) -> i32 {
        if self.is_loop() { util::get_u24(&self.data, 1) as i32 } else { -1 }
    }

    pub fn set_loop_addr(&mut self, address: i32) {
        if self.is_loop() { util::set_u24(&mut self.data, 1, address as u32); }
    }

    // ---- Static factory methods ----

    pub fn create_wait_short(wait: i32) -> Self {
        Self::new(&[Self::WAIT_SHORT | ((wait - 1) as u8 & 0xF)])
    }

    pub fn create_wait_long(wait: i32) -> Self {
        Self::new(&[Self::WAIT_LONG, (wait - 15) as u8])
    }

    pub fn create_wait_commands(wait: i32) -> Vec<Self> {
        let mut result = Vec::new();
        let mut remain = wait;

        while remain > 270 {
            result.push(Self::create_wait_long(270));
            remain -= 270;
        }
        if remain > 14 {
            result.push(Self::create_wait_long(remain));
        } else {
            result.push(Self::create_wait_short(remain));
        }
        result
    }

    pub fn create_frame() -> Self {
        Self::new(&[Self::WAIT_SHORT])
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

    pub fn create_psg_commands(commands: &[VGMCommand]) -> Vec<Self> {
        let mut result = Vec::new();
        let remaining: Vec<VGMCommand> = commands.to_vec();
        let mut tone_commands: Vec<VGMCommand> = Vec::new();
        let mut to_remove: Vec<usize> = Vec::new();

        if commands.is_empty() {
            return result;
        }

        let mut low_byte_com: Option<(usize, VGMCommand)> = None;

        for (idx, com) in remaining.iter().enumerate() {
            if com.is_psg_high_byte_write() {
                if let Some((prev_idx, ref prev_com)) = low_byte_com {
                    if prev_com.is_psg_tone_low_write() {
                        tone_commands.push(prev_com.clone());
                        tone_commands.push(com.clone());
                        to_remove.push(prev_idx);
                        to_remove.push(idx);
                    } else {
                        // This would be env overwrite; simplified handling
                        to_remove.push(idx);
                    }
                }
                low_byte_com = None;
            } else {
                low_byte_com = Some((idx, com.clone()));
            }
        }

        // Get remaining commands (not in tone_commands and not removed)
        let remaining_filtered: Vec<&VGMCommand> = remaining.iter().enumerate()
            .filter(|(idx, _)| !to_remove.contains(idx))
            .map(|(_, c)| c)
            .collect();

        // add complete tone commands
        result.extend(Self::create_psg_tone_commands(&tone_commands));

        // then add byte commands
        for com in remaining_filtered {
            if com.is_psg_low_byte_write() {
                // skip tone commands we already handled
                if !tone_commands.iter().any(|tc| std::ptr::eq(tc, com)) {
                    result.push(Self::create_psg_byte_command(com));
                }
            }
        }

        result
    }

    fn create_psg_tone_commands(commands: &[VGMCommand]) -> Vec<Self> {
        let mut result = Vec::new();
        let mut i = 0;
        while i + 1 < commands.len() {
            result.push(Self::create_psg_tone_command(&commands[i], &commands[i + 1]));
            i += 2;
        }
        result
    }

    fn create_psg_byte_command(command: &VGMCommand) -> Self {
        if command.is_psg_env_write() {
            let base = match command.get_psg_channel() {
                0 => Self::ENV0,
                1 => Self::ENV1,
                2 => Self::ENV2,
                3 => Self::ENV3,
                _ => Self::ENV0,
            };
            Self::new(&[base | (command.get_psg_value() as u8 & 0xF)])
        } else if command.is_psg_tone_low_write() {
            Self::new(&[
                Self::FREQ_LOW,
                0x80 | ((command.get_psg_channel() as u8) << 5) | (command.get_psg_value() as u8 & 0xF),
            ])
        } else {
            eprintln!("Invalid PSG byte command: {:02X}", command.get_psg_value());
            Self::new(&[0])
        }
    }

    fn create_psg_tone_command(vgm_command_low: &VGMCommand, vgm_command_high: &VGMCommand) -> Self {
        let mut data = [0u8; 2];

        data[0] = Self::FREQ;
        // set channel
        data[0] |= (vgm_command_low.get_psg_channel() as u8) << 2;
        // set value
        data[0] |= (vgm_command_high.get_psg_value() as u8 >> 4) & 3;
        data[1] = (vgm_command_high.get_psg_value() as u8 & 0xF) << 4;
        data[1] |= vgm_command_low.get_psg_value() as u8 & 0xF;

        Self::new(&data)
    }

    // ---- Filter methods ----

    pub fn filter_channel(commands: &[XGMPSGCommand], channel: i32, get_wait: bool, get_loop_start: bool) -> Vec<XGMPSGCommand> {
        commands.iter()
            .filter(|c| c.get_channel() == channel || (get_wait && c.is_wait(true)) || (get_loop_start && c.is_loop_start))
            .cloned()
            .collect()
    }

    pub fn filter_env(commands: &[XGMPSGCommand]) -> Vec<XGMPSGCommand> {
        commands.iter().filter(|c| c.is_env()).cloned().collect()
    }

    pub fn filter_freq(commands: &[XGMPSGCommand]) -> Vec<XGMPSGCommand> {
        commands.iter().filter(|c| c.is_freq()).cloned().collect()
    }

    pub fn has_wait_command(commands: &[XGMPSGCommand]) -> bool {
        commands.iter().any(|c| c.get_wait_frame() > 0)
    }
}

impl CommandTrait for XGMPSGCommand {
    fn data(&self) -> &[u8] { &self.data }
    fn data_mut(&mut self) -> &mut Vec<u8> { &mut self.data }
    fn time(&self) -> i32 { self.time }
    fn set_time(&mut self, t: i32) { self.time = t; }
    fn size(&self) -> usize { self.size }
    fn origin_offset(&self) -> i32 { self.origin_offset }
    fn set_origin_offset(&mut self, offset: i32) { self.origin_offset = offset; }

    fn get_channel(&self) -> i32 {
        match self.get_type() {
            Self::ENV0 | Self::ENV0_DELTA | Self::FREQ0_DELTA => 0,
            Self::ENV1 | Self::ENV1_DELTA | Self::FREQ1_DELTA => 1,
            Self::ENV2 | Self::ENV2_DELTA | Self::FREQ2_DELTA => 2,
            Self::ENV3 | Self::ENV3_DELTA | Self::FREQ3_DELTA => 3,
            _ => {
                if self.is_freq() {
                    (util::get_u8(&self.data, 0) as i32 >> 2) & 3
                } else if self.is_freq_low() {
                    (util::get_u8(&self.data, 1) as i32 >> 5) & 3
                } else {
                    -1
                }
            }
        }
    }
}

impl std::fmt::Display for XGMPSGCommand {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        if self.is_dummy() {
            write!(f, "PSG DUMMY - {}", Self::new(&self.data))
        } else if self.is_wait_short() {
            write!(f, "PSG WAIT S #{}", self.get_wait_frame())
        } else if self.is_wait_long() {
            write!(f, "PSG WAIT L #{}", self.get_wait_frame())
        } else if self.is_env() {
            write!(f, "PSG ENV #{:01X}", self.get_env())
        } else if self.is_env_delta_no_wait() {
            write!(f, "PSG ENVD {}", self.get_env_delta())
        } else if self.is_env_delta_wait() {
            write!(f, "PSG ENVD {} W", self.get_env_delta())
        } else if self.is_freq_no_wait() {
            write!(f, "PSG FREQ #{:03X}", self.get_freq())
        } else if self.is_freq_wait() {
            write!(f, "PSG FREQ W #{:03X}", self.get_freq())
        } else if self.is_freq_low_no_wait() {
            write!(f, "PSG FREQL #{:01X}", self.get_freq_low())
        } else if self.is_freq_low_wait() {
            write!(f, "PSG FREQL W #{:01X}", self.get_freq_low())
        } else if self.is_freq_delta_no_wait() {
            write!(f, "PSG FREQD {}", self.get_freq_delta())
        } else if self.is_freq_delta_wait() {
            write!(f, "PSG FREQD W {}", self.get_freq_delta())
        } else if self.is_loop_start {
            write!(f, "PSG LOOP St")
        } else if self.is_loop() {
            write!(f, "PSG LOOP #{:06X}", self.get_loop_addr())
        } else {
            write!(f, "PSG ???")
        }
    }
}
