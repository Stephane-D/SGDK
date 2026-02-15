use crate::vgm_command::*;
use crate::sample_bank::{Sample, SampleBank};
use crate::gd3::GD3;
use crate::ym2612::YM2612;
use crate::psg::PSG;
use crate::{is_silent, is_verbose, sample_rate_fix, sample_ignore, delay_key_off, keep_rf5c68_cmds};

const SAMPLE_END_DELAY: i32 = 400;
const SAMPLE_MIN_SIZE: i32 = 100;
const SAMPLE_MIN_DYNAMIC: i32 = 16;
const SAMPLE_ALLOWED_MARGE: i32 = 64;
const SAMPLE_MIN_MEAN_DELTA: f64 = 1.0;

/// Represents a parsed VGM file
pub struct VGM {
    pub version: u8,
    pub data: Vec<u8>,
    pub offset: usize,
    pub offset_start: usize,
    pub offset_end: usize,
    pub len_in_sample: i32,
    pub loop_start: usize,
    pub loop_len_in_sample: i32,
    pub rate: i32,
    pub gd3: Option<GD3>,
    pub sample_banks: Vec<SampleBank>,
    pub commands: Vec<VGMCommand>,
}

impl VGM {
    /// Parse VGM from raw data with optional conversion/optimization
    pub fn create(data: Vec<u8>, data_size: usize, offset: usize, convert: bool) -> Option<Self> {
        if data.len() < offset + 0x40 {
            println!("Error: VGM file too short!");
            return None;
        }
        if data[offset] != b'V' || data[offset + 1] != b'g' || data[offset + 2] != b'm' || data[offset + 3] != b' ' {
            println!("Error: VGM file not recognized!");
            return None;
        }

        let ver = data[offset + 8];
        if ver < 0x50 {
            println!("Warning: VGM version 1.{:02X} detected!", ver);
            println!("PCM data won't be retrieved (version 1.5 or above required)");
        }

        if !is_silent() {
            if convert { println!("Optimizing VGM..."); }
            else { println!("Parsing VGM file..."); }
        }

        let offset_start = if ver >= 0x50 {
            crate::get_u32(&data, offset + 0x34) as usize + offset + 0x34
        } else {
            offset + 0x40
        };
        let offset_end = crate::get_u32(&data, offset + 0x04) as usize + offset + 0x04;
        let len_in_sample = crate::get_u32(&data, offset + 0x18) as i32;

        let mut loop_start = crate::get_u32(&data, offset + 0x1C) as usize;
        if loop_start != 0 { loop_start += offset + 0x1C; }
        let loop_len_in_sample = crate::get_u32(&data, offset + 0x20) as i32;

        let rate = if ver >= 0x01 {
            let r = crate::get_u32(&data, offset + 0x24) as i32;
            if r != 50 { 60 } else { 50 }
        } else { 60 };

        let gd3_addr = crate::get_u32(&data, offset + 0x14) as usize;
        let gd3 = if gd3_addr != 0 {
            let abs_addr = gd3_addr + offset + 0x14;
            GD3::from_data(&data[abs_addr..])
        } else { None };

        if !is_silent() {
            println!("VGM duration: {} samples ({} seconds)", len_in_sample, len_in_sample / 44100);
        }
        if is_verbose() {
            println!("VGM data start: {:06X}   end: {:06X}", offset_start, offset_end);
            println!("Loop start offset: {:06X}   length: {} ({} seconds)", loop_start, loop_len_in_sample, loop_len_in_sample / 44100);
        }

        let mut vgm = VGM {
            version: ver, data, offset, offset_start, offset_end,
            len_in_sample, loop_start, loop_len_in_sample, rate, gd3,
            sample_banks: Vec::new(), commands: Vec::new(),
        };

        // Parse commands
        vgm.parse();

        if !is_silent() {
            println!("Computed VGM duration: {} samples ({} seconds)", vgm.compute_len(), vgm.compute_len() / 44100);
        }

        // Build samples
        vgm.build_samples(convert);

        // Rebuild data blocks if converting
        if convert {
            // Remove previous data blocks and stream control/data commands
            vgm.commands.retain(|cmd| {
                !cmd.is_data_block() && !cmd.is_stream_control() && !cmd.is_stream_data()
            });

            // Rebuild from sample banks and insert at beginning
            let mut new_front = Vec::new();
            for bank in &vgm.sample_banks {
                new_front.push(bank.get_data_block_command());
                new_front.extend(bank.get_declaration_commands());
            }
            new_front.append(&mut vgm.commands);
            vgm.commands = new_front;
        }

        if is_verbose() {
            println!("VGM sample number: {}", vgm.get_sample_number());
            println!("Sample data size: {}", vgm.get_sample_data_size());
            println!("Sample total len: {}", vgm.get_sample_total_len());
        }

        Some(vgm)
    }

