//! Utility functions for ASM output, compression, and data manipulation.

use crate::types::{Compression, CollisionType, PackedData, SoundDriver, TileOptimization, TileOrdering};
use crate::types::{OptimizationType, OptimizationLevel};

/// ASM format strings indexed by int size.
const FORMAT_ASM: [&str; 5] = ["b", "b", "w", "w", "d"];

// ── Parsing helpers ──

pub fn get_sound_driver(text: &str) -> Result<SoundDriver, String> {
    let up = text.to_uppercase();
    match up.as_str() {
        "" | "PCM" | "DEFAULT" => Ok(SoundDriver::Pcm),
        "DPCM2" => Ok(SoundDriver::Dpcm2),
        "PCM4" => Ok(SoundDriver::Pcm4),
        "XGM" => Ok(SoundDriver::Xgm),
        "XGM2" => Ok(SoundDriver::Xgm2),
        "0" => Err("'0' is not anymore recognized as valid sound driver, use 'PCM' instead.".into()),
        "1" => Err("'1' is not anymore recognized as valid sound driver, use 'DPCM2' instead.".into()),
        "2" => Err("'2' is not anymore recognized as valid sound driver, use 'PCM4' instead.".into()),
        "3" | "4" | "5" => Err(format!("'{}' is not anymore recognized as valid sound driver, use 'XGM' instead.", text)),
        _ => Err(format!("Unrecognized sound driver: '{}'", text)),
    }
}

pub fn get_compression(text: &str) -> Result<Compression, String> {
    let up = text.to_uppercase();
    match up.as_str() {
        "AUTO" | "BEST" | "-1" => Ok(Compression::Auto),
        "" | "NONE" | "0" => Ok(Compression::None),
        "APLIB" | "1" => Ok(Compression::Aplib),
        "LZ4W" | "2" | "FAST" => Ok(Compression::Lz4w),
        _ => Err(format!("Unrecognized compression: '{}'", text)),
    }
}

pub fn get_collision(text: &str) -> Result<CollisionType, String> {
    let up = text.to_uppercase();
    match up.as_str() {
        "" | "NONE" => Ok(CollisionType::None),
        "BOX" => Ok(CollisionType::Box),
        "CIRCLE" => Ok(CollisionType::Circle),
        _ => Err(format!("Unrecognized collision: '{}'", text)),
    }
}

pub fn get_tile_opt(text: &str) -> Result<TileOptimization, String> {
    let up = text.to_uppercase();
    match up.as_str() {
        "NONE" | "0" => Ok(TileOptimization::None),
        "" | "ALL" | "1" => Ok(TileOptimization::All),
        "DUPLICATE" | "2" => Ok(TileOptimization::DuplicateOnly),
        _ => Err(format!("Unrecognized tilemap optimization: '{}'", text)),
    }
}

pub fn get_sprite_opt_type(text: &str) -> Result<OptimizationType, String> {
    let up = text.to_uppercase();
    match up.as_str() {
        "" | "BALANCED" | "0" => Ok(OptimizationType::Balanced),
        "SPRITE" | "1" => Ok(OptimizationType::MinSprite),
        "TILE" | "2" => Ok(OptimizationType::MinTile),
        "NONE" | "3" => Ok(OptimizationType::None),
        _ => Err(format!("Unrecognized sprite optimization type: '{}'", text)),
    }
}

pub fn get_sprite_opt_level(text: &str) -> Result<OptimizationLevel, String> {
    let up = text.to_uppercase();
    let value: i64 = up.parse().unwrap_or(-1);

    if up.is_empty() || up == "FAST" || value == 0 {
        return Ok(OptimizationLevel::Fast);
    }
    if up == "MEDIUM" || value == 1 {
        return Ok(OptimizationLevel::Medium);
    }
    if up == "MAX" || value > 200000 {
        return Ok(OptimizationLevel::Max);
    }
    if up == "SLOW" || value >= 10000 {
        return Ok(OptimizationLevel::Slow);
    }
    if value != -1 {
        return Ok(OptimizationLevel::Fast);
    }
    Err(format!("Unrecognized sprite optimization level: '{}'", text))
}

