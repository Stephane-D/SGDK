use std::fs;
use std::process;

use clap::{Arg, Command};

use xgmtool::*;

const VERSION: &str = "1.76-rs";

fn main() {
    let matches = Command::new("xgmtool")
        .version(VERSION)
        .author("Stephane Dallongeville - copyright 2024")
        .about("XGMTool - VGM/XGM/XGC converter for Sega Megadrive")
        .after_help(
            "XGMTool can do the following operations:\n\
             - Optimize and reduce size of Sega Megadrive VGM file\n\
             - Convert a Sega Megadrive VGM file to XGM/ZGM/XGC file\n\
             - Convert a XGM file to VGM or compile to XGC\n\
             - Convert a XGC file to XGM or VGM (experimental)")
        .arg(Arg::new("input")
            .help("Input file (VGM, XGM or XGC)")
            .required(true)
            .index(1))
        .arg(Arg::new("output")
            .help("Output file (VGM, XGM, XGC, BIN or ZGM)")
            .required(true)
            .index(2))
        .arg(Arg::new("silent")
            .short('s')
            .long("silent")
            .help("Enable silent mode (no message except error and warning)")
            .action(clap::ArgAction::SetTrue))
        .arg(Arg::new("verbose")
            .short('v')
            .long("verbose")
            .help("Enable verbose mode")
            .action(clap::ArgAction::SetTrue))
        .arg(Arg::new("ntsc")
            .short('n')
            .long("ntsc")
            .help("Force NTSC timing")
            .action(clap::ArgAction::SetTrue))
        .arg(Arg::new("pal")
            .short('p')
            .long("pal")
            .help("Force PAL timing")
            .action(clap::ArgAction::SetTrue))
        .arg(Arg::new("disable-sample-ignore")
            .long("di")
            .help("Disable PCM sample auto ignore")
            .action(clap::ArgAction::SetTrue))
        .arg(Arg::new("disable-rate-fix")
            .long("dr")
            .help("Disable PCM sample rate auto fix")
            .action(clap::ArgAction::SetTrue))
        .arg(Arg::new("disable-delay-keyoff")
            .long("dd")
            .help("Disable delayed KEY OFF event")
            .action(clap::ArgAction::SetTrue))
        .arg(Arg::new("keep-rf5c68")
            .short('r')
            .long("keep-rf5c68")
            .help("Keep RF5C68 and RF5C164 register write commands")
            .action(clap::ArgAction::SetTrue))
        .get_matches();

    // Set globals
    let silent = matches.get_flag("silent");
    let verbose = matches.get_flag("verbose");

    set_silent(silent);
    set_verbose(if silent { false } else { verbose });
    set_sample_ignore(!matches.get_flag("disable-sample-ignore"));
    set_sample_rate_fix(!matches.get_flag("disable-rate-fix"));
    set_delay_key_off(!matches.get_flag("disable-delay-keyoff"));
    set_keep_rf5c68_cmds(matches.get_flag("keep-rf5c68"));

    if matches.get_flag("ntsc") {
        set_sys(SYSTEM_NTSC);
    } else if matches.get_flag("pal") {
        set_sys(SYSTEM_PAL);
    } else {
        set_sys(SYSTEM_AUTO);
    }

    let input_path = matches.get_one::<String>("input").unwrap();
    let output_path = matches.get_one::<String>("output").unwrap();

    // Verify input file exists
    if !std::path::Path::new(input_path).exists() {
        eprintln!("Error: the source file {} could not be opened", input_path);
        process::exit(2);
    }

    let in_ext = get_file_extension(input_path);
    let out_ext = get_file_extension(output_path);

    let err_code = match in_ext.as_str() {
        "VGM" | "" => process_vgm(input_path, output_path, &out_ext),
        "XGM" => process_xgm(input_path, output_path, &out_ext),
        "XGC" => process_xgc(input_path, output_path, &out_ext),
        _ => {
            eprintln!("Error: the input file {} is incorrect (should be a VGM, XGM or XGC file)", input_path);
            4
        }
    };

    process::exit(err_code);
}