    /// Create VGM from XGM
    pub fn create_from_xgm(xgm: &crate::xgm::XGM) -> Self {
        let mut commands = Vec::new();
        let mut loop_offset: i32 = -1;
        let mut time: i32 = 0;

        for cmd in &xgm.commands {
            match cmd.get_type() {
                crate::xgm_command::XGM_FRAME => {
                    if xgm.pal {
                        commands.push(VGMCommand::from_owned(vec![0x63], time));
                        time += 0x372;
                    } else {
                        commands.push(VGMCommand::from_owned(vec![0x62], time));
                        time += 0x2DF;
                    }
                }
                crate::xgm_command::XGM_END => {}
                crate::xgm_command::XGM_LOOP => {
                    loop_offset = cmd.get_loop_offset();
                }
                crate::xgm_command::XGM_PCM => {} // not handled
                crate::xgm_command::XGM_PSG => {
                    let count = (cmd.data[0] & 0xF) as usize + 1;
                    for j in 0..count {
                        commands.push(VGMCommand::from_owned(vec![0x50, cmd.data[j + 1]], time));
                    }
                }
                crate::xgm_command::XGM_YM2612_PORT0 => {
                    let count = (cmd.data[0] & 0xF) as usize + 1;
                    for j in 0..count {
                        commands.push(VGMCommand::from_owned(vec![0x52, cmd.data[j * 2 + 1], cmd.data[j * 2 + 2]], time));
                    }
                }
                crate::xgm_command::XGM_YM2612_PORT1 => {
                    let count = (cmd.data[0] & 0xF) as usize + 1;
                    for j in 0..count {
                        commands.push(VGMCommand::from_owned(vec![0x53, cmd.data[j * 2 + 1], cmd.data[j * 2 + 2]], time));
                    }
                }
                crate::xgm_command::XGM_YM2612_REGKEY => {
                    let count = (cmd.data[0] & 0xF) as usize + 1;
                    for j in 0..count {
                        commands.push(VGMCommand::from_owned(vec![0x52, 0x28, cmd.data[j + 1]], time));
                    }
                }
                _ => {}
            }
        }

        // End marker
        commands.push(VGMCommand::from_owned(vec![0x66], time));

        // Handle loop
        if loop_offset != -1 {
            if let Some(xgm_cmd) = xgm.get_command_at_offset(loop_offset) {
                let loop_time = xgm.get_time(&xgm_cmd);
                // Find position in VGM commands at this time
                let mut accumulated = 0i32;
                let mut insert_pos = None;
                for (i, c) in commands.iter().enumerate() {
                    if accumulated >= loop_time {
                        insert_pos = Some(i);
                        break;
                    }
                    accumulated += c.get_wait_value();
                }
                if let Some(pos) = insert_pos {
                    commands.insert(pos, VGMCommand::new(VGM_LOOP_START, time));
                }
                // Insert loop end near end
                let end_idx = commands.len().saturating_sub(1);
                commands.insert(end_idx, VGMCommand::new(VGM_LOOP_END, time));
            }
        }

        VGM {
            version: 0x60,
            data: Vec::new(),
            offset: 0,
            offset_start: 0,
            offset_end: 0,
            len_in_sample: 0,
            loop_start: 0,
            loop_len_in_sample: 0,
            rate: if xgm.pal { 50 } else { 60 },
            gd3: xgm.gd3.clone(),
            sample_banks: Vec::new(),
            commands,
        }
    }

    fn parse(&mut self) {
        let mut time: i32 = 0;
        let mut loop_time_st: i32 = -1;
        let mut off = self.offset_start;

        while off < self.offset_end {
            // Check loop start
            if loop_time_st == -1 && self.loop_start != 0 && off >= self.loop_start {
                self.commands.push(VGMCommand::new(VGM_LOOP_START, time));
                loop_time_st = time;
            }

            let command = VGMCommand::from_data(&self.data, off, time);
            time += command.get_wait_value();
            off += command.size;

            let is_end = command.is_end();
            if !is_end {
                self.commands.push(command);
            }

            // Check loop end
            if loop_time_st >= 0 && self.loop_len_in_sample != 0 {
                if (time - loop_time_st) > self.loop_len_in_sample {
                    self.commands.push(VGMCommand::new(VGM_LOOP_END, time));
                    loop_time_st = -2;
                }
            }

            if is_end { break; }
        }

        // Loop end not yet defined
        if loop_time_st >= 0 && self.loop_len_in_sample != 0 {
            let delta = self.loop_len_in_sample - (time - loop_time_st);
            if delta > (44100 / 100) {
                let com_wait = if self.rate == 60 { VGM_WAIT_NTSC_FRAME } else { VGM_WAIT_PAL_FRAME };
                self.commands.push(VGMCommand::new(com_wait, time));
                time += if self.rate == 60 { 44100 / 60 } else { 44100 / 50 };
            }
            self.commands.push(VGMCommand::new(VGM_LOOP_END, time));
        }

        // End command
        self.commands.push(VGMCommand::new(VGM_END, time));

        if !is_silent() {
            println!("Number of command: {}", self.commands.len());
        }
    }

