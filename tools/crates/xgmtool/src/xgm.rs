use crate::vgm_command::*;
use crate::xgm_command::*;
use crate::xgm_sample::XGMSample;
use crate::gd3::{GD3, XD3};
use crate::vgm::VGM;
use crate::{is_silent, is_verbose};

/// Represents a parsed XGM file
pub struct XGM {
    pub samples: Vec<XGMSample>,
    pub commands: Vec<XGMCommand>,
    pub gd3: Option<GD3>,
    pub xd3: Option<XD3>,
    pub pal: bool,
    pal_set: bool,
}

impl XGM {
    pub fn new() -> Self {
        XGM {
            samples: Vec::new(),
            commands: Vec::new(),
            gd3: None,
            xd3: None,
            pal: false,
            pal_set: false,
        }
    }

    /// Parse XGM from raw data
    pub fn create_from_data(data: &[u8], _data_size: usize) -> Option<Self> {
        let mut result = XGM::new();

        if !is_silent() { println!("Parsing XGM file..."); }

        if data.len() < 0x104 || &data[0..4] != b"XGM " {
            println!("Error: XGM file not recognized!");
            return None;
        }

        // Sample id table
        for s in 1..0x40usize {
            let offset = crate::get_u16(data, s * 4) as usize;
            let len = crate::get_u16(data, s * 4 + 2) as usize;

            if offset != 0xFFFF && len != 0x0100 {
                let abs_offset = (offset << 8) + 0x104;
                let abs_len = len << 8;
                result.samples.push(XGMSample {
                    index: s,
                    data: data[abs_offset..abs_offset + abs_len].to_vec(),
                    data_size: abs_len,
                    origin_addr: (offset << 8) as i32,
                });
            }
        }

        // Music data
        let music_offset = ((crate::get_u16(data, 0x100) as usize) << 8) + 0x104;
        result.pal = (data[0x103] & 1) != 0;
        result.pal_set = true;

        let music_len = crate::get_u32(data, music_offset) as usize;

        if is_verbose() {
            println!("XGM sample number: {}", result.samples.len());
            println!("XGM start music data: {:06X}  len: {}", music_offset + 4, music_len);
        }

        result.parse_music(&data[music_offset + 4..music_offset + 4 + music_len]);

        if !is_silent() {
            println!("XGM duration: {} frames ({} seconds)", result.compute_len_in_frame(), result.compute_len_in_second());
        }

        // GD3 tags
        if (data[0x103] & 2) != 0 {
            result.gd3 = GD3::from_data(&data[music_offset + 4 + music_len..]);
        }

        Some(result)
    }

    /// Parse XGM from XGC data (compiled format)
    pub fn create_from_xgc_data(data: &[u8], _data_size: usize) -> Option<Self> {
        let mut result = XGM::new();

        if !is_silent() { println!("Parsing XGM from XGC file..."); }

        // Sample id table (0x3F entries starting at 0)
        for s in 0..0x3Fusize {
            let offset = crate::get_u16(data, s * 4) as usize;
            let len = crate::get_u16(data, s * 4 + 2) as usize;

            if offset != 0xFFFF && len != 0x0100 {
                let abs_offset = (offset << 8) + 0x104;
                let abs_len = len << 8;
                result.samples.push(XGMSample {
                    index: s + 1,
                    data: data[abs_offset..abs_offset + abs_len].to_vec(),
                    data_size: abs_len,
                    origin_addr: (offset << 8) as i32,
                });
            }
        }

        let music_offset = ((crate::get_u16(data, 0xFC) as usize) << 8) + 0x100;
        result.pal = (data[0xFF] & 1) != 0;
        result.pal_set = true;

        let music_len = crate::get_u32(data, music_offset) as usize;

        if is_verbose() {
            println!("XGM sample number: {}", result.samples.len());
            println!("XGM start music data: {:06X}  len: {}", music_offset + 4, music_len);
        }

        result.parse_music_from_xgc(&data[music_offset + 4..music_offset + 4 + music_len]);

        if !is_silent() {
            println!("XGM duration: {} frames ({} seconds)", result.compute_len_in_frame(), result.compute_len_in_second());
        }

        Some(result)
    }

