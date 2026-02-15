//! Tile data handling for Mega Drive VDP.
//!
//! Each tile is 8x8 pixels at 4bpp, stored as 8 x u32 values.

use crate::types::TileEquality;

/// Tile constants matching Mega Drive VDP format.
pub const TILE_MAX_NUM: u16 = 1 << 11;
pub const TILE_INDEX_MASK: u16 = TILE_MAX_NUM - 1;
pub const TILE_HFLIP_SFT: u16 = 11;
pub const TILE_VFLIP_SFT: u16 = 12;
pub const TILE_PALETTE_SFT: u16 = 13;
pub const TILE_PRIORITY_SFT: u16 = 15;
pub const TILE_HFLIP_FLAG: u16 = 1 << 11;
pub const TILE_VFLIP_FLAG: u16 = 1 << 12;
pub const TILE_PRIORITY_FLAG: u16 = 1 << 15;
pub const TILE_HFLIP_MASK: u16 = TILE_HFLIP_FLAG;
pub const TILE_VFLIP_MASK: u16 = TILE_VFLIP_FLAG;
pub const TILE_PALETTE_MASK: u16 = 3 << 13;
pub const TILE_PRIORITY_MASK: u16 = TILE_PRIORITY_FLAG;

/// Build a tile attribute word.
pub fn tile_attr(pal: u16, prio: bool, vflip: bool, hflip: bool) -> u16 {
    (if hflip { 1u16 } else { 0 } << TILE_HFLIP_SFT)
        | (if vflip { 1u16 } else { 0 } << TILE_VFLIP_SFT)
        | ((pal & 3) << TILE_PALETTE_SFT)
        | (if prio { 1u16 } else { 0 } << TILE_PRIORITY_SFT)
}

/// Build a full tile attribute word with tile index.
pub fn tile_attr_full(pal: u16, prio: bool, vflip: bool, hflip: bool, index: u16) -> u16 {
    tile_attr(pal, prio, vflip, hflip) | (index & TILE_INDEX_MASK)
}

/// A single 8x8 tile in 4bpp format.
///
/// `data` contains 8 u32 values (one per row), each holding 8 pixels at 4 bits each.
#[derive(Clone)]
pub struct Tile {
    /// 4bpp tile data: 8 entries (one per row).
    pub data: Vec<i32>,
    /// Tile size in pixels (always 8 for Mega Drive).
    pub size: usize,
    /// Palette index (0-3).
    pub pal: u8,
    /// Priority flag.
    pub prio: bool,
    /// Plain tile value (-1 if not plain).
    pub plain: i32,
    /// True if all data is zero.
    pub empty: bool,

    // Pre-computed flipped versions for fast comparison.
    h_flip: Vec<i32>,
    v_flip: Vec<i32>,
    hv_flip: Vec<i32>,

    // Hash code (sum of all flip variants for symmetrical matching).
    pub hc: u64,
}

impl Tile {
    pub fn new(data: Vec<i32>, size: usize, pal: u8, prio: bool, plain: i32) -> Self {
        // 8 pixels of 4bpp per i32 entry
        assert_eq!(data.len(), (size * size) / 8, "Tile data length mismatch");

        let empty = data.iter().all(|&v| v == 0);

        let h_flip = Self::get_flipped_static(&data, size, true, false);
        let v_flip = Self::get_flipped_static(&data, size, false, true);
        let hv_flip = Self::get_flipped_static(&data, size, true, true);

        let hc = Self::compute_hash(&data)
            .wrapping_add(Self::compute_hash(&h_flip))
            .wrapping_add(Self::compute_hash(&v_flip))
            .wrapping_add(Self::compute_hash(&hv_flip));

        Self {
            data,
            size,
            pal: pal & 3,
            prio,
            plain,
            empty,
            h_flip,
            v_flip,
            hv_flip,
            hc,
        }
    }

    /// Create a tile from 8bpp pixel data.
    pub fn from_8bpp(pixels: &[u8], size: usize, pal: u8, prio: bool, plain: i32) -> Self {
        let data_4bpp = convert_to_4bpp(pixels, 8);
        let ints = bytes_to_ints(&data_4bpp);
        Self::new(ints, size, pal, prio, plain)
    }

