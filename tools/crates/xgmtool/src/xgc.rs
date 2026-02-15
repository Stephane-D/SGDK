use crate::xgm::XGM;
use crate::xgm_command::XGMCommand;
use crate::xgc_command;
use crate::ym2612::YM2612;
use crate::gd3::XD3;
use crate::{is_silent, is_verbose};

/// Create XGC (compiled XGM) from XGM
pub fn create(xgm: &XGM) -> XGM {
    let mut result = XGM::new();

    if !is_silent() { println!("Converting to XGC..."); }

    result.pal = xgm.pal;
    result.samples = xgm.samples.clone();

    // Extract music
    extract_music(&mut result, xgm);

    // Shift samples for PCM buffer
    let sft = if result.pal { 2 } else { 3 };
    shift_samples(&mut result, sft);

    // GD3/XD3 tags
    if xgm.gd3.is_some() {
        result.gd3 = xgm.gd3.clone();
        let duration = compute_len_in_frame(&result);
        let loop_duration = if let Some(idx) = result.get_loop_pointed_command_idx() {
            compute_len_in_frame_of(&result.commands[idx..])
        } else { 0 };
        if let Some(gd3) = &xgm.gd3 {
            result.xd3 = Some(XD3::from_gd3(gd3, duration, loop_duration));
        }
    }

    if is_verbose() {
        println!("Sample size: {}", result.get_sample_data_size());
        println!("Music data size: {}", result.get_music_data_size());
        println!("Number of sample: {}", result.samples.len());
    }
    if !is_silent() {
        println!("XGC duration: {} frames ({} seconds)", compute_len_in_frame(&result), compute_len_in_second(&result));
    }

    result
}

