use std::io;
use std::path::Path;

/// Align value up to the given alignment
pub fn align(value: usize, alignment: usize) -> usize {
    ((value + (alignment - 1)) / alignment) * alignment
}

/// Align a size value up (alias for align())
pub fn align_size(value: usize, alignment: usize) -> usize {
    align(value, alignment)
}

/// Return a padded copy of data aligned to the given boundary (zero-filled)
pub fn align_data(data: &[u8], alignment: usize) -> Vec<u8> {
    let mut result = data.to_vec();
    let target = align(result.len(), alignment);
    result.resize(target, 0);
    result
}

/// Pad a byte vector to the given alignment with fill bytes
pub fn align_vec(data: &mut Vec<u8>, alignment: usize, fill: u8) {
    let size = data.len();
    let target = align(size, alignment);
    data.resize(target, fill);
}

/// Get null-terminated ASCII string size from byte slice
pub fn get_ascii_string_size(data: &[u8], offset: usize) -> usize {
    let mut len = 0;
    while offset + len < data.len() && data[offset + len] != 0 {
        len += 1;
    }
    len
}

/// Get null-terminated wide (UTF-16LE) string size in characters
pub fn get_wide_string_size(data: &[u8], offset: usize) -> usize {
    let mut len = 0;
    let mut off = offset;
    while off + 1 < data.len() {
        let ch = get_u16(data, off);
        if ch == 0 { break; }
        len += 1;
        off += 2;
    }
    len
}

/// Read null-terminated wide (UTF-16LE) string
pub fn get_wide_string(data: &[u8], offset: usize) -> String {
    let len = get_wide_string_size(data, offset);
    get_wide_string_n(data, offset, len)
}

/// Read null-terminated ASCII string
pub fn get_ascii_string(data: &[u8], offset: usize) -> String {
    let len = get_ascii_string_size(data, offset);
    get_ascii_string_n(data, offset, len)
}

/// Read wide string of given length
pub fn get_wide_string_n(data: &[u8], offset: usize, length: usize) -> String {
    let u16s: Vec<u16> = (0..length)
        .map(|i| get_u16(data, offset + i * 2))
        .collect();
    String::from_utf16_lossy(&u16s)
}

/// Read ASCII string of given length
pub fn get_ascii_string_n(data: &[u8], offset: usize, length: usize) -> String {
    String::from_utf8_lossy(&data[offset..offset + length]).to_string()
}

/// Read UTF-8 string of given length
pub fn get_utf8_string(data: &[u8], offset: usize, length: usize) -> String {
    String::from_utf8_lossy(&data[offset..offset + length]).to_string()
}

/// Get bytes of a string, optionally as UTF-16LE
pub fn get_string_bytes(text: &str, utf16: bool) -> Vec<u8> {
    if utf16 {
        text.encode_utf16()
            .flat_map(|c| c.to_le_bytes())
            .collect()
    } else {
        text.as_bytes().to_vec()
    }
}

pub fn swap_nibbles(value: u8) -> u8 {
    (value >> 4) | (value << 4)
}

pub fn bytes_as_hex_string(data: &[u8], offset: usize, len: usize, maxlen: usize) -> String {
    let mut result = String::new();
    let end = len.min(maxlen);
    for i in 0..end {
        let v = data[offset + i];
        result.push_str(&format!("{:02X}", v));
    }
    if len > maxlen {
        result.push_str("..");
    }
    result
}

pub fn hex_string(value: i32, min_size: usize) -> String {
    let mut result = String::new();
    let mut started = false;
    for i in (0..8).rev() {
        let v = (value >> (i * 4)) & 0xF;
        if v != 0 || i < min_size {
            started = true;
        }
        if started {
            result.push_str(&format!("{:X}", v));
        }
    }
    if result.is_empty() { "0".to_string() } else { result }
}

// Little-endian readers
pub fn get_u8(data: &[u8], offset: usize) -> u8 {
    data[offset]
}

pub fn get_u16(data: &[u8], offset: usize) -> u16 {
    (data[offset] as u16) | ((data[offset + 1] as u16) << 8)
}

pub fn get_i16(data: &[u8], offset: usize) -> i16 {
    get_u16(data, offset) as i16
}

pub fn get_u24(data: &[u8], offset: usize) -> u32 {
    (data[offset] as u32) | ((data[offset + 1] as u32) << 8) | ((data[offset + 2] as u32) << 16)
}

pub fn get_u32(data: &[u8], offset: usize) -> u32 {
    (data[offset] as u32)
        | ((data[offset + 1] as u32) << 8)
        | ((data[offset + 2] as u32) << 16)
        | ((data[offset + 3] as u32) << 24)
}

pub fn get_i32(data: &[u8], offset: usize) -> i32 {
    get_u32(data, offset) as i32
}

// Little-endian writers
pub fn set_u8(data: &mut [u8], offset: usize, value: u8) {
    data[offset] = value;
}

pub fn set_u16(data: &mut [u8], offset: usize, value: u16) {
    data[offset] = value as u8;
    data[offset + 1] = (value >> 8) as u8;
}

pub fn set_u24(data: &mut [u8], offset: usize, value: u32) {
    data[offset] = value as u8;
    data[offset + 1] = (value >> 8) as u8;
    data[offset + 2] = (value >> 16) as u8;
}

pub fn set_u32(data: &mut [u8], offset: usize, value: u32) {
    data[offset] = value as u8;
    data[offset + 1] = (value >> 8) as u8;
    data[offset + 2] = (value >> 16) as u8;
    data[offset + 3] = (value >> 24) as u8;
}