    /// Convert VGM to XGM
    pub fn create_from_vgm(vgm: &VGM) -> Self {
        let mut result = XGM::new();

        if !is_silent() { println!("Converting VGM to XGM..."); }

        if vgm.rate == 60 { result.pal = false; result.pal_set = true; }
        else if vgm.rate == 50 { result.pal = true; result.pal_set = true; }

        result.gd3 = vgm.gd3.clone();

        // Extract samples
        result.extract_samples(vgm);
        // Extract music
        result.extract_music(vgm);

        if is_verbose() {
            println!("XGM sample number: {}", result.samples.len());
            println!("Sample size: {}", result.get_sample_data_size());
            println!("Music data size: {}", result.get_music_data_size());
        }
        if !is_silent() {
            println!("XGM duration: {} frames ({} seconds)", result.compute_len_in_frame(), result.compute_len_in_second());
        }

        result
    }

    fn parse_music(&mut self, data: &[u8]) {
        let mut off = 0;
        while off < data.len() {
            let command = XGMCommand::create_from_data(&data[off..]);
            off += command.size;
            let is_end = command.is_end();
            self.commands.push(command);
            if is_end { break; }
        }
        if !is_silent() {
            println!("Number of command: {}", self.commands.len());
        }
    }

    fn parse_music_from_xgc(&mut self, data: &[u8]) {
        let mut off = 0;
        while off < data.len() {
            let frame_size = data[off] as usize - 1;
            off += 1;
            let mut remaining = frame_size;
            while remaining > 0 {
                let command = crate::xgc_command::create_from_data(&data[off..]);
                let cmd_size = command.size;

                if !crate::xgc_command::is_state(&command) && !crate::xgc_command::is_frame_skip(&command) {
                    self.commands.push(command);
                }

                off += cmd_size;
                remaining = remaining.saturating_sub(cmd_size);
            }
            self.commands.push(XGMCommand::create_frame_command());
        }
        if !is_silent() {
            println!("Number of command: {}", self.commands.len());
        }
    }

    fn extract_samples(&mut self, vgm: &VGM) {
        let mut index = self.samples.len() + 1;

        for bank in &vgm.sample_banks {
            for sample in &bank.samples {
                if index >= 64 {
                    if !is_silent() {
                        println!("Error: XGM does not support music with more than 63 samples!");
                        println!("Input VGM file probably has improper PCM data extraction, try to use another VGM source.");
                    }
                    return;
                }

                if let Some(xgm_sample) = XGMSample::create_from_vgm_sample(bank, sample) {
                    let mut s = xgm_sample;
                    s.index = index;
                    index += 1;
                    self.samples.push(s);
                }
            }
        }
    }