    /// Extract a tile from an 8bpp image at pixel position (x, y).
    pub fn get_tile(image: &[u8], img_w: usize, img_h: usize, x: usize, y: usize, size: usize) -> Self {
        let image_tile = get_image_tile(image, img_w, img_h, x, y, size);
        let mut data = vec![0u8; size * size];

        let mut plain_col: i32 = -1;
        let mut is_plain = true;
        let mut pal: i32 = -1;
        let mut prio_val: i32 = -1;
        let mut trans_pal: i32 = -1;
        let mut trans_prio: i32 = -1;

        for off in 0..(size * size) {
            let pixel = image_tile[off];
            let color = (pixel & 0xF) as i32;

            if plain_col == -1 {
                plain_col = color;
            } else if plain_col != color {
                is_plain = false;
            }

            let cur_pal = ((pixel >> 4) & 3) as i32;
            let cur_prio = ((pixel >> 7) & 1) as i32;

            if color == 0 {
                // Transparent pixel
                if trans_pal == -1 {
                    trans_pal = cur_pal;
                }
                if trans_prio == -1 {
                    trans_prio = cur_prio;
                }
            } else {
                // Opaque pixel
                if pal == -1 {
                    pal = cur_pal;
                }
                if prio_val == -1 {
                    prio_val = cur_prio;
                }
            }

            data[off] = color as u8;
        }

        if pal == -1 {
            pal = if trans_pal == -1 { 0 } else { trans_pal };
        }
        if prio_val == -1 {
            prio_val = if trans_prio == -1 { 0 } else { trans_prio };
        }

        Self::from_8bpp(
            &data,
            size,
            pal as u8,
            prio_val != 0,
            if is_plain { plain_col } else { -1 },
        )
    }

    pub fn is_blank(&self) -> bool {
        self.empty
    }

    pub fn is_plain(&self) -> bool {
        self.plain != -1
    }

    pub fn get_plain_value(&self) -> i32 {
        self.plain
    }

    /// Check equality (including flipped versions).
    pub fn get_equality(&self, other: &Tile) -> TileEquality {
        if self.data == other.data {
            return TileEquality::Equal;
        }
        if self.h_flip == other.data {
            return TileEquality::HFlip;
        }
        if self.v_flip == other.data {
            return TileEquality::VFlip;
        }
        if self.hv_flip == other.data {
            return TileEquality::HVFlip;
        }
        TileEquality::None
    }

    /// Check for flip equality only (not direct equality).
    pub fn get_flip_equality(&self, other: &Tile) -> TileEquality {
        if self.h_flip == other.data {
            return TileEquality::HFlip;
        }
        if self.v_flip == other.data {
            return TileEquality::VFlip;
        }
        if self.hv_flip == other.data {
            return TileEquality::HVFlip;
        }
        TileEquality::None
    }

    fn get_flipped_static(data: &[i32], size: usize, hflip: bool, vflip: bool) -> Vec<i32> {
        let row_size = size / 8;
        let mut result = vec![0i32; data.len()];
        let mut dst_offset = 0;
        let mut base_offset = if vflip { (size - 1) * row_size } else { 0 };

        for _ in 0..size {
            let mut offset = base_offset + if hflip { row_size - 1 } else { 0 };

            for _ in 0..row_size {
                if hflip {
                    result[dst_offset] = swap_nibble32(data[offset]);
                    offset = offset.wrapping_sub(1);
                } else {
                    result[dst_offset] = data[offset];
                    offset += 1;
                }
                dst_offset += 1;
            }

            if vflip {
                base_offset = base_offset.wrapping_sub(row_size);
            } else {
                base_offset += row_size;
            }
        }

        result
    }

    fn compute_hash(data: &[i32]) -> u64 {
        let mut result: u64 = 0;
        for &v in data {
            result = result.wrapping_add(v as u32 as u64);
        }
        result
    }
}

impl std::hash::Hash for Tile {
    fn hash<H: std::hash::Hasher>(&self, state: &mut H) {
        self.hc.hash(state);
    }
}

impl PartialEq for Tile {
    fn eq(&self, other: &Self) -> bool {
        self.hc == other.hc && self.data == other.data
    }
}

