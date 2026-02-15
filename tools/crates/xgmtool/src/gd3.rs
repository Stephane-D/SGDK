/// GD3 tags (VGM metadata, uses UTF-16LE wide strings)
#[derive(Clone, Debug)]
pub struct GD3 {
    pub version: u32,
    pub track_name_en: Vec<u16>,
    pub track_name_jp: Vec<u16>,
    pub game_name_en: Vec<u16>,
    pub game_name_jp: Vec<u16>,
    pub system_name_en: Vec<u16>,
    pub system_name_jp: Vec<u16>,
    pub author_name_en: Vec<u16>,
    pub author_name_jp: Vec<u16>,
    pub date: Vec<u16>,
    pub vgm_conversion_author: Vec<u16>,
    pub notes: Vec<u16>,
}

/// XD3 tags (simplified XGM metadata, ASCII strings)
#[derive(Clone, Debug)]
pub struct XD3 {
    pub track_name: String,
    pub game_name: String,
    pub author_name: String,
    pub date: String,
    pub conversion_author: String,
    pub notes: String,
    pub duration: i32,
    pub loop_duration: i32,
}

const XGMTOOL_PRINT: &str = "Optimized with XGMTool";

impl GD3 {
    pub fn new() -> Self {
        GD3 {
            version: 0x100,
            track_name_en: vec![0], track_name_jp: vec![0],
            game_name_en: vec![0], game_name_jp: vec![0],
            system_name_en: vec![0], system_name_jp: vec![0],
            author_name_en: vec![0], author_name_jp: vec![0],
            date: vec![0],
            vgm_conversion_author: vec![0],
            notes: vec![0],
        }
    }

    pub fn from_data(data: &[u8]) -> Option<Self> {
        if !crate::is_silent() {
            println!("Parsing GD3...");
        }

        if data.len() < 12 {
            println!("Error: GD3 data too small!");
            return None;
        }

        let header = std::str::from_utf8(&data[0..4]).unwrap_or("");
        if !header.eq_ignore_ascii_case("Gd3 ") {
            println!("Error: GD3 header not recognized !");
            return None;
        }

        let mut gd3 = GD3::new();
        gd3.version = crate::get_u32(data, 4);
        // size at offset 8 (ignored)
        let mut offset = 12;

        gd3.track_name_en = read_wide_string(data, &mut offset);
        gd3.track_name_jp = read_wide_string(data, &mut offset);
        gd3.game_name_en = read_wide_string(data, &mut offset);
        gd3.game_name_jp = read_wide_string(data, &mut offset);
        gd3.system_name_en = read_wide_string(data, &mut offset);
        gd3.system_name_jp = read_wide_string(data, &mut offset);
        gd3.author_name_en = read_wide_string(data, &mut offset);
        gd3.author_name_jp = read_wide_string(data, &mut offset);
        gd3.date = read_wide_string(data, &mut offset);
        gd3.vgm_conversion_author = read_wide_string(data, &mut offset);
        gd3.notes = read_wide_string(data, &mut offset);

        // Append XGMTool signature to conversion author
        let author_str = wide_to_string(&gd3.vgm_conversion_author);
        if !author_str.ends_with(XGMTOOL_PRINT) {
            let new_author = format!("{} - {}", author_str, XGMTOOL_PRINT);
            gd3.vgm_conversion_author = string_to_wide(&new_author);
        }

        Some(gd3)
    }

    pub fn compute_data_size(&self) -> usize {
        (self.track_name_en.len() + self.track_name_jp.len() +
         self.game_name_en.len() + self.game_name_jp.len() +
         self.system_name_en.len() + self.system_name_jp.len() +
         self.author_name_en.len() + self.author_name_jp.len() +
         self.date.len() + self.vgm_conversion_author.len() +
         self.notes.len()) * 2
    }

