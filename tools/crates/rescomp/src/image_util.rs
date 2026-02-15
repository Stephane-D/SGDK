//! Image loading and processing utilities for Mega Drive resources.
//!
//! Handles PNG/BMP reading, palette extraction, and conversion to 8bpp indexed format.

use crate::types::BasicImageInfo;
use anyhow::{bail, Result};
use image::{DynamicImage, GenericImageView, ImageReader};

/// Get basic info about an image file.
pub fn get_basic_info(path: &str) -> Result<BasicImageInfo> {
    let img = ImageReader::open(path)?.decode()?;
    let (w, h) = img.dimensions();
    let bpp = match img.color() {
        image::ColorType::L8 | image::ColorType::La8 => 8,
        image::ColorType::Rgb8 | image::ColorType::Rgba8 => 32,
        image::ColorType::L16 | image::ColorType::La16 => 16,
        image::ColorType::Rgb16 | image::ColorType::Rgba16 => 48,
        _ => 32,
    };
    Ok(BasicImageInfo {
        w: w as usize,
        h: h as usize,
        bpp,
    })
}

/// Load an image and convert to 8bpp indexed format.
///
/// The 8bpp format encodes: bits 0-3 = color index, bits 4-5 = palette index, bit 7 = priority.
///
/// If `check_tile_align` is true, verifies the image dimensions are multiples of 8.
///
/// Returns `None` if an RGB image doesn't contain embedded palette data.
pub fn get_image_as_8bpp(path: &str, check_tile_align: bool, crop_palette: bool) -> Result<Option<Vec<u8>>> {
    let img = ImageReader::open(path)?.decode()?;
    let (w, h) = img.dimensions();
    let w = w as usize;
    let h = h as usize;

    if check_tile_align {
        if w % 8 != 0 || h % 8 != 0 {
            bail!("Image '{}' dimensions ({}x{}) are not aligned to 8 pixels", path, w, h);
        }
    }

    match &img {
        DynamicImage::ImageLuma8(_) | DynamicImage::ImageLumaA8(_) => {
            // Grayscale - treat as indexed
            let pixels = img.to_luma8();
            let mut result = Vec::with_capacity(w * h);
            for y in 0..h {
                for x in 0..w {
                    result.push(pixels.get_pixel(x as u32, y as u32).0[0]);
                }
            }
            Ok(Some(result))
        }
        _ => {
            // Try to handle as indexed color (palette-based) via raw bytes
            // For PNG with indexed color, image crate decodes to RGBA.
            // We need a different approach for indexed images.
            get_indexed_or_rgb_as_8bpp(path, &img, w, h, crop_palette)
        }
    }
}

/// Handle both indexed and RGB images.
fn get_indexed_or_rgb_as_8bpp(
    path: &str,
    img: &DynamicImage,
    w: usize,
    h: usize,
    _crop_palette: bool,
) -> Result<Option<Vec<u8>>> {
    // Try to read as indexed PNG using the png crate directly
    if let Ok(indexed) = read_indexed_png(path) {
        return Ok(Some(indexed));
    }

    // Fall back to RGB conversion
    // For RGB images, we quantize to 64 colors (4 palettes of 16)
    let rgba = img.to_rgba8();
    let mut palette: Vec<[u8; 4]> = Vec::new();
    let mut result = Vec::with_capacity(w * h);

    for y in 0..h {
        for x in 0..w {
            let px = rgba.get_pixel(x as u32, y as u32).0;

            // Find or add color to palette
            let color_idx = match palette.iter().position(|c| c[0] == px[0] && c[1] == px[1] && c[2] == px[2]) {
                Some(idx) => idx,
                None => {
                    if palette.len() >= 64 {
                        // Too many colors
                        return Ok(None);
                    }
                    palette.push(px);
                    palette.len() - 1
                }
            };

            // Encode: bits 0-3 = color within palette (0-15), bits 4-5 = palette index (0-3)
            let pal_idx = color_idx / 16;
            let col_idx = color_idx % 16;
            result.push(((pal_idx as u8) << 4) | (col_idx as u8));
        }
    }

    Ok(Some(result))
}