    fn build_samples(&mut self, convert: bool) {
        // Build data blocks
        let mut data_block_cmds: Vec<VGMCommand> = Vec::new();
        for cmd in &self.commands {
            if cmd.is_data_block() {
                data_block_cmds.push(cmd.clone());
            }
        }
        for cmd in &data_block_cmds {
            self.add_data_block(cmd);
        }

        // Clean seek
        self.clean_seek_commands();

        // Extract samples from seek commands
        // This is complex because we need to modify commands while iterating.
        // We'll collect seek positions and process them.
        let mut i = 0;
        while i < self.commands.len() {
            if self.commands[i].is_seek() {
                i = self.extract_sample_from_seek(i, convert);
            } else {
                i += 1;
            }
        }

        // Adjust samples info from stream commands
        let mut sample_id_banks = [-1i32; 256];
        let mut sample_id_frequencies = [0u32; 256];

        // We need to handle stream info and do conversions
        let mut replacements: Vec<(usize, VGMCommand)> = Vec::new();

        for (idx, cmd) in self.commands.iter().enumerate() {
            if cmd.is_stream_data() {
                sample_id_banks[cmd.get_stream_id() as usize] = cmd.get_stream_bank_id();
            }
            if cmd.is_stream_frequency() {
                sample_id_frequencies[cmd.get_stream_id() as usize] = cmd.get_stream_frequency() as u32;
            }

            if cmd.is_stream_start() {
                let bank_id = sample_id_banks[cmd.get_stream_id() as usize];
                if let Some(bank) = self.sample_banks.iter_mut().find(|b| b.id == bank_id as u8) {
                    let sample_id = cmd.get_stream_block_id() as usize;
                    if let Some(sample) = bank.samples.iter_mut().find(|s| s.id == sample_id) {
                        let freq = sample_id_frequencies[cmd.get_stream_id() as usize];
                        sample.set_rate(freq);
                        if convert {
                            let new_cmd = sample.get_start_long_command_ex(bank.id, sample.len);
                            replacements.push((idx, new_cmd));
                        }
                    } else if !is_silent() {
                        println!("Warning: sample id {:02X} not found!", sample_id);
                    }
                } else if !is_silent() {
                    println!("Warning: sample bank id {:02X} not found!", bank_id);
                }
            }

            if cmd.is_stream_start_long() {
                let bank_id = sample_id_banks[cmd.get_stream_id() as usize];
                if let Some(bank) = self.sample_banks.iter_mut().find(|b| b.id == bank_id as u8) {
                    let sample_address = cmd.get_stream_sample_address() as usize;
                    let sample_len = cmd.get_stream_sample_size() as usize;
                    let freq = sample_id_frequencies[cmd.get_stream_id() as usize];
                    bank.add_sample(sample_address, sample_len, freq);
                }
            }
        }

        // Apply replacements
        for (idx, new_cmd) in replacements {
            self.commands[idx] = new_cmd;
        }

        if convert {
            self.remove_seek_and_play_pcm_commands();
        }
    }

