use crate::util;
use crate::xd3::XD3;

const XGMTOOL_PRINT: &str = "Optimized with XGMTool";

/// GD3 metadata tags (VGM format)
#[derive(Clone, Debug)]
pub struct GD3 {
    pub version: i32,
    pub track_name_en: String,
    pub track_name_jp: String,
    pub game_name_en: String,
    pub game_name_jp: String,
    pub system_name_en: String,
    pub system_name_jp: String,
    pub author_name_en: String,
    pub author_name_jp: String,
    pub date: String,
    pub vgm_conversion_author: String,
    pub notes: String,
}

impl GD3 {
    pub fn new() -> Self {
        GD3 {
            version: 0x100,
            track_name_en: String::new(), track_name_jp: String::new(),
            game_name_en: String::new(), game_name_jp: String::new(),
            system_name_en: String::new(), system_name_jp: String::new(),
            author_name_en: String::new(), author_name_jp: String::new(),
            date: String::new(), vgm_conversion_author: String::new(),
            notes: String::new(),
        }
    }

    pub fn from_xd3(xd3: &XD3) -> Self {
        GD3 {
            version: 0x100,
            track_name_en: xd3.track_name.clone(),
            track_name_jp: xd3.track_name.clone(),
            game_name_en: xd3.game_name.clone(),
            game_name_jp: xd3.game_name.clone(),
            system_name_en: "SEGA Mega Drive".to_string(),
            system_name_jp: "SEGA Mega Drive".to_string(),
            author_name_en: xd3.author_name.clone(),
            author_name_jp: xd3.author_name.clone(),
            date: xd3.date.clone(),
            vgm_conversion_author: xd3.conversion_author.clone(),
            notes: xd3.notes.clone(),
        }
    }

    pub fn from_data(data: &[u8], base_offset: usize) -> Self {
        let mut offset = base_offset;

        if !crate::is_silent() { println!("Parsing GD3..."); }

        let id = util::get_ascii_string_n(data, offset, 4);
        if id != "Gd3 " { println!("Error: GD3 header not recognized!"); }
        offset += 4;

        let version = util::get_i32(data, offset);
        offset += 4;
        // skip total size
        offset += 4;

        let track_name_en = util::get_wide_string(data, offset);
        offset += (track_name_en.len() + 1) * 2;
        let track_name_jp = util::get_wide_string(data, offset);
        offset += (track_name_jp.len() + 1) * 2;
        let game_name_en = util::get_wide_string(data, offset);
        offset += (game_name_en.len() + 1) * 2;
        let game_name_jp = util::get_wide_string(data, offset);
        offset += (game_name_jp.len() + 1) * 2;
        let system_name_en = util::get_wide_string(data, offset);
        offset += (system_name_en.len() + 1) * 2;
        let system_name_jp = util::get_wide_string(data, offset);
        offset += (system_name_jp.len() + 1) * 2;
        let author_name_en = util::get_wide_string(data, offset);
        offset += (author_name_en.len() + 1) * 2;
        let author_name_jp = util::get_wide_string(data, offset);
        offset += (author_name_jp.len() + 1) * 2;
        let date = util::get_wide_string(data, offset);
        offset += (date.len() + 1) * 2;
        let mut vgm_conversion_author = util::get_wide_string(data, offset);
        offset += (vgm_conversion_author.len() + 1) * 2;
        let notes = util::get_wide_string(data, offset);

        // Add XGMTool signature
        if !vgm_conversion_author.ends_with(XGMTOOL_PRINT) {
            if !vgm_conversion_author.is_empty() {
                vgm_conversion_author.push_str(" - ");
            }
            vgm_conversion_author.push_str(XGMTOOL_PRINT);
        }

        GD3 {
            version, track_name_en, track_name_jp,
            game_name_en, game_name_jp,
            system_name_en, system_name_jp,
            author_name_en, author_name_jp,
            date, vgm_conversion_author, notes,
        }
    }

    fn compute_data_size(&self) -> usize {
        (self.track_name_en.len() + self.track_name_jp.len()
            + self.game_name_en.len() + self.game_name_jp.len()
            + self.system_name_en.len() + self.system_name_jp.len()
            + self.author_name_en.len() + self.author_name_jp.len()
            + self.date.len() + self.vgm_conversion_author.len()
            + self.notes.len()) * 2 + (11 * 2)
    }

    pub fn get_total_data_size(&self) -> usize {
        self.compute_data_size() + 12
    }

    pub fn as_byte_array(&self) -> Vec<u8> {
        let data_size = self.compute_data_size();
        let mut result = vec![0u8; data_size + 12];
        let mut offset: usize = 0;

        // Header "Gd3 "
        result[0..4].copy_from_slice(b"Gd3 ");
        offset += 4;
        util::set_i32(&mut result, offset, self.version);
        offset += 4;
        util::set_i32(&mut result, offset, data_size as i32);
        offset += 4;

        // Helper to write wide string field
        let mut write_field = |s: &str| {
            let bytes = util::get_string_bytes(s, true);
            result[offset..offset + bytes.len()].copy_from_slice(&bytes);
            offset += bytes.len() + 2; // +2 for null terminator
        };

        write_field(&self.track_name_en);
        write_field(&self.track_name_jp);
        write_field(&self.game_name_en);
        write_field(&self.game_name_jp);
        write_field(&self.system_name_en);
        write_field(&self.system_name_jp);
        write_field(&self.author_name_en);
        write_field(&self.author_name_jp);
        write_field(&self.date);
        write_field(&self.vgm_conversion_author);
        write_field(&self.notes);

        result
    }
}