/// Read a PNG file as indexed color (palette-based) if possible.
fn read_indexed_png(path: &str) -> Result<Vec<u8>> {
    use std::fs::File;
    use std::io::BufReader;

    let file = File::open(path)?;
    let decoder = png::Decoder::new(BufReader::new(file));
    let mut reader = decoder.read_info()?;

    let info = reader.info();
    let w = info.width as usize;
    let h = info.height as usize;
    let color_type = info.color_type;

    if color_type != png::ColorType::Indexed {
        bail!("Not an indexed PNG");
    }

    let bit_depth = info.bit_depth as u8;

    let palette = info.palette.as_ref()
        .ok_or_else(|| anyhow::anyhow!("No palette in indexed PNG"))?
        .to_vec();
    let _num_colors = palette.len() / 3;

    let mut buf = vec![0u8; reader.output_buffer_size()];
    let output_info = reader.next_frame(&mut buf)?;
    let data = &buf[..output_info.buffer_size()];

    // First, expand sub-byte pixels to 1-byte-per-pixel indices
    let indices: Vec<u8> = match bit_depth {
        8 => data.to_vec(),
        4 => {
            let mut out = Vec::with_capacity(w * h);
            let row_bytes = (w + 1) / 2; // ceil(w/2) bytes per row
            for y in 0..h {
                let row_start = y * row_bytes;
                for x in 0..w {
                    let byte_idx = row_start + x / 2;
                    let idx = if x % 2 == 0 {
                        (data[byte_idx] >> 4) & 0x0F
                    } else {
                        data[byte_idx] & 0x0F
                    };
                    out.push(idx);
                }
            }
            out
        }
        2 => {
            let mut out = Vec::with_capacity(w * h);
            let row_bytes = (w + 3) / 4;
            for y in 0..h {
                let row_start = y * row_bytes;
                for x in 0..w {
                    let byte_idx = row_start + x / 4;
                    let shift = 6 - (x % 4) * 2;
                    let idx = (data[byte_idx] >> shift) & 0x03;
                    out.push(idx);
                }
            }
            out
        }
        1 => {
            let mut out = Vec::with_capacity(w * h);
            let row_bytes = (w + 7) / 8;
            for y in 0..h {
                let row_start = y * row_bytes;
                for x in 0..w {
                    let byte_idx = row_start + x / 8;
                    let shift = 7 - (x % 8);
                    let idx = (data[byte_idx] >> shift) & 0x01;
                    out.push(idx);
                }
            }
            out
        }
        _ => bail!("Unsupported indexed PNG bit depth: {}", bit_depth),
    };

    // Convert to SGDK 8bpp format: bits 4-5 = palette group, bits 0-3 = color index
    let mut result = Vec::with_capacity(w * h);
    for &idx in &indices {
        let pal_idx = (idx / 16) as u8;
        let col_idx = (idx % 16) as u8;
        result.push((pal_idx << 4) | col_idx);
    }

    Ok(result)
}

/// Get RGBA4444 palette (VDP format) from a .pal file.
pub fn get_rgba4444_palette_from_pal_file(path: &str, default_color: u16) -> Result<Vec<u16>> {
    let data = std::fs::read(path)?;

    // .pal file format: each color is 2 bytes in VDP format, or 3 bytes RGB
    if data.len() >= 6 && data.len() % 3 == 0 && data.len() % 2 != 0 {
        // RGB format (3 bytes per color)
        let num_colors = data.len() / 3;
        let mut palette = vec![default_color; num_colors];
        for i in 0..num_colors {
            let r = data[i * 3];
            let g = data[i * 3 + 1];
            let b = data[i * 3 + 2];
            palette[i] = to_vdp_color(b, g, r);
        }
        Ok(palette)
    } else {
        // VDP format (2 bytes per color, big-endian)
        let num_colors = data.len() / 2;
        let mut palette = vec![default_color; num_colors];
        for i in 0..num_colors {
            palette[i] = ((data[i * 2] as u16) << 8) | data[i * 2 + 1] as u16;
        }
        Ok(palette)
    }
}

/// Get RGBA4444 palette from an indexed color image.
pub fn get_rgba4444_palette_from_image(path: &str, default_color: u16) -> Result<Vec<u16>> {
    // Try indexed PNG first
    if let Ok(palette) = get_palette_from_indexed_png(path, default_color) {
        return Ok(palette);
    }

    // Try BMP
    if let Ok(palette) = get_palette_from_bmp(path, default_color) {
        return Ok(palette);
    }

    bail!("Cannot extract palette from '{}'", path)
}

/// Extract palette from indexed PNG.
fn get_palette_from_indexed_png(path: &str, default_color: u16) -> Result<Vec<u16>> {
    use std::fs::File;
    use std::io::BufReader;

    let file = File::open(path)?;
    let decoder = png::Decoder::new(BufReader::new(file));
    let reader = decoder.read_info()?;
    let info = reader.info();

    if info.color_type != png::ColorType::Indexed {
        bail!("Not indexed");
    }

    let palette_data = info.palette.as_ref()
        .ok_or_else(|| anyhow::anyhow!("No palette"))?;

    let num_colors = palette_data.len() / 3;
    let mut palette = vec![default_color; num_colors];

    for i in 0..num_colors {
        let r = palette_data[i * 3];
        let g = palette_data[i * 3 + 1];
        let b = palette_data[i * 3 + 2];
        palette[i] = to_vdp_color(b, g, r);
    }

    Ok(palette)
}

