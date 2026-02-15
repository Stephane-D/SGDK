/// ConvSym utility version 2.12 - Rust implementation.
/// Symbol table converter for SGDK.

use anyhow::Result;
use regex::Regex;

use convsym::input;
use convsym::output;
use convsym::symbol_table::{OffsetConversionOptions, SymbolTable, SymbolToOffsetResolveTable};

fn print_help() {
    println!(
        "ConvSym utility version 2.12.1 (Rust)\n\
        (c) 2016-2024, vladikcomper\n\
        \n\
        Command line arguments:\n\
        \x20 convsym [input_file|-] [output_file|-] <options>\n\
        \n\
        Using \"-\" as a file name redirects I/O to stdin or stdout respectively.\n\
        \n\
        EXAMPLES:\n\
        \x20 convsym listing.lst symbols.log -input as_lst -output log\n\
        \x20 convsym listing.lst rom.bin -input as_lst -output deb2 -a\n\
        \n\
        OPTIONS:\n\
        \x20 -in [format]\n\
        \x20 -input [format]\n\
        \x20   Selects input file format. Supported formats: asm68k_sym, asm68k_lst, as_lst, as_lst_exp, log, txt\n\
        \x20   Default: asm68k_sym\n\
        \n\
        \x20 -out [format]\n\
        \x20 -output [format]\n\
        \x20   Selects output file format. Supported formats: asm, deb1, deb2, log\n\
        \x20   Default: deb2\n\
        \n\
        \x20 -inopt [options]\n\
        \x20   Additional options specific for the input format.\n\
        \n\
        \x20 -outopt [options]\n\
        \x20   Additional options specific for the output format.\n\
        \n\
        Offsets conversion options:\n\
        \x20 -base [offset]     Sets base offset (subtracted from every symbol offset). Default: 0\n\
        \x20 -mask [offset]     Sets mask for offsets. Default: FFFFFF\n\
        \x20 -range [bottom] [upper]  Offset range filter. Default: 0 3FFFFF\n\
        \x20 -a                 Append mode: append to end of output file.\n\
        \x20 -noalign           Don't align on append.\n\
        \n\
        Symbol table dump options:\n\
        \x20 -org [offset|@sym]   Place data at specified offset (or resolved symbol).\n\
        \x20 -ref [offset|@sym]   Write 32-bit BE pointer at offset.\n\
        \n\
        Symbols conversion and filtering options:\n\
        \x20 -toupper           Convert all symbol names to uppercase.\n\
        \x20 -tolower           Convert all symbol names to lowercase.\n\
        \x20 -addprefix [str]   Prepend prefix to every symbol.\n\
        \x20 -filter [regex]    Filter symbols by regex.\n\
        \x20 -exclude           Exclude matching symbols (used with -filter).\n\
        \x20 -debug             Enable debug output.\n"
    );
}