    fn extract_sample_from_seek(&mut self, seek_idx: usize, convert: bool) -> usize {
        let sample_addr = self.commands[seek_idx].get_seek_address();

        let bank_id = self.sample_banks.last().map(|b| b.id);

        let mut len: i32 = 0;
        let mut wait: i32 = -1;
        let mut delta: i32;
        let mut delta_mean: f64 = 0.0;
        let mut end_play_wait: i32 = 0;

        let mut sample_data: i32 = 128;
        let mut sample_min_data: i32 = 128;
        let mut sample_max_data: i32 = 128;
        let mut sample_mean_delta: f64 = 0.0;

        let mut start_play_idx: Option<usize> = None;
        let mut end_play_idx: Option<usize> = None;

        let mut new_commands_before: Vec<(usize, Vec<VGMCommand>)> = Vec::new();
        let mut new_commands_after: Vec<(usize, Vec<VGMCommand>)> = Vec::new();

        let mut i = seek_idx + 1;
        let mut current_sample_addr = sample_addr;

        while i < self.commands.len() {
            let cmd = &self.commands[i];

            if cmd.is_data_block() || cmd.is_end() { break; }
            if cmd.is_seek() {
                let seek_addr = cmd.get_seek_address();
                let cur_addr = current_sample_addr + len;
                if (cur_addr + SAMPLE_ALLOWED_MARGE) < seek_addr || (cur_addr - SAMPLE_ALLOWED_MARGE) > seek_addr {
                    break;
                } else if is_verbose() {
                    println!("Seek command found with small offset change ({}) --> considering continue play", cur_addr - seek_addr);
                }
            }

            if wait != -1 {
                delta = wait - end_play_wait;
                if delta < 20 {
                    if delta_mean == 0.0 { delta_mean = delta as f64; }
                    else { delta_mean = (delta as f64 * 0.1) + (delta_mean * 0.9); }
                }

                if delta > SAMPLE_END_DELAY {
                    if len > 0 && end_play_wait > 0 && start_play_idx != end_play_idx {
                        if len < SAMPLE_MIN_SIZE && sample_ignore() {
                            if is_verbose() { println!("Sample at {:06X} is too small ({}) --> ignored", current_sample_addr, len); }
                        } else if (sample_max_data - sample_min_data) < SAMPLE_MIN_DYNAMIC && sample_ignore() {
                            if is_verbose() { println!("Sample at {:06X} has a too quiet global dynamic ({}) --> ignored", current_sample_addr, sample_max_data - sample_min_data); }
                        } else if (sample_mean_delta / len as f64) < SAMPLE_MIN_MEAN_DELTA && sample_ignore() {
                            if is_verbose() { println!("Sample at {:06X} is too quiet (mean delta value = {}) --> ignored", current_sample_addr, sample_mean_delta / len as f64); }
                        } else if let Some(bid) = bank_id {
                            if let Some(bank) = self.sample_banks.iter_mut().find(|b| b.id == bid) {
                                let rate = ((44100.0 * len as f64) / end_play_wait as f64).round() as u32;
                                let _sample_idx = bank.add_sample(current_sample_addr as usize, len as usize, rate);

                                if convert {
                                    if let Some(spi) = start_play_idx {
                                        if let Some(sample) = bank.get_sample_by_offset(current_sample_addr as usize) {
                                            let rate_cmd = sample.get_set_rate_command(bid, sample.rate);
                                            let start_cmd = sample.get_start_long_command_ex(bid, len as usize);
                                            new_commands_before.push((spi, vec![rate_cmd, start_cmd]));
                                        }
                                    }
                                    if let Some(epi) = end_play_idx {
                                        if let Some(sample) = bank.get_sample_by_offset(current_sample_addr as usize) {
                                            let stop_cmd = sample.get_stop_command(bid);
                                            new_commands_after.push((epi, vec![stop_cmd]));
                                        }
                                    }
                                }
                            }
                        }
                    }

                    current_sample_addr += len;
                    len = 0;
                    wait = -1;
                    delta_mean = 0.0;
                    end_play_wait = 0;
                    sample_data = 128;
                    sample_min_data = 128;
                    sample_max_data = 128;
                    sample_mean_delta = 0.0;
                    start_play_idx = None;
                    end_play_idx = None;
                }
            }

            let cmd = &self.commands[i];
            if cmd.is_pcm() {
                if wait == -1 {
                    wait = 0;
                    start_play_idx = Some(i);
                }

                if sample_rate_fix() && delta_mean != 0.0 {
                    let mean = delta_mean.round() as i32;
                    let d = wait - end_play_wait;
                    if d < (mean - 2) { wait += mean - d; }
                    else if d > (mean + 2) { wait -= d - mean; }
                }

                end_play_wait = wait;
                end_play_idx = Some(i);

                if let Some(bid) = bank_id {
                    if let Some(bank) = self.sample_banks.iter().find(|b| b.id == bid) {
                        let data_idx = current_sample_addr as usize + 7 + len as usize;
                        if data_idx < bank.data.len() {
                            let d = bank.data[data_idx] as i32;
                            sample_mean_delta += (d - sample_data).abs() as f64;
                            if sample_min_data > d { sample_min_data = d; }
                            if sample_max_data < d { sample_max_data = d; }
                            sample_data = d;
                        }
                    }
                }

                wait += cmd.get_wait_value();
                len += 1;
            } else if wait != -1 {
                wait += cmd.get_wait_value();
            }

            i += 1;
        }

        // Final sample
        if len > 0 && end_play_wait > 0 && start_play_idx != end_play_idx {
            if len < SAMPLE_MIN_SIZE && sample_ignore() {
                if is_verbose() { println!("Sample at {:06X} is too small ({}) --> ignored", current_sample_addr, len); }
            } else if (sample_max_data - sample_min_data) < SAMPLE_MIN_DYNAMIC && sample_ignore() {
                if is_verbose() { println!("Sample at {:06X} has too quiet global dynamic ({}) --> ignored", current_sample_addr, sample_max_data - sample_min_data); }
            } else if (sample_mean_delta / len as f64) < SAMPLE_MIN_MEAN_DELTA && sample_ignore() {
                if is_verbose() { println!("Sample at {:06X} is too quiet (mean delta value = {}) --> ignored", current_sample_addr, sample_mean_delta / len as f64); }
            } else if let Some(bid) = bank_id {
                if let Some(bank) = self.sample_banks.iter_mut().find(|b| b.id == bid) {
                    let rate = ((44100.0 * len as f64) / end_play_wait as f64).round() as u32;
                    let _sample_idx = bank.add_sample(current_sample_addr as usize, len as usize, rate);

                    if convert {
                        if let Some(spi) = start_play_idx {
                            if let Some(sample) = bank.get_sample_by_offset(current_sample_addr as usize) {
                                let rate_cmd = sample.get_set_rate_command(bid, sample.rate);
                                let start_cmd = sample.get_start_long_command_ex(bid, len as usize);
                                new_commands_before.push((spi, vec![rate_cmd, start_cmd]));
                            }
                        }
                        if let Some(epi) = end_play_idx {
                            if let Some(sample) = bank.get_sample_by_offset(current_sample_addr as usize) {
                                let stop_cmd = sample.get_stop_command(bid);
                                new_commands_after.push((epi, vec![stop_cmd]));
                            }
                        }
                    }
                }
            }
        }

        // Insert new commands (in reverse order to keep indices valid)
        // After commands first (they shift things less)
        new_commands_after.sort_by(|a, b| b.0.cmp(&a.0));
        for (idx, cmds) in new_commands_after {
            for (j, cmd) in cmds.into_iter().enumerate() {
                self.commands.insert(idx + 1 + j, cmd);
            }
        }
        new_commands_before.sort_by(|a, b| b.0.cmp(&a.0));
        for (idx, cmds) in new_commands_before {
            for (j, cmd) in cmds.into_iter().rev().enumerate() {
                self.commands.insert(idx, cmd);
            }
        }

        i
    }