/// Extract palette from BMP file.
fn get_palette_from_bmp(path: &str, default_color: u16) -> Result<Vec<u16>> {
    let img = ImageReader::open(path)?.decode()?;
    let rgba = img.to_rgba8();
    let (w, h) = (rgba.width() as usize, rgba.height() as usize);

    // Collect unique colors
    let mut colors: Vec<[u8; 3]> = Vec::new();
    for y in 0..h {
        for x in 0..w {
            let px = rgba.get_pixel(x as u32, y as u32).0;
            let rgb = [px[0], px[1], px[2]];
            if !colors.contains(&rgb) {
                colors.push(rgb);
                if colors.len() >= 64 {
                    break;
                }
            }
        }
        if colors.len() >= 64 {
            break;
        }
    }

    let mut palette = vec![default_color; colors.len()];
    for (i, rgb) in colors.iter().enumerate() {
        palette[i] = to_vdp_color(rgb[2], rgb[1], rgb[0]);
    }

    Ok(palette)
}

/// Get RGBA4444 palette from tiles in an image.
pub fn get_rgba4444_palette_from_tiles(path: &str, default_color: u16) -> Result<Option<Vec<u16>>> {
    // For RGB images, try to extract palette
    let img = ImageReader::open(path)?.decode()?;
    let rgba = img.to_rgba8();
    let (w, h) = (rgba.width() as usize, rgba.height() as usize);

    let mut colors: Vec<[u8; 3]> = Vec::new();
    for y in 0..h {
        for x in 0..w {
            let px = rgba.get_pixel(x as u32, y as u32).0;
            let rgb = [px[0], px[1], px[2]];
            if !colors.contains(&rgb) {
                colors.push(rgb);
                if colors.len() > 64 {
                    return Ok(None);
                }
            }
        }
    }

    let mut palette = vec![default_color; colors.len()];
    for (i, rgb) in colors.iter().enumerate() {
        palette[i] = to_vdp_color(rgb[2], rgb[1], rgb[0]);
    }

    Ok(Some(palette))
}

/// Convert RGB to VDP color format (RGBA4444).
pub fn to_vdp_color(b: u8, g: u8, r: u8) -> u16 {
    let ri = ((r >> 4) & 0xE) as u16;
    let gi = ((g >> 4) & 0xE) as u16;
    let bi = ((b >> 4) & 0xE) as u16;
    ri | (gi << 4) | (bi << 8)
}

/// Check if a pixel has opaque content.
pub fn has_opaque_pixel(image: &[u8], w: usize, h: usize, x: usize, y: usize, size: usize) -> bool {
    for j in 0..size {
        let py = y + j;
        if py >= h {
            continue;
        }
        for i in 0..size {
            let px = x + i;
            if px >= w {
                continue;
            }
            if (image[py * w + px] & 0xF) != 0 {
                return true;
            }
        }
    }
    false
}

/// Get the bounding rectangle of opaque pixels in a region.
pub fn get_opaque_rect(image: &[u8], w: usize, _h: usize, rx: usize, ry: usize, rw: usize, rh: usize) -> Option<crate::types::Rect> {
    let mut min_x = rw as i32;
    let mut min_y = rh as i32;
    let mut max_x: i32 = -1;
    let mut max_y: i32 = -1;

    for j in 0..rh {
        for i in 0..rw {
            let px = rx + i;
            let py = ry + j;
            if (image[py * w + px] & 0xF) != 0 {
                min_x = min_x.min(i as i32);
                min_y = min_y.min(j as i32);
                max_x = max_x.max(i as i32);
                max_y = max_y.max(j as i32);
            }
        }
    }

    if max_x < 0 {
        None
    } else {
        Some(crate::types::Rect::new(
            rx as i32 + min_x,
            ry as i32 + min_y,
            max_x - min_x + 1,
            max_y - min_y + 1,
        ))
    }
}

/// Count opaque pixels in a region.
pub fn get_opaque_pixel_count(image: &[u8], w: usize, _h: usize, rx: usize, ry: usize, rw: usize, rh: usize) -> usize {
    let mut count = 0;
    for j in 0..rh {
        for i in 0..rw {
            let px = rx + i;
            let py = ry + j;
            if (image[py * w + px] & 0xF) != 0 {
                count += 1;
            }
        }
    }
    count
}

/// Check if a rectangular region has any opaque pixel.
pub fn has_opaque_pixel_region(image: &[u8], w: usize, x: usize, y: usize, rw: usize, rh: usize) -> bool {
    for j in 0..rh {
        let py = y + j;
        for i in 0..rw {
            let px = x + i;
            if px < w && py * w + px < image.len() {
                if (image[py * w + px] & 0xF) != 0 {
                    return true;
                }
            }
        }
    }
    false
}

/// Extract a sub-image from a larger image.
pub fn get_sub_image(image: &[u8], w: usize, x: usize, y: usize, sw: usize, sh: usize) -> Vec<u8> {
    let mut result = vec![0u8; sw * sh];
    for j in 0..sh {
        let src_y = y + j;
        for i in 0..sw {
            let src_x = x + i;
            if src_x < w && src_y * w + src_x < image.len() {
                result[j * sw + i] = image[src_y * w + src_x];
            }
        }
    }
    result
}
