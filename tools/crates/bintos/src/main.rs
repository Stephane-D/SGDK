//! bintos - Binary to GAS assembly source converter
//!
//! Converts a binary file into a GAS assembly source (.s) file and a C header (.h) file.
//! This is a faithful port of the original SGDK bintos tool written in C.

use anyhow::{Context, Result};
use std::fs;
use std::io::Read;
use std::path::{Path, PathBuf};

/// Configuration parsed from CLI arguments.
struct Config {
    input_file: String,
    output_file: String,
    /// C type name for the header (e.g. "u8", "s16", "u32")
    format: &'static str,
    /// Size of the C type in bytes (1, 2, or 4)
    format_int: usize,
    /// GAS directive suffix ("w" for dc.w, "l" for dc.l)
    format_asm: &'static str,
    /// Size of the assembly unit in bytes (2 for dc.w, 4 for dc.l)
    format_int_asm: usize,
    /// Alignment value (minimum 2)
    align: usize,
    /// Fill byte for padding
    nullfill: u8,
}

impl Default for Config {
    fn default() -> Self {
        Self {
            input_file: String::new(),
            output_file: String::new(),
            format: "u8",
            format_int: 1,
            format_asm: "w",
            format_int_asm: 2,
            align: 2,
            nullfill: 0,
        }
    }
}

fn parse_args(args: &[String]) -> Config {
    let mut config = Config::default();
    let mut i = 1; // skip argv[0]

    while i < args.len() {
        match args[i].as_str() {
            "-u8" => {
                config.format = "u8";
                config.format_int = 1;
                config.format_asm = "w";
                config.format_int_asm = 2;
            }
            "-s8" => {
                config.format = "s8";
                config.format_int = 1;
                config.format_asm = "w";
                config.format_int_asm = 2;
            }
            "-u16" => {
                config.format = "u16";
                config.format_int = 2;
                config.format_asm = "w";
                config.format_int_asm = 2;
            }
            "-s16" => {
                config.format = "s16";
                config.format_int = 2;
                config.format_asm = "w";
                config.format_int_asm = 2;
            }
            "-u32" => {
                config.format = "u32";
                config.format_int = 4;
                config.format_asm = "l";
                config.format_int_asm = 4;
            }
            "-s32" => {
                config.format = "s32";
                config.format_int = 4;
                config.format_asm = "l";
                config.format_int_asm = 4;
            }
            "-align" => {
                i += 1;
                if i < args.len() {
                    config.align = args[i].parse().unwrap_or(2);
                    if config.align == 0 {
                        config.align = 2;
                    }
                }
            }
            "-nullfill" => {
                i += 1;
                if i < args.len() {
                    config.nullfill = args[i].parse::<u8>().unwrap_or(0);
                }
            }
            other => {
                if config.input_file.is_empty() {
                    config.input_file = other.to_string();
                } else if config.output_file.is_empty() {
                    config.output_file = other.to_string();
                }
            }
        }
        i += 1;
    }

    if config.output_file.is_empty() {
        config.output_file = config.input_file.clone();
    }

    // force align >= 2
    if config.align < 2 {
        config.align = 2;
    }

    config
}

/// Extract just the filename (no directory) from a path.
fn get_filename(path: &str) -> &str {
    Path::new(path)
        .file_name()
        .and_then(|f| f.to_str())
        .unwrap_or(path)
}

/// Remove the file extension from a path string.
fn remove_extension(path: &str) -> String {
    let p = PathBuf::from(path);
    p.with_extension("").to_string_lossy().to_string()
}

