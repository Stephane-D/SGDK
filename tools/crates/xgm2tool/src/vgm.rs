use std::collections::{HashMap, HashSet};

use crate::command::{self, CommandTrait};
use crate::gd3::GD3;
use crate::psg_state::PSGState;
use crate::sample_bank::{InternalSample, SampleBank};
use crate::util;
use crate::vgm_command::{self, VGMCommand};
use crate::xgm::XGM;
use crate::xgm_fm_command::XGMFMCommand;
use crate::xgm_psg_command::XGMPSGCommand;
use crate::xgm_sample;
use crate::ym2612_state::YM2612State;
use anyhow::Result;

pub const SAMPLE_END_DELAY: i32 = 400;
pub const SAMPLE_MIN_SIZE: i32 = 100;
pub const SAMPLE_MIN_DYNAMIC: i32 = 16;
pub const SAMPLE_ALLOWED_MARGE: i32 = 64;
pub const SAMPLE_MIN_MEAN_DELTA: f64 = 1.0;

/// Sega Megadrive VGM file decoder / encoder
pub struct VGM {
    pub data: Option<Vec<u8>>,
    pub sample_banks: Vec<SampleBank>,
    pub commands: Vec<VGMCommand>,
    pub version: i32,
    pub offset_start: usize,
    pub offset_end: usize,
    pub len_in_sample: i32,
    pub loop_start: i32,
    pub loop_len_in_sample: i32,
    pub rate: i32,
    pub gd3: Option<GD3>,
}

