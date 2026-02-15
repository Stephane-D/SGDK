use crate::vgm_command::VGMCommand;
use crate::xgm_command::{XGMCommand, XGM_PSG, XGM_YM2612_REGKEY, XGM_YM2612_PORT0,
    XGM_YM2612_PORT1, XGM_LOOP, XGM_END, XGM_PCM};

/// XGC-specific command type constants
pub const XGC_PSG_TONE: u8 = 0x10;
pub const XGC_PSG_ENV: u8 = 0x18;
pub const XGC_PCM: u8 = 0x50;
pub const XGC_STATE: u8 = 0x60;
pub const XGC_FRAME_SKIP: u8 = 0x7D;
pub const XGC_FRAME_SIZE: u8 = 1; // special marker

/// Create a frame size command (holds the frame byte-count)
pub fn create_frame_size_command(size: u8) -> XGMCommand {
    XGMCommand::new_ex(XGC_FRAME_SIZE, vec![size])
}

/// Create a frame skip command
pub fn create_frame_skip_command() -> XGMCommand {
    XGMCommand::new(vec![XGC_FRAME_SKIP])
}

/// Create XGC command from XGM command (copy)
pub fn create_from_command(command: &XGMCommand) -> XGMCommand {
    XGMCommand::new(command.data.clone())
}

/// Parse XGC command from raw data (XGC→XGM conversion, shifts command byte right by 1)
pub fn create_from_data(data: &[u8]) -> XGMCommand {
    let mut d = data.to_vec();
    // convert XGC --> XGM: shift right by 1
    d[0] >>= 1;
    let command = d[0];

    let size = if command == XGM_LOOP {
        4
    } else if command == XGM_END {
        1
    } else if command == XGC_FRAME_SKIP {
        1
    } else {
        match command & 0xF0 {
            0x10 => { // PSG
                if command & 0x8 != 0 {
                    let c = command & !8;
                    d[0] = c;
                    1 + (c & 0x3) as usize + 1
                } else {
                    1 + (command & 0x7) as usize + 1
                }
            }
            0x40 => 1 + (command & 0xF) as usize + 1, // YM_REGKEY
            0x20 | 0x30 | 0x60 => 1 + 2 * ((command & 0xF) as usize + 1), // YM_PORT0, YM_PORT1, STATE
            0x50 => 2, // PCM
            _ => 1,
        }
    };

    let d = d[..size.min(d.len())].to_vec();
    XGMCommand { data: d, offset: 0, command, size }
}

pub fn get_frame_size(command: &XGMCommand) -> usize {
    command.data[0] as usize
}

pub fn set_frame_size(command: &mut XGMCommand, value: u8) {
    command.data[0] = value;
}

pub fn get_type(command: &XGMCommand) -> u8 {
    if command.command == XGC_FRAME_SIZE { return XGC_FRAME_SIZE; }
    if command.command == XGM_LOOP { return XGM_LOOP; }
    if command.command == XGM_END { return XGM_END; }
    command.command & 0xF0
}

pub fn is_frame_size(command: &XGMCommand) -> bool {
    command.command == XGC_FRAME_SIZE
}

pub fn is_frame_skip(command: &XGMCommand) -> bool {
    command.command == XGC_FRAME_SKIP
}

pub fn is_psg_env_write(command: &XGMCommand) -> bool {
    (command.command & 0xF8) == XGC_PSG_ENV
}

pub fn is_psg_tone_write(command: &XGMCommand) -> bool {
    (command.command & 0xF8) == XGC_PSG_TONE
}

pub fn is_pcm(command: &XGMCommand) -> bool {
    (command.command & 0xF0) == XGC_PCM
}

pub fn get_pcm_id(command: &XGMCommand) -> i32 {
    if is_pcm(command) { command.data[1] as i32 } else { -1 }
}

pub fn is_state(command: &XGMCommand) -> bool {
    (command.command & 0xF0) == XGC_STATE
}

// PSG env command creation
fn create_psg_env_command(commands: &[VGMCommand], max: usize) -> (XGMCommand, usize) {
    let size = max.min(commands.len());
    let mut data = vec![XGC_PSG_ENV | ((size - 1) as u8)];
    for i in 0..size {
        data.push(commands[i].get_psg_value() as u8);
    }
    (XGMCommand::new(data), size)
}

