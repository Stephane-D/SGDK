use crate::util;

/// Base command structure (shared between VGMCommand and XGM commands)
/// In Java this is an abstract class; in Rust we use a trait + concrete structs.

/// Common command trait
pub trait CommandTrait {
    fn data(&self) -> &[u8];
    fn data_mut(&mut self) -> &mut Vec<u8>;
    fn time(&self) -> i32;
    fn set_time(&mut self, t: i32);
    fn size(&self) -> usize;
    fn origin_offset(&self) -> i32;
    fn set_origin_offset(&mut self, offset: i32);
    fn get_channel(&self) -> i32;

    fn get_command(&self) -> u8 {
        util::get_u8(self.data(), 0)
    }

    fn get_milli_second(&self) -> i64 {
        (self.time() as i64 * 1000) / 44100
    }

    fn get_frame(&self, pal: bool) -> i32 {
        if pal { self.time() / 882 } else { self.time() / 735 }
    }

    fn is_same(&self, other: &dyn CommandTrait) -> bool {
        self.data() == other.data()
    }

    fn as_byte_array(&self) -> &[u8] {
        self.data()
    }

    fn to_hex_string(&self) -> String {
        util::bytes_as_hex_string(self.data(), 0, self.size(), 32)
    }
}

/// Check if a command is in the list
pub fn contains<T: CommandTrait>(commands: &[T], command: &T) -> bool {
    commands.iter().any(|c| c.data() == command.data())
}

/// Get total data size of commands
pub fn get_data_size<T: CommandTrait>(commands: &[T]) -> usize {
    commands.iter().map(|c| c.size()).sum()
}

/// Return index of command at given time
pub fn get_command_index_at_time<T: CommandTrait>(commands: &[T], time: i32) -> Option<usize> {
    for (i, c) in commands.iter().enumerate() {
        if c.time() >= time {
            return Some(i);
        }
    }
    if commands.is_empty() { None } else { Some(commands.len() - 1) }
}

/// Return command index at given origin offset
pub fn get_command_index_at_offset<T: CommandTrait>(commands: &[T], offset: i32) -> Option<usize> {
    for (i, c) in commands.iter().enumerate() {
        if c.origin_offset() == offset {
            return Some(i);
        }
    }
    None
}

/// Update origin offset for all commands
pub fn compute_offsets<T: CommandTrait>(commands: &mut [T], start_offset: i32) {
    let mut off = start_offset;
    for com in commands.iter_mut() {
        com.set_origin_offset(off);
        off += com.size() as i32;
    }
}

/// Get YM command description (delegated to util)
pub fn get_ym_command_desc(port: i32, reg: i32, value: i32) -> String {
    util::get_ym_command_desc(port, reg, value)
}