/// Used for sorting mixed FM/PSG commands from XGM
enum MixedCmd<'a> {
    FM(&'a XGMFMCommand),
    PSG(&'a XGMPSGCommand),
}

impl<'a> MixedCmd<'a> {
    fn time(&self) -> i32 {
        match self {
            MixedCmd::FM(c) => c.time,
            MixedCmd::PSG(c) => c.time,
        }
    }
}

impl VGM {
    /// Parse VGM from raw binary data
    pub fn from_data(data: &[u8], convert: bool) -> Result<Self> {
        let magic = std::str::from_utf8(&data[0..4]).unwrap_or("");
        if !magic.eq_ignore_ascii_case("VGM ") {
            anyhow::bail!("File format not recognized!");
        }

        let version = (data[8] & 0xFF) as i32;
        if version < 0x50 {
            println!(
                "Warning: VGM version 1.{:02X} detected: 1.5 required for PCM data!",
                version
            );
        }

        if !crate::is_silent() {
            if convert {
                println!("Optimizing VGM...");
            } else {
                println!("Parsing VGM file...");
            }
        }

        // start offset
        let offset_start = if version >= 0x50 {
            util::get_i32(data, 0x34) as usize + 0x34
        } else {
            0x40
        };
        // end offset
        let offset_end = util::get_i32(data, 0x04) as usize + 0x04;
        // track len
        let len_in_sample = util::get_i32(data, 0x18);
        // loop start offset
        let mut loop_start = util::get_i32(data, 0x1C);
        if loop_start != 0 {
            loop_start += 0x1C;
        }
        // loop len
        let loop_len_in_sample = util::get_i32(data, 0x20);

        // 50 or 60 Hz
        let rate = if version >= 0x01 {
            let r = util::get_i32(data, 0x24);
            if r != 50 { 60 } else { 50 }
        } else {
            60
        };

        // GD3 tags
        let gd3 = {
            let addr = util::get_i32(data, 0x14);
            if addr != 0 {
                let abs_addr = (addr + 0x14) as usize;
                Some(GD3::from_data(data, abs_addr))
            } else {
                None
            }
        };

        if !crate::is_silent() {
            println!(
                "VGM length: {} ({} seconds)",
                len_in_sample,
                len_in_sample / 44100
            );
        }

        if crate::is_verbose() {
            println!(
                "VGM data start: {:06X}   end: {:06X}",
                offset_start, offset_end
            );
            println!(
                "Loop start offset: {:06X}   length: {} ({} seconds)",
                loop_start, loop_len_in_sample,
                loop_len_in_sample / 44100
            );
        }

        let mut vgm = VGM {
            data: Some(data.to_vec()),
            sample_banks: Vec::new(),
            commands: Vec::new(),
            version,
            offset_start,
            offset_end,
            len_in_sample,
            loop_start,
            loop_len_in_sample,
            rate,
            gd3,
        };

        // build command list
        vgm.parse();
        vgm.update_times();
        vgm.update_offsets();

        // build samples
        vgm.build_samples(convert);
        vgm.update_times();
        vgm.update_offsets();

        if convert {
            vgm.convert_waits();
            vgm.clean_commands();
            vgm.clean_samples();

            vgm.update_times();
            vgm.update_offsets();

            vgm.fix_key_commands();
            vgm.remove_dummy_stream_stop_commands();
            vgm.pack_wait();

            vgm.update_times();
            vgm.update_offsets();
        }

        if !crate::is_silent() {
            println!(
                "Computed VGM duration: {} samples ({} seconds)",
                vgm.get_total_time(),
                vgm.get_total_time() / 44100
            );
        }
        if crate::is_verbose() {
            println!("VGM sample number: {}", vgm.get_sample_number());
            println!("Sample data size: {}", vgm.get_sample_data_size());
            println!("Sample total len: {}", vgm.get_sample_total_len());
        }

        Ok(vgm)
    }

    /// Create VGM from another VGM (re-parse with optional optimization)
    pub fn from_vgm(vgm: &VGM, convert: bool) -> Result<Self> {
        if let Some(ref data) = vgm.data {
            Self::from_data(data, convert)
        } else {
            anyhow::bail!("Source VGM has no raw data")
        }
    }

    /// Convert XGM to VGM format
    pub fn from_xgm(xgm: &XGM) -> Result<Self> {
        let mut sample_addr: HashMap<i32, i32> = HashMap::new();
        let mut ym_state = [[0u8; 0x100]; 2];
        // 0 = env, 1 = freq
        let mut psg_state = [[0i16; 4]; 2];

        if !crate::is_silent() {
            println!("Converting XGM to VGM...");
        }

        // build mixed commands sorted by time
        let mut mixed_commands: Vec<MixedCmd> = Vec::new();
        for c in &xgm.fm_commands {
            mixed_commands.push(MixedCmd::FM(c));
        }
        for c in &xgm.psg_commands {
            mixed_commands.push(MixedCmd::PSG(c));
        }
        mixed_commands.sort_by_key(|c| c.time());

        let rate = if xgm.pal { 50 } else { 60 };
        let gd3 = if let Some(ref g) = xgm.gd3 {
            Some(g.clone())
        } else if let Some(ref x) = xgm.xd3 {
            Some(GD3::from_xd3(x))
        } else {
            None
        };

        let mut commands: Vec<VGMCommand> = Vec::new();
        let mut sample_banks: Vec<SampleBank> = Vec::new();

        // build sample data block / stream declaration commands
        if !xgm.samples.is_empty() {
            let pcm_data_size = xgm.get_pcm_data_size();

            // build data block command
            let mut com_data = vec![0u8; pcm_data_size + 7];
            com_data[0] = 0x67;
            com_data[1] = 0x66;
            com_data[2] = 0x00;
            util::set_i32(&mut com_data, 3, pcm_data_size as i32);

            // copy sample data and build address map
            let mut off = 0usize;
            for sample in &xgm.samples {
                let len = sample.get_length();
                com_data[7 + off..7 + off + len].copy_from_slice(&sample.data[..len]);
                sample_addr.insert(sample.id, off as i32);
                off += len;
            }

            // create data block command
            let command = VGMCommand::new(com_data);
            commands.push(command.clone());

            // add single stream control / data (id = 0)
            commands.push(VGMCommand::new(vec![
                vgm_command::STREAM_CONTROL, 0x00, 0x02, 0x00, 0x2A,
            ]));
            commands.push(VGMCommand::new(vec![
                vgm_command::STREAM_DATA, 0x00, 0x00, 0x01, 0x00,
            ]));

            // add data block
            let mut bank = SampleBank::from_command(&command);
            // then add samples
            for sample in &xgm.samples {
                let addr = *sample_addr.get(&sample.id).unwrap_or(&0);
                bank.add_sample(addr, sample.get_length() as i32, xgm_sample::XGM_FULL_RATE);
            }
            sample_banks.push(bank);
        }

        let mut time = 0i32;
        let mut loop_offset: i32 = -1;

        for cmd in &mixed_commands {
            match cmd {
                MixedCmd::FM(fm_command) => {
                    // insert wait commands to reach the correct time
                    while time < fm_command.time {
                        if rate == 50 {
                            commands.push(VGMCommand::new(vec![0x63]));
                            time += 882;
                        } else {
                            commands.push(VGMCommand::new(vec![0x62]));
                            time += 735;
                        }
                    }

                    let port = fm_command.get_ym_port();
                    let mut ch = fm_command.get_ym_channel();

                    match fm_command.get_type() {
                        XGMFMCommand::LOOP => {
                            loop_offset = fm_command.get_loop_addr();
                            if loop_offset == 0xFFFFFF {
                                loop_offset = -1;
                            }
                        }

                        XGMFMCommand::PCM => {
                            let id = fm_command.get_pcm_id();
                            if id == 0 {
                                // stop command
                                commands.push(VGMCommand::new(vec![
                                    vgm_command::STREAM_STOP, 0x00,
                                ]));
                            } else {
                                let sample = xgm.get_sample(id);
                                if let Some(sample) = sample {
                                    let addr = *sample_addr.get(&id).unwrap_or(&0);
                                    let len = sample.get_length() as i32;
                                    let r = if fm_command.get_pcm_half_rate() {
                                        xgm_sample::XGM_HALF_RATE
                                    } else {
                                        xgm_sample::XGM_FULL_RATE
                                    };

                                    commands.push(VGMCommand::new(vec![
                                        vgm_command::STREAM_FREQUENCY,
                                        0x00,
                                        (r & 0xFF) as u8,
                                        ((r >> 8) & 0xFF) as u8,
                                        0x00,
                                        0x00,
                                    ]));
                                    commands.push(VGMCommand::new(vec![
                                        vgm_command::STREAM_START_LONG,
                                        0x00,
                                        (addr & 0xFF) as u8,
                                        ((addr >> 8) & 0xFF) as u8,
                                        ((addr >> 16) & 0xFF) as u8,
                                        0x00,
                                        0x01,
                                        (len & 0xFF) as u8,
                                        ((len >> 8) & 0xFF) as u8,
                                        ((len >> 16) & 0xFF) as u8,
                                        0x00,
                                    ]));
                                }
                            }
                        }

                        XGMFMCommand::FM_LOAD_INST => {
                            let mut d = 1usize;
                            // slot writes
                            for r in 0..7 {
                                for s in 0..4 {
                                    let reg = 0x30 + (r << 4) + (s << 2) + ch;
                                    let value = fm_command.data[d];
                                    d += 1;
                                    ym_state[port as usize][reg as usize] = value;
                                    commands.push(VGMCommand::new(vec![
                                        vgm_command::WRITE_YM2612_PORT0 + port as u8,
                                        reg as u8,
                                        value,
                                    ]));
                                }
                            }
                            // ch writes
                            let reg = 0xB0 + ch;
                            let value = fm_command.data[d];
                            d += 1;
                            ym_state[port as usize][reg as usize] = value;
                            commands.push(VGMCommand::new(vec![
                                vgm_command::WRITE_YM2612_PORT0 + port as u8,
                                reg as u8,
                                value,
                            ]));
                            let reg = 0xB4 + ch;
                            let value = fm_command.data[d];
                            ym_state[port as usize][(0xB4 + ch) as usize] = value;
                            commands.push(VGMCommand::new(vec![
                                vgm_command::WRITE_YM2612_PORT0 + port as u8,
                                reg as u8,
                                value,
                            ]));
                        }

                        XGMFMCommand::FM_WRITE => {
                            let comsize = fm_command.get_ym_num_write();
                            for j in 0..comsize {
                                let reg = util::get_u8(&fm_command.data, (j * 2 + 1) as usize) as i32;
                                let value = fm_command.data[(j * 2 + 2) as usize];
                                ym_state[port as usize][reg as usize] = value;
                                commands.push(VGMCommand::new(vec![
                                    vgm_command::WRITE_YM2612_PORT0 + port as u8,
                                    reg as u8,
                                    value,
                                ]));
                            }
                        }

                        XGMFMCommand::FM0_PAN | XGMFMCommand::FM1_PAN => {
                            let reg = 0xB4 + ch;
                            let value = (ym_state[port as usize][reg as usize] & 0x3F)
                                | ((fm_command.data[0] << 4) & 0xC0);
                            ym_state[port as usize][reg as usize] = value;
                            commands.push(VGMCommand::new(vec![
                                vgm_command::WRITE_YM2612_PORT0 + port as u8,
                                reg as u8,
                                value,
                            ]));
                        }

                        XGMFMCommand::FM_FREQ | XGMFMCommand::FM_FREQ_WAIT => {
                            // pre-key off ?
                            if (fm_command.data[1] & 0x40) != 0 {
                                commands.push(VGMCommand::new(vec![
                                    vgm_command::WRITE_YM2612_PORT0,
                                    0x28,
                                    (0x00 + (port << 2) + ch) as u8,
                                ]));
                            }

                            let reg = if fm_command.is_ym_freq_special_write() {
                                0xA8
                            } else {
                                0xA0
                            };
                            if fm_command.is_ym_freq_special_write() {
                                ch = fm_command.get_ym_slot() - 1;
                            }
                            let lvalue = fm_command.get_ym_freq_value();
                            ym_state[port as usize][(reg + ch + 4) as usize] =
                                ((lvalue >> 8) & 0x3F) as u8;
                            ym_state[port as usize][(reg + ch) as usize] =
                                (lvalue & 0xFF) as u8;
                            commands.push(VGMCommand::new(vec![
                                vgm_command::WRITE_YM2612_PORT0 + port as u8,
                                (reg + ch + 4) as u8,
                                ym_state[port as usize][(reg + ch + 4) as usize],
                            ]));
                            commands.push(VGMCommand::new(vec![
                                vgm_command::WRITE_YM2612_PORT0 + port as u8,
                                (reg + ch) as u8,
                                ym_state[port as usize][(reg + ch) as usize],
                            ]));

                            // post-key on ?
                            if (fm_command.data[1] & 0x80) != 0 {
                                commands.push(VGMCommand::new(vec![
                                    vgm_command::WRITE_YM2612_PORT0,
                                    0x28,
                                    (0xF0 + (port << 2) + ch) as u8,
                                ]));
                            }
                        }

                        XGMFMCommand::FM_FREQ_DELTA | XGMFMCommand::FM_FREQ_DELTA_WAIT => {
                            let reg = if fm_command.is_ym_freq_delta_special_write() {
                                0xA8
                            } else {
                                0xA0
                            };
                            if fm_command.is_ym_freq_delta_special_write() {
                                ch = fm_command.get_ym_slot() - 1;
                            }
                            let mut lvalue =
                                ((ym_state[port as usize][(reg + ch + 4) as usize] & 0x3F) as i32)
                                    << 8;
                            lvalue |=
                                (ym_state[port as usize][(reg + ch) as usize] & 0xFF) as i32;
                            lvalue += fm_command.get_ym_freq_delta_value();
                            ym_state[port as usize][(reg + ch + 4) as usize] =
                                ((lvalue >> 8) & 0x3F) as u8;
                            ym_state[port as usize][(reg + ch) as usize] =
                                (lvalue & 0xFF) as u8;
                            commands.push(VGMCommand::new(vec![
                                vgm_command::WRITE_YM2612_PORT0 + port as u8,
                                (reg + ch + 4) as u8,
                                ym_state[port as usize][(reg + ch + 4) as usize],
                            ]));
                            commands.push(VGMCommand::new(vec![
                                vgm_command::WRITE_YM2612_PORT0 + port as u8,
                                (reg + ch) as u8,
                                ym_state[port as usize][(reg + ch) as usize],
                            ]));
                        }

                        XGMFMCommand::FM_TL => {
                            let reg = 0x40 + (fm_command.get_ym_slot() << 2) + ch;
                            ym_state[port as usize][reg as usize] =
                                fm_command.get_ym_tl_value() as u8;
                            commands.push(VGMCommand::new(vec![
                                vgm_command::WRITE_YM2612_PORT0 + port as u8,
                                reg as u8,
                                ym_state[port as usize][reg as usize],
                            ]));
                        }

                        XGMFMCommand::FM_TL_DELTA | XGMFMCommand::FM_TL_DELTA_WAIT => {
                            let reg = 0x40 + (fm_command.get_ym_slot() << 2) + ch;
                            let mut lvalue =
                                ym_state[port as usize][reg as usize] as i32;
                            lvalue += fm_command.get_ym_tl_delta() as i8 as i32;
                            ym_state[port as usize][reg as usize] = lvalue as u8;
                            commands.push(VGMCommand::new(vec![
                                vgm_command::WRITE_YM2612_PORT0 + port as u8,
                                reg as u8,
                                ym_state[port as usize][reg as usize],
                            ]));
                        }

                        XGMFMCommand::FM_KEY => {
                            let key_val = if (fm_command.data[0] & 8) != 0 {
                                0xF0
                            } else {
                                0x00
                            };
                            commands.push(VGMCommand::new(vec![
                                vgm_command::WRITE_YM2612_PORT0,
                                0x28,
                                (key_val + (port << 2) + ch) as u8,
                            ]));
                        }

                        XGMFMCommand::FM_KEY_SEQ => {
                            if (fm_command.data[0] & 8) != 0 {
                                // ON-OFF sequence
                                commands.push(VGMCommand::new(vec![
                                    vgm_command::WRITE_YM2612_PORT0,
                                    0x28,
                                    (0xF0 + (port << 2) + ch) as u8,
                                ]));
                                commands.push(VGMCommand::new(vec![
                                    vgm_command::WRITE_YM2612_PORT0,
                                    0x28,
                                    (0x00 + (port << 2) + ch) as u8,
                                ]));
                            } else {
                                // OFF-ON sequence
                                commands.push(VGMCommand::new(vec![
                                    vgm_command::WRITE_YM2612_PORT0,
                                    0x28,
                                    (0x00 + (port << 2) + ch) as u8,
                                ]));
                                commands.push(VGMCommand::new(vec![
                                    vgm_command::WRITE_YM2612_PORT0,
                                    0x28,
                                    (0xF0 + (port << 2) + ch) as u8,
                                ]));
                            }
                        }

                        XGMFMCommand::FM_KEY_ADV => {
                            commands.push(VGMCommand::new(vec![
                                vgm_command::WRITE_YM2612_PORT0,
                                0x28,
                                fm_command.data[1],
                            ]));
                        }

                        XGMFMCommand::FM_DAC_ON => {
                            ym_state[0][0x2B] = 0x80;
                            commands.push(VGMCommand::new(vec![0x52, 0x2B, 0x80]));
                        }

                        XGMFMCommand::FM_DAC_OFF => {
                            ym_state[0][0x2B] = 0x00;
                            commands.push(VGMCommand::new(vec![0x52, 0x2B, 0x00]));
                        }

                        XGMFMCommand::FM_LFO => {
                            ym_state[0][0x22] = fm_command.data[1];
                            commands.push(VGMCommand::new(vec![
                                vgm_command::WRITE_YM2612_PORT0,
                                0x22,
                                ym_state[0][0x22],
                            ]));
                        }

                        XGMFMCommand::FM_CH3_SPECIAL_ON => {
                            let value = (ym_state[0][0x27] & 0xBF) | 0x40;
                            ym_state[0][0x27] = value;
                            commands.push(VGMCommand::new(vec![0x52, 0x27, value]));
                        }

                        XGMFMCommand::FM_CH3_SPECIAL_OFF => {
                            let value = ym_state[0][0x27] & 0xBF;
                            ym_state[0][0x27] = value;
                            commands.push(VGMCommand::new(vec![0x52, 0x27, value]));
                        }

                        _ => {}
                    }
                }

                MixedCmd::PSG(psg_command) => {
                    // insert wait commands to reach the correct time
                    while time < psg_command.time {
                        if rate == 50 {
                            commands.push(VGMCommand::new(vec![0x63]));
                            time += 882;
                        } else {
                            commands.push(VGMCommand::new(vec![0x62]));
                            time += 735;
                        }
                    }

                    let ch = psg_command.get_channel();

                    match psg_command.get_type() {
                        XGMPSGCommand::ENV0 | XGMPSGCommand::ENV1
                        | XGMPSGCommand::ENV2 | XGMPSGCommand::ENV3 => {
                            psg_state[0][ch as usize] = psg_command.get_env() as i16;
                            let value =
                                (0x90 + (ch << 5)) as u8 | psg_state[0][ch as usize] as u8;
                            commands.push(VGMCommand::new(vec![
                                vgm_command::WRITE_SN76489,
                                value,
                            ]));
                        }

                        XGMPSGCommand::ENV0_DELTA | XGMPSGCommand::ENV1_DELTA
                        | XGMPSGCommand::ENV2_DELTA | XGMPSGCommand::ENV3_DELTA => {
                            psg_state[0][ch as usize] =
                                (psg_command.get_env_delta() + (psg_state[0][ch as usize] & 0xF)
                                    as i32) as i16;
                            let value =
                                (0x90 + (ch << 5)) as u8 | psg_state[0][ch as usize] as u8;
                            commands.push(VGMCommand::new(vec![
                                vgm_command::WRITE_SN76489,
                                value,
                            ]));
                        }

                        XGMPSGCommand::FREQ | XGMPSGCommand::FREQ_WAIT => {
                            let old_high_freq =
                                psg_state[1][ch as usize] as i32 & 0x03F0;
                            let lvalue = psg_command.get_freq();
                            psg_state[1][ch as usize] = lvalue as i16;
                            let value =
                                (0x80 + (ch << 5)) as u8 | (lvalue & 0x0F) as u8;
                            commands.push(VGMCommand::new(vec![
                                vgm_command::WRITE_SN76489,
                                value,
                            ]));
                            // high byte changed and not channel 3
                            if (old_high_freq != (lvalue & 0x3F0)) && (ch < 3) {
                                let value = ((lvalue >> 4) & 0x3F) as u8;
                                commands.push(VGMCommand::new(vec![
                                    vgm_command::WRITE_SN76489,
                                    value,
                                ]));
                            }
                        }

                        XGMPSGCommand::FREQ_LOW => {
                            let lvalue = (psg_state[1][ch as usize] as i32 & 0x03F0)
                                | (psg_command.get_freq_low() & 0xF);
                            psg_state[1][ch as usize] = lvalue as i16;
                            let value =
                                (0x80 + (ch << 5)) as u8 | (lvalue & 0x0F) as u8;
                            commands.push(VGMCommand::new(vec![
                                vgm_command::WRITE_SN76489,
                                value,
                            ]));
                        }

                        XGMPSGCommand::FREQ0_DELTA | XGMPSGCommand::FREQ1_DELTA
                        | XGMPSGCommand::FREQ2_DELTA | XGMPSGCommand::FREQ3_DELTA => {
                            let old_high_freq =
                                psg_state[1][ch as usize] as i32 & 0x03F0;
                            let lvalue = (psg_state[1][ch as usize] as i32 & 0x03FF)
                                + psg_command.get_freq_delta();
                            psg_state[1][ch as usize] = lvalue as i16;
                            let value =
                                (0x80 + (ch << 5)) as u8 | (lvalue & 0x0F) as u8;
                            commands.push(VGMCommand::new(vec![
                                vgm_command::WRITE_SN76489,
                                value,
                            ]));
                            // high byte changed and not channel 3
                            if (old_high_freq != (lvalue & 0x3F0)) && (ch < 3) {
                                let value = ((lvalue >> 4) & 0x3F) as u8;
                                commands.push(VGMCommand::new(vec![
                                    vgm_command::WRITE_SN76489,
                                    value,
                                ]));
                            }
                        }

                        _ => {}
                    }
                }
            }
        }

        let mut vgm = VGM {
            data: None,
            sample_banks,
            commands,
            version: 0x60,
            offset_start: 0,
            offset_end: 0,
            len_in_sample: 0,
            loop_start: 0,
            loop_len_in_sample: 0,
            rate,
            gd3,
        };

        // pack wait and update times/offsets
        vgm.pack_wait();

        // handle loop
        if loop_offset != -1 {
            let cmd = xgm.get_fm_command_at_offset(loop_offset);
            if let Some(cmd) = cmd {
                let idx = command::get_command_index_at_time(&vgm.commands, cmd.time);
                if let Some(idx) = idx {
                    vgm.commands.insert(idx, VGMCommand::new_loop_start());
                }
            }
            vgm.update_times();
        }

        // end marker
        vgm.commands
            .push(VGMCommand::from_command(vgm_command::END));

        vgm.update_offsets();
        vgm.update_times();

        Ok(vgm)
    }

    // --- Core helpers ---

    pub fn update_times(&mut self) {
        let mut time = 0;
        for com in &mut self.commands {
            com.time = time;
            time += com.get_wait_value();
        }
    }

    pub fn update_offsets(&mut self) {
        command::compute_offsets(&mut self.commands, self.offset_start as i32);
    }

    pub fn get_total_time(&self) -> i32 {
        if self.commands.is_empty() {
            return 0;
        }
        self.commands.last().unwrap().time
    }

    pub fn get_time_from(&self, from: &VGMCommand) -> i32 {
        self.get_total_time() - from.time
    }

    // --- Parse ---

    fn parse(&mut self) {
        let data = match &self.data {
            Some(d) => d.clone(),
            None => return,
        };

        let mut time = 0i32;
        let mut loop_time_st: i32 = -1;
        let mut last_psg_low_write: u8 = 0;

        let mut off = self.offset_start;
        while off < self.offset_end {
            // check for loop start
            if loop_time_st == -1 && self.loop_start != 0 && off >= self.loop_start as usize {
                self.commands.push(VGMCommand::new_loop_start());
                loop_time_st = time;
            }

            let mut command = VGMCommand::from_data(&data, off);
            time += command.get_wait_value();
            off += command.size;

            // PSG write tracking
            if command.is_psg_write() {
                if command.is_psg_low_byte_write() {
                    last_psg_low_write = command.get_psg_value();
                } else if (last_psg_low_write & 0x10) == 0x10 {
                    // high byte write for env write → format as low byte write
                    command.data[1] = (last_psg_low_write & 0xF0) | (command.data[1] & 0x0F);
                }
            }

            // stop here
            if command.is_end() {
                break;
            }

            self.commands.push(command);
        }

        // handle loop timing
        if loop_time_st >= 0 && self.loop_len_in_sample != 0 {
            let delta = self.loop_len_in_sample - (time - loop_time_st);
            if delta > (44100 / 100) {
                let wait_cmd = if self.rate == 60 {
                    vgm_command::WAIT_NTSC_FRAME
                } else {
                    vgm_command::WAIT_PAL_FRAME
                };
                self.commands.push(VGMCommand::from_command(wait_cmd));
            }
        }

        // add final end command
        self.commands
            .push(VGMCommand::from_command(vgm_command::END));

        if crate::is_verbose() {
            println!("Number of command: {}", self.commands.len());
        }
    }

    // --- Samples ---

    fn build_samples(&mut self, convert: bool) {
        // build data blocks
        let data_block_indices: Vec<usize> = self
            .commands
            .iter()
            .enumerate()
            .filter(|(_, c)| c.is_data_block())
            .map(|(i, _)| i)
            .collect();

        for idx in data_block_indices {
            let cmd = self.commands[idx].clone();
            self.add_data_block(&cmd);
        }

        // clean seek
        self.clean_seek_commands();

        // extract samples from seek commands
        let mut ind = 0;
        while ind < self.commands.len() {
            if self.commands[ind].is_seek() {
                ind = self.extract_sample_from_seek(ind, convert);
            } else {
                ind += 1;
            }
        }

        // set bank id and frequency to -1 by default
        let mut sample_id_banks = [-1i32; 0x100];
        let mut sample_id_frequencies = [0i32; 0x100];

        // first pass to extract sample info from stream commands
        ind = 0;
        while ind < self.commands.len() {
            let command = &self.commands[ind];

            if command.is_stream_data() {
                sample_id_banks[command.get_stream_id() as usize] = command.get_stream_bank_id();
            } else if command.is_stream_frequency() {
                sample_id_frequencies[command.get_stream_id() as usize] =
                    command.get_stream_frequency();
            } else if command.is_stream_start() {
                let bank_id = sample_id_banks[command.get_stream_id() as usize];
                let bank = self.get_data_bank(bank_id);

                if let Some(bank) = bank {
                    let sample_id = command.get_stream_block_id();
                    let sample = bank.get_sample_by_id(sample_id);

                    if let Some(sample) = sample {
                        let freq = sample_id_frequencies[command.get_stream_id() as usize];
                        let sample_len = sample.len;
                        let bank_id_val = bank.id;

                        // clone needed data before mutable borrow
                        let sample_clone = sample.clone();

                        // adjust frequency
                        if let Some(bank) = self.get_data_bank_mut(bank_id) {
                            if let Some(s) = bank.get_sample_by_id_mut(sample_id) {
                                s.set_rate(freq);
                            }
                        }

                        // convert to long command
                        if convert {
                            let new_cmd =
                                sample_clone.get_start_long_command_with_len(bank_id_val, sample_len);
                            self.commands[ind] = new_cmd;
                        }
                    } else if !crate::is_silent() {
                        println!("Sample id {:02X} not found!", sample_id);
                    }
                } else if !crate::is_silent() {
                    println!("Sample bank {:02X} not found!", bank_id);
                }
            } else if command.is_stream_start_long() {
                let bank_id = sample_id_banks[command.get_stream_id() as usize];
                let sample_address = command.get_stream_sample_address();
                let sample_len = command.get_stream_sample_size();
                let freq = sample_id_frequencies[command.get_stream_id() as usize];

                if let Some(bank) = self.get_data_bank_mut(bank_id) {
                    bank.add_sample(sample_address, sample_len, freq);
                }
            }

            ind += 1;
        }

        if convert {
            self.remove_seek_and_play_pcm_commands();
            self.update_sample_data_blocks();
        }
    }

    fn extract_sample_from_seek(&mut self, index: usize, convert: bool) -> usize {
        let mut sample_addr = self.commands[index].get_seek_address();

        let bank_len = if !self.sample_banks.is_empty() {
            self.sample_banks.last().unwrap().get_length() as i32
        } else {
            0
        };
        let has_bank = !self.sample_banks.is_empty();

        let mut len: i32 = 0;
        let mut wait: i32 = -1;
        let mut delta: i32 = 0;
        let mut delta_mean: f64 = 0.0;
        let mut end_play_wait: i32 = 0;

        let mut sample_data: i32 = 128;
        let mut sample_min_data: i32 = 128;
        let mut sample_max_data: i32 = 128;
        let mut sample_mean_delta: f64 = 0.0;

        let mut start_play_ind: i32 = -1;
        let mut end_play_ind: i32 = -1;

        let mut ind = index + 1;
        while ind < self.commands.len() {
            let command = &self.commands[ind];

            // sample done
            if command.is_data_block() || command.is_end() {
                break;
            }
            if command.is_seek() {
                let seek_addr = command.get_seek_address();
                let cur_addr = sample_addr + len;
                if (cur_addr + SAMPLE_ALLOWED_MARGE) < seek_addr
                    || (cur_addr - SAMPLE_ALLOWED_MARGE) > seek_addr
                {
                    break;
                } else if crate::is_verbose() {
                    println!(
                        "Seek command found with small offset change ({}) --> considering continue play",
                        cur_addr - seek_addr
                    );
                }
            }

            // playing?
            if wait != -1 {
                delta = wait - end_play_wait;
                if delta < 20 {
                    if delta_mean == 0.0 {
                        delta_mean = delta as f64;
                    } else {
                        delta_mean = (delta as f64 * 0.1) + (delta_mean * 0.9);
                    }
                }

                // sample ended
                if delta > SAMPLE_END_DELAY {
                    if len > 0 && end_play_wait > 0 && start_play_ind != end_play_ind {
                        self.try_add_extracted_sample(
                            sample_addr,
                            len,
                            end_play_wait,
                            sample_min_data,
                            sample_max_data,
                            sample_mean_delta,
                            start_play_ind as usize,
                            end_play_ind as usize,
                            convert,
                        );
                    }

                    // reset
                    sample_addr += len;
                    len = 0;
                    wait = -1;
                    delta = 0;
                    delta_mean = 0.0;
                    end_play_wait = 0;
                    sample_data = 128;
                    sample_min_data = 128;
                    sample_max_data = 128;
                    sample_mean_delta = 0.0;
                    start_play_ind = -1;
                    end_play_ind = -1;
                }
            }

            // compute sample len
            let command = &self.commands[ind];
            if command.is_pcm() {
                if wait == -1 {
                    wait = 0;
                    start_play_ind = ind as i32;
                }

                // sample rate fix
                if crate::sample_rate_fix() {
                    if delta_mean != 0.0 {
                        let mean = delta_mean.round() as i32;
                        if delta < (mean - 2) {
                            wait += mean - delta;
                        } else if delta > (mean + 2) {
                            wait -= delta - mean;
                        }
                    }
                }

                end_play_wait = wait;
                end_play_ind = ind as i32;

                // get current sample value for stats
                if has_bank {
                    let addr_offset = (sample_addr + len) as usize;
                    if addr_offset < bank_len as usize {
                        let bank_data = &self.sample_banks.last().unwrap().data;
                        let d = bank_data[addr_offset] as i32;
                        sample_mean_delta += (d - sample_data).abs() as f64;
                        if sample_min_data > d {
                            sample_min_data = d;
                        }
                        if sample_max_data < d {
                            sample_max_data = d;
                        }
                        sample_data = d;
                    }
                }

                wait += command.get_wait_value();
                len += 1;
            } else if wait != -1 {
                wait += command.get_wait_value();
            }

            ind += 1;
        }

        // found a sample at end
        if len > 0 && end_play_wait > 0 && start_play_ind != end_play_ind {
            self.try_add_extracted_sample(
                sample_addr,
                len,
                end_play_wait,
                sample_min_data,
                sample_max_data,
                sample_mean_delta,
                start_play_ind as usize,
                end_play_ind as usize,
                convert,
            );
        }

        ind
    }

    /// Helper for extract_sample_from_seek: validates sample quality and adds it
    fn try_add_extracted_sample(
        &mut self,
        sample_addr: i32,
        len: i32,
        end_play_wait: i32,
        sample_min_data: i32,
        sample_max_data: i32,
        sample_mean_delta: f64,
        start_play_ind: usize,
        end_play_ind: usize,
        convert: bool,
    ) {
        // ignore too short sample
        if len < SAMPLE_MIN_SIZE && crate::sample_ignore() {
            if crate::is_verbose() {
                println!(
                    "Sample at {:06X} is too small ({}) --> ignored",
                    sample_addr, len
                );
            }
            return;
        }
        // ignore sample with too small dynamic
        if (sample_max_data - sample_min_data) < SAMPLE_MIN_DYNAMIC && crate::sample_ignore() {
            if crate::is_verbose() {
                println!(
                    "Sample at {:06X} has a too quiet global dynamic ({}) --> ignored",
                    sample_addr,
                    sample_max_data - sample_min_data
                );
            }
            return;
        }
        // ignore sample too quiet
        if (sample_mean_delta / len as f64) < SAMPLE_MIN_MEAN_DELTA && crate::sample_ignore() {
            if crate::is_verbose() {
                println!(
                    "Sample at {:06X} is too quiet (mean delta value = {:.6}) --> ignored",
                    sample_addr,
                    sample_mean_delta / len as f64
                );
            }
            return;
        }

        if self.sample_banks.is_empty() {
            return;
        }

        let r = ((44100.0 * len as f64) / end_play_wait as f64).round() as i32;
        let bank_idx = self.sample_banks.len() - 1;
        let bank_id = self.sample_banks[bank_idx].id;
        let sample_idx = self.sample_banks[bank_idx].add_sample(sample_addr, len, r);

        if convert {
            let sample = &self.sample_banks[bank_idx].samples[sample_idx];
            let rate_cmd = sample.get_set_rate_command(bank_id, sample.rate);
            let start_cmd = sample.get_start_long_command_with_len(bank_id, len);
            let stop_cmd = sample.get_stop_command(bank_id);

            self.commands.insert(start_play_ind, rate_cmd);
            self.commands.insert(start_play_ind + 1, start_cmd);
            // +2 offset from the two insertions above
            let adjusted_end = end_play_ind + 2;
            if adjusted_end <= self.commands.len() {
                self.commands.insert(adjusted_end, stop_cmd);
            }
        }
    }

    fn get_data_bank(&self, id: i32) -> Option<&SampleBank> {
        self.sample_banks.iter().find(|b| b.id == id)
    }

    fn get_data_bank_mut(&mut self, id: i32) -> Option<&mut SampleBank> {
        self.sample_banks.iter_mut().find(|b| b.id == id)
    }

    fn add_data_block(&mut self, command: &VGMCommand) -> usize {
        let data_bank_id = command.get_data_bank_id() as i32;
        let existing = self.sample_banks.iter().position(|b| b.id == data_bank_id);

        if let Some(idx) = existing {
            self.sample_banks[idx].add_block(command);
            idx
        } else {
            if !crate::is_silent() && !self.sample_banks.is_empty() {
                println!("Warning: VGM file contains more than 1 data block bank (may not work correctly)");
            }
            let bank = SampleBank::from_command(command);
            self.sample_banks.push(bank);
            self.sample_banks.len() - 1
        }
    }

    fn clean_seek_commands(&mut self) {
        let mut removed: HashSet<usize> = HashSet::new();
        let mut sample_played = false;

        for ind in (0..self.commands.len()).rev() {
            let command = &self.commands[ind];
            if command.is_seek() {
                if !sample_played {
                    if crate::is_verbose() {
                        println!(
                            "Useless seek command found at {:06X}",
                            command.origin_offset()
                        );
                    }
                    removed.insert(ind);
                }
                sample_played = false;
            } else if command.is_pcm() {
                sample_played = true;
            }
        }

        if !removed.is_empty() {
            let new_comms: Vec<VGMCommand> = self
                .commands
                .iter()
                .enumerate()
                .filter(|(i, _)| !removed.contains(i))
                .map(|(_, c)| c.clone())
                .collect();
            self.commands = new_comms;
        }

        self.update_times();
        self.update_offsets();
    }

    fn remove_dummy_stream_stop_commands(&mut self) {
        let mut ind = self.commands.len().saturating_sub(2);
        loop {
            if self.commands[ind].is_stream_stop()
                && ind + 1 < self.commands.len()
                && self.commands[ind + 1].is_stream_frequency()
            {
                self.commands.remove(ind);
            }
            if ind == 0 {
                break;
            }
            ind -= 1;
        }
    }

    fn remove_seek_and_play_pcm_commands(&mut self) {
        let mut new_comms: Vec<VGMCommand> = Vec::with_capacity(self.commands.len());

        for command in &self.commands {
            if command.is_seek() {
                continue;
            } else if command.is_pcm() {
                let wait = command.get_wait_value();
                if wait == 0 {
                    continue;
                }
                new_comms.push(VGMCommand::from_command(0x70 + (wait - 1) as u8));
            } else {
                new_comms.push(command.clone());
            }
        }

        self.commands = new_comms;

        if !crate::is_silent() {
            println!(
                "Number of command after PCM commands remove: {}",
                self.commands.len()
            );
        }
    }

    fn clean_key_commands(frame_commands: &mut Vec<VGMCommand>) {
        let mut to_remove: HashSet<usize> = HashSet::new();
        let mut has_key_on = [false; 6];
        let mut has_key_off = [false; 6];

        // start from end of frame
        for c in (0..frame_commands.len()).rev() {
            let com = &frame_commands[c];
            if com.is_ym2612_key_on_write() {
                let ch = com.get_ym2612_channel();
                if ch == -1 || has_key_on[ch as usize] {
                    to_remove.insert(c);
                } else {
                    has_key_on[ch as usize] = true;
                }
            } else if com.is_ym2612_key_off_write() {
                let ch = com.get_ym2612_channel();
                if ch == -1 || has_key_off[ch as usize] {
                    to_remove.insert(c);
                } else if has_key_on[ch as usize] {
                    has_key_off[ch as usize] = true;
                }
            }
        }

        let temp: Vec<VGMCommand> = frame_commands
            .iter()
            .enumerate()
            .filter(|(i, _)| !to_remove.contains(i))
            .map(|(_, c)| c.clone())
            .collect();

        // additional pass: track key state in normal order
        let mut key_state: [Option<bool>; 6] = [None; 6];
        let mut to_remove2: HashSet<usize> = HashSet::new();

        for (idx, com) in temp.iter().enumerate() {
            if com.is_ym2612_key_on_write() {
                let ch = com.get_ym2612_channel();
                if ch != -1 {
                    if key_state[ch as usize] == Some(true) {
                        to_remove2.insert(idx);
                    }
                    key_state[ch as usize] = Some(true);
                }
            } else if com.is_ym2612_key_off_write() {
                let ch = com.get_ym2612_channel();
                if ch != -1 {
                    if key_state[ch as usize] == Some(false) {
                        to_remove2.insert(idx);
                    }
                    key_state[ch as usize] = Some(false);
                }
            }
        }

        *frame_commands = temp
            .into_iter()
            .enumerate()
            .filter(|(i, _)| !to_remove2.contains(i))
            .map(|(_, c)| c)
            .collect();
    }

    pub fn clean_commands(&mut self) {
        let mut frame_commands: Vec<VGMCommand> = Vec::new();
        let mut new_commands: Vec<VGMCommand> = Vec::new();
        let mut optimized_commands: Vec<VGMCommand> = Vec::new();
        let mut key_on_off_commands: Vec<VGMCommand> = Vec::new();
        let mut ym_commands: Vec<VGMCommand> = Vec::new();
        let mut last_commands: Vec<VGMCommand> = Vec::new();

        let mut ym_loop_state: Option<YM2612State> = None;
        let mut psg_loop_state: Option<PSGState> = None;
        let mut ym_old_state = YM2612State::new();
        let mut ym_state: YM2612State;
        let mut psg_old_state = PSGState::new();
        let mut psg_state: PSGState;

        let mut start_ind: usize = 0;

        loop {
            let mut end_ind = start_ind;
            frame_commands.clear();

            // build frame commands
            loop {
                let command = self.commands[end_ind].clone();
                let is_wait = command.is_wait();
                let is_end = command.is_end();
                frame_commands.push(command);
                end_ind += 1;
                if end_ind >= self.commands.len() || is_wait || is_end {
                    break;
                }
            }

            // clean duplicated key commands
            Self::clean_key_commands(&mut frame_commands);

            psg_state = PSGState::from_state(&psg_old_state);
            ym_state = YM2612State::from_state(&ym_old_state);

            // first frame: reset special FM feature state
            if start_ind == 0 {
                ym_state.set(0, 0x22, 0); // LFO
                ym_state.set(0, 0x27, 0); // FM2 SPE mode / CSM
                ym_state.set(0, 0xB4, 0xC0); // default panning
                ym_state.set(0, 0xB5, 0xC0);
                ym_state.set(0, 0xB6, 0xC0);
                ym_state.set(1, 0xB4, 0xC0);
                ym_state.set(1, 0xB5, 0xC0);
                ym_state.set(1, 0xB6, 0xC0);
            }

            optimized_commands.clear();
            key_on_off_commands.clear();
            ym_commands.clear();
            last_commands.clear();

            let mut has_key_com = false;
            let mut last_command_is_end = false;

            for command in &frame_commands {
                if command.is_data_block() || command.is_stream() || command.is_loop_start {
                    optimized_commands.push(command.clone());

                    if command.is_loop_start {
                        ym_loop_state = Some(YM2612State::from_state(&ym_old_state));
                        psg_loop_state = Some(PSGState::from_state(&psg_old_state));
                    }
                } else if command.is_psg_write() {
                    psg_state.write(command.get_psg_value() as i32);
                } else if command.is_ym2612_write() {
                    if command.is_ym2612_key_write() {
                        let port = command.get_ym2612_port() as usize;
                        let reg = command.get_ym2612_register() as usize;
                        let val = command.get_ym2612_value();
                        if ym_state.set(port, reg, val) {
                            key_on_off_commands.push(command.clone());
                            has_key_com = true;
                        }
                    } else {
                        // flush if needed for accurate key event ordering
                        if has_key_com {
                            ym_commands.extend(ym_old_state.get_delta(&ym_state, true));
                            ym_commands.extend(key_on_off_commands.drain(..));
                            ym_old_state = YM2612State::from_state(&ym_state);
                            has_key_com = false;
                        }

                        let port = command.get_ym2612_port() as usize;
                        let reg = command.get_ym2612_register() as usize;
                        let val = command.get_ym2612_value();
                        ym_state.set(port, reg, val);
                    }
                } else if command.is_wait() || command.is_seek() {
                    last_commands.push(command.clone());
                } else if command.is_end() {
                    last_command_is_end = true;
                } else if crate::is_verbose() {
                    println!("Command ignored: {:02X}", command.get_command());
                }
            }

            // check single stream per frame
            let mut has_stream_start = false;
            let mut has_stream_rate = false;
            let mut remove_indices: Vec<usize> = Vec::new();

            for idx in (0..optimized_commands.len()).rev() {
                let command = &optimized_commands[idx];
                if command.is_stream_start_long() {
                    if has_stream_start {
                        if !crate::is_silent() {
                            println!("Warning: more than 1 PCM command in a single frame!");
                            println!(
                                "Command stream start {} removed at {}",
                                command,
                                command.time as f64 / 44100.0
                            );
                        }
                        remove_indices.push(idx);
                    }
                    has_stream_start = true;
                } else if command.is_stream_frequency() {
                    if has_stream_rate {
                        if crate::is_verbose() {
                            println!(
                                "Command stream rate {} removed at {}",
                                command,
                                command.time as f64 / 44100.0
                            );
                        }
                        remove_indices.push(idx);
                    }
                    has_stream_rate = true;
                }
            }

            for idx in &remove_indices {
                optimized_commands.remove(*idx);
            }

            // end of track: use loop point state
            if end_ind >= self.commands.len() || last_command_is_end {
                if let Some(ref state) = ym_loop_state {
                    ym_state = YM2612State::from_state(state);
                }
                if let Some(ref state) = psg_loop_state {
                    psg_state = PSGState::from_state(state);
                }
            }

            // merge commands
            optimized_commands.extend(ym_commands.drain(..));
            optimized_commands.extend(ym_old_state.get_delta(&ym_state, true));
            optimized_commands.extend(key_on_off_commands.drain(..));
            optimized_commands.extend(psg_old_state.get_delta(&psg_state));
            optimized_commands.extend(last_commands.drain(..));

            new_commands.extend(optimized_commands.drain(..));

            if end_ind >= self.commands.len() || last_command_is_end {
                break;
            }

            ym_old_state = YM2612State::from_state(&ym_state);
            psg_old_state = PSGState::from_state(&psg_state);

            start_ind = end_ind;
        }

        // end command
        new_commands.push(VGMCommand::from_command(vgm_command::END));

        self.commands = new_commands;
        self.update_times();
        self.update_offsets();

        if !crate::is_silent() {
            println!("Music data size: {}", self.get_music_data_size());
            println!("Len (samples): {}", self.get_total_time());
            println!(
                "Number of command after commands clean: {}",
                self.commands.len()
            );
        }
    }

    pub fn clean_samples(&mut self) {
        if crate::is_verbose() {
            println!("Clean samples");
        }

        // detect unused samples
        for b in (0..self.sample_banks.len()).rev() {
            let bank_id = self.sample_banks[b].id;

            let mut s_idx = self.sample_banks[b].samples.len();
            while s_idx > 0 {
                s_idx -= 1;
                let sample_id = self.sample_banks[b].samples[s_idx].id;
                let sample_len = self.sample_banks[b].samples[s_idx].len;
                let sample_addr = self.sample_banks[b].samples[s_idx].addr;
                let min_len = (sample_len - 50).max(0);
                let max_len = sample_len + 50;
                let mut used = false;
                let mut current_bank_id = -1i32;

                for c in 0..self.commands.len().saturating_sub(1) {
                    let command = &self.commands[c];
                    if command.is_stream_data() {
                        current_bank_id = command.get_stream_bank_id();
                    }
                    if bank_id == current_bank_id {
                        if command.is_stream_start() {
                            if sample_id == command.get_stream_block_id() {
                                used = true;
                                break;
                            }
                        } else if command.is_stream_start_long() {
                            let cmd_sample_len = command.get_stream_sample_size();
                            let matches_addr = self.sample_banks[b]
                                .samples[s_idx]
                                .match_address(command.get_stream_sample_address());
                            if matches_addr
                                && cmd_sample_len >= min_len
                                && cmd_sample_len <= max_len
                            {
                                used = true;
                                break;
                            }
                        }
                    }
                }

                if !used {
                    if crate::is_verbose() {
                        println!(
                            "Sample at offset {:06X} (len = {}) is not used --> removed",
                            sample_addr, sample_len
                        );
                    }
                    self.sample_banks[b].samples.remove(s_idx);
                }
            }
        }

        // save old sample addresses (map<comm_index, sample_address>)
        let mut old_sample_addresses: HashMap<usize, i32> = HashMap::new();
        let mut current_bank: Option<&SampleBank> = None;

        for c in 0..self.commands.len().saturating_sub(1) {
            if self.commands[c].is_stream_data() {
                let bid = self.commands[c].get_stream_bank_id();
                current_bank = self.get_data_bank(bid);
            }
            if self.commands[c].is_stream_start_long() {
                if let Some(bank) = current_bank {
                    let addr = self.commands[c].get_stream_sample_address();
                    if let Some(sample) = bank.get_sample_by_address(addr) {
                        old_sample_addresses.insert(c, sample.addr);
                    } else {
                        eprintln!(
                            "Warning: cleanSamples - Cannot find matching sample address 0x{:06X}",
                            addr
                        );
                    }
                }
            }
        }

        // optimize sample data banks
        let mut bank_sample_addr_change: HashMap<i32, HashMap<i32, i32>> = HashMap::new();
        for bank in &mut self.sample_banks {
            bank_sample_addr_change.insert(bank.id, bank.optimize());
        }

        // update samples address in commands
        let mut sample_address_changes: Option<&HashMap<i32, i32>> = None;
        for c in 0..self.commands.len().saturating_sub(1) {
            if self.commands[c].is_stream_data() {
                let bid = self.commands[c].get_stream_bank_id();
                sample_address_changes = bank_sample_addr_change.get(&bid);
            }
            if self.commands[c].is_stream_start_long() {
                if let Some(changes) = sample_address_changes {
                    if let Some(&old_addr) = old_sample_addresses.get(&c) {
                        if let Some(&new_addr) = changes.get(&old_addr) {
                            self.commands[c].set_stream_sample_address(new_addr);
                        } else {
                            eprintln!(
                                "Warning: cleanSamples - Cannot update sample address at offset 0x{:06X}",
                                self.commands[c].origin_offset()
                            );
                        }
                    } else {
                        eprintln!(
                            "Warning: cleanSamples - Cannot update sample address at offset 0x{:06X}",
                            self.commands[c].origin_offset()
                        );
                    }
                } else {
                    eprintln!(
                        "Warning: cleanSamples - Cannot update sample address at offset 0x{:06X}",
                        self.commands[c].origin_offset()
                    );
                }
            }
        }

        // update sample banks declaration
        self.update_sample_data_blocks();

        if crate::is_verbose() {
            println!(
                "Sample num = {} - data size: {}",
                self.get_sample_number(),
                self.get_sample_data_size()
            );
        }
    }

    pub fn update_sample_data_blocks(&mut self) {
        let mut new_comms: Vec<VGMCommand> = Vec::with_capacity(self.commands.len());

        for bank in &self.sample_banks {
            new_comms.push(bank.get_data_block_command());
            new_comms.extend(bank.get_declaration_commands());
        }

        for command in &self.commands {
            if command.is_data_block() || command.is_stream_control() || command.is_stream_data() {
                continue;
            }
            new_comms.push(command.clone());
        }

        self.commands = new_comms;
    }

    pub fn fix_key_commands(&mut self) {
        let mut delayed_commands: Vec<VGMCommand> = Vec::new();
        let max_delta = (44100 / self.rate) / 4;
        let mut key_off_time = [-1i32; 6];
        let mut key_on_time = [-1i32; 6];

        let mut frame = 0;
        let mut ind = 0;
        while ind < self.commands.len() {
            let command = self.commands[ind].clone();

            if command.is_wait() {
                // insert delayed commands after current wait
                if !delayed_commands.is_empty() {
                    let insert_pos = ind + 1;
                    for (i, dc) in delayed_commands.iter().enumerate() {
                        self.commands.insert(insert_pos + i, dc.clone());
                    }
                    ind += delayed_commands.len();
                    delayed_commands.clear();
                }
                // reset key traces
                for i in 0..6 {
                    key_off_time[i] = -1;
                    key_on_time[i] = -1;
                }
                frame += 1;
            } else if command.is_ym2612_key_write() {
                let ch = command.get_ym2612_channel();
                if ch != -1 {
                    let ch = ch as usize;
                    if command.is_ym2612_key_off_write() {
                        key_off_time[ch] = command.time;

                        if key_on_time[ch] != -1 {
                            if command.time != -1
                                && (command.time - key_on_time[ch]) > max_delta
                            {
                                if crate::delay_key_off() {
                                    if !crate::is_silent() {
                                        println!(
                                            "Warning: CH{} delayed key OFF command at frame {}",
                                            ch, frame
                                        );
                                        println!("You can try to use the -dd switch if you experience missing or incorrect FM instrument sound.");
                                    }

                                    let removed_cmd = self.commands.remove(ind);
                                    // add to delayed only if not already present
                                    let has_key_off = delayed_commands.iter().any(|c| {
                                        c.is_ym2612_key_off_write()
                                            && c.get_ym2612_channel() == ch as i32
                                    });
                                    if !has_key_off {
                                        delayed_commands.push(removed_cmd);
                                    }
                                    continue; // don't increment ind
                                } else if !crate::is_silent() {
                                    println!(
                                        "Warning: CH{} key ON/OFF events occured at frame {} and delayed key OFF has been disabled.",
                                        ch, frame
                                    );
                                }
                            }
                        }
                    } else {
                        // key on
                        key_on_time[ch] = command.time;
                    }
                }
            }

            ind += 1;
        }
    }

    // --- Wait conversion ---

    pub fn convert_waits(&mut self) {
        let mut new_commands: Vec<VGMCommand> = Vec::new();
        let sample_per_frame = 44100 / self.rate;
        let sample_per_frame_percent = (sample_per_frame * 15) / 100;
        let sample_per_frame_limit = sample_per_frame - sample_per_frame_percent;
        let com_wait = if self.rate == 60 {
            vgm_command::WAIT_NTSC_FRAME
        } else {
            vgm_command::WAIT_PAL_FRAME
        };

        self.update_times();
        if crate::is_verbose() {
            print!("Original len (samples): {}", self.get_total_time());
        }

        let mut sample_cnt = 0;
        let mut last_wait = 0;
        for command in &self.commands {
            if !command.is_wait() {
                new_commands.push(command.clone());
            } else {
                last_wait = command.get_wait_value();
                sample_cnt += last_wait;
            }

            let threshold = if last_wait > sample_per_frame_percent {
                sample_per_frame_limit
            } else {
                sample_per_frame
            };
            while sample_cnt > threshold {
                new_commands.push(VGMCommand::from_command(com_wait));
                sample_cnt -= sample_per_frame;
            }
        }

        self.commands = new_commands;
        self.update_times();
        self.update_offsets();

        if crate::is_verbose() {
            println!(
                " - new len after wait conversion: {}",
                self.get_total_time()
            );
            println!("Number of command: {}", self.commands.len());
        }
    }

    pub fn pack_wait(&mut self) {
        let mut new_commands: Vec<VGMCommand> = Vec::with_capacity(self.commands.len());

        let mut c = 0;
        while c < self.commands.len() {
            // get next wait command
            while !self.commands[c].is_wait() {
                new_commands.push(self.commands[c].clone());
                c += 1;
                if c >= self.commands.len() {
                    self.commands = new_commands;
                    self.update_times();
                    self.update_offsets();
                    return;
                }
            }

            let mut wait = 0;
            while c < self.commands.len() && self.commands[c].is_wait() {
                wait += self.commands[c].get_wait_value();
                c += 1;
            }

            new_commands.extend(VGMCommand::create_wait_commands(wait));
        }

        self.commands = new_commands;
        self.update_times();
        self.update_offsets();
    }

    pub fn shift_samples(&mut self, sft: i32) {
        if sft == 0 {
            return;
        }

        let sft = sft as usize;
        let mut sample_commands: Vec<Vec<VGMCommand>> = vec![Vec::new(); sft];

        let mut frame_read: usize = 0;
        let mut frame_write: usize = 1;
        let mut index = self.commands.len() as i32 - 1;

        while index >= 0 {
            let idx = index as usize;
            let command = &self.commands[idx];

            if command.is_stream() {
                let cmd = command.clone();
                sample_commands[frame_read].push(cmd);
                self.commands.remove(idx);
            } else if command.is_wait() || command.is_end() {
                frame_read = (frame_read + 1) % sft;
                frame_write = (frame_write + 1) % sft;

                while !sample_commands[frame_write].is_empty() {
                    let cmd = sample_commands[frame_write].pop().unwrap();
                    self.commands.insert(idx, cmd);
                }
            }

            index -= 1;
        }

        // add last remaining samples
        for cmds in &mut sample_commands {
            while !cmds.is_empty() {
                let cmd = cmds.pop().unwrap();
                self.commands.insert(0, cmd);
            }
        }

        self.update_times();
        self.update_offsets();
    }

    // --- Queries ---

    pub fn get_sample(&self, sample_address: i32) -> Option<&InternalSample> {
        for bank in &self.sample_banks {
            if let Some(sample) = bank.get_sample_by_address(sample_address) {
                return Some(sample);
            }
        }
        None
    }

    fn get_sample_data_size(&self) -> usize {
        self.sample_banks.iter().map(|b| b.get_length()).sum()
    }

    fn get_sample_total_len(&self) -> i32 {
        self.sample_banks
            .iter()
            .flat_map(|b| &b.samples)
            .map(|s| s.len)
            .sum()
    }

    fn get_sample_number(&self) -> usize {
        self.sample_banks
            .iter()
            .map(|b| b.samples.len())
            .sum()
    }

    fn get_music_data_size(&self) -> usize {
        self.commands
            .iter()
            .filter(|c| !c.is_data_block())
            .map(|c| c.size)
            .sum()
    }

    // --- Serialization ---

    pub fn as_byte_array(&self) -> Result<Vec<u8>> {
        let mut result: Vec<u8> = Vec::new();

        // 00: VGM magic
        result.extend_from_slice(b"Vgm ");
        // 04: len (reserve 4 bytes)
        result.extend_from_slice(&[0x00; 4]);
        // 08: version 1.60
        result.extend_from_slice(&[0x60, 0x01, 0x00, 0x00]);
        // 0C: SN76489 clock
        result.extend_from_slice(&[0x99, 0x9E, 0x36, 0x00]);
        // 10: YM2413 clock
        result.extend_from_slice(&[0x00; 4]);
        // 14: GD3 offset (reserve)
        result.extend_from_slice(&[0x00; 4]);
        // 18: total samples (reserve)
        result.extend_from_slice(&[0x00; 4]);
        // 1C: loop offset (reserve)
        result.extend_from_slice(&[0x00; 4]);
        // 20: loop duration (reserve)
        result.extend_from_slice(&[0x00; 4]);
        // 24: rate
        result.extend_from_slice(&[0x3C, 0x00, 0x00, 0x00]);
        // 28: SN76489 flags
        result.extend_from_slice(&[0x09, 0x00, 0x10, 0x00]);
        // 2C: YM2612 clock
        result.extend_from_slice(&[0xB5, 0x0A, 0x75, 0x00]);
        // 30: YM2151 clock
        result.extend_from_slice(&[0x00; 4]);
        // 34: VGM data offset
        result.extend_from_slice(&[0x4C, 0x00, 0x00, 0x00]);
        // 38-3C: Sega PCM
        result.extend_from_slice(&[0x00; 8]);
        // 40-80: padding
        for _ in 0x40..0x80 {
            result.push(0x00);
        }

        let mut loop_command: Option<&VGMCommand> = None;
        let mut loop_offset: i32 = 0;

        // write commands (ignore loop marker)
        for command in &self.commands {
            if command.is_loop_start {
                loop_command = Some(command);
                loop_offset = result.len() as i32 - 0x1C;
            } else {
                result.extend_from_slice(&command.data);
            }
        }

        // write GD3 tags if present
        let mut gd3_offset: usize = 0;
        if let Some(ref gd3) = self.gd3 {
            gd3_offset = result.len();
            result.extend_from_slice(&gd3.as_byte_array());
        }

        // fix header fields
        if let Some(loop_cmd) = loop_command {
            util::set_i32(&mut result, 0x1C, loop_offset);
            util::set_i32(&mut result, 0x20, self.get_time_from(loop_cmd));
        }
        if self.gd3.is_some() {
            util::set_i32(&mut result, 0x14, (gd3_offset - 0x14) as i32);
        }
        // file size
        let file_size = (result.len() - 4) as i32;
        util::set_i32(&mut result, 0x04, file_size);
        // len in samples
        util::set_i32(&mut result, 0x18, self.get_total_time() - 1);

        Ok(result)
    }

    /// Get SampleBank by id (mutable)
    fn _get_sample_by_id_in_bank(bank: &SampleBank, id: i32) -> Option<InternalSample> {
        bank.get_sample_by_id(id).cloned()
    }
}