fn create_psg_tone_command(commands: &[VGMCommand], max: usize) -> (XGMCommand, usize) {
    let size = max.min(commands.len());
    let mut data = vec![XGC_PSG_TONE | ((size - 1) as u8)];
    for i in 0..size {
        data.push(commands[i].get_psg_value() as u8);
    }
    (XGMCommand::new(data), size)
}

fn create_state_command(states: &[(u8, u8)], max: usize) -> (XGMCommand, usize) {
    let size = max.min(states.len());
    let mut data = vec![XGC_STATE | ((size - 1) as u8)];
    for i in 0..size {
        data.push(states[i].0);
        data.push(states[i].1);
    }
    (XGMCommand::new(data), size)
}

pub fn create_psg_env_commands(commands: &[VGMCommand]) -> Vec<XGMCommand> {
    if commands.len() > 4 && !crate::is_silent() {
        println!("Warning: more than 4 PSG env command in a single frame !");
    }
    let mut result = Vec::new();
    let mut offset = 0;
    while offset < commands.len() {
        let (cmd, consumed) = create_psg_env_command(&commands[offset..], 4);
        result.push(cmd);
        offset += consumed;
    }
    result
}

pub fn create_psg_tone_commands(commands: &[VGMCommand]) -> Vec<XGMCommand> {
    let mut result = Vec::new();
    let mut offset = 0;
    while offset < commands.len() {
        let (cmd, consumed) = create_psg_tone_command(&commands[offset..], 8);
        result.push(cmd);
        offset += consumed;
    }
    result
}

pub fn create_ym_key_commands_xgc(commands: &[VGMCommand]) -> Vec<XGMCommand> {
    if commands.len() > 6 && !crate::is_silent() {
        println!("Warning: more than 6 Key off or Key on command in a single frame !");
    }
    let mut result = Vec::new();
    let mut offset = 0;
    while offset < commands.len() {
        let (cmd, consumed) = XGMCommand::create_ym_key_command(&commands[offset..], 6);
        result.push(cmd);
        offset += consumed;
    }
    result
}

pub fn create_state_commands(states: &[(u8, u8)]) -> Vec<XGMCommand> {
    let mut result = Vec::new();
    let mut offset = 0;
    while offset < states.len() {
        let (cmd, consumed) = create_state_command(&states[offset..], 16);
        result.push(cmd);
        offset += consumed;
    }
    result
}

/// Convert a single XGM command to XGC commands
pub fn convert_single(command: &XGMCommand) -> Vec<XGMCommand> {
    match command.get_type() {
        XGM_PSG => {
            let size = (command.data[0] & 0xF) as usize + 1;
            let mut env_commands = Vec::new();
            let mut tone_commands = Vec::new();

            for i in 0..size {
                let data = vec![0x50, command.data[i + 1]];
                let vgm_cmd = VGMCommand::from_owned(data, -1);
                if vgm_cmd.is_psg_env_write() {
                    env_commands.push(vgm_cmd);
                } else {
                    tone_commands.push(vgm_cmd);
                }
            }

            let mut result = Vec::new();
            if !env_commands.is_empty() {
                result.extend(create_psg_env_commands(&env_commands));
            }
            if !tone_commands.is_empty() {
                result.extend(create_psg_tone_commands(&tone_commands));
            }
            result
        }
        XGM_YM2612_REGKEY => {
            let size = (command.data[0] & 0xF) as usize + 1;
            let mut key_commands = Vec::new();
            for i in 0..size {
                let data = vec![0x52, 0x28, command.data[i + 1]];
                key_commands.push(VGMCommand::from_owned(data, -1));
            }
            create_ym_key_commands_xgc(&key_commands)
        }
        _ => vec![command.clone()],
    }
}

/// Convert a list of XGM commands to XGC commands
pub fn convert(commands: &[XGMCommand]) -> Vec<XGMCommand> {
    let mut result = Vec::new();
    for cmd in commands {
        result.extend(convert_single(cmd));
    }
    result
}
