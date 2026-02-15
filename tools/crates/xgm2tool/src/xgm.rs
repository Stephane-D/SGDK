use std::collections::HashMap;

use crate::command::{self, CommandTrait};
use crate::gd3::GD3;
use crate::util;
use crate::vgm::VGM;
use crate::vgm_command::VGMCommand;
use crate::xd3::XD3;
use crate::xgc_packer;
use crate::xgm_fm_command::XGMFMCommand;
use crate::xgm_psg_command::XGMPSGCommand;
use crate::xgm_sample::XGMSample;
use crate::ym2612_state::YM2612State;
use anyhow::Result;

/// XGM2 format handler
pub struct XGM {
    pub samples: Vec<XGMSample>,
    pub fm_commands: Vec<XGMFMCommand>,
    pub psg_commands: Vec<XGMPSGCommand>,
    pub gd3: Option<GD3>,
    pub xd3: Option<XD3>,
    pub pal: bool,
    pub packed: bool,
}

impl XGM {
    /// Empty XGM
    pub fn new() -> Self {
        XGM {
            samples: Vec::new(),
            fm_commands: Vec::new(),
            psg_commands: Vec::new(),
            gd3: None,
            xd3: None,
            pal: false,
            packed: false,
        }
    }

    /// Parse XGM2 from binary data
    pub fn from_data(data: &[u8]) -> Result<Self> {
        let mut xgm = Self::new();

        if !crate::is_silent() {
            println!("Parsing XGM file...");
        }

        let magic = util::get_ascii_string_n(data, 0, 4);
        if !magic.eq_ignore_ascii_case("XGM2") {
            anyhow::bail!("Error: XGM2 file not recognized!");
        }

        let flags = util::get_u8(data, 5);
        xgm.pal = (flags & 1) != 0;
        if (flags & 2) != 0 {
            anyhow::bail!("Cannot convert from multi tracks XGM file!");
        }
        let has_gd3 = (flags & 4) != 0;
        xgm.packed = (flags & 8) != 0;

        let pcm_len = util::get_u16(data, 0x0006) as usize * 256;
        let fm_len = util::get_u16(data, 0x0008) as usize * 256;
        let psg_len = util::get_u16(data, 0x000A) as usize * 256;

        // Parse sample table (124 entries = 248 bytes)
        for s in 0..123 {
            let addr = util::get_u16(data, (s * 2) + 0x000C) as usize;
            let naddr = util::get_u16(data, ((s + 1) * 2) + 0x000C) as usize;

            if addr != 0xFFFF && naddr != 0xFFFF {
                xgm.samples.push(XGMSample::new_simple(
                    (s + 1) as i32,
                    data[0x0104 + (addr << 8)..0x0104 + (naddr << 8)].to_vec(),
                ));
            }
        }

        let offset = 0x0104 + pcm_len;

        if xgm.packed {
            let fm_data = xgc_packer::unpack(&data[offset..offset + fm_len]);
            let psg_data = xgc_packer::unpack(&data[offset + fm_len..offset + fm_len + psg_len]);
            xgm.parse_fm_music(&fm_data);
            xgm.parse_psg_music(&psg_data);
            xgm.set_fm_loop_address(0);
            xgm.set_psg_loop_address(0);
        } else {
            xgm.parse_fm_music(&data[offset..offset + fm_len]);
            xgm.parse_psg_music(&data[offset + fm_len..offset + fm_len + psg_len]);
        }

        xgm.update_times();
        xgm.update_offsets();

        // GD3 tags
        if has_gd3 {
            let tag_offset = offset + fm_len + psg_len;
            if xgm.packed {
                let xd3 = XD3::from_data(data, tag_offset);
                xgm.gd3 = Some(GD3::from_xd3(&xd3));
                xgm.xd3 = Some(xd3);
            } else {
                let gd3 = GD3::from_data(data, tag_offset);
                let total_frames = xgm.get_total_time_in_frame();
                let loop_frames = xgm.get_loop_duration_in_frame();
                xgm.xd3 = Some(XD3::from_gd3(&gd3, total_frames, loop_frames));
                xgm.gd3 = Some(gd3);
            }
        }

        if !crate::is_silent() {
            println!("Number of PCM sample: {}", xgm.samples.len());
            println!("Number of FM command : {}", xgm.fm_commands.len());
            println!("Number of PSG command : {}", xgm.psg_commands.len());
            println!("PCM data size: {}", xgm.get_pcm_data_size());
            println!("FM music data size: {}", xgm.get_fm_music_data_size());
            println!("PSG music data size: {}", xgm.get_psg_music_data_size());
            println!(
                "XGM duration: {} frames ({} seconds) - loop: {} frames ({} seconds)",
                xgm.get_total_time_in_frame(),
                xgm.get_total_time_in_second(),
                xgm.get_loop_duration_in_frame(),
                xgm.get_loop_duration_in_second(),
            );
        }

        Ok(xgm)
    }

    /// Convert VGM to XGM
    pub fn from_vgm(vgm: &VGM, pack: bool) -> Result<Self> {
        let mut xgm = Self::new();

        if !crate::is_silent() {
            println!("Converting VGM to XGM...");
        }

        xgm.pal = vgm.rate == 50;
        xgm.gd3 = vgm.gd3.clone();
        xgm.packed = pack;

        // extract samples from VGM
        xgm.extract_samples(vgm);
        // extract music data
        xgm.extract_music(vgm);
        // XGM optimization
        xgm.optimize_commands();
        // samples optimization
        xgm.optimize_samples();

        // update times and offsets
        xgm.update_times();
        xgm.update_offsets();

        // update loop points
        xgm.update_loop_offsets();

        // build XD3 after duration has been computed
        if let Some(ref gd3) = xgm.gd3 {
            let total_frames = xgm.get_total_time_in_frame();
            let loop_frames = xgm.get_loop_duration_in_frame();
            xgm.xd3 = Some(XD3::from_gd3(gd3, total_frames, loop_frames));
        }

        if !crate::is_silent() {
            println!("Number of PCM sample: {}", xgm.samples.len());
            println!("Number of FM command : {}", xgm.fm_commands.len());
            println!("Number of PSG command : {}", xgm.psg_commands.len());
            println!("PCM data size: {}", xgm.get_pcm_data_size());
            println!("FM music data size: {}", xgm.get_fm_music_data_size());
            println!("PSG music data size: {}", xgm.get_psg_music_data_size());
            println!(
                "XGM duration: {} frames ({} seconds) - loop: {} frames ({} seconds)",
                xgm.get_total_time_in_frame(),
                xgm.get_total_time_in_second(),
                xgm.get_loop_duration_in_frame(),
                xgm.get_loop_duration_in_second(),
            );
        }

        Ok(xgm)
    }

    // --- Parse ---