fn main() -> Result<()> {
    let args: Vec<String> = std::env::args().collect();

    if args.len() < 3 {
        print_help();
        std::process::exit(-1);
    }

    // Default configuration
    let mut opt_append = false;
    let mut opt_debug = false;
    let mut opt_filter_exclude = false;
    let mut opt_no_align_on_append = false;
    let mut opt_to_upper = false;
    let mut opt_to_lower = false;

    let mut offset_opts = OffsetConversionOptions::default();

    let mut input_wrapper_name = String::from("asm68k_sym");
    let mut output_wrapper_name = String::from("deb2");
    let mut input_opts = String::new();
    let mut output_opts = String::new();
    let mut append_offset_raw = String::new();
    let mut pointer_offset_raw = String::new();
    let mut filter_regex_str = String::new();
    let mut prefix_str = String::new();

    let input_file_name = &args[1];
    let output_file_name = &args[2];

    // Parse remaining arguments (custom argument parser to match C++ behavior exactly)
    let mut i = 3;
    while i < args.len() {
        let arg = &args[i];
        match arg.as_str() {
            "-base" => {
                i += 1;
                if i < args.len() {
                    offset_opts.base_offset = u32::from_str_radix(&args[i], 16)
                        .unwrap_or_else(|_| {
                            eprintln!("Fatal: Couldn't parse hex number: {}", &args[i]);
                            std::process::exit(-1);
                        });
                }
            }
            "-mask" => {
                i += 1;
                if i < args.len() {
                    offset_opts.offset_mask = u32::from_str_radix(&args[i], 16)
                        .unwrap_or_else(|_| {
                            eprintln!("Fatal: Couldn't parse hex number: {}", &args[i]);
                            std::process::exit(-1);
                        });
                }
            }
            "-range" => {
                i += 1;
                if i < args.len() {
                    offset_opts.offset_left_boundary = u32::from_str_radix(&args[i], 16)
                        .unwrap_or_else(|_| {
                            eprintln!("Fatal: Couldn't parse hex number: {}", &args[i]);
                            std::process::exit(-1);
                        });
                }
                i += 1;
                if i < args.len() {
                    offset_opts.offset_right_boundary = u32::from_str_radix(&args[i], 16)
                        .unwrap_or_else(|_| {
                            eprintln!("Fatal: Couldn't parse hex number: {}", &args[i]);
                            std::process::exit(-1);
                        });
                }
            }
            "-a" => opt_append = true,
            "-noalign" => opt_no_align_on_append = true,
            "-debug" => opt_debug = true,
            "-in" | "-input" => {
                i += 1;
                if i < args.len() {
                    input_wrapper_name = args[i].clone();
                }
            }
            "-inopt" => {
                i += 1;
                if i < args.len() {
                    input_opts = args[i].clone();
                }
            }
            "-out" | "-output" => {
                i += 1;
                if i < args.len() {
                    output_wrapper_name = args[i].clone();
                }
            }
            "-outopt" => {
                i += 1;
                if i < args.len() {
                    output_opts = args[i].clone();
                }
            }
            "-org" => {
                i += 1;
                if i < args.len() {
                    append_offset_raw = args[i].clone();
                }
            }
            "-ref" => {
                i += 1;
                if i < args.len() {
                    pointer_offset_raw = args[i].clone();
                }
            }
            "-filter" => {
                i += 1;
                if i < args.len() {
                    filter_regex_str = args[i].clone();
                }
            }
            "-exclude" => opt_filter_exclude = true,
            "-addprefix" => {
                i += 1;
                if i < args.len() {
                    prefix_str = args[i].clone();
                }
            }
            "-toupper" => opt_to_upper = true,
            "-tolower" => opt_to_lower = true,
            _ => {
                eprintln!("Warning: Unknown parameter: {}", arg);
            }
        }
        i += 1;
    }

    // Apply configuration
    if opt_append {
        if !append_offset_raw.is_empty() {
            eprintln!("Warning: Using conflicting parameters: -a and -org. The -org parameter has no effect");
            append_offset_raw.clear();
        }
    }
    if opt_filter_exclude && filter_regex_str.is_empty() {
        eprintln!("Warning: Using -exclude without -filter. The -exclude parameter has no effect");
        opt_filter_exclude = false;
    }
    if opt_to_upper && opt_to_lower {
        eprintln!("Warning: Using conflicting parameters: -toupper and -tolower. The -toupper parameter has no effect");
        opt_to_upper = false;
    }

    // Parse offset parameters, or set up symbol resolution
    let mut append_offset: i64 = if opt_append { -1 } else { 0 };
    let mut pointer_offset: u32 = 0;
    let mut resolve_table = SymbolToOffsetResolveTable::new();

    // Handle -ref
    if !pointer_offset_raw.is_empty() {
        if pointer_offset_raw.starts_with('@') {
            let sym_name = pointer_offset_raw[1..].to_string();
            resolve_table.insert(sym_name, u32::MAX - 1); // sentinel
        } else {
            pointer_offset = u32::from_str_radix(&pointer_offset_raw, 16).map_err(|_| {
                anyhow::anyhow!(
                    "Couldn't parse hex number in parameters: {}",
                    pointer_offset_raw
                )
            })?;
        }
    }

    // Handle -org
    if !append_offset_raw.is_empty() {
        if append_offset_raw.starts_with('@') {
            let sym_name = append_offset_raw[1..].to_string();
            resolve_table.insert(sym_name, u32::MAX - 1); // sentinel
            append_offset = -2; // marker for "resolve later"
        } else {
            append_offset = i64::from(
                u32::from_str_radix(&append_offset_raw, 16).map_err(|_| {
                    anyhow::anyhow!(
                        "Couldn't parse hex number in parameters: {}",
                        append_offset_raw
                    )
                })?,
            );
        }
    }

    // Retrieve symbols from input
    let mut symbol_table = SymbolTable::new(offset_opts, resolve_table);
    symbol_table.debug = opt_debug;

    let input_parser = input::get_input_parser(&input_wrapper_name)?;
    input_parser.parse(&mut symbol_table, input_file_name, &input_opts)?;

    // Check that all referenced symbols have been resolved
    for (label, offset_val) in &symbol_table.resolve_table {
        if *offset_val == u32::MAX - 1 {
            eprintln!("Fatal: Couldn't resolve symbol \"{}\"", label);
            std::process::exit(-2);
        }
    }

    // Resolve -ref @Symbol
    if !pointer_offset_raw.is_empty() && pointer_offset_raw.starts_with('@') {
        let sym_name = &pointer_offset_raw[1..];
        if let Some(&val) = symbol_table.resolve_table.get(sym_name) {
            pointer_offset = val;
        }
    }

    // Resolve -org @Symbol
    if append_offset == -2 && !append_offset_raw.is_empty() && append_offset_raw.starts_with('@')
    {
        let sym_name = &append_offset_raw[1..];
        if let Some(&val) = symbol_table.resolve_table.get(sym_name) {
            append_offset = val as i64;
        }
    }

    // Apply transformations
    if opt_to_upper {
        symbol_table.to_upper();
        filter_regex_str = filter_regex_str.to_uppercase();
    }
    if opt_to_lower {
        symbol_table.to_lower();
        filter_regex_str = filter_regex_str.to_lowercase();
    }
    if !prefix_str.is_empty() {
        symbol_table.add_prefix(&prefix_str);
    }

    // Apply regex filter
    if !filter_regex_str.is_empty() {
        let regex = Regex::new(&filter_regex_str).map_err(|e| {
            anyhow::anyhow!("Invalid filter regex '{}': {}", filter_regex_str, e)
        })?;
        symbol_table.filter_regex(&regex, opt_filter_exclude);
    }

    // Generate output
    if !symbol_table.is_empty() {
        let symbols = symbol_table.to_multimap();
        let output_gen = output::get_output_generator(&output_wrapper_name)?;
        output_gen.generate(
            &symbols,
            output_file_name,
            append_offset,
            pointer_offset,
            &output_opts,
            !opt_no_align_on_append,
        )?;
    } else {
        eprintln!("Error: No symbols passed for output, operation aborted");
        std::process::exit(-3);
    }

    Ok(())
}
