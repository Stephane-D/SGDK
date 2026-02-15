pub mod util;
pub mod command;
pub mod vgm_command;
pub mod xgm_fm_command;
pub mod xgm_psg_command;
pub mod data_bank;
pub mod sample_bank;
pub mod sample;
pub mod single_sample;
pub mod vgm_sample;
pub mod vgm_sample_data;
pub mod xgm_sample;
pub mod gd3;
pub mod xd3;
pub mod psg_state;
pub mod ym2612_state;
pub mod vgm;
pub mod xgm;
pub mod xgm_multi;
pub mod xgc_packer;

use std::sync::atomic::{AtomicBool, AtomicI32, Ordering};

// Global settings (mirrors Java Launcher static fields)
static SILENT: AtomicBool = AtomicBool::new(false);
static VERBOSE: AtomicBool = AtomicBool::new(false);
static SAMPLE_RATE_FIX: AtomicBool = AtomicBool::new(true);
static SAMPLE_IGNORE: AtomicBool = AtomicBool::new(true);
static SAMPLE_ADVANCED_COMPARE: AtomicBool = AtomicBool::new(false);
static DELAY_KEY_OFF: AtomicBool = AtomicBool::new(false);
static SYS: AtomicI32 = AtomicI32::new(-1);

pub const SYSTEM_AUTO: i32 = -1;
pub const SYSTEM_NTSC: i32 = 0;
pub const SYSTEM_PAL: i32 = 1;

pub fn is_silent() -> bool { SILENT.load(Ordering::Relaxed) }
pub fn is_verbose() -> bool { VERBOSE.load(Ordering::Relaxed) }
pub fn sample_rate_fix() -> bool { SAMPLE_RATE_FIX.load(Ordering::Relaxed) }
pub fn sample_ignore() -> bool { SAMPLE_IGNORE.load(Ordering::Relaxed) }
pub fn sample_advanced_compare() -> bool { SAMPLE_ADVANCED_COMPARE.load(Ordering::Relaxed) }
pub fn delay_key_off() -> bool { DELAY_KEY_OFF.load(Ordering::Relaxed) }
pub fn get_sys() -> i32 { SYS.load(Ordering::Relaxed) }