fn extract_music(xgc: &mut XGM, xgm: &XGM) {
    let loop_command_idx = xgm.get_loop_pointed_command_idx();
    let mut loop_offset: i32 = -1;
    let mut loop_end = false;

    let mut ym_loop_state: Option<YM2612> = None;
    let mut ym_old_state: Option<YM2612> = None;
    let mut ym_state = YM2612::new();

    // Add 3 dummy frames (reserve for PCM shift)
    let mut xgc_commands: Vec<XGMCommand> = vec![
        xgc_command::create_frame_size_command(0),
        xgc_command::create_frame_size_command(0),
        xgc_command::create_frame_size_command(0),
    ];

    let mut com_idx = 0;
    while com_idx < xgm.commands.len() {
        // Build frame commands
        let mut frame_commands: Vec<&XGMCommand> = Vec::new();

        while com_idx < xgm.commands.len() {
            let command = &xgm.commands[com_idx];
            com_idx += 1;

            // Check if this is the loop target
            if let Some(lidx) = loop_command_idx {
                if com_idx - 1 == lidx && loop_offset == -1 {
                    loop_offset = music_data_size_of(&xgc_commands) as i32;
                    ym_loop_state = Some(ym_state.clone());
                }
            }

            if command.is_end() { continue; }
            if command.is_loop() { loop_end = true; continue; }
            if command.is_frame() { break; }
            frame_commands.push(command);
        }

        // Update state
        ym_old_state = Some(ym_state.clone());

        // Create new frame
        let size_cmd = xgc_command::create_frame_size_command(0);
        let size_cmd_idx = xgc_commands.len();
        xgc_commands.push(size_cmd);

        // Group commands
        let mut ym_other_commands: Vec<&XGMCommand> = Vec::new();
        let mut ym_key_commands: Vec<&XGMCommand> = Vec::new();
        let mut ym_commands: Vec<XGMCommand> = Vec::new();
        let mut psg_commands: Vec<&XGMCommand> = Vec::new();
        let mut other_commands: Vec<&XGMCommand> = Vec::new();
        let mut has_key_com = false;

        for cmd in &frame_commands {
            if cmd.is_psg_write() {
                psg_commands.push(cmd);
            } else if cmd.is_ym2612_reg_key_write() {
                ym_key_commands.push(cmd);
                has_key_com = true;
            } else if cmd.is_ym2612_write() {
                if has_key_com {
                    if !ym_other_commands.is_empty() {
                        let as_xgm: Vec<XGMCommand> = ym_other_commands.iter().map(|c| (*c).clone()).collect();
                        ym_commands.extend(xgc_command::convert(&as_xgm));
                        ym_other_commands.clear();
                    }
                    if !ym_key_commands.is_empty() {
                        let as_xgm: Vec<XGMCommand> = ym_key_commands.iter().map(|c| (*c).clone()).collect();
                        ym_commands.extend(xgc_command::convert(&as_xgm));
                        ym_key_commands.clear();
                    }
                    has_key_com = false;
                }

                // Update YM state
                let write_count = cmd.get_ym2612_write_count() as usize;
                for j in 0..write_count {
                    if cmd.is_ym2612_port0_write() {
                        ym_state.set(0, cmd.data[j * 2 + 1] as usize & 0xFF, cmd.data[j * 2 + 2] as i32 & 0xFF);
                    } else {
                        ym_state.set(1, cmd.data[j * 2 + 1] as usize & 0xFF, cmd.data[j * 2 + 2] as i32 & 0xFF);
                    }
                }

                // Remove $2B writes (DAC enable)
                let mut cmd_clone = (*cmd).clone();
                if cmd_clone.remove_ym2612_reg_write(0, 0x2B) {
                    ym_other_commands.push(cmd);
                }
            } else {
                other_commands.push(cmd);
            }
        }

        // Discard multiple PCM commands per channel
        let mut pcm_ch: [bool; 4] = [false; 4];
        other_commands.retain(|cmd| {
            if cmd.is_pcm() {
                let ch = cmd.get_pcm_channel() as usize;
                if pcm_ch[ch] {
                    if !is_silent() {
                        let frame_idx = compute_len_in_frame_of(&xgc_commands);
                        let id = cmd.get_pcm_id();
                        if id != 0 {
                            println!("Warning: multiple PCM command on {} --> play {:02X} removed", frame_idx, id);
                        }
                    }
                    return false;
                }
                pcm_ch[ch] = true;
            }
            true
        });

        // Merge remaining YM commands
        if !ym_other_commands.is_empty() {
            let as_xgm: Vec<XGMCommand> = ym_other_commands.iter().map(|c| (*c).clone()).collect();
            ym_commands.extend(xgc_command::convert(&as_xgm));
        }
        if !ym_key_commands.is_empty() {
            let as_xgm: Vec<XGMCommand> = ym_key_commands.iter().map(|c| (*c).clone()).collect();
            ym_commands.extend(xgc_command::convert(&as_xgm));
        }

        // Build new frame: PSG first, then YM, then other (PCM)
        let mut new_commands: Vec<XGMCommand> = Vec::new();
        if !psg_commands.is_empty() {
            let as_xgm: Vec<XGMCommand> = psg_commands.iter().map(|c| (*c).clone()).collect();
            new_commands.extend(xgc_command::convert(&as_xgm));
        }
        new_commands.extend(ym_commands);
        if !other_commands.is_empty() {
            let as_xgm: Vec<XGMCommand> = other_commands.iter().map(|c| (*c).clone()).collect();
            new_commands.extend(xgc_command::convert(&as_xgm));
        }

        // State change
        if let Some(ref old) = ym_old_state {
            let state_changes = get_state_change(&ym_state, old);
            if !state_changes.is_empty() {
                new_commands.extend(xgc_command::create_state_commands(&state_changes));
            }
        }

        // Loop
        if loop_end {
            if loop_offset != -1 {
                new_commands.push(xgc_command::create_frame_skip_command());
                new_commands.push(XGMCommand::create_loop_command(loop_offset));
                loop_offset = -1;
            }
        }

        // Last frame?
        if com_idx >= xgm.commands.len() {
            if loop_offset != -1 {
                new_commands.push(xgc_command::create_frame_skip_command());
                new_commands.push(XGMCommand::create_loop_command(loop_offset));
            } else {
                new_commands.push(XGMCommand::create_end_command());
            }
        }

        // Frame size limit (250 bytes safe)
        let mut size: usize = 0;
        let mut split_positions: Vec<usize> = Vec::new();
        for (j, cmd) in new_commands.iter().enumerate() {
            if (size + cmd.size) >= 250 {
                if !is_silent() {
                    let frame_idx = compute_len_in_frame_of(&xgc_commands) + (compute_len_in_frame_of_slice(&new_commands) as i32 - 1);
                    println!("Warning: frame >= 256 at frame {:04X} (need to split frame)", frame_idx);
                }
                split_positions.push(j);
                size = 1; // frame skip size
            }
            size += cmd.size;
        }

        // Insert frame skip + size commands at split positions (reverse order)
        for &pos in split_positions.iter().rev() {
            new_commands.insert(pos, xgc_command::create_frame_size_command(0));
            new_commands.insert(pos, xgc_command::create_frame_skip_command());
        }

        // Compute frame sizes for all size commands
        // The first one is at size_cmd_idx, then one at each split
        // Recalculate all frame sizes
        let total_size: usize = new_commands.iter().map(|c| c.size).sum();
        xgc_command::set_frame_size(&mut xgc_commands[size_cmd_idx], (total_size + 1) as u8);

        // For simplicity, just set the main frame size
        if split_positions.is_empty() {
            xgc_command::set_frame_size(&mut xgc_commands[size_cmd_idx], (size + 1) as u8);
        }

        xgc_commands.extend(new_commands);
    }

    xgc.commands = xgc_commands;

    xgc.compute_all_offset();
    compute_all_frame_size(xgc);

    if !is_silent() {
        println!("Number of command: {}", xgc.commands.len());
    }
}