    fn add_data_block(&mut self, command: &VGMCommand) {
        let bank_id = command.get_data_bank_id();

        if let Some(bank) = self.sample_banks.iter_mut().find(|b| b.id == bank_id) {
            if is_verbose() {
                println!("Add data block {:06X} to bank {:02X}", command.offset, bank_id);
            }
            bank.add_block(command);
        } else {
            if is_verbose() {
                println!("Add data bank {:06X}:{:02X}", command.offset, bank_id);
            }
            if let Some(bank) = SampleBank::create(command) {
                self.sample_banks.push(bank);
            }
        }
    }

    fn clean_seek_commands(&mut self) {
        // Scan backwards: remove seek if no PCM played after it
        let mut sample_played = false;
        let mut to_remove = Vec::new();

        for i in (0..self.commands.len()).rev() {
            if self.commands[i].is_seek() {
                if !sample_played {
                    if !is_silent() {
                        println!("Useless seek command found at {:06X}", self.commands[i].offset);
                    }
                    to_remove.push(i);
                }
                sample_played = false;
            } else if self.commands[i].is_pcm() {
                sample_played = true;
            }
        }

        for idx in to_remove {
            self.commands.remove(idx);
        }
    }

    fn remove_seek_and_play_pcm_commands(&mut self) {
        let mut time = 0;
        let mut replacements = Vec::new();
        let mut removals = Vec::new();

        for (i, cmd) in self.commands.iter().enumerate() {
            let wait = cmd.get_wait_value();

            if cmd.is_seek() {
                removals.push(i);
            } else if cmd.is_pcm() {
                if wait == 0 {
                    removals.push(i);
                } else {
                    replacements.push((i, VGMCommand::new(0x70 + (wait as u8 - 1), time)));
                }
            }

            time += wait;
        }

        // Apply replacements (indices are stable since we haven't removed anything yet)
        for (idx, new_cmd) in &replacements {
            self.commands[*idx] = new_cmd.clone();
        }

        // Remove in reverse order
        removals.sort_unstable();
        for idx in removals.into_iter().rev() {
            self.commands.remove(idx);
        }

        if !is_silent() {
            println!("Number of command after PCM command remove: {}", self.commands.len());
        }
        if is_verbose() {
            println!("Computed VGM duration: {} samples ({} seconds)", self.compute_len(), self.compute_len() / 44100);
        }
    }