    fn parse_fm_music(&mut self, data: &[u8]) {
        let mut off = 0;
        while off < data.len() {
            let command = XGMFMCommand::from_data(data, off);
            let is_loop = command.is_loop();
            off += command.size;
            self.fm_commands.push(command);
            if is_loop {
                break;
            }
        }
    }

    fn parse_psg_music(&mut self, data: &[u8]) {
        let mut off = 0;
        while off < data.len() {
            let command = XGMPSGCommand::from_data(data, off);
            let is_loop = command.is_loop();
            off += command.size;
            self.psg_commands.push(command);
            if is_loop {
                break;
            }
        }
    }

    // --- Sample extraction ---

    fn extract_samples(&mut self, vgm: &VGM) {
        for bank in &vgm.sample_banks {
            for sample in &bank.samples {
                let id = (self.samples.len() + 1) as i32;
                if let Some(xgm_sample) = XGMSample::create_from_vgm_sample(id, sample, &bank.data) {
                    self.samples.push(xgm_sample);
                }
            }
        }
    }

    // --- Music extraction ---

    fn extract_music(&mut self, vgm: &VGM) {
        let mut ym_key_commands: HashMap<i32, VGMCommand> = HashMap::new();
        let mut ym_channel_set_commands: HashMap<i32, Vec<VGMCommand>> = HashMap::new();
        let mut ym_freq_commands: HashMap<i32, i32> = HashMap::new();
        let mut ym_misc_commands: Vec<VGMCommand> = Vec::new();
        let mut psg_commands: Vec<VGMCommand> = Vec::new();
        let mut sample_commands: Vec<VGMCommand> = Vec::new();
        let mut frame_commands: Vec<VGMCommand> = Vec::new();

        let mut new_fm_commands: Vec<XGMFMCommand> = Vec::new();
        let mut new_psg_commands: Vec<XGMPSGCommand> = Vec::new();

        let mut index = 0;
        let mut high_freq_latch: i32 = 0;

        while index < vgm.commands.len() {
            new_fm_commands.clear();
            new_psg_commands.clear();

            let mut frame_to_wait: i32 = 0;

            // get frame commands
            frame_commands.clear();
            while index < vgm.commands.len() {
                let command = &vgm.commands[index];
                index += 1;

                if command.is_loop_start {
                    new_fm_commands.push(XGMFMCommand::new_loop_start());
                    new_psg_commands.push(XGMPSGCommand::new_loop_start());
                    continue;
                }
                if command.is_data_block() {
                    continue;
                }
                if command.is_wait() {
                    frame_to_wait += command.get_wait_value();
                    while index < vgm.commands.len() {
                        let next = &vgm.commands[index];
                        if !next.is_wait() {
                            break;
                        }
                        frame_to_wait += next.get_wait_value();
                        index += 1;
                    }
                    break;
                }
                if command.is_end() {
                    break;
                }
                frame_commands.push(command.clone());
            }

            // group commands
            ym_key_commands.clear();
            ym_channel_set_commands.clear();
            ym_freq_commands.clear();
            ym_misc_commands.clear();
            psg_commands.clear();
            sample_commands.clear();

            for command in &frame_commands {
                if command.is_stream() {
                    sample_commands.push(command.clone());
                } else if command.is_psg_write() {
                    psg_commands.push(command.clone());
                } else if command.is_ym2612_write() {
                    let ch = command.get_ym2612_channel();

                    // pending key event for this channel → transfer now
                    if ym_key_commands.contains_key(&ch) {
                        let compiled = Self::compile_ym_commands(
                            &mut ym_channel_set_commands,
                            &mut ym_freq_commands,
                            &mut ym_misc_commands,
                            &mut ym_key_commands,
                        );
                        new_fm_commands.extend(compiled);
                    }

                    if command.is_ym2612_freq_write() {
                        let reg = command.get_ym2612_register();
                        if (reg & 4) == 4 {
                            high_freq_latch = (command.get_ym2612_value() & 0x3F) << 8;
                        } else {
                            let c = if (reg & 8) != 0 {
                                8 + (reg & 3)
                            } else {
                                ch
                            };
                            ym_freq_commands.insert(
                                c,
                                command.get_ym2612_value() | high_freq_latch,
                            );
                        }
                    } else if command.is_ym2612_key_write() {
                        ym_key_commands.insert(ch, command.clone());
                    } else if command.is_ym2612_channel_set() {
                        ym_channel_set_commands
                            .entry(ch)
                            .or_insert_with(Vec::new)
                            .push(command.clone());
                    } else {
                        ym_misc_commands.push(command.clone());
                    }
                } else if crate::is_verbose() {
                    println!("Command {} ignored", command);
                }
            }

            // YM commands first
            let compiled = Self::compile_ym_commands(
                &mut ym_channel_set_commands,
                &mut ym_freq_commands,
                &mut ym_misc_commands,
                &mut ym_key_commands,
            );
            new_fm_commands.extend(compiled);

            // PCM commands
            if !sample_commands.is_empty() {
                new_fm_commands.extend(XGMFMCommand::create_pcm_commands(self, &sample_commands));
            }
            // PSG commands
            if !psg_commands.is_empty() {
                new_psg_commands.extend(XGMPSGCommand::create_psg_commands(&psg_commands));
            }

            // last frame?
            if index >= vgm.commands.len() {
                new_fm_commands.push(XGMFMCommand::create_end());
                new_psg_commands.push(XGMPSGCommand::create_end());
            } else {
                while frame_to_wait > 0 {
                    new_fm_commands.push(XGMFMCommand::create_frame_command());
                    new_psg_commands.push(XGMPSGCommand::create_frame());
                    if self.pal {
                        frame_to_wait -= 882;
                    } else {
                        frame_to_wait -= 735;
                    }
                }
            }

            self.fm_commands.extend(new_fm_commands.drain(..));
            self.psg_commands.extend(new_psg_commands.drain(..));
        }
    }

    fn compile_ym_commands(
        ym_channel_set: &mut HashMap<i32, Vec<VGMCommand>>,
        ym_freq: &mut HashMap<i32, i32>,
        ym_misc: &mut Vec<VGMCommand>,
        ym_key: &mut HashMap<i32, VGMCommand>,
    ) -> Vec<XGMFMCommand> {
        let mut result = Vec::new();

        // channel set YM commands (sorted by channel)
        let mut channels: Vec<i32> = ym_channel_set.keys().cloned().collect();
        channels.sort();
        for ch_key in &channels {
            if let Some(coms) = ym_channel_set.get(ch_key) {
                result.extend(XGMFMCommand::create_ym_ch_commands(coms, *ch_key));
            }
        }

        // frequency commands (sorted by channel)
        let mut freq_channels: Vec<i32> = ym_freq.keys().cloned().collect();
        freq_channels.sort();
        for ch_key in &freq_channels {
            let ch = *ch_key;
            let freq = ym_freq[ch_key];
            let special = (ch & 8) != 0;
            let ch_val = ch & 7;
            result.push(XGMFMCommand::create_ym_freq_command(
                ch_val, special, freq, false, false,
            ));
        }

        // misc commands
        if !ym_misc.is_empty() {
            result.extend(XGMFMCommand::create_ym_misc_commands(ym_misc));
        }

        // key commands
        let key_values: Vec<VGMCommand> = ym_key.values().cloned().collect();
        if !key_values.is_empty() {
            result.extend(XGMFMCommand::create_ym_key_commands(&key_values));
        }

        ym_channel_set.clear();
        ym_freq.clear();
        ym_misc.clear();
        ym_key.clear();

        result
    }