fn process_vgm(input_path: &str, output_path: &str, out_ext: &str) -> i32 {
    match out_ext {
        "VGM" | "XGM" | "BIN" | "XGC" | "ZGM" => {}
        _ => {
            eprintln!("Error: the output file {} is incorrect (should be a VGM, XGM or BIN/XGC file)", output_path);
            return 4;
        }
    }

    let mut in_data = match fs::read(input_path) {
        Ok(d) => d,
        Err(e) => { eprintln!("Error reading {}: {}", input_path, e); return 1; }
    };

    // Force timing if requested
    if get_sys() == SYSTEM_NTSC {
        in_data[0x24] = 60;
    } else if get_sys() == SYSTEM_PAL {
        in_data[0x24] = 50;
    }

    let data_len = in_data.len();
    let mut vgm_obj = match vgm::VGM::create(in_data, data_len, 0, true) {
        Some(v) => v,
        None => { eprintln!("Error: failed to parse VGM file"); return 1; }
    };
    vgm_obj.convert_waits();
    vgm_obj.clean_commands();
    vgm_obj.clean_samples();
    vgm_obj.fix_key_commands();

    match out_ext {
        "VGM" => {
            let out_data = vgm_obj.as_byte_array();
            write_output(output_path, &out_data)
        }
        "ZGM" => {
            let (stream, data_blocks) = vgm_obj.as_byte_array2();
            let compressed = match compress::lz77c_compress_buf(&stream) {
                Some(c) => c,
                None => { eprintln!("Error: compression failed"); return 1; }
            };
            let mut out = compressed;
            out.extend_from_slice(&data_blocks);
            write_output(output_path, &out)
        }
        _ => {
            // XGM, BIN, XGC
            let xgm_obj = xgm::XGM::create_from_vgm(&vgm_obj);
            if out_ext == "XGM" {
                let out_data = xgm_obj.as_byte_array();
                write_output(output_path, &out_data)
            } else {
                let xgc_obj = xgc::create(&xgm_obj);
                let out_data = xgc::as_byte_array(&xgc_obj);
                write_output(output_path, &out_data)
            }
        }
    }
}

fn process_xgm(input_path: &str, output_path: &str, out_ext: &str) -> i32 {
    match out_ext {
        "VGM" | "BIN" | "XGC" => {}
        _ => {
            eprintln!("Error: the output file {} is incorrect (should be a VGM or BIN/XGC file)", output_path);
            return 4;
        }
    }

    let in_data = match fs::read(input_path) {
        Ok(d) => d,
        Err(e) => { eprintln!("Error reading {}: {}", input_path, e); return 1; }
    };

    let xgm_obj = match xgm::XGM::create_from_data(&in_data, in_data.len()) {
        Some(x) => x,
        None => { eprintln!("Error: failed to parse XGM file"); return 1; }
    };

    match out_ext {
        "VGM" => {
            let vgm_obj = vgm::VGM::create_from_xgm(&xgm_obj);
            let out_data = vgm_obj.as_byte_array();
            write_output(output_path, &out_data)
        }
        _ => {
            let xgc_obj = xgc::create(&xgm_obj);
            let out_data = xgc::as_byte_array(&xgc_obj);
            write_output(output_path, &out_data)
        }
    }
}

fn process_xgc(input_path: &str, output_path: &str, out_ext: &str) -> i32 {
    match out_ext {
        "VGM" | "XGM" => {}
        _ => {
            eprintln!("Error: the output file {} is incorrect (should be a XGM or VGM file)", output_path);
            return 4;
        }
    }

    let in_data = match fs::read(input_path) {
        Ok(d) => d,
        Err(e) => { eprintln!("Error reading {}: {}", input_path, e); return 1; }
    };

    let xgm_obj = match xgm::XGM::create_from_xgc_data(&in_data, in_data.len()) {
        Some(x) => x,
        None => { eprintln!("Error: failed to parse XGC file"); return 1; }
    };

    match out_ext {
        "VGM" => {
            let vgm_obj = vgm::VGM::create_from_xgm(&xgm_obj);
            let out_data = vgm_obj.as_byte_array();
            write_output(output_path, &out_data)
        }
        _ => {
            let out_data = xgm_obj.as_byte_array();
            write_output(output_path, &out_data)
        }
    }
}

fn write_output(path: &str, data: &[u8]) -> i32 {
    match fs::write(path, data) {
        Ok(_) => 0,
        Err(e) => {
            eprintln!("Error writing {}: {}", path, e);
            1
        }
    }
}