    fn extract_music(&mut self, vgm: &VGM) {
        let mut loop_offset: i32 = -1;
        let mut frame = 0;
        let mut vgm_idx = 0;

        while vgm_idx < vgm.commands.len() {
            // Collect frame commands
            let mut frame_commands: Vec<&VGMCommand> = Vec::new();
            let mut loop_end = false;

            while vgm_idx < vgm.commands.len() {
                let command = &vgm.commands[vgm_idx];
                vgm_idx += 1;

                if command.is_data_block() { continue; }
                if command.is_loop_start() {
                    if loop_offset == -1 {
                        loop_offset = self.get_music_data_size_of(&self.commands) as i32;
                    }
                    continue;
                }
                if command.is_loop_end() { loop_end = true; continue; }
                if command.is_wait() {
                    if !self.pal_set {
                        if command.is_wait_pal() { self.pal = true; self.pal_set = true; }
                        else if command.is_wait_ntsc() { self.pal = false; self.pal_set = true; }
                    }
                    break;
                }
                if command.is_end() { break; }
                frame_commands.push(command);
            }

            // Group commands
            let mut ym_key_commands: Vec<&VGMCommand> = Vec::new();
            let mut ym_port0_commands: Vec<&VGMCommand> = Vec::new();
            let mut ym_port1_commands: Vec<&VGMCommand> = Vec::new();
            let mut psg_commands: Vec<&VGMCommand> = Vec::new();
            let mut sample_commands: Vec<&VGMCommand> = Vec::new();
            let mut xgm_commands: Vec<XGMCommand> = Vec::new();
            let mut has_key_com = false;

            for cmd in &frame_commands {
                if cmd.is_stream() {
                    sample_commands.push(cmd);
                } else if cmd.is_psg_write() {
                    psg_commands.push(cmd);
                } else if cmd.is_ym2612_key_write() {
                    ym_key_commands.push(cmd);
                    has_key_com = true;
                } else if cmd.is_ym2612_write() {
                    if has_key_com {
                        // Flush current groups
                        if !ym_port0_commands.is_empty() {
                            let vgm_cmds: Vec<VGMCommand> = ym_port0_commands.iter().map(|c| (*c).clone()).collect();
                            xgm_commands.extend(XGMCommand::create_ym_port0_commands(&vgm_cmds));
                            ym_port0_commands.clear();
                        }
                        if !ym_port1_commands.is_empty() {
                            let vgm_cmds: Vec<VGMCommand> = ym_port1_commands.iter().map(|c| (*c).clone()).collect();
                            xgm_commands.extend(XGMCommand::create_ym_port1_commands(&vgm_cmds));
                            ym_port1_commands.clear();
                        }
                        if !ym_key_commands.is_empty() {
                            let vgm_cmds: Vec<VGMCommand> = ym_key_commands.iter().map(|c| (*c).clone()).collect();
                            xgm_commands.extend(XGMCommand::create_ym_key_commands(&vgm_cmds));
                            ym_key_commands.clear();
                        }
                        has_key_com = false;
                    }

                    if cmd.is_ym2612_port0_write() {
                        ym_port0_commands.push(cmd);
                    } else {
                        ym_port1_commands.push(cmd);
                    }
                } else if is_verbose() {
                    println!("Command {} ignored at frame {}", cmd.command, frame);
                }
            }

            // Flush remaining groups
            if !ym_port0_commands.is_empty() {
                let vgm_cmds: Vec<VGMCommand> = ym_port0_commands.iter().map(|c| (*c).clone()).collect();
                xgm_commands.extend(XGMCommand::create_ym_port0_commands(&vgm_cmds));
            }
            if !ym_port1_commands.is_empty() {
                let vgm_cmds: Vec<VGMCommand> = ym_port1_commands.iter().map(|c| (*c).clone()).collect();
                xgm_commands.extend(XGMCommand::create_ym_port1_commands(&vgm_cmds));
            }
            if !ym_key_commands.is_empty() {
                let vgm_cmds: Vec<VGMCommand> = ym_key_commands.iter().map(|c| (*c).clone()).collect();
                xgm_commands.extend(XGMCommand::create_ym_key_commands(&vgm_cmds));
            }
            if !psg_commands.is_empty() {
                let vgm_cmds: Vec<VGMCommand> = psg_commands.iter().map(|c| (*c).clone()).collect();
                xgm_commands.extend(XGMCommand::create_psg_commands(&vgm_cmds));
            }
            if !sample_commands.is_empty() {
                let vgm_cmds: Vec<VGMCommand> = sample_commands.iter().map(|c| (*c).clone()).collect();
                xgm_commands.extend(XGMCommand::create_pcm_commands(&self.samples, vgm, &vgm_cmds));
            }

            // Loop
            if loop_end && loop_offset != -1 {
                xgm_commands.push(XGMCommand::create_loop_command(loop_offset));
                loop_offset = -1;
            }

            // Last frame?
            if vgm_idx >= vgm.commands.len() {
                if loop_offset != -1 {
                    xgm_commands.push(XGMCommand::create_loop_command(loop_offset));
                }
                xgm_commands.push(XGMCommand::create_end_command());
            } else {
                xgm_commands.push(XGMCommand::create_frame_command());
            }

            let num_com = xgm_commands.len();
            if num_com > 200 && !is_silent() {
                println!("Warning: Heavy frame at position {} ({} commands), playback may be altered!", frame, num_com);
            }

            self.commands.extend(xgm_commands);
            frame += 1;
        }

        self.compute_all_offset();

        if !is_silent() {
            println!("Number of command: {}", self.commands.len());
        }
    }

    pub fn compute_all_offset(&mut self) {
        let mut offset = 0;
        for cmd in &mut self.commands {
            cmd.set_offset(offset);
            offset += cmd.size as i32;
        }
    }