    // --- Loop management ---

    fn set_fm_loop_address(&mut self, addr: i32) {
        if let Some(last) = self.fm_commands.last_mut() {
            last.set_loop_addr(addr);
        }
    }

    fn set_psg_loop_address(&mut self, addr: i32) {
        if let Some(last) = self.psg_commands.last_mut() {
            last.set_loop_addr(addr);
        }
    }

    fn update_loop_offsets(&mut self) {
        let fm_offset = self
            .get_fm_loop_start_command_index()
            .map(|i| self.fm_commands[i].origin_offset);
        let psg_offset = self
            .get_psg_loop_start_command_index()
            .map(|i| self.psg_commands[i].origin_offset);

        if let Some(off) = fm_offset {
            self.set_fm_loop_address(off);
        }
        if let Some(off) = psg_offset {
            self.set_psg_loop_address(off);
        }
    }

    pub fn get_fm_loop_start_command_index(&self) -> Option<usize> {
        self.fm_commands
            .iter()
            .position(|c| c.is_loop_start)
    }

    fn get_fm_loop_start_command(&self) -> Option<&XGMFMCommand> {
        self.get_fm_loop_start_command_index()
            .map(|i| &self.fm_commands[i])
    }

    fn _get_fm_loop_command(&self) -> Option<&XGMFMCommand> {
        self.fm_commands.iter().find(|c| c.is_loop())
    }

    pub fn get_psg_loop_start_command_index(&self) -> Option<usize> {
        self.psg_commands
            .iter()
            .position(|c| c.is_loop_start)
    }

    fn _get_psg_loop_start_command(&self) -> Option<&XGMPSGCommand> {
        self.get_psg_loop_start_command_index()
            .map(|i| &self.psg_commands[i])
    }

    // --- Time/offset management ---

    pub fn update_times(&mut self) {
        let mut time = 0i32;
        for com in &mut self.fm_commands {
            com.time = time;
            if self.pal {
                time += com.get_wait_frame() * 882;
            } else {
                time += com.get_wait_frame() * 735;
            }
        }
        time = 0;
        for com in &mut self.psg_commands {
            com.time = time;
            if self.pal {
                time += com.get_wait_frame() * 882;
            } else {
                time += com.get_wait_frame() * 735;
            }
        }
    }

    pub fn update_offsets(&mut self) {
        command::compute_offsets(&mut self.fm_commands, 0);
        command::compute_offsets(&mut self.psg_commands, 0);
    }

    pub fn get_total_time(&self) -> i32 {
        if self.fm_commands.is_empty() {
            return 0;
        }
        self.fm_commands.last().unwrap().time
    }

    pub fn get_total_time_in_frame(&self) -> i32 {
        self.get_total_time() / if self.pal { 882 } else { 735 }
    }

    pub fn get_total_time_in_second(&self) -> i32 {
        self.get_total_time_in_frame() / if self.pal { 50 } else { 60 }
    }

    pub fn get_loop_duration_in_second(&self) -> i32 {
        self.get_loop_duration_in_frame() / if self.pal { 50 } else { 60 }
    }

    pub fn get_loop_duration_in_frame(&self) -> i32 {
        if let Some(cmd) = self.get_fm_loop_start_command() {
            (self.get_total_time() - cmd.time) / if self.pal { 882 } else { 735 }
        } else {
            0
        }
    }

    // --- Command queries ---

    pub fn get_fm_command_at_offset(&self, offset: i32) -> Option<&XGMFMCommand> {
        command::get_command_index_at_offset(&self.fm_commands, offset)
            .map(|i| &self.fm_commands[i])
    }

    pub fn get_fm_command_at_time(&self, time: i32) -> Option<&XGMFMCommand> {
        command::get_command_index_at_time(&self.fm_commands, time)
            .map(|i| &self.fm_commands[i])
    }

    // --- Sample queries ---

    pub fn get_sample(&self, id: i32) -> Option<&XGMSample> {
        self.samples.iter().find(|s| s.id == id)
    }

    pub fn get_sample_len(&self, id: i32) -> usize {
        self.get_sample(id).map_or(0, |s| s.get_length())
    }

    pub fn get_sample_by_origin_id(&self, id: i32) -> Option<&XGMSample> {
        self.samples.iter().find(|s| s.origin_id == id)
    }

    pub fn get_sample_by_origin_address(&self, addr: i32) -> Option<&XGMSample> {
        self.samples.iter().find(|s| s.origin_addr == addr)
    }

    // --- Data access ---

    pub fn get_pcm_data_size(&self) -> usize {
        self.samples.iter().map(|s| s.data.len()).sum()
    }

    pub fn get_fm_music_data_size(&self) -> usize {
        self.fm_commands.iter().map(|c| c.size).sum()
    }

    pub fn get_psg_music_data_size(&self) -> usize {
        self.psg_commands.iter().map(|c| c.size).sum()
    }

    pub fn get_fm_music_data_array(&self) -> Vec<u8> {
        let mut result = Vec::new();
        for command in &self.fm_commands {
            result.extend_from_slice(&command.data);
        }
        result
    }

    pub fn get_psg_music_data_array(&self) -> Vec<u8> {
        let mut result = Vec::new();
        for command in &self.psg_commands {
            result.extend_from_slice(&command.data);
        }
        result
    }

    pub fn get_fm_music_data_array_split(&self, before_loop: bool) -> Vec<u8> {
        let loop_ind = self
            .get_fm_loop_start_command_index()
            .unwrap_or(self.fm_commands.len());
        let mut result = Vec::new();

        let range = if before_loop {
            0..loop_ind
        } else {
            loop_ind..self.fm_commands.len()
        };

        for i in range {
            result.extend_from_slice(&self.fm_commands[i].data);
        }
        result
    }

    pub fn get_psg_music_data_array_split(&self, before_loop: bool) -> Vec<u8> {
        let loop_ind = self
            .get_psg_loop_start_command_index()
            .unwrap_or(self.psg_commands.len());
        let mut result = Vec::new();

        let range = if before_loop {
            0..loop_ind
        } else {
            loop_ind..self.psg_commands.len()
        };

        for i in range {
            result.extend_from_slice(&self.psg_commands[i].data);
        }
        result
    }