impl Eq for Tile {}

/// Swap nibbles in a 32-bit value (horizontal flip for 4bpp tiles).
pub fn swap_nibble32(val: i32) -> i32 {
    let v = val as u32;
    let result = ((v & 0x0000000F) << 28)
        | ((v & 0x000000F0) << 20)
        | ((v & 0x00000F00) << 12)
        | ((v & 0x0000F000) << 4)
        | ((v & 0x000F0000) >> 4)
        | ((v & 0x00F00000) >> 12)
        | ((v & 0x0F000000) >> 20)
        | ((v & 0xF0000000) >> 28);
    result as i32
}

/// Get a tile (8x8 pixels) from an 8bpp image at pixel position (x,y).
pub fn get_image_tile(image: &[u8], img_w: usize, img_h: usize, x: usize, y: usize, size: usize) -> Vec<u8> {
    let mut result = vec![0u8; size * size];
    let mut dst = 0;
    for j in 0..size {
        let py = y + j;
        for i in 0..size {
            let px = x + i;
            if px < img_w && py < img_h {
                result[dst] = image[py * img_w + px];
            }
            dst += 1;
        }
    }
    result
}

/// Copy a tile into a destination image at pixel position (x,y).
pub fn copy_tile(dest: &mut [u8], img_w: usize, tile: &[u8], x: usize, y: usize, size: usize) {
    let mut src = 0;
    for j in 0..size {
        let dst_off = (y + j) * img_w + x;
        for i in 0..size {
            dest[dst_off + i] = tile[src];
            src += 1;
        }
    }
}

/// Convert 8bpp pixel data to 4bpp (pack 2 pixels per byte).
pub fn convert_to_4bpp(data: &[u8], _src_bpp: u8) -> Vec<u8> {
    let len = (data.len() + 1) / 2;
    let mut result = vec![0u8; len];
    for i in (0..data.len()).step_by(2) {
        let hi = (data[i] & 0xF) << 4;
        let lo = if i + 1 < data.len() { data[i + 1] & 0xF } else { 0 };
        result[i / 2] = hi | lo;
    }
    result
}

/// Convert 4bpp data to 8bpp (unpack 2 pixels per byte).
pub fn convert_to_8bpp(data: &[u8]) -> Vec<u8> {
    let mut result = Vec::with_capacity(data.len() * 2);
    for &b in data {
        result.push((b >> 4) & 0xF);
        result.push(b & 0xF);
    }
    result
}

/// Convert a byte array to i32 array (big-endian, 4 bytes per int).
pub fn bytes_to_ints(data: &[u8]) -> Vec<i32> {
    let count = data.len() / 4;
    let mut result = Vec::with_capacity(count);
    for i in 0..count {
        let off = i * 4;
        let v = ((data[off] as u32) << 24)
            | ((data[off + 1] as u32) << 16)
            | ((data[off + 2] as u32) << 8)
            | (data[off + 3] as u32);
        result.push(v as i32);
    }
    result
}

/// Convert i32 array to byte array (big-endian).
pub fn ints_to_bytes(data: &[i32]) -> Vec<u8> {
    let mut result = Vec::with_capacity(data.len() * 4);
    for &v in data {
        let u = v as u32;
        result.push((u >> 24) as u8);
        result.push((u >> 16) as u8);
        result.push((u >> 8) as u8);
        result.push(u as u8);
    }
    result
}

/// Convert short array to byte array (big-endian).
pub fn shorts_to_bytes(data: &[i16]) -> Vec<u8> {
    let mut result = Vec::with_capacity(data.len() * 2);
    for &v in data {
        let u = v as u16;
        result.push((u >> 8) as u8);
        result.push(u as u8);
    }
    result
}

/// Convert byte array to short array (big-endian).
pub fn bytes_to_shorts(data: &[u8]) -> Vec<i16> {
    let count = data.len() / 2;
    let mut result = Vec::with_capacity(count);
    for i in 0..count {
        let off = i * 2;
        let v = ((data[off] as u16) << 8) | (data[off + 1] as u16);
        result.push(v as i16);
    }
    result
}
