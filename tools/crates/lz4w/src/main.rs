//! LZ4W CLI - Pack/Unpack tool for SGDK
//!
//! Usage:
//!   lz4w p <input_file> <output_file>
//!   lz4w p <prev_file>@<input_file> <output_file>
//!   lz4w u <input_file> <output_file>

use anyhow::{bail, Context, Result};
use std::fs;

fn show_usage() {
    println!("LZ4W packer v1.43 by Stephane Dallongeville (Copyright 2020)");
    println!("  Pack:     lz4w p <input_file> <output_file>");
    println!("            lz4w p <prev_file>@<input_file> <output_file>");
    println!("  Unpack:   lz4w u <input_file> <output_file>");
    println!("            lz4w u <prev_file>@<input_file> <output_file>");
    println!();
    println!("Tip: using an extra parameter after <output_file> will act as 'silent mode' switch");
}

fn main() -> Result<()> {
    let args: Vec<String> = std::env::args().collect();

    if args.len() < 3 {
        show_usage();
        std::process::exit(2);
    }

    let cmd = args[1].to_lowercase();
    if cmd != "p" && cmd != "u" {
        show_usage();
        std::process::exit(2);
    }

    let input_arg = &args[2];

    // Check for prev_file@input_file format
    let (prev_file, input_file) = if let Some(sep) = input_arg.find('@') {
        (
            input_arg[..sep].to_string(),
            input_arg[sep + 1..].to_string(),
        )
    } else {
        (String::new(), input_arg.clone())
    };

    let output_file = if args.len() > 3 {
        args[3].clone()
    } else if cmd == "p" {
        "packed.dat".to_string()
    } else {
        "unpacked.dat".to_string()
    };

    let silent = args.len() > 4;

    // Read data
    let data1 = if prev_file.is_empty() {
        Vec::new()
    } else {
        lz4w::pad_to_word(
            &fs::read(&prev_file).with_context(|| format!("Failed to read {}", prev_file))?,
        )
    };

    let data2 = fs::read(&input_file).with_context(|| format!("Failed to read {}", input_file))?;

    // Concatenate
    let mut data = Vec::with_capacity(data1.len() + data2.len());
    data.extend_from_slice(&data1);
    data.extend_from_slice(&data2);

    let result = if cmd == "p" {
        if !silent {
            println!("Packing {}...", input_file);
        }

        let packed = match lz4w::pack(&data, data1.len(), silent) {
            Ok(r) => r,
            Err(_) => {
                // Retry without previous data block
                lz4w::pack(&data2, 0, silent).map_err(|e| anyhow::anyhow!(e))?
            }
        };

        // Verify compression
        let mut full_packed = Vec::with_capacity(data1.len() + packed.len());
        full_packed.extend_from_slice(&data1);
        full_packed.extend_from_slice(&packed);

        let unpacked = lz4w::unpack(&full_packed, data1.len());

        if !silent {
            println!(
                "Initial size {} --> packed to {} ({}%)",
                data2.len(),
                packed.len(),
                (100 * packed.len()) / data2.len()
            );
        }

        if data2 != unpacked {
            bail!("Error while verifying compression, result data mismatch input data !");
        }

        packed
    } else {
        if !silent {
            println!("Unpacking {}...", input_file);
        }

        let unpacked = lz4w::unpack(&data, data1.len());

        if !silent {
            println!(
                "Initial size {} --> unpacked to {}",
                data2.len(),
                unpacked.len()
            );
        }

        unpacked
    };

    fs::write(&output_file, &result)
        .with_context(|| format!("Failed to write {}", output_file))?;

    Ok(())
}