    pub fn get_fm_music_frame_offsets_split(&self, before_loop: bool) -> Vec<i32> {
        let mut result = Vec::new();
        let loop_ind = self
            .get_fm_loop_start_command_index()
            .unwrap_or(self.fm_commands.len());

        if before_loop {
            for i in 0..loop_ind {
                let c = &self.fm_commands[i];
                if c.is_wait(false) || c.is_loop() || c.is_frame_delay() {
                    result.push(c.origin_offset + c.size as i32);
                }
            }
        } else {
            let base_offset = if let Some(idx) = self.get_fm_loop_start_command_index() {
                self.fm_commands[idx].origin_offset
            } else {
                0
            };
            for i in loop_ind..self.fm_commands.len() {
                let c = &self.fm_commands[i];
                if c.is_wait(false) || c.is_loop() || c.is_frame_delay() {
                    result.push((c.origin_offset + c.size as i32) - base_offset);
                }
            }
        }
        result
    }

    pub fn get_psg_music_frame_offsets_split(&self, before_loop: bool) -> Vec<i32> {
        let mut result = Vec::new();
        let loop_ind = self
            .get_psg_loop_start_command_index()
            .unwrap_or(self.psg_commands.len());

        if before_loop {
            for i in 0..loop_ind {
                let c = &self.psg_commands[i];
                if c.is_wait(false) || c.is_loop() {
                    result.push(c.origin_offset + c.size as i32);
                }
            }
        } else {
            let base_offset = if let Some(idx) = self.get_psg_loop_start_command_index() {
                self.psg_commands[idx].origin_offset
            } else {
                0
            };
            for i in loop_ind..self.psg_commands.len() {
                let c = &self.psg_commands[i];
                if c.is_wait(false) || c.is_loop() {
                    result.push((c.origin_offset + c.size as i32) - base_offset);
                }
            }
        }
        result
    }

    pub fn get_packed_fm_music_data_array(&mut self) -> Result<Vec<u8>> {
        let mut result = Vec::new();

        let before_data = self.get_fm_music_data_array_split(true);
        let before_offsets = self.get_fm_music_frame_offsets_split(true);
        result.extend(xgc_packer::pack(&before_data, &before_offsets, 0));

        if self.get_fm_loop_start_command_index().is_some() {
            self.set_fm_loop_address(result.len() as i32);
        } else {
            self.set_fm_loop_address(-1);
        }

        let after_data = self.get_fm_music_data_array_split(false);
        let after_offsets = self.get_fm_music_frame_offsets_split(false);
        result.extend(xgc_packer::pack(&after_data, &after_offsets, result.len() as i32));

        Ok(result)
    }

    pub fn get_packed_psg_music_data_array(&mut self) -> Result<Vec<u8>> {
        let mut result = Vec::new();

        let before_data = self.get_psg_music_data_array_split(true);
        let before_offsets = self.get_psg_music_frame_offsets_split(true);
        result.extend(xgc_packer::pack(&before_data, &before_offsets, 0));

        if self.get_psg_loop_start_command_index().is_some() {
            self.set_psg_loop_address(result.len() as i32);
        } else {
            self.set_psg_loop_address(-1);
        }

        let after_data = self.get_psg_music_data_array_split(false);
        let after_offsets = self.get_psg_music_frame_offsets_split(false);
        result.extend(xgc_packer::pack(&after_data, &after_offsets, result.len() as i32));

        Ok(result)
    }

    fn get_pcm_data_array(&self) -> Vec<u8> {
        let mut result = Vec::new();
        for sample in &self.samples {
            let mut signed = sample.data.clone();
            for b in &mut signed {
                *b = b.wrapping_add(0x80);
            }
            result.extend(signed);
        }
        result
    }

    // --- Optimization ---

    fn get_fm_commands_per_frame(&self) -> Vec<(i32, Vec<usize>)> {
        let mut result: Vec<(i32, Vec<usize>)> = Vec::new();
        let mut frame = 0i32;
        let mut c = 0;

        while c < self.fm_commands.len() {
            let mut frame_indices = Vec::new();

            while c < self.fm_commands.len() && !self.fm_commands[c].is_wait(false) {
                frame_indices.push(c);
                c += 1;
            }

            if c < self.fm_commands.len() {
                frame_indices.push(c);
                frame += self.fm_commands[c].get_wait_frame();
                c += 1;
            }

            result.push((frame, frame_indices));
        }
        result
    }

    fn get_psg_commands_per_frame(&self) -> Vec<(i32, Vec<usize>)> {
        let mut result: Vec<(i32, Vec<usize>)> = Vec::new();
        let mut frame = 0i32;
        let mut c = 0;

        while c < self.psg_commands.len() {
            let mut frame_indices = Vec::new();

            while c < self.psg_commands.len() && !self.psg_commands[c].is_wait(false) {
                frame_indices.push(c);
                c += 1;
            }

            if c < self.psg_commands.len() {
                frame_indices.push(c);
                frame += self.psg_commands[c].get_wait_frame();
                c += 1;
            }

            result.push((frame, frame_indices));
        }
        result
    }

    fn pack_wait_fm(&mut self) {
        let mut c = 0;
        while c < self.fm_commands.len() {
            while c < self.fm_commands.len() && !self.fm_commands[c].is_wait(true) {
                c += 1;
            }
            if c >= self.fm_commands.len() {
                return;
            }

            let start_ind = c;
            let mut wait = 0;
            while c < self.fm_commands.len() && self.fm_commands[c].is_wait(true) {
                wait += self.fm_commands[c].get_wait_frame();
                c += 1;
            }

            // remove all wait commands
            self.fm_commands.drain(start_ind..c);

            let wait_coms = XGMFMCommand::create_wait_commands(wait);
            let count = wait_coms.len();
            for (i, wc) in wait_coms.into_iter().enumerate() {
                self.fm_commands.insert(start_ind + i, wc);
            }
            c = start_ind + count;
        }
    }

    fn pack_wait_psg(&mut self) {
        let mut c = 0;
        while c < self.psg_commands.len() {
            while c < self.psg_commands.len() && !self.psg_commands[c].is_wait(true) {
                c += 1;
            }
            if c >= self.psg_commands.len() {
                return;
            }

            let start_ind = c;
            let mut wait = 0;
            while c < self.psg_commands.len() && self.psg_commands[c].is_wait(true) {
                if !self.psg_commands[c].is_dummy() {
                    wait += self.psg_commands[c].get_wait_frame();
                }
                c += 1;
            }

            self.psg_commands.drain(start_ind..c);

            let wait_coms = XGMPSGCommand::create_wait_commands(wait);
            let count = wait_coms.len();
            for (i, wc) in wait_coms.into_iter().enumerate() {
                self.psg_commands.insert(start_ind + i, wc);
            }
            c = start_ind + count;
        }
    }

