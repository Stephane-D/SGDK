pub mod vgm_command;
pub mod xgm_command;
pub mod xgc_command;
pub mod sample_bank;
pub mod xgm_sample;
pub mod gd3;
pub mod ym2612;
pub mod psg;
pub mod lz77;
pub mod compress;
pub mod vgm;
pub mod xgm;
pub mod xgc;

use std::sync::atomic::{AtomicBool, AtomicI32, Ordering};

/// Global settings (mirrors C globals)
static SILENT: AtomicBool = AtomicBool::new(false);
static VERBOSE: AtomicBool = AtomicBool::new(false);
static SAMPLE_RATE_FIX: AtomicBool = AtomicBool::new(true);
static SAMPLE_IGNORE: AtomicBool = AtomicBool::new(true);
static DELAY_KEY_OFF: AtomicBool = AtomicBool::new(true);
static KEEP_RF5C68_CMDS: AtomicBool = AtomicBool::new(false);
static SYS: AtomicI32 = AtomicI32::new(-1);

pub const SYSTEM_AUTO: i32 = -1;
pub const SYSTEM_NTSC: i32 = 0;
pub const SYSTEM_PAL: i32 = 1;

pub fn is_silent() -> bool { SILENT.load(Ordering::Relaxed) }
pub fn is_verbose() -> bool { VERBOSE.load(Ordering::Relaxed) }
pub fn sample_rate_fix() -> bool { SAMPLE_RATE_FIX.load(Ordering::Relaxed) }
pub fn sample_ignore() -> bool { SAMPLE_IGNORE.load(Ordering::Relaxed) }
pub fn delay_key_off() -> bool { DELAY_KEY_OFF.load(Ordering::Relaxed) }
pub fn keep_rf5c68_cmds() -> bool { KEEP_RF5C68_CMDS.load(Ordering::Relaxed) }
pub fn get_sys() -> i32 { SYS.load(Ordering::Relaxed) }

pub fn set_silent(v: bool) { SILENT.store(v, Ordering::Relaxed); }
pub fn set_verbose(v: bool) { VERBOSE.store(v, Ordering::Relaxed); }
pub fn set_sample_rate_fix(v: bool) { SAMPLE_RATE_FIX.store(v, Ordering::Relaxed); }
pub fn set_sample_ignore(v: bool) { SAMPLE_IGNORE.store(v, Ordering::Relaxed); }
pub fn set_delay_key_off(v: bool) { DELAY_KEY_OFF.store(v, Ordering::Relaxed); }
pub fn set_keep_rf5c68_cmds(v: bool) { KEEP_RF5C68_CMDS.store(v, Ordering::Relaxed); }
pub fn set_sys(v: i32) { SYS.store(v, Ordering::Relaxed); }

/// Helper to read little-endian u16 from byte slice
pub fn get_u16(data: &[u8], offset: usize) -> u16 {
    (data[offset] as u16) | ((data[offset + 1] as u16) << 8)
}

/// Helper to read little-endian u24 from byte slice
pub fn get_u24(data: &[u8], offset: usize) -> u32 {
    (data[offset] as u32) | ((data[offset + 1] as u32) << 8) | ((data[offset + 2] as u32) << 16)
}

/// Helper to read little-endian u32 from byte slice
pub fn get_u32(data: &[u8], offset: usize) -> u32 {
    (data[offset] as u32)
        | ((data[offset + 1] as u32) << 8)
        | ((data[offset + 2] as u32) << 16)
        | ((data[offset + 3] as u32) << 24)
}

/// Helper to write little-endian u16 to byte slice
pub fn set_u16(data: &mut [u8], offset: usize, value: u32) {
    data[offset] = value as u8;
    data[offset + 1] = (value >> 8) as u8;
}

/// Helper to write little-endian u24 to byte slice
pub fn set_u24(data: &mut [u8], offset: usize, value: u32) {
    data[offset] = value as u8;
    data[offset + 1] = (value >> 8) as u8;
    data[offset + 2] = (value >> 16) as u8;
}

/// Helper to write little-endian u32 to byte slice
pub fn set_u32(data: &mut [u8], offset: usize, value: u32) {
    data[offset] = value as u8;
    data[offset + 1] = (value >> 8) as u8;
    data[offset + 2] = (value >> 16) as u8;
    data[offset + 3] = (value >> 24) as u8;
}

/// Get file extension (uppercase) from path
pub fn get_file_extension(path: &str) -> String {
    std::path::Path::new(path)
        .extension()
        .map(|e| e.to_string_lossy().to_uppercase())
        .unwrap_or_default()
}
