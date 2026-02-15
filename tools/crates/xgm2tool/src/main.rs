use std::path::Path;
use std::process;

use clap::Parser;
use xgm2tool::util;
use xgm2tool::vgm::VGM;
use xgm2tool::xgm::XGM;
use xgm2tool::xgm_multi::XGMMulti;

const VERSION: &str = "1.07";

use xgm2tool::{SYSTEM_AUTO, SYSTEM_NTSC, SYSTEM_PAL};

#[derive(Parser)]
#[command(name = "xgm2tool")]
#[command(about = "XGM2 Tool - VGM/XGM/XGC format converter", version = VERSION)]
struct Cli {
    /// Input/output files (last file is output)
    #[arg(required = true)]
    files: Vec<String>,

    /// Silent mode (no message except error and warning)
    #[arg(short = 's')]
    silent: bool,

    /// Verbose mode (give more info about conversion)
    #[arg(short = 'v')]
    verbose: bool,

    /// Force NTSC timing
    #[arg(short = 'n')]
    ntsc: bool,

    /// Force PAL timing
    #[arg(short = 'p')]
    pal: bool,

    /// Disable PCM sample auto ignore
    #[arg(long = "di")]
    disable_sample_ignore: bool,

    /// Disable PCM sample rate auto fix
    #[arg(long = "dr")]
    disable_sample_rate_fix: bool,

    /// Disable delayed KEY OFF event
    #[arg(long = "dd")]
    disable_delay_key_off: bool,

    /// Enable fingerprint compare on PCM merging
    #[arg(long = "ac")]
    advanced_compare: bool,
}

fn load_vgm(file: &str, sys: i32) -> VGM {
    let mut data = util::read_binary_file(file).unwrap_or_else(|e| {
        eprintln!("Error reading file '{}': {}", file, e);
        process::exit(1);
    });

    if sys == SYSTEM_NTSC {
        if data.len() > 0x24 {
            data[0x24] = 60;
        }
    } else if sys == SYSTEM_PAL {
        if data.len() > 0x24 {
            data[0x24] = 50;
        }
    }

    VGM::from_data(&data, true).unwrap_or_else(|e| {
        eprintln!("Error parsing VGM file '{}': {}", file, e);
        process::exit(1);
    })
}

fn load_xgm(file: &str) -> XGM {
    let data = util::read_binary_file(file).unwrap_or_else(|e| {
        eprintln!("Error reading file '{}': {}", file, e);
        process::exit(1);
    });

    XGM::from_data(&data).unwrap_or_else(|e| {
        eprintln!("Error parsing XGM file '{}': {}", file, e);
        process::exit(1);
    })
}