    fn remove_duplicate_set_freq(commands: &mut [XGMFMCommand]) {
        let mut freq_set = false;
        for c in (0..commands.len()).rev() {
            if commands[c].is_ym_freq_special_write() || commands[c].is_ym_freq_delta_special_write() {
                continue;
            }
            if commands[c].is_ym_freq_write() || commands[c].is_ym_freq_delta_write() {
                if freq_set {
                    commands[c].set_dummy();
                } else {
                    freq_set = true;
                }
            }
        }
    }

    fn clean_key_commands_fm(commands: &mut [XGMFMCommand]) {
        let mut has_key_on = false;
        let mut has_key_off = false;

        for c in (0..commands.len()).rev() {
            if commands[c].is_ym_key_on_write() {
                if has_key_on {
                    commands[c].set_dummy();
                } else {
                    has_key_on = true;
                }
            } else if commands[c].is_ym_key_off_write() {
                if has_key_off {
                    commands[c].set_dummy();
                } else if has_key_on {
                    has_key_off = true;
                }
            }
        }

        let mut key_state: Option<bool> = None;
        for com in commands.iter_mut() {
            if com.is_ym_key_on_write() {
                if key_state == Some(true) {
                    com.set_dummy();
                }
                key_state = Some(true);
            } else if com.is_ym_key_off_write() {
                if key_state == Some(false) {
                    com.set_dummy();
                }
                key_state = Some(false);
            }
        }
    }

    fn optimize_key_seq_commands(commands: &mut [XGMFMCommand]) {
        let mut last_key_off: Option<usize> = None;
        let mut last_key_on: Option<usize> = None;

        for i in 0..commands.len() {
            if commands[i].is_ym_key_off_write() {
                if let Some(on_idx) = last_key_on {
                    commands[on_idx].set_dummy();
                    commands[i].to_key_seq(true);
                    last_key_on = None;
                } else {
                    last_key_off = Some(i);
                }
            } else if commands[i].is_ym_key_on_write() {
                if let Some(off_idx) = last_key_off {
                    commands[off_idx].set_dummy();
                    commands[i].to_key_seq(false);
                    last_key_off = None;
                } else {
                    last_key_on = Some(i);
                }
            } else if commands[i].is_ym_set_tl()
                || commands[i].is_ym_key_adv_write()
                || commands[i].is_ym_load_inst()
                || commands[i].is_ym_write()
            {
                last_key_off = None;
                last_key_on = None;
            }
        }
    }

    fn combine_set_freq_key_commands(commands: &mut [XGMFMCommand]) {
        // Safe key-on combine (from end)
        let mut can_combine = false;
        let mut last_key_on: Option<usize> = None;

        for c in (0..commands.len()).rev() {
            if commands[c].is_ym_freq_write() && !commands[c].is_ym_freq_special_write() {
                if can_combine {
                    commands[c].set_ym_freq_key_on();
                    if let Some(idx) = last_key_on {
                        commands[idx].set_dummy();
                    }
                }
                can_combine = false;
            } else if commands[c].is_ym_key_on_write() {
                can_combine = true;
                last_key_on = Some(c);
            } else if commands[c].is_ym_key_off_write()
                || commands[c].is_ym_set_tl()
                || commands[c].is_ym_key_adv_write()
                || commands[c].is_ym_key_sequence()
                || commands[c].is_ym_load_inst()
                || commands[c].is_ym_write()
            {
                can_combine = false;
            }
        }

        // Safe key-off combine (from start)
        can_combine = false;
        let mut last_key_off: Option<usize> = None;

        for c in 0..commands.len() {
            if commands[c].is_ym_freq_write() && !commands[c].is_ym_freq_special_write() {
                if can_combine {
                    commands[c].set_ym_freq_key_off();
                    if let Some(idx) = last_key_off {
                        commands[idx].set_dummy();
                    }
                }
                can_combine = false;
            } else if commands[c].is_ym_key_off_write() {
                can_combine = true;
                last_key_off = Some(c);
            } else if commands[c].is_ym_key_on_write()
                || commands[c].is_ym_set_tl()
                || commands[c].is_ym_key_adv_write()
                || commands[c].is_ym_key_sequence()
                || commands[c].is_ym_load_inst()
                || commands[c].is_ym_write()
            {
                can_combine = false;
            }
        }

        // Isolated key off/on combine
        let mut has_key_on = false;
        let mut can_combine_off = false;
        let mut can_combine_on = false;
        let mut set_freq_idx: Option<usize> = None;
        last_key_off = None;
        let mut last_key_on_idx: Option<usize> = None;

        for i in 0..commands.len() {
            if commands[i].is_ym_key_off_write() {
                if !has_key_on {
                    can_combine_off = true;
                    last_key_off = Some(i);
                }
            } else if commands[i].is_ym_key_on_write() {
                can_combine_on = true;
                has_key_on = true;
                last_key_on_idx = Some(i);
            } else if commands[i].is_ym_set_tl()
                || commands[i].is_ym_key_adv_write()
                || commands[i].is_ym_key_sequence()
                || commands[i].is_ym_load_inst()
                || commands[i].is_ym_write()
            {
                if let Some(sf_idx) = set_freq_idx {
                    if can_combine_off {
                        commands[sf_idx].set_ym_freq_key_off();
                        if let Some(ko_idx) = last_key_off {
                            commands[ko_idx].set_dummy();
                        }
                    }
                    if can_combine_on {
                        commands[sf_idx].set_ym_freq_key_on();
                        if let Some(kon_idx) = last_key_on_idx {
                            commands[kon_idx].set_dummy();
                        }
                    }
                }
                can_combine_off = false;
                can_combine_on = false;
                set_freq_idx = None;
                has_key_on = false;
            } else if commands[i].is_ym_freq_write() && !commands[i].is_ym_freq_special_write() {
                set_freq_idx = Some(i);
                if commands[i].is_ym_freq_with_key_on() {
                    has_key_on = true;
                }
            }
        }

        // last set freq
        if let Some(sf_idx) = set_freq_idx {
            if can_combine_off {
                commands[sf_idx].set_ym_freq_key_off();
                if let Some(ko_idx) = last_key_off {
                    commands[ko_idx].set_dummy();
                }
            }
            if can_combine_on {
                commands[sf_idx].set_ym_freq_key_on();
                if let Some(kon_idx) = last_key_on_idx {
                    commands[kon_idx].set_dummy();
                }
            }
        }
    }