pub fn shift_samples(source: &mut XGM, sft: usize) {
    if sft == 0 { return; }

    let loop_cmd_offset = source.get_loop_command().map(|c| c.offset);
    let loop_pointed_idx = source.get_loop_pointed_command_idx();

    // Collect PCM commands per frame, working backwards
    let mut frame_pcm: Vec<Vec<XGMCommand>> = Vec::new();
    let mut current_pcm: Vec<XGMCommand> = Vec::new();
    let mut loop_frame_index: i32 = sft as i32;

    // Walk backwards through commands
    for i in (0..source.commands.len()).rev() {
        let cmd = &source.commands[i];

        if let Some(lidx) = loop_pointed_idx {
            if i == lidx { loop_frame_index = 0; }
        }

        if cmd.is_pcm() {
            current_pcm.push(cmd.clone());
        }

        if xgc_command::is_frame_size(cmd) {
            frame_pcm.push(current_pcm.clone());
            current_pcm.clear();
        }
    }
    if !current_pcm.is_empty() {
        frame_pcm.push(current_pcm);
    }

    // Remove all PCM commands from source
    source.commands.retain(|c| !c.is_pcm());

    // Re-insert PCM commands shifted by `sft` frames
    let mut frame_count = 0;
    let mut frame_indices: Vec<usize> = Vec::new();

    for (i, cmd) in source.commands.iter().enumerate() {
        if xgc_command::is_frame_size(cmd) {
            frame_indices.push(i);
            frame_count += 1;
        }
    }

    // Simple approach: for each frame N, take PCM from frame N-sft
    // and insert after the frame size command
    let mut insertions: Vec<(usize, Vec<XGMCommand>)> = Vec::new();

    for (frame_n, &frame_start_idx) in frame_indices.iter().enumerate() {
        let source_frame = frame_n.wrapping_sub(sft);
        if source_frame < frame_pcm.len() {
            let pcm_cmds = &frame_pcm[source_frame];
            if !pcm_cmds.is_empty() {
                insertions.push((frame_start_idx + 1, pcm_cmds.clone()));
            }
        }
    }

    // Apply insertions in reverse order
    for (pos, cmds) in insertions.into_iter().rev() {
        for (j, cmd) in cmds.into_iter().enumerate() {
            source.commands.insert(pos + j, cmd);
        }
    }

    // Recompute offsets & frame sizes
    source.compute_all_offset();
    compute_all_frame_size(source);

    // Fix loop address
    if let Some(loop_off) = loop_cmd_offset {
        if let Some(loop_pointed) = loop_pointed_idx {
            if loop_pointed < source.commands.len() {
                let target_offset = source.commands[loop_pointed].offset;
                if let Some(loop_cmd) = source.commands.iter_mut().find(|c| c.is_loop()) {
                    loop_cmd.data[1] = target_offset as u8;
                    loop_cmd.data[2] = (target_offset >> 8) as u8;
                    loop_cmd.data[3] = (target_offset >> 16) as u8;
                }
            }
        }
    }
}

