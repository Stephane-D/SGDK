//! SGDK Resource Compiler (rescomp)
//!
//! Compiles .res resource files into 68000 assembly (.s) and C header (.h) files.

use std::env;
use std::process;

fn main() {
    let args: Vec<String> = env::args().collect();

    if args.len() < 2 {
        eprintln!("ResComp 4.30 - SGDK Resource Compiler");
        eprintln!("  Usage: rescomp input [output] [-noasm] [-noheader] [-dep target]");
        process::exit(1);
    }

    let mut input = String::new();
    let mut output = String::new();
    let mut do_asm = true;
    let mut do_header = true;
    let mut dep_target: Option<String> = None;

    let mut i = 1;
    while i < args.len() {
        match args[i].as_str() {
            "-noasm" => do_asm = false,
            "-noheader" => do_header = false,
            "-dep" => {
                i += 1;
                if i < args.len() {
                    dep_target = Some(args[i].clone());
                } else {
                    eprintln!("Error: -dep requires a target argument");
                    process::exit(1);
                }
            }
            _ => {
                if input.is_empty() {
                    input = args[i].clone();
                } else if output.is_empty() {
                    output = args[i].clone();
                }
            }
        }
        i += 1;
    }

    if input.is_empty() {
        eprintln!("Error: no input file specified");
        process::exit(1);
    }

    // Default output: same as input with .s extension
    if output.is_empty() {
        if let Some(dot) = input.rfind('.') {
            output = format!("{}.s", &input[..dot]);
        } else {
            output = format!("{}.s", input);
        }
    }

    let mut compiler = rescomp::compiler::Compiler::new();
    let success = compiler.compile(
        &input,
        &output,
        do_asm,
        do_header,
        dep_target.as_deref(),
    );

    if !success {
        process::exit(1);
    }
}