    fn pack_fm_freq_commands(commands: &mut [XGMFMCommand]) {
        let mut last_freq = [-1i32; 4];

        for com in commands.iter_mut() {
            if com.is_loop_start {
                last_freq = [-1; 4];
            } else if com.is_ym_freq_write() {
                let freq = com.get_ym_freq_value();
                let slot = if com.is_ym_freq_special_write() {
                    com.get_ym_slot() as usize
                } else {
                    0
                };

                if last_freq[slot] != -1 && !com.is_ym_freq_with_key_write() {
                    let delta = freq - last_freq[slot];
                    if delta == 0 {
                        com.set_dummy();
                    } else if delta.abs() <= 128 {
                        com.to_freq_delta(delta);
                    }
                }
                last_freq[slot] = freq;
            }
        }
    }

    fn convert_fm_tl_commands(commands: &mut [XGMFMCommand]) {
        let mut last_tl = [-1i32; 4];

        for com in commands.iter_mut() {
            if com.is_loop_start {
                last_tl = [-1; 4];
            } else if com.is_ym_load_inst() {
                for s in 0..4 {
                    last_tl[s] = (com.data[1 + 4 + s] & 0x7F) as i32;
                }
            } else if com.is_ym_set_tl() {
                let tl = com.get_ym_tl_value();
                let slot = com.get_ym_slot() as usize;

                if last_tl[slot] != -1 {
                    let delta = tl - last_tl[slot];
                    if delta == 0 {
                        if crate::is_verbose() {
                            println!("TL delta = 0");
                        }
                        com.set_dummy();
                    } else if delta.abs() <= 64 {
                        com.to_tl_delta(delta);
                    }
                }
                last_tl[slot] = tl;
            }
        }
    }

    fn _use_ext_wait_fm_command(commands: &mut [XGMFMCommand]) -> i32 {
        if commands.len() <= 1 {
            return -1;
        }

        let last_idx = commands.len() - 1;
        if !commands[last_idx].is_wait(false) || commands[last_idx].get_wait_frame() > 1 {
            return -1;
        }

        let mut has_key_ch = [false; 6];

        for c in (0..last_idx).rev() {
            if commands[c].support_wait() && !has_key_ch[commands[c].get_channel() as usize] {
                commands[c].add_wait();
                commands[last_idx].set_dummy();
                return c as i32;
            } else if commands[c].is_ym_key_write() {
                let ch = commands[c].get_channel();
                if ch >= 0 && (ch as usize) < 6 {
                    has_key_ch[ch as usize] = true;
                }
            }
        }
        -1
    }

    fn optimize_fm_commands(&mut self) {
        let _new_commands: Vec<XGMFMCommand> = Vec::new();
        let _ym_state = YM2612State::new();

        // Get frame data (we need indices into self.fm_commands)
        // Note: the Java code uses indexOf() heavily; we work with indices directly
        let frames = self.get_fm_commands_per_frame();

        for (_frame_num, frame_indices) in &frames {
            for channel in 0..6 {
                let _port = if channel >= 3 { 1 } else { 0 };
                let _ch = if channel >= 3 { channel - 3 } else { channel };

                // Get commands for current channel
                let mut fm_channel_commands: Vec<XGMFMCommand> = frame_indices
                    .iter()
                    .filter(|&&i| {
                        let cmd = &self.fm_commands[i];
                        !cmd.is_wait(false) && !cmd.is_loop_start && cmd.get_channel() == channel as i32
                    })
                    .map(|&i| self.fm_commands[i].clone())
                    .collect();

                if !fm_channel_commands.is_empty() {
                    Self::remove_duplicate_set_freq(&mut fm_channel_commands);
                    Self::clean_key_commands_fm(&mut fm_channel_commands);
                    Self::combine_set_freq_key_commands(&mut fm_channel_commands);
                    Self::optimize_key_seq_commands(&mut fm_channel_commands);

                    // Apply back changes to original commands
                    let mut ch_idx = 0;
                    for &orig_idx in frame_indices {
                        let orig = &self.fm_commands[orig_idx];
                        if !orig.is_wait(false)
                            && !orig.is_loop_start
                            && orig.get_channel() == channel as i32
                        {
                            if ch_idx < fm_channel_commands.len() {
                                self.fm_commands[orig_idx] = fm_channel_commands[ch_idx].clone();
                                ch_idx += 1;
                            }
                        }
                    }
                }
            }
        }

        // Global optimizations per channel
        self.update_times();

        for channel in 0..6 {
            let mut ch_commands: Vec<XGMFMCommand> = self
                .fm_commands
                .iter()
                .filter(|c| {
                    c.get_channel() == channel as i32
                        || c.is_wait(false)
                        || c.is_loop_start
                })
                .cloned()
                .collect();

            Self::pack_fm_freq_commands(&mut ch_commands);
            Self::convert_fm_tl_commands(&mut ch_commands);

            // Apply changes back - only for channel-specific commands
            let mut ch_idx = 0;
            for com in &mut self.fm_commands {
                if com.get_channel() == channel as i32 {
                    if ch_idx < ch_commands.len() {
                        // Find matching command in filtered list
                        while ch_idx < ch_commands.len()
                            && (ch_commands[ch_idx].is_wait(false) || ch_commands[ch_idx].is_loop_start)
                        {
                            ch_idx += 1;
                        }
                        if ch_idx < ch_commands.len() {
                            *com = ch_commands[ch_idx].clone();
                            ch_idx += 1;
                        }
                    }
                }
            }
        }

        self.pack_wait_fm();
        self.rebuild_fm_commands();
    }

    pub fn rebuild_fm_commands(&mut self) {
        let mut new_commands: Vec<XGMFMCommand> = Vec::new();
        let mut frame_size = 0;

        for com in &self.fm_commands {
            if !com.is_dummy() {
                if (frame_size + com.size) > (xgc_packer::FRAME_MAX_SIZE - 1) {
                    println!(
                        "Warning: maximum frame size exceeded (FM frame #{})",
                        com.get_frame(self.pal)
                    );
                    new_commands.push(XGMFMCommand::create_frame_delay());
                    frame_size = 0;
                } else if com.is_wait(false) {
                    frame_size = 0;
                } else {
                    frame_size += com.size;
                }
                new_commands.push(com.clone());
            }
        }

        self.fm_commands = new_commands;
        self.update_offsets();
        self.update_times();
    }

    pub fn rebuild_psg_commands(&mut self) {
        let mut new_commands: Vec<XGMPSGCommand> = Vec::new();
        let mut frame_size = 0;

        for com in &self.psg_commands {
            if !com.is_dummy() {
                if (frame_size + com.size) > (xgc_packer::FRAME_MAX_SIZE - 1) {
                    println!(
                        "Warning: maximum frame size exceeded (PSG frame #{})",
                        com.get_frame(self.pal)
                    );
                    new_commands.push(XGMPSGCommand::create_frame());
                    frame_size = 0;
                } else if com.is_wait(false) {
                    frame_size = 0;
                } else {
                    frame_size += com.size;
                }
                new_commands.push(com.clone());
            }
        }

        self.psg_commands = new_commands;
        self.update_offsets();
        self.update_times();
    }

