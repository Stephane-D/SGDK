//! sizebnd - ROM size alignment and checksum tool
//!
//! Pads a Mega Drive ROM binary to a power-of-2 aligned size and optionally
//! computes and injects the SGDK checksum.
//! Faithful port of the original Java sizebnd tool.

use anyhow::{Context, Result};
use std::fs;

struct Config {
    file: String,
    align: usize,
    fill: u8,
    checksum: bool,
}

fn parse_args(args: &[String]) -> Option<Config> {
    if args.len() < 3 {
        return None;
    }

    let mut file = String::new();
    let mut align: usize = 256;
    let mut fill: u8 = 0;
    let mut checksum = false;

    let mut iter = args[1..].iter();
    while let Some(arg) = iter.next() {
        let param = arg.to_lowercase();
        match param.as_str() {
            "-sizealign" => {
                if let Some(next) = iter.next() {
                    align = next.parse().unwrap_or(256);
                }
            }
            "-fill" => {
                if let Some(next) = iter.next() {
                    fill = next.parse().unwrap_or(0);
                }
            }
            "-checksum" => {
                checksum = true;
            }
            other => {
                if !other.starts_with('-') {
                    file = other.to_string();
                }
            }
        }
    }

    if file.is_empty() {
        return None;
    }

    // Always set a minimum alignment of 4 bytes
    if align < 4 {
        align = 4;
    }

    Some(Config {
        file,
        align,
        fill,
        checksum,
    })
}

/// Pad data to next multiple of `align` bytes.
fn pad(data: &[u8], align: usize, fill: u8) -> Vec<u8> {
    let len = data.len();
    let remainder = len & (align - 1);
    let needed = if remainder != 0 { align - remainder } else { 0 };

    let mut result = Vec::with_capacity(len + needed);
    result.extend_from_slice(data);
    result.resize(len + needed, fill);
    result
}

/// Read a big-endian 32-bit integer from `data` at `offset`.
fn get_int(data: &[u8], offset: usize) -> u32 {
    ((data[offset] as u32) << 24)
        | ((data[offset + 1] as u32) << 16)
        | ((data[offset + 2] as u32) << 8)
        | (data[offset + 3] as u32)
}

/// Compute the SGDK checksum: XOR all 32-bit words, then fold to 16-bit.
fn get_checksum(data: &[u8]) -> u16 {
    let mut checksum: u32 = 0;
    let mut i = 0;
    while i + 3 < data.len() {
        checksum ^= get_int(data, i);
        i += 4;
    }
    (checksum ^ (checksum >> 16)) as u16
}

/// Compute and inject the checksum at offset 0x18E (big-endian).
fn set_checksum(data: &mut [u8]) {
    const OFFSET: usize = 0x18E;

    // Reset checksum bytes
    data[OFFSET] = 0;
    data[OFFSET + 1] = 0;

    // Compute checksum
    let checksum = get_checksum(data);

    // Store checksum (big-endian)
    data[OFFSET] = ((checksum >> 8) & 0xFF) as u8;
    data[OFFSET + 1] = (checksum & 0xFF) as u8;
}

fn show_usage() {
    println!("Sizebnd tool v1.1 by Stephane Dallongeville (Copyright 2021)");
    println!();
    println!("Usage: sizebnd <file> -sizealign <size>");
    println!();
    println!("Options");
    println!(" -fill <value>: set fill value");
    println!(" -checksum: set ROM checksum (SGDK checksum format)");
}

fn main() -> Result<()> {
    let args: Vec<String> = std::env::args().collect();

    let config = match parse_args(&args) {
        Some(c) => c,
        None => {
            show_usage();
            std::process::exit(2);
        }
    };

    // Read file
    let mut data = fs::read(&config.file)
        .with_context(|| format!("Failed to read file: {}", config.file))?;

    // Pad
    if config.align != 0 {
        data = pad(&data, config.align, config.fill);
    }

    // Set checksum if needed
    if config.checksum {
        set_checksum(&mut data);
    }

    // Write back
    fs::write(&config.file, &data)
        .with_context(|| format!("Failed to write file: {}", config.file))?;

    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_pad_no_padding_needed() {
        let data = vec![0u8; 256];
        let result = pad(&data, 256, 0xFF);
        assert_eq!(result.len(), 256);
    }

    #[test]
    fn test_pad_needs_padding() {
        let data = vec![0u8; 100];
        let result = pad(&data, 256, 0xFF);
        assert_eq!(result.len(), 256);
        // First 100 bytes should be 0
        assert!(result[..100].iter().all(|&b| b == 0));
        // Padding bytes should be 0xFF
        assert!(result[100..].iter().all(|&b| b == 0xFF));
    }

    #[test]
    fn test_get_int() {
        let data = [0xDE, 0xAD, 0xBE, 0xEF];
        assert_eq!(get_int(&data, 0), 0xDEADBEEF);
    }

    #[test]
    fn test_checksum_roundtrip() {
        // Create a minimal ROM-like buffer (at least 0x190 bytes)
        let mut data = vec![0u8; 0x200];
        data[0] = 0x12;
        data[1] = 0x34;

        set_checksum(&mut data);

        // The checksum at 0x18E should be non-zero (data has 0x1234 at offset 0)
        let cs = ((data[0x18E] as u16) << 8) | (data[0x18F] as u16);
        assert_ne!(cs, 0);

        // Verify: set_checksum is idempotent — calling it again should yield the same result
        let mut data2 = data.clone();
        set_checksum(&mut data2);
        assert_eq!(data, data2);
    }
}
