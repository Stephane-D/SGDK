use std::env;
use std::path::PathBuf;
use sjasm::assembler::{Assembler, OutputMode};

const VERSION: &str = "SjASM Z80 Assembler v0.39j-rs";

const ERR_NO_INPUT: i32 = 1;
const ERR_BAD_CODE: i32 = 2;

fn main() {
    let args: Vec<String> = env::args().collect();

    let mut asm = Assembler::new();
    asm.listfile = true;

    if args.len() < 2 {
        println!("{}", VERSION);
        println!("Copyright 2006 Sjoerd Mastijn - www.xl2s.tk");
        println!("Copyright 2022 Konamiman - www.konamiman.com");
        println!("Rust port");
        println!();
        println!("Usage:");
        println!("sjasm [-options] sourcefile [targetfile [listfile [exportfile]]]");
        println!();
        println!("Option flags as follows:");
        println!("  -l        Label table in listing");
        println!("  -s        Generate .SYM symbol file");
        println!("  -q        No listing");
        println!("  -i<path>  Includepath");
        println!("  -e        Send errors to standard error pipe");
        println!("  -c        Enable Compass compatibility");
        println!("  -v        Produce error messages with Visual Studio format");
        println!("            (should be the first option)");
        std::process::exit(ERR_NO_INPUT);
    }

    println!("{}", VERSION);

    // Parse args: options intermixed with positional args
    let mut positional = Vec::new();
    let mut i = 1;
    while i < args.len() {
        let arg = &args[i];
        if arg.starts_with('-') {
            let chars: Vec<char> = arg[1..].chars().collect();
            let mut j = 0;
            while j < chars.len() {
                match chars[j].to_ascii_lowercase() {
                    'q' => asm.listfile = false,
                    's' => asm.symfile = true,
                    'l' => asm.labellisting = true,
                    'e' => asm.err_to_stderr = true,
                    'c' => asm.compass_compat = true,
                    'v' => asm.use_vs_error_format = true,
                    'i' => {
                        let path: String = chars[j + 1..].iter().collect();
                        if !path.is_empty() {
                            asm.include_dirs.push(path);
                        }
                        j = chars.len(); // consume rest
                        continue;
                    }
                    c => {
                        eprintln!("Unrecognised option: {}", c);
                        std::process::exit(ERR_NO_INPUT);
                    }
                }
                j += 1;
            }
        } else {
            positional.push(arg.clone());
        }
        i += 1;
    }

    // Assign positional args
    if let Some(src) = positional.get(0) {
        asm.sourcefilename = src.clone();
    } else {
        eprintln!("No inputfile");
        std::process::exit(ERR_NO_INPUT);
    }
    if let Some(dst) = positional.get(1) {
        asm.destfilename = dst.clone();
    }
    if let Some(lst) = positional.get(2) {
        asm.listfilename = lst.clone();
    }
    if let Some(exp) = positional.get(3) {
        asm.expfilename = exp.clone();
    }

    // Derive default filenames
    if asm.destfilename.is_empty() {
        asm.destfilename = change_extension(&asm.sourcefilename, "out");
    }
    if asm.listfilename.is_empty() {
        asm.listfilename = change_extension(&asm.sourcefilename, "lst");
    }
    if asm.symfilename.is_empty() {
        asm.symfilename = change_extension(&asm.sourcefilename, "sym");
    }
    if asm.expfilename.is_empty() {
        asm.expfilename = change_extension(&asm.sourcefilename, "exp");
    }

    // Initialize tables
    asm.insert_directives();
    asm.init_z80();

    // Pass 1
    asm.init_pass(1);
    asm.open_list();
    asm.open_file(&asm.sourcefilename.clone());
    println!("Pass 1 complete ({} errors)", asm.nerror);

    // Pass 2
    asm.init_pass(2);
    asm.open_dest(OutputMode::Truncate);
    asm.open_file(&asm.sourcefilename.clone());

    if asm.labellisting {
        if let Some(ref mut fp) = asm.list_fp {
            asm.labtab.dump(fp);
        }
    }

    println!("Pass 2 complete");

    asm.close();

    if asm.symfile {
        asm.labtab.dumpsym(&asm.symfilename);
    }

    println!("Errors: {}", asm.nerror);

    std::process::exit(if asm.nerror == 0 { 0 } else { ERR_BAD_CODE });
}

fn change_extension(filename: &str, ext: &str) -> String {
    let p = PathBuf::from(filename);
    p.with_extension(ext).to_string_lossy().into_owned()
}