    fn remove_duplicated_freq_env(commands: &mut [XGMPSGCommand]) {
        let mut freq_low_set = false;
        let mut freq_set = false;
        let mut env_set = false;

        for c in (0..commands.len()).rev() {
            if commands[c].is_env() {
                if env_set {
                    commands[c].set_dummy();
                } else {
                    env_set = true;
                }
            } else if commands[c].is_freq() {
                if freq_set {
                    commands[c].set_dummy();
                } else {
                    freq_set = true;
                }
            } else if commands[c].is_freq_low() {
                if freq_low_set || freq_set {
                    commands[c].set_dummy();
                } else {
                    freq_low_set = true;
                }
            }
        }
    }

    fn _use_ext_wait_psg_command(commands: &mut [XGMPSGCommand]) -> i32 {
        if commands.len() < 2 {
            return -1;
        }
        let last_idx = commands.len() - 1;
        if !commands[last_idx].is_wait(false) || commands[last_idx].get_wait_frame() > 1 {
            return -1;
        }

        for c in (0..last_idx).rev() {
            if commands[c].support_wait() {
                commands[c].add_wait();
                commands[last_idx].set_dummy();
                return c as i32;
            }
        }
        -1
    }

    fn remove_silent_psg_freq_commands(commands: &mut [XGMPSGCommand]) {
        let mut silent = false;
        let mut last_dummy_freq: Option<usize> = None;
        let mut last_dummy_freq_low: Option<usize> = None;
        let mut last_freq: i32 = 0;

        for i in 0..commands.len() {
            if commands[i].is_dummy() {
                continue;
            }
            if commands[i].is_loop_start {
                silent = false;
                last_dummy_freq = None;
                last_dummy_freq_low = None;
            } else if commands[i].is_env() {
                if commands[i].get_env() == 0xF {
                    silent = true;
                    last_dummy_freq = None;
                    last_dummy_freq_low = None;
                } else if silent {
                    // Restore last removed freq
                    if let Some(idx) = last_dummy_freq {
                        commands[idx].clear_dummy();
                        commands[idx].set_freq(last_freq);
                    } else if let Some(idx) = last_dummy_freq_low {
                        commands[idx].clear_dummy();
                        commands[idx].set_freq_low(last_freq & 0xF);
                    }
                    silent = false;
                }
            } else if commands[i].is_freq() {
                if silent {
                    last_freq = commands[i].get_freq();
                    commands[i].set_dummy();
                    last_dummy_freq = Some(i);
                }
            } else if commands[i].is_freq_low() {
                if silent {
                    last_freq = (last_freq & 0x3F0) | commands[i].get_freq_low();
                    commands[i].set_dummy();
                    last_dummy_freq_low = Some(i);
                }
            }
        }
    }

    fn pack_psg_freq_commands(commands: &mut [XGMPSGCommand]) {
        let mut last_freq: i32 = -1;

        for com in commands.iter_mut() {
            if com.is_dummy() {
                continue;
            }
            if com.is_loop_start {
                last_freq = -1;
            } else if com.is_freq() {
                let freq = com.get_freq();
                if last_freq != -1 {
                    let delta = freq - last_freq;
                    if delta == 0 {
                        com.set_dummy();
                    } else if delta.abs() <= 4 {
                        com.to_freq_delta(delta);
                    } else if (last_freq & 0xFF0) == (freq & 0xFF0) {
                        com.to_freq_low(freq & 0xF);
                    }
                }
                last_freq = freq;
            } else if com.is_freq_low() {
                if last_freq != -1 {
                    let freq = (last_freq & 0xFF0) | com.get_freq_low();
                    let delta = freq - last_freq;
                    if delta == 0 {
                        com.set_dummy();
                    } else if delta.abs() <= 4 {
                        com.to_freq_delta(delta);
                    }
                    last_freq = freq;
                }
            }
        }
    }

    fn convert_psg_env_commands(commands: &mut [XGMPSGCommand]) {
        let mut last_env: i32 = -1;

        for com in commands.iter_mut() {
            if com.is_dummy() {
                continue;
            }
            if com.is_loop_start {
                last_env = -1;
            } else if com.is_env() {
                let env = com.get_env();
                if last_env != -1 {
                    let delta = env - last_env;
                    if delta == 0 {
                        if crate::is_verbose() {
                            println!("env delta = 0");
                        }
                        com.set_dummy();
                    } else if delta.abs() <= 4 {
                        com.to_env_delta(delta);
                    }
                }
                last_env = env;
            }
        }
    }

    fn optimize_psg_commands(&mut self) {
        // Frame optimization
        let frames = self.get_psg_commands_per_frame();

        for (_frame_num, frame_indices) in &frames {
            for channel in 0..4 {
                let ch_indices: Vec<usize> = frame_indices
                    .iter()
                    .filter(|&&i| {
                        let cmd = &self.psg_commands[i];
                        !cmd.is_wait(false) && !cmd.is_loop_start && cmd.get_channel() == channel as i32
                    })
                    .copied()
                    .collect();

                if !ch_indices.is_empty() {
                    let mut ch_cmds: Vec<XGMPSGCommand> =
                        ch_indices.iter().map(|&i| self.psg_commands[i].clone()).collect();
                    Self::remove_duplicated_freq_env(&mut ch_cmds);

                    // Apply back
                    for (j, &idx) in ch_indices.iter().enumerate() {
                        if j < ch_cmds.len() {
                            self.psg_commands[idx] = ch_cmds[j].clone();
                        }
                    }
                }
            }
        }

        // Global channel optimizations
        for channel in 0..4 {
            let mut ch_cmds: Vec<XGMPSGCommand> = self
                .psg_commands
                .iter()
                .filter(|c| {
                    c.get_channel() == channel as i32
                        || c.is_wait(true)
                        || c.is_loop_start
                })
                .cloned()
                .collect();

            if channel < 2 {
                Self::remove_silent_psg_freq_commands(&mut ch_cmds);
            }
            Self::pack_psg_freq_commands(&mut ch_cmds);
            Self::convert_psg_env_commands(&mut ch_cmds);

            // Apply changes back for channel-specific commands
            let mut ch_idx = 0;
            for com in &mut self.psg_commands {
                if com.get_channel() == channel as i32 {
                    while ch_idx < ch_cmds.len()
                        && (ch_cmds[ch_idx].is_wait(true) || ch_cmds[ch_idx].is_loop_start)
                    {
                        ch_idx += 1;
                    }
                    if ch_idx < ch_cmds.len() {
                        *com = ch_cmds[ch_idx].clone();
                        ch_idx += 1;
                    }
                }
            }
        }

        // Clean dummy commands
        self.psg_commands.retain(|c| !c.is_dummy());

        self.pack_wait_psg();
        self.rebuild_psg_commands();
    }