    pub fn as_byte_array(&self) -> Vec<u8> {
        let data_size = self.compute_data_size();
        let mut result = Vec::with_capacity(12 + data_size);

        // Header
        result.extend_from_slice(b"Gd3 ");
        result.extend_from_slice(&self.version.to_le_bytes());
        result.extend_from_slice(&(data_size as u32).to_le_bytes());

        // Fields
        write_wide_string(&mut result, &self.track_name_en);
        write_wide_string(&mut result, &self.track_name_jp);
        write_wide_string(&mut result, &self.game_name_en);
        write_wide_string(&mut result, &self.game_name_jp);
        write_wide_string(&mut result, &self.system_name_en);
        write_wide_string(&mut result, &self.system_name_jp);
        write_wide_string(&mut result, &self.author_name_en);
        write_wide_string(&mut result, &self.author_name_jp);
        write_wide_string(&mut result, &self.date);
        write_wide_string(&mut result, &self.vgm_conversion_author);
        write_wide_string(&mut result, &self.notes);

        result
    }
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

    pub fn from_gd3(gd3: &GD3, duration: i32, loop_duration: i32) -> Self {
        if !crate::is_silent() {
            println!("Converting GD3 to XD3...");
        }
        XD3 {
            track_name: wide_to_string(&gd3.track_name_en),
            game_name: wide_to_string(&gd3.game_name_en),
            author_name: wide_to_string(&gd3.author_name_en),
            date: wide_to_string(&gd3.date),
            conversion_author: wide_to_string(&gd3.vgm_conversion_author),
            notes: wide_to_string(&gd3.notes),
            duration,
            loop_duration,
        }
    }

    pub fn compute_data_size(&self) -> usize {
        self.track_name.len() + self.game_name.len() + self.author_name.len()
            + self.date.len() + self.conversion_author.len() + self.notes.len()
            + 6 + 8 // 6 null terminators + 8 bytes for durations
    }

    pub fn as_byte_array(&self) -> Vec<u8> {
        let size = (self.compute_data_size() + 1) & !1; // align to 2 bytes
        let mut result = Vec::with_capacity(size + 4);

        result.extend_from_slice(&(size as u32).to_le_bytes());

        write_c_string(&mut result, &self.track_name);
        write_c_string(&mut result, &self.game_name);
        write_c_string(&mut result, &self.author_name);
        write_c_string(&mut result, &self.date);
        write_c_string(&mut result, &self.conversion_author);
        write_c_string(&mut result, &self.notes);
        result.extend_from_slice(&(self.duration as u32).to_le_bytes());
        result.extend_from_slice(&(self.loop_duration as u32).to_le_bytes());

        // Pad to target size
        while result.len() < size + 4 {
            result.push(0);
        }

        result
    }
}

// --- Helper functions ---

fn read_wide_string(data: &[u8], offset: &mut usize) -> Vec<u16> {
    let mut result = Vec::new();
    while *offset + 1 < data.len() {
        let ch = (data[*offset] as u16) | ((data[*offset + 1] as u16) << 8);
        *offset += 2;
        result.push(ch);
        if ch == 0 { break; }
    }
    if result.is_empty() || *result.last().unwrap() != 0 {
        result.push(0);
    }
    result
}

fn write_wide_string(buf: &mut Vec<u8>, s: &[u16]) {
    for &ch in s {
        buf.push(ch as u8);
        buf.push((ch >> 8) as u8);
    }
}

fn wide_to_string(wstr: &[u16]) -> String {
    wstr.iter()
        .take_while(|&&c| c != 0)
        .map(|&c| c as u8 as char)
        .collect()
}

fn string_to_wide(s: &str) -> Vec<u16> {
    let mut result: Vec<u16> = s.bytes().map(|b| b as u16).collect();
    result.push(0);
    result
}

fn write_c_string(buf: &mut Vec<u8>, s: &str) {
    buf.extend_from_slice(s.as_bytes());
    buf.push(0);
}