pub fn get_tile_ordering(text: &str) -> Result<TileOrdering, String> {
    let up = text.to_uppercase();
    match up.as_str() {
        "COLUMN" => Ok(TileOrdering::Column),
        "ROW" => Ok(TileOrdering::Row),
        _ => Err(format!("Unrecognized tile ordering: '{}'", text)),
    }
}

// ── Size alignment ──

/// Pad data to the given alignment, filling with `fill`.
pub fn size_align(data: &[u8], align: usize, fill: u8) -> Vec<u8> {
    if align <= 1 {
        return data.to_vec();
    }
    let new_size = ((data.len() + align - 1) / align) * align;
    let mut result = vec![fill; new_size];
    result[..data.len()].copy_from_slice(data);
    result
}

/// Align binary output stream to given boundary.
pub fn align_stream(out: &mut Vec<u8>, align: usize) -> bool {
    if align > 2 {
        out.clear();
        return false;
    }
    if align == 2 && (out.len() & 1) != 0 {
        out.push(0);
    }
    true
}

// ── ASM output ──

/// ASM declaration for a struct.
pub fn decl(out_s: &mut String, out_h: &mut String, type_name: &str, name: &str, align: usize, global: bool) {
    let a = if align < 2 { 2 } else { align };
    out_s.push_str(&format!("    .align {}\n", a));
    if global {
        out_s.push_str(&format!("    .global {}\n", name));
    }
    out_s.push_str(&format!("{}:\n", name));
    if global {
        out_h.push_str(&format!("extern const {} {};\n", type_name, name));
    }
}

/// ASM declaration for an array.
pub fn decl_array(out_s: &mut String, out_h: &mut String, type_name: &str, name: &str, size: usize, align: usize, global: bool) {
    let a = if align < 2 { 2 } else { align };
    out_s.push_str(&format!("    .align  {}\n", a));
    if global {
        out_s.push_str(&format!("    .global {}\n", name));
    }
    out_s.push_str(&format!("{}:\n", name));
    if global {
        out_h.push_str(&format!("extern const {} {}[{}];\n", type_name, name, size));
    }
}

/// ASM end-of-array size label.
pub fn decl_array_end(out_s: &mut String, _out_h: &mut String, _type_name: &str, name: &str, _size: usize, _align: usize, global: bool) {
    if global {
        out_s.push_str(&format!("    .global {}_size\n", name));
    }
    out_s.push_str(&format!("{}_size = .-{}\n", name, name));
}

/// Output binary data as ASM hex declarations.
pub fn out_s(out: &mut String, data: &[u8], int_size: usize) {
    let mut offset = 0;
    let mut remain = data.len() as isize;

    while remain > 0 {
        out.push_str(&format!("    dc.{}    ", FORMAT_ASM[int_size]));
        let chunk = std::cmp::min(16, remain as usize);
        let count = chunk / int_size;

        for i in 0..count {
            if i > 0 {
                out.push_str(", ");
            }
            out.push_str("0x");
            let end = offset + int_size;
            // Big-endian output within each element
            for j in 0..int_size {
                out.push_str(&format!("{:02X}", data[end - j - 1]));
            }
            offset += int_size;
        }

        out.push('\n');
        remain -= 16;
    }

    // Pad to word alignment
    if int_size == 1 && (data.len() & 1) != 0 {
        out.push_str("    dc.b    0x00\n");
    }
}

/// Write short to binary stream (big-endian by default for M68K).
pub fn out_b_short(out: &mut Vec<u8>, data: i16, swap: bool) {
    let v = data as u16;
    if swap {
        out.push((v >> 8) as u8);
        out.push(v as u8);
    } else {
        out.push(v as u8);
        out.push((v >> 8) as u8);
    }
}

/// Write int to binary stream.
pub fn out_b_int(out: &mut Vec<u8>, data: i32, swap: bool) {
    let v = data as u32;
    if swap {
        out.push((v >> 24) as u8);
        out.push((v >> 16) as u8);
        out.push((v >> 8) as u8);
        out.push(v as u8);
    } else {
        out.push(v as u8);
        out.push((v >> 8) as u8);
        out.push((v >> 16) as u8);
        out.push((v >> 24) as u8);
    }
}