    fn optimize_commands(&mut self) {
        self.optimize_fm_commands();
        self.optimize_psg_commands();
    }

    // --- Sample optimization ---

    fn optimize_samples(&mut self) {
        let num_sample = self.samples.len();

        // merge from end
        let mut s = self.samples.len();
        while s > 0 {
            s -= 1;
            self.merge_sample(s);
        }

        // reset sample ids
        for s in 0..self.samples.len() {
            let old_id = self.samples[s].id;
            let new_id = (s + 1) as i32;
            if old_id != new_id {
                self.update_sample_commands(old_id, new_id, -1);
                self.samples[s].id = new_id;
            }
        }

        if self.samples.len() >= 123 {
            eprintln!("Warning: XGM cannot have more than 123 samples, some samples will be lost!");
            while self.samples.len() > 123 {
                self.samples.pop();
            }
            return;
        }

        self.rebuild_fm_commands();

        if self.samples.len() != num_sample && !crate::is_silent() {
            println!("Merged {} sample(s)", num_sample - self.samples.len());
        }
    }

    fn find_matching_sample(&self, sample_idx: usize) -> Option<usize> {
        let sample = &self.samples[sample_idx];
        let mut best_idx: Option<usize> = None;
        let mut best_score: f64 = 0.0;

        for (i, s) in self.samples.iter().enumerate() {
            if i != sample_idx {
                let score = s.get_similarity_score(sample);
                if score > best_score {
                    best_idx = Some(i);
                    best_score = score;
                }
            }
        }

        if best_score >= 1.0 {
            best_idx
        } else {
            None
        }
    }

    fn merge_sample(&mut self, sample_index: usize) {
        if let Some(match_idx) = self.find_matching_sample(sample_index) {
            let sample_id = self.samples[sample_index].id;
            let match_id = self.samples[match_idx].id;
            let sample_len = self.samples[sample_index].get_length();
            let match_len = self.samples[match_idx].get_length();
            let same_duration =
                (sample_len as f64 / 60.0).round() as i64 == (match_len as f64 / 60.0).round() as i64;

            let duration = if same_duration {
                -1i64
            } else {
                (sample_len as i64 * 44100) / crate::xgm_sample::XGM_FULL_RATE as i64
            };

            self.update_sample_commands(sample_id, match_id, duration);
            self.samples.remove(sample_index);

            println!(
                "Found duplicated sample #{} (merged with #{})",
                sample_id, match_id
            );
        }
    }

    pub fn update_sample_commands(&mut self, origin_id: i32, replace_id: i32, duration_in_sample: i64) {
        if origin_id == replace_id && duration_in_sample == -1 {
            return;
        }

        let mut i = 0;
        while i < self.fm_commands.len() {
            if self.fm_commands[i].is_pcm() && self.fm_commands[i].get_pcm_id() == origin_id {
                self.fm_commands[i].set_pcm_id(replace_id);

                if duration_in_sample != -1 {
                    let half_rate = self.fm_commands[i].get_pcm_half_rate();
                    let duration = duration_in_sample * if half_rate { 2 } else { 1 };
                    let end_time = self.fm_commands[i].time as i64 + duration;
                    let mut end_time_frame: i64 = -1;
                    let mut add_stop = false;
                    let pcm_channel = self.fm_commands[i].get_pcm_channel();

                    let mut j = i + 1;
                    while j < self.fm_commands.len() {
                        if self.fm_commands[j].is_pcm() {
                            break;
                        }
                        if self.fm_commands[j].time as i64 >= end_time {
                            if end_time_frame == -1 {
                                end_time_frame = self.fm_commands[j].time as i64;
                            } else if self.fm_commands[j].time as i64 > end_time_frame {
                                add_stop = true;
                                break;
                            }
                        }
                        j += 1;
                    }

                    if add_stop {
                        let stop_cmd = XGMFMCommand::create_pcm_stop(pcm_channel);
                        self.fm_commands.insert(j, stop_cmd);
                    }
                }
            }
            i += 1;
        }
    }

    // --- Serialization ---

    pub fn as_byte_array(&mut self) -> Result<Vec<u8>> {
        let mut result = Vec::new();

        // 0000: XGM2 id
        if !self.packed {
            result.extend_from_slice(b"XGM2");
        }
        // 0004: version
        result.push(0x10);

        // 0005: format flags
        let mut flags = 0u8;
        if self.pal {
            flags |= 1;
        }
        if self.gd3.is_some() {
            flags |= 4;
        }
        if self.packed {
            flags |= 8;
        }
        result.push(flags);

        // Get data blocks
        let pcm_data = util::align_data(&self.get_pcm_data_array(), 256);
        let fm_data;
        let psg_data;

        if self.packed {
            fm_data = util::align_data(&self.get_packed_fm_music_data_array()?, 256);
            psg_data = util::align_data(&self.get_packed_psg_music_data_array()?, 256);
        } else {
            fm_data = util::align_data(&self.get_fm_music_data_array(), 256);
            psg_data = util::align_data(&self.get_psg_music_data_array(), 256);
        }

        // 0006-0007: SLEN
        let slen = (pcm_data.len() >> 8) as u16;
        result.push((slen & 0xFF) as u8);
        result.push(((slen >> 8) & 0xFF) as u8);
        // 0008-0009: FMLEN
        let fmlen = (fm_data.len() >> 8) as u16;
        result.push((fmlen & 0xFF) as u8);
        result.push(((fmlen >> 8) & 0xFF) as u8);
        // 000A-000B: PSGLEN
        let psglen = (psg_data.len() >> 8) as u16;
        result.push((psglen & 0xFF) as u8);
        result.push(((psglen >> 8) & 0xFF) as u8);

        // 000C-0103: SID table (124 entries = 248 bytes)
        let mut offset = 0usize;
        for sample in &self.samples {
            let len = sample.data.len();
            result.push((offset >> 8) as u8);
            result.push((offset >> 16) as u8);
            offset += len;
        }
        // last entry for size calculation
        result.push((offset >> 8) as u8);
        result.push((offset >> 16) as u8);
        // fill remaining with 0xFFFF
        let entries_written = self.samples.len() + 1;
        for _ in entries_written..(248 / 2) {
            result.push(0xFF);
            result.push(0xFF);
        }

        // Sample data + FM data + PSG data
        result.extend(&pcm_data);
        result.extend(&fm_data);
        result.extend(&psg_data);

        // GD3/XD3 tags
        if self.packed {
            if let Some(ref xd3) = self.xd3 {
                result.extend(xd3.as_byte_array());
            }
        } else if let Some(ref gd3) = self.gd3 {
            result.extend(gd3.as_byte_array());
        }

        Ok(result)
    }
}