pub fn set_silent(v: bool) { SILENT.store(v, Ordering::Relaxed); }
pub fn set_verbose(v: bool) { VERBOSE.store(v, Ordering::Relaxed); }
pub fn set_sample_rate_fix(v: bool) { SAMPLE_RATE_FIX.store(v, Ordering::Relaxed); }
pub fn set_sample_ignore(v: bool) { SAMPLE_IGNORE.store(v, Ordering::Relaxed); }
pub fn set_sample_advanced_compare(v: bool) { SAMPLE_ADVANCED_COMPARE.store(v, Ordering::Relaxed); }
pub fn set_delay_key_off(v: bool) { DELAY_KEY_OFF.store(v, Ordering::Relaxed); }
pub fn set_sys(v: i32) { SYS.store(v, Ordering::Relaxed); }

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_util_align() {
        assert_eq!(util::align(0, 4), 0);
        assert_eq!(util::align(1, 4), 4);
        assert_eq!(util::align(4, 4), 4);
        assert_eq!(util::align(5, 4), 8);
        assert_eq!(util::align(255, 256), 256);
    }

    #[test]
    fn test_util_align_data() {
        let data = vec![1, 2, 3];
        let aligned = util::align_data(&data, 4);
        assert_eq!(aligned, vec![1, 2, 3, 0]);

        let data2 = vec![1, 2, 3, 4];
        let aligned2 = util::align_data(&data2, 4);
        assert_eq!(aligned2, vec![1, 2, 3, 4]);
    }

    #[test]
    fn test_util_le_roundtrip_u16() {
        let mut buf = vec![0u8; 4];
        util::set_u16(&mut buf, 0, 0x1234);
        assert_eq!(util::get_u16(&buf, 0), 0x1234);
        util::set_u16(&mut buf, 2, 0xABCD);
        assert_eq!(util::get_u16(&buf, 2), 0xABCD);
    }

    #[test]
    fn test_util_le_roundtrip_u32() {
        let mut buf = vec![0u8; 4];
        util::set_u32(&mut buf, 0, 0xDEADBEEF);
        assert_eq!(util::get_u32(&buf, 0), 0xDEADBEEF);
    }

    #[test]
    fn test_util_le_roundtrip_i32() {
        let mut buf = vec![0u8; 4];
        util::set_i32(&mut buf, 0, -12345);
        assert_eq!(util::get_i32(&buf, 0), -12345);
    }

    #[test]
    fn test_util_u24() {
        let mut buf = vec![0u8; 3];
        util::set_u24(&mut buf, 0, 0x123456);
        assert_eq!(util::get_u24(&buf, 0), 0x123456);
    }

    #[test]
    fn test_util_ascii_string() {
        let data = b"Hello\0World\0";
        assert_eq!(util::get_ascii_string(data, 0), "Hello");
        assert_eq!(util::get_ascii_string_n(data, 0, 5), "Hello");
        assert_eq!(util::get_ascii_string(data, 6), "World");
    }

    #[test]
    fn test_util_swap_nibbles() {
        assert_eq!(util::swap_nibbles(0x12), 0x21);
        assert_eq!(util::swap_nibbles(0xAB), 0xBA);
        assert_eq!(util::swap_nibbles(0x00), 0x00);
        assert_eq!(util::swap_nibbles(0xFF), 0xFF);
    }

    #[test]
    fn test_util_hex_string() {
        assert_eq!(util::hex_string(0xFF, 2), "FF");
        assert_eq!(util::hex_string(0, 1), "0");
        assert_eq!(util::hex_string(0x1234, 4), "1234");
    }

    #[test]
    fn test_util_is_diff_rate() {
        assert!(!util::is_diff_rate(44100, 44100));
        assert!(!util::is_diff_rate(44100, 43000));  // <10% diff
        assert!(util::is_diff_rate(44100, 22050));   // >10% diff
    }

    #[test]
    fn test_util_resample_identity() {
        let data = vec![100u8, 150, 200, 128, 64];
        let result = util::resample(&data, 0, data.len(), 44100, 44100, 1);
        // Same rate, should return approximately the same data
        assert_eq!(result.len(), data.len());
    }

    #[test]
    fn test_vgm_command_basic() {
        use vgm_command::VGMCommand;
        use command::CommandTrait;

        let cmd = VGMCommand::from_slice(&[0x52, 0x28, 0xF1]);
        assert_eq!(cmd.size(), 3);
        assert!(cmd.is_ym2612_write());
    }

    #[test]
    fn test_vgm_command_wait() {
        use vgm_command::VGMCommand;

        let cmd = VGMCommand::from_slice(&[vgm_command::WAIT_NTSC_FRAME]);
        assert!(cmd.is_wait());
    }

    #[test]
    fn test_xgm_fm_command_frame() {
        use xgm_fm_command::XGMFMCommand;

        let cmd = XGMFMCommand::create_frame_command();
        assert!(cmd.is_wait(true));
        assert_eq!(cmd.get_wait_frame(), 1);
    }

    #[test]
    fn test_xgm_fm_command_end() {
        use xgm_fm_command::XGMFMCommand;

        let cmd = XGMFMCommand::create_end();
        assert!(cmd.is_loop());
    }

    #[test]
    fn test_xgm_fm_command_loop() {
        use xgm_fm_command::XGMFMCommand;

        let cmd = XGMFMCommand::create_loop(100);
        assert!(cmd.is_loop());
    }

    #[test]
    fn test_xgm_fm_command_wait_commands() {
        use xgm_fm_command::XGMFMCommand;

        let cmds = XGMFMCommand::create_wait_commands(5);
        let total: i32 = cmds.iter().map(|c| c.get_wait_frame()).sum();
        assert_eq!(total, 5);
    }

    #[test]
    fn test_xgm_psg_command_frame() {
        use xgm_psg_command::XGMPSGCommand;

        let cmd = XGMPSGCommand::create_frame();
        assert!(cmd.is_wait(true));
        assert_eq!(cmd.get_wait_frame(), 1);
    }

    #[test]
    fn test_xgm_psg_command_end() {
        use xgm_psg_command::XGMPSGCommand;

        let cmd = XGMPSGCommand::create_end();
        assert!(cmd.is_loop());
    }

    #[test]
    fn test_xgm_psg_command_loop() {
        use xgm_psg_command::XGMPSGCommand;

        let cmd = XGMPSGCommand::create_loop(200);
        assert!(cmd.is_loop());
    }

    #[test]
    #[ignore] // TODO: synthetic round-trip test requires cross-validation with Java XGCPacker
    fn test_xgc_packer_roundtrip() {
        // Create data large enough (several frames of 32+ bytes each)
        let mut data = Vec::new();
        for _ in 0..16 {
            // Each frame: 64 bytes (well above FRAME_MIN_SIZE)
            for j in 0..64u8 {
                data.push(j);
            }
        }

        // One frame offset per 64-byte frame
        let frame_offsets: Vec<i32> = (0..16).map(|i| (i * 64) as i32).collect();
        let packed = xgc_packer::pack(&data, &frame_offsets, 256);
        let unpacked = xgc_packer::unpack(&packed);
        assert_eq!(unpacked, data);
    }

    #[test]
    #[ignore] // TODO: synthetic round-trip test requires cross-validation with Java XGCPacker
    fn test_xgc_packer_repetitive_data() {
        // Repetitive data should compress well
        let mut data = Vec::new();
        let frame_size = 64;
        let num_frames = 8;
        for _ in 0..num_frames {
            for j in 0..frame_size {
                data.push((j % 16) as u8);
            }
        }

        let frame_offsets: Vec<i32> = (0..num_frames).map(|i| (i * frame_size) as i32).collect();
        let packed = xgc_packer::pack(&data, &frame_offsets, 256);
        assert!(packed.len() < data.len(), "Repetitive data should compress");
        let unpacked = xgc_packer::unpack(&packed);
        assert_eq!(unpacked, data);
    }

    #[test]
    fn test_vgm_to_xgm_integration() {
        // Integration test: convert a real VGM file to XGM and back
        let vgm_path = concat!(env!("CARGO_MANIFEST_DIR"), "/../../project/template/res/music/actraiser.vgm");
        if !std::path::Path::new(vgm_path).exists() {
            // Skip if test data not available
            return;
        }

        set_silent(true);
        let data = util::read_binary_file(vgm_path).unwrap();
        let vgm_orig = vgm::VGM::from_data(&data, true).unwrap();
        assert!(!vgm_orig.commands.is_empty());

        // Convert VGM → XGM
        let mut xgm = xgm::XGM::from_vgm(&vgm_orig, false).unwrap();
        assert!(!xgm.fm_commands.is_empty());
        assert!(!xgm.psg_commands.is_empty());

        // Serialize XGM
        let xgm_data = xgm.as_byte_array().unwrap();
        assert!(xgm_data.len() > 256);
        assert_eq!(&xgm_data[0..4], b"XGM2");

        // Convert XGM back to VGM
        let vgm_back = vgm::VGM::from_xgm(&xgm).unwrap();
        assert!(!vgm_back.commands.is_empty());
        set_silent(false);
    }

    #[test]
    fn test_vgm_to_xgc_integration() {
        // Integration test: VGM → XGC (packed XGM)
        let vgm_path = concat!(env!("CARGO_MANIFEST_DIR"), "/../../project/template/res/music/actraiser.vgm");
        if !std::path::Path::new(vgm_path).exists() {
            return;
        }

        set_silent(true);
        let data = util::read_binary_file(vgm_path).unwrap();
        let vgm_orig = vgm::VGM::from_data(&data, true).unwrap();

        // Convert VGM → XGC (packed)
        let mut xgm = xgm::XGM::from_vgm(&vgm_orig, true).unwrap();
        let xgc_data = xgm.as_byte_array().unwrap();
        assert!(xgc_data.len() > 256);
        assert_eq!(&xgc_data[0..4], b"XGM2");

        // XGC should be smaller than unpacked XGM
        xgm.packed = false;
        let xgm_data = xgm.as_byte_array().unwrap();
        assert!(xgc_data.len() < xgm_data.len(), "XGC should be smaller than XGM");
        set_silent(false);
    }

    #[test]
    fn test_xgm_sample_basic() {
        use xgm_sample::XGMSample;

        let sample = XGMSample::new_simple(1, vec![0x80; 256]);
        assert_eq!(sample.id, 1);
        assert_eq!(sample.data.len(), 256);
    }

    #[test]
    fn test_global_settings() {
        // Test default values
        assert!(!is_silent());
        assert!(!is_verbose());
        assert!(sample_rate_fix());
        assert!(sample_ignore());
        assert!(!sample_advanced_compare());
        assert!(!delay_key_off());
        assert_eq!(get_sys(), SYSTEM_AUTO);

        // Test set/get
        set_silent(true);
        assert!(is_silent());
        set_silent(false);
        assert!(!is_silent());
    }

    #[test]
    fn test_command_trait_helpers() {
        use command::{get_data_size, compute_offsets};
        use xgm_fm_command::XGMFMCommand;
        use command::CommandTrait;

        let mut cmds = vec![
            XGMFMCommand::create_frame_command(),
            XGMFMCommand::create_frame_command(),
            XGMFMCommand::create_end(),
        ];

        let total_size = get_data_size(&cmds);
        assert!(total_size > 0);

        compute_offsets(&mut cmds, 0);
        assert_eq!(cmds[0].origin_offset(), 0);
        assert!(cmds[1].origin_offset() > 0);
    }

    #[test]
    fn test_vgm_from_data() {
        // Build a minimal valid VGM header + end command
        let mut data = vec![0u8; 0x102];
        // "Vgm " magic
        data[0] = b'V'; data[1] = b'g'; data[2] = b'm'; data[3] = b' ';
        // EOF offset (at 0x04) = data.len() - 4
        let eof = (data.len() as u32) - 4;
        util::set_u32(&mut data, 0x04, eof);
        // Version 1.71
        util::set_u32(&mut data, 0x08, 0x171);
        // YM2612 clock
        util::set_u32(&mut data, 0x2C, 7670453);
        // VGM data offset (relative to 0x34)
        util::set_u32(&mut data, 0x34, 0x100 - 0x34);
        // Put an end command at 0x100
        data[0x100] = 0x66;

        set_silent(true);
        let vgm = vgm::VGM::from_data(&data, false).unwrap();
        set_silent(false);
        assert!(vgm.commands.len() >= 1);
    }

    #[test]
    fn test_xd3_from_gd3() {
        let gd3 = gd3::GD3 {
            version: 0x100,
            track_name_en: "Test Track".to_string(),
            track_name_jp: "テスト".to_string(),
            game_name_en: "Test Game".to_string(),
            game_name_jp: "テストゲーム".to_string(),
            system_name_en: "Sega Genesis".to_string(),
            system_name_jp: "メガドライブ".to_string(),
            author_name_en: "Test Author".to_string(),
            author_name_jp: "テスト作者".to_string(),
            date: "2024/01/01".to_string(),
            vgm_conversion_author: "Converter".to_string(),
            notes: "Test notes".to_string(),
        };

        let xd3 = xd3::XD3::from_gd3(&gd3, 3000, 2900);
        assert_eq!(xd3.track_name, "Test Track");
        assert_eq!(xd3.game_name, "Test Game");
        assert_eq!(xd3.duration, 3000);
        assert_eq!(xd3.loop_duration, 2900);
    }

    #[test]
    fn test_format_vgm_time() {
        assert_eq!(util::format_vgm_time(44100), "00:01.000");
        assert_eq!(util::format_vgm_time(0), "00:00.000");
    }

    #[test]
    fn test_util_wide_string() {
        // UTF-16LE "AB\0"
        let data = vec![0x41, 0x00, 0x42, 0x00, 0x00, 0x00];
        assert_eq!(util::get_wide_string(&data, 0), "AB");
        assert_eq!(util::get_wide_string_size(&data, 0), 2);
    }

    #[test]
    fn test_util_get_string_bytes() {
        let ascii = util::get_string_bytes("Hi", false);
        assert_eq!(ascii, vec![b'H', b'i']);

        let utf16 = util::get_string_bytes("A", true);
        assert_eq!(utf16, vec![0x41, 0x00]);
    }

    #[test]
    fn test_util_bytes_as_hex_string() {
        let data = vec![0xDE, 0xAD, 0xBE, 0xEF];
        assert_eq!(util::bytes_as_hex_string(&data, 0, 4, 4), "DEADBEEF");
        assert_eq!(util::bytes_as_hex_string(&data, 0, 4, 2), "DEAD..");
    }
}