fn main() {
    let cli = Cli::parse();

    // Set globals
    xgm2tool::set_silent(cli.silent);
    xgm2tool::set_verbose(cli.verbose);

    // silent overrides verbose
    if cli.silent {
        xgm2tool::set_verbose(false);
    }

    let sys = if cli.ntsc {
        SYSTEM_NTSC
    } else if cli.pal {
        SYSTEM_PAL
    } else {
        SYSTEM_AUTO
    };

    xgm2tool::set_sys(sys);
    xgm2tool::set_sample_rate_fix(!cli.disable_sample_rate_fix);
    xgm2tool::set_sample_ignore(!cli.disable_sample_ignore);
    xgm2tool::set_sample_advanced_compare(cli.advanced_compare);
    xgm2tool::set_delay_key_off(!cli.disable_delay_key_off);

    let files = &cli.files;
    if files.len() < 2 {
        eprintln!("Error: at least 2 file arguments required (input and output)");
        process::exit(2);
    }

    let out_file = &files[files.len() - 1];
    let out_ext = util::get_file_extension(out_file);

    // Multi-track mode
    if files.len() > 2 {
        for i in 0..files.len() - 1 {
            let file = &files[i];
            if !Path::new(file).exists() {
                eprintln!("Error: the source file '{}' could not be found", file);
                process::exit(2);
            }
            let ext = util::get_file_extension(file);
            if !ext.is_empty() && ext != "VGM" {
                eprintln!(
                    "Error: expected VGM file as input for multi tracks conversion (found {})",
                    ext
                );
                process::exit(2);
            }
        }

        if out_ext != "XGM" && out_ext != "XGC" {
            eprintln!(
                "Error: expected XGM or XGC file as output for multi tracks conversion (found {})",
                out_ext
            );
            process::exit(2);
        }

        let pack = out_ext == "XGC";
        let mut xgms = Vec::new();
        for i in 0..files.len() - 1 {
            let vgm = load_vgm(&files[i], sys);
            let xgm = XGM::from_vgm(&vgm, pack).unwrap_or_else(|e| {
                eprintln!("Error converting VGM to XGM: {}", e);
                process::exit(1);
            });
            xgms.push(xgm);
        }

        let mut multi = XGMMulti::new(xgms, pack);
        let out_data = multi.as_byte_array().unwrap_or_else(|e| {
            eprintln!("Error writing XGM multi: {}", e);
            process::exit(1);
        });

        util::write_binary_file(&out_data, out_file).unwrap_or_else(|e| {
            eprintln!("Error writing output file '{}': {}", out_file, e);
            process::exit(1);
        });

        process::exit(0);
    }

    // Single file conversion
    let in_file = &files[0];
    if !Path::new(in_file).exists() {
        eprintln!("Error: the source file '{}' could not be found", in_file);
        process::exit(2);
    }

    let in_ext = util::get_file_extension(in_file);

    if in_ext == "VGM" || in_ext.is_empty() {
        let vgm = load_vgm(in_file, sys);

        if out_ext == "VGM" {
            let data = vgm.as_byte_array().unwrap_or_else(|e| {
                eprintln!("Error serializing VGM: {}", e);
                process::exit(1);
            });
            util::write_binary_file(&data, out_file).unwrap_or_else(|e| {
                eprintln!("Error writing output file '{}': {}", out_file, e);
                process::exit(1);
            });
        } else {
            let pack = out_ext != "XGM";
            let mut xgm = XGM::from_vgm(&vgm, pack).unwrap_or_else(|e| {
                eprintln!("Error converting VGM to XGM: {}", e);
                process::exit(1);
            });
            let data = xgm.as_byte_array().unwrap_or_else(|e| {
                eprintln!("Error serializing XGM: {}", e);
                process::exit(1);
            });
            util::write_binary_file(&data, out_file).unwrap_or_else(|e| {
                eprintln!("Error writing output file '{}': {}", out_file, e);
                process::exit(1);
            });
        }
    } else if in_ext == "XGM" {
        let mut xgm = load_xgm(in_file);

        if out_ext == "VGM" {
            let vgm = VGM::from_xgm(&xgm).unwrap_or_else(|e| {
                eprintln!("Error converting XGM to VGM: {}", e);
                process::exit(1);
            });
            let data = vgm.as_byte_array().unwrap_or_else(|e| {
                eprintln!("Error serializing VGM: {}", e);
                process::exit(1);
            });
            util::write_binary_file(&data, out_file).unwrap_or_else(|e| {
                eprintln!("Error writing output file '{}': {}", out_file, e);
                process::exit(1);
            });
        } else {
            xgm.packed = out_ext == "XGC";
            let data = xgm.as_byte_array().unwrap_or_else(|e| {
                eprintln!("Error serializing XGM: {}", e);
                process::exit(1);
            });
            util::write_binary_file(&data, out_file).unwrap_or_else(|e| {
                eprintln!("Error writing output file '{}': {}", out_file, e);
                process::exit(1);
            });
        }
    } else if in_ext == "XGC" {
        let mut xgm = load_xgm(in_file);

        if out_ext == "VGM" {
            let vgm = VGM::from_xgm(&xgm).unwrap_or_else(|e| {
                eprintln!("Error converting XGM to VGM: {}", e);
                process::exit(1);
            });
            let data = vgm.as_byte_array().unwrap_or_else(|e| {
                eprintln!("Error serializing VGM: {}", e);
                process::exit(1);
            });
            util::write_binary_file(&data, out_file).unwrap_or_else(|e| {
                eprintln!("Error writing output file '{}': {}", out_file, e);
                process::exit(1);
            });
        } else if out_ext == "XGM" {
            xgm.packed = false;
            let data = xgm.as_byte_array().unwrap_or_else(|e| {
                eprintln!("Error serializing XGM: {}", e);
                process::exit(1);
            });
            util::write_binary_file(&data, out_file).unwrap_or_else(|e| {
                eprintln!("Error writing output file '{}': {}", out_file, e);
                process::exit(1);
            });
        } else {
            eprintln!(
                "Error: the output file '{}' is incorrect (should be a VGM or XGM file)",
                out_file
            );
            process::exit(4);
        }
    } else {
        eprintln!(
            "Error: the input file '{}' is incorrect (should be a VGM, XGM or XGC file)",
            in_file
        );
        process::exit(4);
    }
}
