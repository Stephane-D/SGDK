//! APJ CLI - aPLib Pack/Unpack tool for SGDK
//!
//! Usage:
//!   apj p <input_file> <output_file>       - Pack (fast mode)
//!   apj pp <input_file> <output_file>      - Pack (ultra/optimal mode)
//!   apj u <input_file> <output_file>       - Unpack

use anyhow::{bail, Context, Result};
use std::fs;
use std::path::Path;

fn show_usage() {
    println!("APJ (ApLib for Rust) packer v1.32 by Stephane Dallongeville (Copyright 2021)");
    println!("  Pack:       apj p <input_file> <output_file>");
    println!("  Pack max:   apj pp <input_file> <output_file>");
    println!("  Unpack:     apj u <input_file> <output_file>");
    println!();
    println!("Tip: using an extra parameter after <output_file> will act as 'silent mode' switch");
}

fn valid_input(args: &[String]) -> bool {
    args.len() >= 4
        && (args[1] == "p" || args[1] == "pp" || args[1] == "u")
        && Path::new(&args[2]).exists()
        && args[2] != args[3]
}

fn main() -> Result<()> {
    let args: Vec<String> = std::env::args().collect();

    if !valid_input(&args) {
        show_usage();
        std::process::exit(1);
    }

    let command = &args[1];
    let load_name = &args[2];
    let save_name = &args[3];
    let silent = args.len() > 4;

    let input = fs::read(load_name).with_context(|| format!("Failed to read {}", load_name))?;

    let output = if command == "p" || command == "pp" {
        let ultra = command == "pp";

        let start = std::time::Instant::now();
        let packed = apj::pack(&input, ultra, silent);
        let elapsed = start.elapsed();

        // Verify compression
        let unpacked = apj::unpack(&packed, Some(&input));
        if input != unpacked {
            bail!("Error while verifying compression, result data mismatch input data !");
        }

        if !silent {
            println!(
                "Initial size {} --> packed to {} ({}%) in {} ms",
                input.len(),
                packed.len(),
                (100 * packed.len()) / input.len(),
                elapsed.as_millis()
            );
        }

        packed
    } else {
        let start = std::time::Instant::now();
        let unpacked = apj::unpack(&input, None);
        let elapsed = start.elapsed();

        if !silent {
            println!(
                "Initial size {} --> unpacked to {} in {} ms",
                input.len(),
                unpacked.len(),
                elapsed.as_millis()
            );
        }

        unpacked
    };

    fs::write(save_name, &output).with_context(|| format!("Failed to write {}", save_name))?;

    std::process::exit(0);
}