pub fn get_state_change(current: &YM2612, old: &YM2612) -> Vec<(u8, u8)> {
    let mut result = Vec::new();
    let mut addr: u8 = 0x44;

    for port in 0..2 {
        let mut reg = 0x80;
        while reg < 0x90 {
            if current.is_diff(old, port, reg) {
                result.push((addr, current.get(port, reg) as u8));
            }
            addr += 1;
            if (reg & 3) == 2 { reg += 2; } else { reg += 1; }
        }
    }

    // DAC Enable
    if current.is_diff(old, 0, 0x2B) {
        result.push((0x44 + 0x1C, current.get(0, 0x2B) as u8));
    }

    result
}

pub fn compute_all_frame_size(source: &mut XGM) {
    let mut size_idx: Option<usize> = None;
    let mut size: usize = 0;
    let mut frame = 0;

    for i in 0..source.commands.len() {
        if xgc_command::is_frame_size(&source.commands[i]) {
            if let Some(si) = size_idx {
                if size > 255 && !is_silent() {
                    println!("Error: frame {:04X} has a size > 255! Can't continue...", frame);
                }
                xgc_command::set_frame_size(&mut source.commands[si], size as u8);
            }
            size_idx = Some(i);
            size = 1;
            frame += 1;
        } else {
            size += source.commands[i].size;
        }
    }

    if let Some(si) = size_idx {
        if size > 255 && !is_silent() {
            println!("Error: frame {:04X} has a size > 255! Can't continue...", frame);
        }
        xgc_command::set_frame_size(&mut source.commands[si], size as u8);
    }
}

fn compute_len_in_frame_of(commands: &[XGMCommand]) -> i32 {
    let mut result: i32 = 0;
    for cmd in commands {
        if xgc_command::is_frame_size(cmd) { result += 1; }
        else if xgc_command::is_frame_skip(cmd) { result -= 1; }
    }
    result
}

fn compute_len_in_frame_of_slice(commands: &[XGMCommand]) -> i32 {
    compute_len_in_frame_of(commands)
}

pub fn compute_len_in_frame(source: &XGM) -> i32 {
    compute_len_in_frame_of(&source.commands)
}

pub fn compute_len_in_second(source: &XGM) -> i32 {
    compute_len_in_frame(source) / if source.pal { 50 } else { 60 }
}

fn music_data_size_of(commands: &[XGMCommand]) -> usize {
    commands.iter().map(|c| c.size).sum()
}

/// Serialize XGC to byte array
pub fn as_byte_array(source: &XGM) -> Vec<u8> {
    let mut out = Vec::new();

    // 0000-00FB: sample id table (63 entries of 4 bytes)
    let mut sample_offset: usize = 0;
    let mut s = 0;
    for sample in &source.samples {
        let len = sample.data_size;
        out.push((sample_offset >> 8) as u8);
        out.push((sample_offset >> 16) as u8);
        out.push((len >> 8) as u8);
        out.push((len >> 16) as u8);
        sample_offset += len;
        s += 1;
    }
    while s < 0x3F {
        out.extend_from_slice(&[0xFF, 0xFF, 0x01, 0x00]);
        s += 1;
    }

    // 00FC-00FD: sample block size * 256
    out.push((sample_offset >> 8) as u8);
    out.push((sample_offset >> 16) as u8);

    // 00FE: XGM version
    out.push(0x00);

    // 00FF: misc info
    let mut flags: u8 = 0;
    if source.pal { flags |= 1; }
    if source.xd3.is_some() { flags |= 2; }
    out.push(flags);

    // 0100+: sample data
    for sample in &source.samples {
        out.extend_from_slice(&sample.data[..sample.data_size]);
    }

    // Music data size (4 bytes)
    let music_len = source.get_music_data_size();
    out.push(music_len as u8);
    out.push((music_len >> 8) as u8);
    out.push((music_len >> 16) as u8);
    out.push((music_len >> 24) as u8);

    // Music data - with XGC command byte shifting
    for cmd in &source.commands {
        if xgc_command::is_frame_size(cmd) {
            out.extend_from_slice(&cmd.data[..cmd.size]);
        } else {
            // For Z80 16-bit jump table: shift command byte left by 1
            out.push(cmd.data[0] << 1);
            if cmd.size > 1 {
                out.extend_from_slice(&cmd.data[1..cmd.size]);
            }
        }
    }

    // XD3 tags if present
    if let Some(xd3) = &source.xd3 {
        out.extend_from_slice(&xd3.as_byte_array());
    }

    out
}