    pub fn clean_commands(&mut self) {
        let mut new_commands: Vec<VGMCommand> = Vec::new();

        let mut ym_old_state = YM2612::new();
        let mut psg_old_state = PSG::new();

        let mut start = 0;
        while start < self.commands.len() {
            // Find frame boundary (next wait or end)
            let mut end = start;
            loop {
                let cmd = &self.commands[end];
                end += 1;
                if cmd.is_wait() || cmd.is_end() || end >= self.commands.len() { break; }
            }

            let mut ym_state = ym_old_state.clone();
            let mut psg_state = psg_old_state.clone();

            let mut optimized: Vec<VGMCommand> = Vec::new();
            let mut key_on_off: Vec<VGMCommand> = Vec::new();
            let mut ym_commands: Vec<VGMCommand> = Vec::new();
            let mut last_commands: Vec<VGMCommand> = Vec::new();
            let mut has_key_com = false;

            for i in start..end {
                let cmd = &self.commands[i];

                if cmd.is_data_block() || cmd.is_stream() || cmd.is_loop_start() || cmd.is_loop_end()
                    || (keep_rf5c68_cmds() && cmd.is_rf5c68_control())
                {
                    optimized.push(cmd.clone());
                    if cmd.is_loop_start() {
                        ym_old_state = YM2612::new();
                        psg_old_state = PSG::new();
                    }
                } else if cmd.is_psg_write() {
                    psg_state.write(cmd.get_psg_value());
                } else if cmd.is_ym2612_write() {
                    if cmd.is_ym2612_key_write() {
                        key_on_off.push(cmd.clone());
                        has_key_com = true;
                    } else {
                        if has_key_com {
                            ym_commands.extend(ym_old_state.get_delta(&ym_state));
                            ym_commands.append(&mut key_on_off);
                            ym_old_state = ym_state.clone();
                            ym_state = ym_old_state.clone();
                            has_key_com = false;
                        }
                    }
                    ym_state.set(cmd.get_ym2612_port() as usize, cmd.get_ym2612_register() as usize, cmd.get_ym2612_value());
                } else if cmd.is_wait() || cmd.is_seek() {
                    last_commands.push(cmd.clone());
                } else if is_verbose() {
                    println!("Command ignored: {:02X}", cmd.command);
                }
            }

            // Check for duplicate stream commands
            let mut has_stream_start = false;
            let mut has_stream_rate = false;
            optimized.retain(|cmd| {
                if cmd.is_stream_start_long() {
                    if has_stream_start {
                        if !is_silent() {
                            println!("Warning: more than 1 PCM command in a single frame!");
                        }
                        return false;
                    }
                    has_stream_start = true;
                }
                if cmd.is_stream_frequency() {
                    if has_stream_rate {
                        if !is_silent() {
                            println!("Command stream rate removed");
                        }
                        return false;
                    }
                    has_stream_rate = true;
                }
                true
            });

            // Merge commands in order
            new_commands.append(&mut optimized);
            new_commands.append(&mut ym_commands);

            // Delta YM
            new_commands.extend(ym_old_state.get_delta(&ym_state));
            // Key on/off
            new_commands.append(&mut key_on_off);
            // Delta PSG
            new_commands.extend(psg_old_state.get_delta(&psg_state));
            // Wait/other
            new_commands.append(&mut last_commands);

            ym_old_state = ym_state;
            psg_old_state = psg_state;
            start = end;
        }

        new_commands.push(VGMCommand::new(VGM_END, -1));
        self.commands = new_commands;

        if is_verbose() {
            println!("Music data size: {}", self.get_music_data_size());
        }
        if !is_silent() {
            println!("Computed VGM duration: {} samples ({} seconds)", self.compute_len(), self.compute_len() / 44100);
            println!("Number of command after commands clean: {}", self.commands.len());
        }
    }

    pub fn clean_samples(&mut self) {
        for bank in &mut self.sample_banks {
            let commands = &self.commands;
            bank.samples.retain(|sample| {
                let mut current_bank_id: i32 = -1;
                for cmd in commands.iter() {
                    if cmd.is_stream_data() {
                        current_bank_id = cmd.get_stream_bank_id();
                    }
                    if current_bank_id == bank.id as i32 {
                        if cmd.is_stream_start() && sample.id == cmd.get_stream_block_id() as usize {
                            return true;
                        }
                        if cmd.is_stream_start_long() {
                            let addr = cmd.get_stream_sample_address() as usize;
                            let slen = cmd.get_stream_sample_size() as usize;
                            let min_len = sample.len.saturating_sub(50);
                            let max_len = sample.len + 50;
                            if sample.data_offset == addr && slen >= min_len && slen <= max_len {
                                return true;
                            }
                        }
                    }
                }
                if is_verbose() {
                    println!("Sample at offset {:06X} (len = {}) is not used --> removed", sample.data_offset, sample.len);
                }
                false
            });
        }
    }

    pub fn get_sample(&self, sample_offset: i32) -> Option<&Sample> {
        for bank in &self.sample_banks {
            if let Some(sample) = bank.get_sample_by_offset(sample_offset as usize) {
                return Some(sample);
            }
        }
        None
    }

    pub fn convert_waits(&mut self) {
        let mut new_commands: Vec<VGMCommand> = Vec::new();
        let limit = 44100.0 / self.rate as f64;
        let min_limit = limit - (limit * 15.0 / 100.0);
        let com_wait = if self.rate == 60 { VGM_WAIT_NTSC_FRAME } else { VGM_WAIT_PAL_FRAME };
        let mut sample_cnt: f64 = 0.0;
        let mut time = 0;

        for cmd in &self.commands {
            let wait = cmd.get_wait_value();

            if !cmd.is_wait() {
                new_commands.push(cmd.clone());
            } else {
                sample_cnt += wait as f64;
            }

            let mut ttime = time;
            while sample_cnt > min_limit {
                new_commands.push(VGMCommand::new(com_wait, ttime));
                sample_cnt -= limit;
                ttime += limit as i32;
            }

            time += wait;
        }

        self.commands = new_commands;

        if !is_silent() {
            println!("VGM duration after wait command conversion: {} samples ({} seconds)", self.compute_len(), self.compute_len() / 44100);
            println!("Number of command: {}", self.commands.len());
        }
    }