/// Write bytes to binary stream with optional alignment.
pub fn out_b_bytes(out: &mut Vec<u8>, data: &[u8], align: usize) {
    if align_stream(out, align) {
        out.extend_from_slice(data);
    }
}

// ── Compression ──

/// Check if compression is valuable (enough gain).
pub fn is_compression_valuable(method: Compression, compressed_size: usize, uncompressed_size: usize) -> bool {
    let min_diff = if method == Compression::Aplib { 120 } else { 60 };
    if uncompressed_size.saturating_sub(compressed_size) <= min_diff {
        return false;
    }
    let max_pct = if method == Compression::Aplib { 85.0 } else { 95.0 };
    let pct = (compressed_size as f64 * 100.0) / uncompressed_size as f64;
    pct.round() <= max_pct
}

/// Pack data with the specified compression.
pub fn pack(data: &[u8], compression: Compression, bin: &[u8], force: bool) -> PackedData {
    if compression == Compression::None {
        return PackedData::new(data.to_vec(), Compression::None);
    }

    if force && compression == Compression::Auto {
        panic!("Cannot use AUTO compression!");
    }

    // Fast path for LZ4W
    if compression == Compression::Lz4w {
        if let Some(result) = lz4w_pack(bin, data) {
            if force || is_compression_valuable(Compression::Lz4w, result.len(), data.len()) {
                return PackedData::new(result, Compression::Lz4w);
            }
        } else if force {
            panic!("Cannot use wanted compression on resource, try removing compression.");
        }
        if !force {
            return PackedData::new(data.to_vec(), Compression::None);
        }
    }

    // Try all compression methods for AUTO, or just the specified one
    let mut best_data = data.to_vec();
    let mut best_comp = Compression::None;
    let mut best_size = data.len();

    let methods = if compression == Compression::Auto {
        vec![Compression::Aplib, Compression::Lz4w]
    } else {
        vec![compression]
    };

    for method in methods {
        let packed = match method {
            Compression::Aplib => ap_pack(data),
            Compression::Lz4w => lz4w_pack(bin, data),
            _ => None,
        };

        if force {
            if let Some(result) = packed {
                return PackedData::new(result, method);
            }
            panic!("Cannot use desired compression on resource! Try removing compression.");
        }

        if let Some(result) = packed {
            if is_compression_valuable(method, result.len(), data.len()) && result.len() < best_size {
                best_size = result.len();
                best_data = result;
                best_comp = method;
            }
        }
    }

    PackedData::new(best_data, best_comp)
}

/// APlib compression.
pub fn ap_pack(data: &[u8]) -> Option<Vec<u8>> {
    let result = apj::pack(data, false, true);
    if result.is_empty() { None } else { Some(result) }
}

/// LZ4W compression with optional previous data context.
pub fn lz4w_pack(prev: &[u8], data: &[u8]) -> Option<Vec<u8>> {
    let prev_len = prev.len();
    let mut buf = Vec::with_capacity(prev_len + data.len());
    buf.extend_from_slice(prev);
    buf.extend_from_slice(data);

    match lz4w::pack(&buf, prev_len, true) {
        Ok(result) => Some(result),
        Err(_) => {
            // Try without previous data
            match lz4w::pack(data, 0, true) {
                Ok(result) => Some(result),
                Err(_) => None,
            }
        }
    }
}

// ── DPCM compression ──

const DELTA_TAB: [i32; 16] = [-34, -21, -13, -8, -5, -3, -2, -1, 0, 1, 2, 3, 5, 8, 13, 21];

fn get_best_delta_index(wanted_level: i32, cur_level: i32) -> usize {
    let wdelta = wanted_level - cur_level;
    let mut ind = 0;
    let mut mindiff = (wdelta - DELTA_TAB[0]).abs();

    for i in 1..16 {
        let diff = (wdelta - DELTA_TAB[i]).abs();
        if diff < mindiff {
            mindiff = diff;
            ind = i;
        }
    }

    let new_level = DELTA_TAB[ind] + cur_level;
    if new_level > 127 && ind > 0 {
        return ind - 1;
    }
    if new_level < -128 && ind < 15 {
        return ind + 1;
    }
    ind
}