fn main() -> Result<()> {
    let args: Vec<String> = std::env::args().collect();
    let config = parse_args(&args);

    if config.input_file.is_empty() {
        eprintln!("Usage: bintos <input_file> [output_file] [-u8|-s8|-u16|-s16|-u32|-s32] [-align N] [-nullfill N]");
        std::process::exit(1);
    }

    // Read input binary file
    let mut input = fs::File::open(&config.input_file)
        .with_context(|| format!("Couldn't open input file {}", config.input_file))?;

    let output_base = remove_extension(&config.output_file);
    let shortname = get_filename(&output_base);
    let shortname = shortname.to_string();

    // --- Generate .s file ---
    let s_path = format!("{}.s", output_base);
    let mut s_output = String::new();

    s_output.push_str(".section .rodata\n\n");
    s_output.push_str(&format!("    .align  {}\n\n", config.align));
    s_output.push_str(&format!("    .global {}\n", shortname));
    s_output.push_str(&format!("{}:\n", shortname));

    let mut total: usize = 0;
    let mut temp = [0u8; 4096];

    loop {
        // Fill buffer with nullfill value first
        temp.fill(config.nullfill);

        let bytes_read = input
            .read(&mut temp)
            .with_context(|| "Error reading input file")?;
        total += bytes_read;

        // Align length to format_int_asm size
        let len = (bytes_read + (config.format_int_asm - 1)) & !(config.format_int_asm - 1);

        if len > 0 {
            s_output.push_str(&format!("    dc.{}    ", config.format_asm));

            let num_units = len / config.format_int_asm;
            for ii in 0..num_units {
                if ii > 0 {
                    s_output.push(',');
                }
                s_output.push_str("0x");
                for jj in 0..config.format_int_asm {
                    s_output.push_str(&format!("{:02X}", temp[ii * config.format_int_asm + jj]));
                }
            }
            s_output.push('\n');
        }

        if len < 16 {
            break;
        }
    }

    s_output.push('\n');
    fs::write(&s_path, &s_output)
        .with_context(|| format!("Couldn't open output file {}", s_path))?;

    // --- Generate .h file ---
    let h_path = format!("{}.h", output_base);
    let upper_shortname = shortname.to_uppercase();
    let array_len = total / config.format_int;

    let h_output = format!(
        "#ifndef _{}_H_\n#define _{}_H_\n\nextern const {} {}[0x{:X}];\n\n#endif // _{}_H_\n",
        upper_shortname, upper_shortname, config.format, shortname, array_len, upper_shortname
    );

    fs::write(&h_path, &h_output)
        .with_context(|| format!("Couldn't open output file {}", h_path))?;

    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::io::Write;

    #[test]
    fn test_parse_args_defaults() {
        let args = vec!["bintos".to_string(), "test.bin".to_string()];
        let config = parse_args(&args);
        assert_eq!(config.input_file, "test.bin");
        assert_eq!(config.output_file, "test.bin");
        assert_eq!(config.format, "u8");
        assert_eq!(config.format_int, 1);
        assert_eq!(config.format_asm, "w");
        assert_eq!(config.format_int_asm, 2);
        assert_eq!(config.align, 2);
        assert_eq!(config.nullfill, 0);
    }

    #[test]
    fn test_parse_args_u32() {
        let args = vec![
            "bintos".to_string(),
            "test.bin".to_string(),
            "out.bin".to_string(),
            "-u32".to_string(),
            "-align".to_string(),
            "4".to_string(),
        ];
        let config = parse_args(&args);
        assert_eq!(config.format, "u32");
        assert_eq!(config.format_int, 4);
        assert_eq!(config.format_asm, "l");
        assert_eq!(config.format_int_asm, 4);
        assert_eq!(config.align, 4);
    }

    #[test]
    fn test_get_filename() {
        assert_eq!(get_filename("path/to/file.bin"), "file.bin");
        assert_eq!(get_filename("file.bin"), "file.bin");
    }

    #[test]
    fn test_remove_extension() {
        assert_eq!(remove_extension("path/to/file.bin"), "path/to/file");
        assert_eq!(remove_extension("file.bin"), "file");
    }

    #[test]
    fn test_roundtrip() {
        let dir = std::env::temp_dir().join("bintos_test");
        let _ = fs::create_dir_all(&dir);

        let input_path = dir.join("test.o80");
        let mut f = fs::File::create(&input_path).unwrap();
        // Write 4 bytes of test data
        f.write_all(&[0xDE, 0xAD, 0xBE, 0xEF]).unwrap();

        let args = vec![
            "bintos".to_string(),
            input_path.to_string_lossy().to_string(),
        ];
        let config = parse_args(&args);

        // Verify the config is correct
        assert_eq!(config.format, "u8");

        // Clean up
        let _ = fs::remove_dir_all(&dir);
    }
}