    pub fn fix_key_commands(&mut self) {
        let max_delta = (44100 / self.rate) / 4;
        let mut key_off_time = [-1i32; 6];
        let mut key_on_time = [-1i32; 6];
        let mut frame = 0;
        let mut delayed_commands: Vec<VGMCommand> = Vec::new();

        let mut i = 0;
        while i < self.commands.len() {
            let cmd = &self.commands[i];

            if cmd.is_wait() {
                // Insert delayed commands after the wait
                if !delayed_commands.is_empty() {
                    let insert = delayed_commands.drain(..).collect::<Vec<_>>();
                    let insert_pos = i + 1;
                    for (j, c) in insert.into_iter().enumerate() {
                        self.commands.insert(insert_pos + j, c);
                    }
                }
                for k in 0..6 { key_off_time[k] = -1; key_on_time[k] = -1; }
                frame += 1;
            } else if cmd.is_ym2612_key_write() {
                let ch = cmd.get_ym2612_key_channel();
                if ch != -1 {
                    let ch = ch as usize;
                    if cmd.is_ym2612_key_off_write() {
                        key_off_time[ch] = cmd.time;
                        if key_on_time[ch] != -1 && cmd.time != -1 && (cmd.time - key_on_time[ch]) > max_delta {
                            if delay_key_off() {
                                if !is_silent() {
                                    println!("Warning: CH{} delayed key OFF command at frame {}", ch, frame);
                                    println!("You can try to use the -dd switch if you experience missing or incorrect FM instrument sound.");
                                }
                                let removed = self.commands.remove(i);
                                if get_key_off_command(&delayed_commands, ch as i32).is_none() {
                                    delayed_commands.push(removed);
                                }
                                continue; // Don't increment i
                            } else if !is_silent() {
                                println!("Warning: CH{} key ON/OFF events occurred at frame {} and delayed key OFF has been disabled.", ch, frame);
                            }
                        }
                    } else {
                        key_on_time[ch] = cmd.time;
                    }
                }
            }
            i += 1;
        }
    }

    // Compute methods
    pub fn compute_len(&self) -> i32 {
        self.compute_len_from(None)
    }

    pub fn compute_len_from(&self, from: Option<usize>) -> i32 {
        let mut result = 0;
        let mut count = from.is_none();

        for (i, cmd) in self.commands.iter().enumerate() {
            if let Some(f) = from {
                if i == f { count = true; }
            }
            if count { result += cmd.get_wait_value(); }
        }
        result
    }

    pub fn get_time(&self, cmd_idx: usize) -> i32 {
        let mut result = 0;
        for (i, cmd) in self.commands.iter().enumerate() {
            if i == cmd_idx { return result; }
            result += cmd.get_wait_value();
        }
        0
    }

    pub fn get_time_in_frame(&self, cmd_idx: usize) -> i32 {
        self.get_time(cmd_idx) / (44100 / self.rate)
    }

    pub fn get_command_idx_at_time(&self, time: i32) -> Option<usize> {
        let mut result = 0;
        for (i, cmd) in self.commands.iter().enumerate() {
            if result >= time { return Some(i); }
            result += cmd.get_wait_value();
        }
        None
    }

    fn get_sample_data_size(&self) -> i32 {
        self.sample_banks.iter().map(|b| b.len as i32).sum()
    }

    fn get_sample_total_len(&self) -> i32 {
        self.sample_banks.iter().flat_map(|b| &b.samples).map(|s| s.len as i32).sum()
    }

    fn get_sample_number(&self) -> usize {
        self.sample_banks.iter().map(|b| b.samples.len()).sum()
    }

    fn get_music_data_size(&self) -> usize {
        self.commands.iter().filter(|c| !c.is_data_block()).map(|c| c.size).sum()
    }