    pub fn get_loop_command(&self) -> Option<&XGMCommand> {
        self.commands.iter().find(|c| c.is_loop())
    }

    pub fn get_loop_pointed_command_idx(&self) -> Option<usize> {
        if let Some(loop_cmd) = self.get_loop_command() {
            let target_offset = loop_cmd.get_loop_offset();
            return self.get_command_idx_at_offset(target_offset);
        }
        None
    }

    pub fn compute_len_in_frame(&self) -> i32 {
        self.commands.iter().filter(|c| c.is_frame()).count() as i32
    }

    pub fn compute_len_in_second(&self) -> i32 {
        self.compute_len_in_frame() / if self.pal { 50 } else { 60 }
    }

    pub fn get_time(&self, cmd: &XGMCommand) -> i32 {
        let mut result: i32 = -1;
        for c in &self.commands {
            if c.is_frame() { result += 1; }
            if std::ptr::eq(c, cmd) { break; }
        }
        (result * 44100) / if self.pal { 50 } else { 60 }
    }

    pub fn get_command_at_offset(&self, offset: i32) -> Option<&XGMCommand> {
        let idx = self.get_command_idx_at_offset(offset)?;
        Some(&self.commands[idx])
    }

    pub fn get_command_idx_at_offset(&self, offset: i32) -> Option<usize> {
        let mut cur_offset: i32 = 0;
        for (i, cmd) in self.commands.iter().enumerate() {
            if cur_offset == offset { return Some(i); }
            cur_offset += cmd.size as i32;
        }
        None
    }

    pub fn get_sample_by_index(&self, index: usize) -> Option<&XGMSample> {
        self.samples.iter().find(|s| s.index == index)
    }

    pub fn get_sample_by_address(&self, origin_addr: i32) -> Option<&XGMSample> {
        self.samples.iter().find(|s| s.origin_addr == origin_addr)
    }

    pub fn get_sample_data_size(&self) -> usize {
        self.samples.iter().map(|s| s.data_size).sum()
    }

    fn get_music_data_size_of(&self, commands: &[XGMCommand]) -> usize {
        commands.iter().map(|c| c.size).sum()
    }

    pub fn get_music_data_size(&self) -> usize {
        self.get_music_data_size_of(&self.commands)
    }

    /// Serialize XGM to byte array
    pub fn as_byte_array(&self) -> Vec<u8> {
        let mut out = Vec::new();

        // 0000-0003: "XGM "
        out.extend_from_slice(b"XGM ");

        // 0004-00FF: sample id table (63 entries of 4 bytes)
        let mut sample_offset: usize = 0;
        let mut i = 0;
        for sample in &self.samples {
            let len = sample.data_size;
            out.push((sample_offset >> 8) as u8);
            out.push((sample_offset >> 16) as u8);
            out.push((len >> 8) as u8);
            out.push((len >> 16) as u8);
            sample_offset += len;
            i += 1;
        }
        // Fill remaining with 0xFFFF / 0x0100
        while i < 0x3F {
            out.extend_from_slice(&[0xFF, 0xFF, 0x00, 0x00]);
            i += 1;
        }

        // 0100-0101: sample block size * 256
        out.push((sample_offset >> 8) as u8);
        out.push((sample_offset >> 16) as u8);

        // 0102: XGM version
        out.push(0x01);

        // 0103: flags
        let mut flags: u8 = 0;
        if self.pal { flags |= 1; }
        if self.gd3.is_some() { flags |= 2; }
        out.push(flags);

        // 0104+: sample data
        for sample in &self.samples {
            out.extend_from_slice(&sample.data[..sample.data_size]);
        }

        // Music data size (4 bytes)
        let music_len = self.get_music_data_size();
        out.push(music_len as u8);
        out.push((music_len >> 8) as u8);
        out.push((music_len >> 16) as u8);
        out.push((music_len >> 24) as u8);

        // Music data
        for cmd in &self.commands {
            out.extend_from_slice(&cmd.data[..cmd.size]);
        }

        // GD3 tags
        if let Some(gd3) = &self.gd3 {
            out.extend_from_slice(&gd3.as_byte_array());
        }

        out
    }
}
