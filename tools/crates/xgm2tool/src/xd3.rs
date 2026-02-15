use crate::gd3::GD3;
use crate::util;

/// XD3 metadata tags (XGM2 format)
#[derive(Clone, Debug)]
pub struct XD3 {
    pub track_name: String,
    pub game_name: String,
    pub author_name: String,
    pub date: String,
    pub conversion_author: String,
    pub notes: String,
    pub duration: u32,
    pub loop_duration: u32,
}

impl XD3 {
    pub fn new() -> Self {
        XD3 {
            track_name: String::new(),
            game_name: String::new(),
            author_name: String::new(),
            date: String::new(),
            conversion_author: String::new(),
            notes: String::new(),
            duration: 0,
            loop_duration: 0,
        }
    }

    pub fn from_gd3(gd3: &GD3, duration_frames: i32, loop_duration_frames: i32) -> Self {
        XD3 {
            track_name: gd3.track_name_en.clone(),
            game_name: gd3.game_name_en.clone(),
            author_name: gd3.author_name_en.clone(),
            date: gd3.date.clone(),
            conversion_author: gd3.vgm_conversion_author.clone(),
            notes: gd3.notes.clone(),
            duration: duration_frames as u32,
            loop_duration: loop_duration_frames as u32,
        }
    }

    pub fn from_data(data: &[u8], base_offset: usize) -> Self {
        let mut offset = base_offset;

        if !crate::is_silent() { println!("Parsing XD3..."); }

        let id = util::get_ascii_string_n(data, offset, 4);
        if id != "XD3 " { println!("Error: XD3 header not recognized!"); }
        offset += 4;

        // skip total size
        offset += 4;

        let track_name = util::get_ascii_string(data, offset);
        offset += track_name.len() + 1;
        let game_name = util::get_ascii_string(data, offset);
        offset += game_name.len() + 1;
        let author_name = util::get_ascii_string(data, offset);
        offset += author_name.len() + 1;
        let date = util::get_ascii_string(data, offset);
        offset += date.len() + 1;
        let conversion_author = util::get_ascii_string(data, offset);
        offset += conversion_author.len() + 1;
        let notes = util::get_ascii_string(data, offset);
        offset += notes.len() + 1;

        let duration = util::get_u32(data, offset);
        offset += 4;
        let loop_duration = util::get_u32(data, offset);

        XD3 {
            track_name, game_name, author_name,
            date, conversion_author, notes,
            duration, loop_duration,
        }
    }

    fn compute_data_size(&self) -> usize {
        self.track_name.len() + 1
            + self.game_name.len() + 1
            + self.author_name.len() + 1
            + self.date.len() + 1
            + self.conversion_author.len() + 1
            + self.notes.len() + 1
            + 4 + 4
    }

    pub fn get_total_data_size(&self) -> usize {
        self.compute_data_size() + 8
    }

    pub fn as_byte_array(&self) -> Vec<u8> {
        let data_size = self.compute_data_size();
        let mut result = vec![0u8; data_size + 8];
        let mut offset: usize = 0;

        // Header "XD3 "
        result[0..4].copy_from_slice(b"XD3 ");
        offset += 4;
        util::set_u32(&mut result, offset, data_size as u32);
        offset += 4;

        let mut write_field = |s: &str| {
            let bytes = s.as_bytes();
            result[offset..offset + bytes.len()].copy_from_slice(bytes);
            offset += bytes.len() + 1; // +1 for null terminator
        };

        write_field(&self.track_name);
        write_field(&self.game_name);
        write_field(&self.author_name);
        write_field(&self.date);
        write_field(&self.conversion_author);
        write_field(&self.notes);

        util::set_u32(&mut result, offset, self.duration);
        offset += 4;
        util::set_u32(&mut result, offset, self.loop_duration);

        result
    }
}