pub fn set_i32(data: &mut [u8], offset: usize, value: i32) {
    set_u32(data, offset, value as u32);
}

/// Read binary file
pub fn read_binary_file(path: &str) -> io::Result<Vec<u8>> {
    std::fs::read(path)
}

/// Write binary file
pub fn write_binary_file(data: &[u8], path: &str) -> io::Result<()> {
    std::fs::write(path, data)
}

/// Resample PCM data using simple linear interpolation (no javax dependency)
pub fn resample(data: &[u8], offset: usize, len: usize, input_rate: usize, output_rate: usize, alignment: usize) -> Vec<u8> {
    let step = input_rate as f64 / output_rate as f64;
    let mut result = Vec::new();

    let mut value: f64 = 0.0;
    let mut last_sample: f64 = 0.0;
    let mut sample: f64;
    let mut off: usize = 0;

    let mut d_off = 0.0f64;
    while d_off <= (len as f64 - step) {
        sample = 0.0;
        if step >= 1.0 {
            if value < 0.0 {
                sample += last_sample * -value;
            }
            value += step;
            while value > 0.0 {
                if off < len {
                    last_sample = data[off + offset] as f64;
                    off += 1;
                }
                if value >= 1.0 {
                    sample += last_sample;
                } else {
                    sample += last_sample * value;
                }
                value -= 1.0;
            }
            sample /= step;
        } else if d_off == d_off.floor() {
            let idx = d_off as usize;
            if idx < len {
                sample = data[idx + offset] as f64;
            }
        } else {
            let idx0 = d_off.floor() as usize;
            let idx1 = d_off.ceil() as usize;
            let sample0 = if idx0 < len { data[idx0 + offset] as f64 } else { 0.0 };
            let sample1 = if idx1 < len { data[idx1 + offset] as f64 } else { 0.0 };
            sample = sample0 * (d_off.ceil() - d_off) + sample1 * (d_off - d_off.floor());
        }

        result.push(sample.round() as u8);
        d_off += step;
    }

    // Alignment padding
    if alignment > 1 {
        let mask = alignment - 1;
        let pad = alignment - (result.len() & mask);
        if pad != alignment {
            for _ in 0..pad {
                result.push(0x80);
            }
        }
    }

    result
}

/// Format VGM time (1/44100s) to mm:ss.SSS
pub fn format_vgm_time(vgm_time: i64) -> String {
    let ms = vgm_time * 1000 / 44100;
    let minutes = ms / 60000;
    let seconds = (ms % 60000) / 1000;
    let millis = ms % 1000;
    format!("{:02}:{:02}.{:03}", minutes, seconds, millis)
}

/// Check if two sample rates are different enough to matter (>10% difference)
pub fn is_diff_rate(rate1: i32, rate2: i32) -> bool {
    if rate1 == rate2 { return false; }
    let r1 = rate1.max(100) as f64;
    let r2 = rate2.max(100) as f64;
    let f = r1 / r2;
    (100.0 - (f * 100.0)).abs() > 10.0
}

/// Get file extension (uppercase)
pub fn get_file_extension(path: &str) -> String {
    Path::new(path)
        .extension()
        .map(|e| e.to_string_lossy().to_uppercase())
        .unwrap_or_default()
}

/// Get YM register write description
pub fn get_ym_command_desc(port: i32, reg: i32, value: i32) -> String {
    if reg < 0x20 { return "Unknown YM write".to_string(); }

    if reg < 0x30 {
        return match reg & 0x0F {
            0x2 => "Set LFO".to_string(),
            0x4 => "Timer A MSB".to_string(),
            0x5 => "Timer A LSB".to_string(),
            0x6 => "Timer B".to_string(),
            0x7 => "CH2 mode / timer reset".to_string(),
            0x8 => {
                let res = if (value & 0xF0) == 0 { "KEY OFF" } else { "KEY ON" };
                let ch = if (value & 0x0F) >= 4 { (value & 3) + 3 } else { value & 3 };
                format!("{} CH{}", res, ch)
            }
            0xA => "DAC value".to_string(),
            0xB => "DAC enable".to_string(),
            _ => "Unknown YM write".to_string(),
        };
    }

    if reg >= 0xA0 {
        let res = if reg >= 0xA8 && reg < 0xB0 {
            let ch = if port > 0 { "5" } else { "2" };
            format!("CH{} OP{} - ", ch, (reg & 3) + 1)
        } else {
            let ch = if port > 0 { (reg & 3) + 3 } else { reg & 3 };
            format!("CH{} - ", ch)
        };
        let suffix = match reg & 0xFC {
            0xA0 => "FREQ LSB",
            0xA4 => "FREQ MSB",
            0xA8 => "FREQ LSB (SPE)",
            0xAC => "FREQ MSB (SPE)",
            0xB0 => "Feedback / AlgoR",
            0xB4 => "PAN / AMS / FMS",
            _ => "Unknown CH set",
        };
        return format!("{}{}", res, suffix);
    }

    // 0x30..0xA0
    let ch = if port > 0 { (reg & 3) + 3 } else { reg & 3 };
    let op = (reg >> 2) & 3;
    let res = format!("CH{} OP{} - ", ch, op);
    let suffix = match reg & 0xF0 {
        0x30 => "DT / MUL",
        0x40 => "TL",
        0x50 => "RS / AR",
        0x60 => "AM / D1R",
        0x70 => "D2R",
        0x80 => "SL / RR",
        0x90 => "SSG-EG",
        _ => "Unknown slot set",
    };
    format!("{}{}", res, suffix)
}