    /// Serialize VGM to byte array
    pub fn as_byte_array(&self) -> Vec<u8> {
        let mut out = Vec::new();

        // Header (0x80 bytes)
        out.extend_from_slice(b"Vgm ");                     // 00-03
        out.extend_from_slice(&[0, 0, 0, 0]);               // 04-07: file size (fill later)
        out.extend_from_slice(&[0x60, 0x01, 0x00, 0x00]);   // 08-0B: version 1.60
        out.extend_from_slice(&[0x99, 0x9E, 0x36, 0x00]);   // 0C-0F: SN76489 clock
        out.extend_from_slice(&[0, 0, 0, 0]);                // 10-13: YM2413 clock
        out.extend_from_slice(&[0, 0, 0, 0]);                // 14-17: GD3 offset (fill later)
        out.extend_from_slice(&[0, 0, 0, 0]);                // 18-1B: total samples (fill later)
        out.extend_from_slice(&[0, 0, 0, 0]);                // 1C-1F: loop offset (fill later)
        out.extend_from_slice(&[0, 0, 0, 0]);                // 20-23: loop samples (fill later)
        out.push(self.rate as u8);                            // 24: rate
        out.extend_from_slice(&[0, 0, 0]);                   // 25-27
        out.extend_from_slice(&[0x09, 0x00, 0x10, 0x00]);   // 28-2B: SN76489 flags
        out.extend_from_slice(&[0xB5, 0x0A, 0x75, 0x00]);   // 2C-2F: YM2612 clock
        out.extend_from_slice(&[0, 0, 0, 0]);                // 30-33: YM2151 clock
        out.extend_from_slice(&[0x4C, 0, 0, 0]);             // 34-37: VGM data offset
        out.extend_from_slice(&[0, 0, 0, 0]);                // 38-3B: Sega PCM clock
        out.extend_from_slice(&[0, 0, 0, 0]);                // 3C-3F: Sega PCM interface
        out.resize(0x80, 0);                                  // 40-7F: zeroes

        let mut loop_cmd_idx: Option<usize> = None;
        let mut loop_offset: i32 = 0;

        // Write commands (skip loop markers)
        for (i, cmd) in self.commands.iter().enumerate() {
            if cmd.is_loop_start() {
                loop_cmd_idx = Some(i);
                loop_offset = out.len() as i32 - 0x1C;
            } else if !cmd.is_loop_end() {
                out.extend_from_slice(&cmd.as_byte_array());
            }
        }

        // Write GD3 tags
        let gd3_offset = if let Some(gd3) = &self.gd3 {
            let go = out.len();
            out.extend_from_slice(&gd3.as_byte_array());
            Some(go)
        } else { None };

        // Fix header fields
        if let Some(lcidx) = loop_cmd_idx {
            crate::set_u32(&mut out, 0x1C, loop_offset as u32);
            crate::set_u32(&mut out, 0x20, self.compute_len_from(Some(lcidx)) as u32);
        }
        if let Some(go) = gd3_offset {
            crate::set_u32(&mut out, 0x14, (go - 0x14) as u32);
        }
        let eof_offset = (out.len() - 4) as u32;
        crate::set_u32(&mut out, 0x04, eof_offset);
        let total_len = self.compute_len();
        crate::set_u32(&mut out, 0x18, (total_len - 1).max(0) as u32);

        out
    }

    /// Serialize VGM to byte array, separating data blocks
    pub fn as_byte_array2(&self) -> (Vec<u8>, Vec<u8>) {
        let mut out = Vec::new();
        let mut data_blocks = Vec::new();

        // Same header as as_byte_array
        out.extend_from_slice(b"Vgm ");
        out.extend_from_slice(&[0, 0, 0, 0]);
        out.extend_from_slice(&[0x60, 0x01, 0x00, 0x00]);
        out.extend_from_slice(&[0x99, 0x9E, 0x36, 0x00]);
        out.extend_from_slice(&[0, 0, 0, 0]);
        out.extend_from_slice(&[0, 0, 0, 0]);
        out.extend_from_slice(&[0, 0, 0, 0]);
        out.extend_from_slice(&[0, 0, 0, 0]);
        out.extend_from_slice(&[0, 0, 0, 0]);
        out.push(self.rate as u8);
        out.extend_from_slice(&[0, 0, 0]);
        out.extend_from_slice(&[0x09, 0x00, 0x10, 0x00]);
        out.extend_from_slice(&[0xB5, 0x0A, 0x75, 0x00]);
        out.extend_from_slice(&[0, 0, 0, 0]);
        out.extend_from_slice(&[0x4C, 0, 0, 0]);
        out.extend_from_slice(&[0, 0, 0, 0]);
        out.extend_from_slice(&[0, 0, 0, 0]);
        out.resize(0x80, 0);

        let mut loop_cmd_idx: Option<usize> = None;
        let mut loop_offset: i32 = 0;

        for (i, cmd) in self.commands.iter().enumerate() {
            if cmd.is_loop_start() {
                loop_cmd_idx = Some(i);
                loop_offset = out.len() as i32 - 0x1C;
            } else if !cmd.is_loop_end() {
                if cmd.is_data_block() {
                    data_blocks.extend_from_slice(&cmd.as_byte_array());
                } else {
                    out.extend_from_slice(&cmd.as_byte_array());
                }
            }
        }

        // GD3
        let gd3_offset = if let Some(gd3) = &self.gd3 {
            let go = out.len();
            out.extend_from_slice(&gd3.as_byte_array());
            Some(go)
        } else { None };

        // Fix header
        if let Some(lcidx) = loop_cmd_idx {
            crate::set_u32(&mut out, 0x1C, loop_offset as u32);
            crate::set_u32(&mut out, 0x20, self.compute_len_from(Some(lcidx)) as u32);
        }
        if let Some(go) = gd3_offset {
            crate::set_u32(&mut out, 0x14, (go - 0x14) as u32);
        }
        let eof_offset2 = (out.len() - 4) as u32;
        crate::set_u32(&mut out, 0x04, eof_offset2);
        let total_len = self.compute_len();
        crate::set_u32(&mut out, 0x18, (total_len - 1).max(0) as u32);

        (out, data_blocks)
    }
}