/// DPCM 4-bit compression for audio data.
pub fn dpcm_pack(input: &[u8]) -> Vec<u8> {
    let result_len = (input.len() / 2) + (input.len() & 1);
    let mut result = vec![0u8; result_len];
    let mut cur_level: i32 = 0;

    for (dst_off, src_off) in (0..input.len()).step_by(2).enumerate() {
        let ind1 = get_best_delta_index(input[src_off] as i8 as i32, cur_level);
        cur_level += DELTA_TAB[ind1];

        let sample2 = if src_off + 1 < input.len() {
            input[src_off + 1] as i8 as i32
        } else {
            0
        };
        let ind2 = get_best_delta_index(sample2, cur_level);
        cur_level += DELTA_TAB[ind2];

        result[dst_off] = (ind1 | (ind2 << 4)) as u8;
    }

    result
}

// ── External tool execution ──

/// Find a sibling tool (in the same directory as the current executable).
fn find_sibling_tool(name: &str) -> String {
    if let Ok(exe) = std::env::current_exe() {
        if let Some(dir) = exe.parent() {
            let candidate = dir.join(name);
            if candidate.exists() {
                return candidate.to_string_lossy().to_string();
            }
        }
    }
    // Fallback: just use the name and hope it's in PATH
    name.to_string()
}

/// Execute xgmtool to convert VGM to XGM binary.
pub fn xgmtool(_current_dir: &str, fin: &str, fout: &str, timing: i32, options: &str) -> bool {
    let _ = std::fs::remove_file(fout);

    let xgmtool_path = find_sibling_tool("xgmtool");
    let timing_flag = match timing {
        0 => "-n",
        1 => "-p",
        _ => "",
    };

    let mut args = vec![fin.to_string(), fout.to_string(), "-s".to_string()];
    if !timing_flag.is_empty() {
        args.push(timing_flag.to_string());
    }
    if !options.is_empty() {
        args.push(options.to_string());
    }

    println!("Executing {} {}", xgmtool_path, args.join(" "));

    match std::process::Command::new(&xgmtool_path).args(&args).status() {
        Ok(status) => status.success() && std::path::Path::new(fout).exists(),
        Err(_) => false,
    }
}

/// Execute xgm2tool to convert VGM to XGM2 binary.
pub fn xgm2tool(_current_dir: &str, fins: &[String], fout: &str, options: &str) -> bool {
    let _ = std::fs::remove_file(fout);

    let xgm2tool_path = find_sibling_tool("xgm2tool");
    let mut args = Vec::new();
    for f in fins {
        args.push(f.clone());
    }
    args.push(fout.to_string());
    if !options.is_empty() {
        args.push(options.to_string());
    }
    args.push("-s".to_string());

    println!("Executing {} {}", xgm2tool_path, args.join(" "));

    match std::process::Command::new(&xgm2tool_path).args(&args).status() {
        Ok(status) => status.success() && std::path::Path::new(fout).exists(),
        Err(_) => false,
    }
}

/// Adjust a relative path based on a base directory.
/// Also normalizes backslashes to forward slashes for cross-platform compatibility.
pub fn adjust_path(base: &str, path: &str) -> String {
    let path = &path.replace('\\', "/");
    if std::path::Path::new(path).is_absolute() {
        return path.to_string();
    }
    if base.is_empty() {
        return path.to_string();
    }
    let base_path = std::path::Path::new(base);
    base_path.join(path).to_string_lossy().to_string()
}

/// Parse an integer from string with a default value.
pub fn parse_int(s: &str, default: i32) -> i32 {
    if s.is_empty() {
        return default;
    }
    // Handle hex
    if let Some(hex) = s.strip_prefix("0x").or_else(|| s.strip_prefix("0X")) {
        return i32::from_str_radix(hex, 16).unwrap_or(default);
    }
    s.parse().unwrap_or(default)
}

/// Parse a boolean from string with a default value.
pub fn parse_bool(s: &str, default: bool) -> bool {
    let up = s.to_uppercase();
    match up.as_str() {
        "TRUE" | "1" | "YES" => true,
        "FALSE" | "0" | "NO" => false,
        _ => default,
    }
}
