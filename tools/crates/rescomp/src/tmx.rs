//! TMX map processing for rescomp.
//!
//! This module handles the conversion of Tiled map (.tmx) data into an 8bpp image
//! that can be fed into the existing tileset/tilemap/map creation pipeline.

use anyhow::{bail, Result};

use crate::image_util;
use crate::xml_util;
use crate::util;

// TMX flip flags (bits 31, 30, 29 of tile ID)
const TMX_HFLIP: u32 = 1 << 31;
const TMX_VFLIP: u32 = 1 << 30;
const TMX_AXE_FLIP: u32 = 1 << 29;
const TMX_TILE_IND_MASK: u32 = 0x0FFF_FFFF;

/// Resolved tileset with loaded image data.
pub struct ResolvedTileset {
    pub first_gid: u32,
    pub num_tiles: usize,
    pub tile_size: usize,       // in pixels (square tiles)
    pub columns: usize,
    pub image_8bpp: Vec<u8>,
    pub image_width: usize,
    pub image_height: usize,
    pub image_path: String,     // resolved path to the image
}

/// Processed TMX map data.
pub struct TmxProcessedMap {
    /// Reconstructed 8bpp image of the entire map.
    pub map_image: Vec<u8>,
    /// Width of the map image in pixels.
    pub image_width: usize,
    /// Height of the map image in pixels.
    pub image_height: usize,
    /// Tile size used in this map (8, 16, 24, or 32 pixels).
    pub tile_size: usize,
    /// Map width in TMX tiles.
    pub map_w: usize,
    /// Map height in TMX tiles.
    pub map_h: usize,
    /// Resolved tileset image paths (for dependency tracking).
    pub tileset_image_paths: Vec<String>,
}

/// Resolve all tileset references from a TMX file.
fn resolve_tilesets(tmx_path: &str, tileset_refs: &[xml_util::TmxTilesetRef]) -> Result<Vec<ResolvedTileset>> {
    let tmx_dir = std::path::Path::new(tmx_path)
        .parent()
        .map(|p| p.to_string_lossy().to_string())
        .unwrap_or_default();

    let mut resolved = Vec::new();

    for ts_ref in tileset_refs {
        let tsx_path = util::adjust_path(&tmx_dir, &ts_ref.source);
        let tsx = xml_util::parse_tsx(&tsx_path)?;

        let image_path = tsx.image_path.clone();
        if image_path.is_empty() {
            bail!("Tileset '{}' has no image", tsx.name);
        }

        // Load image as 8bpp
        let img_8bpp = image_util::get_image_as_8bpp(&image_path, false, false)
            .map_err(|e| anyhow::anyhow!("Cannot load tileset image '{}': {}", image_path, e))?
            .ok_or_else(|| anyhow::anyhow!("Cannot convert tileset image '{}' to 8bpp", image_path))?;

        let columns = if tsx.columns > 0 {
            tsx.columns
        } else {
            tsx.image_width / tsx.tile_width
        };
        let num_tiles = if tsx.tile_count > 0 {
            tsx.tile_count
        } else {
            columns * (tsx.image_height / tsx.tile_height)
        };

        resolved.push(ResolvedTileset {
            first_gid: ts_ref.first_gid,
            num_tiles,
            tile_size: tsx.tile_width,
            columns,
            image_8bpp: img_8bpp,
            image_width: tsx.image_width,
            image_height: tsx.image_height,
            image_path,
        });
    }

    // Sort by first_gid
    resolved.sort_by_key(|r| r.first_gid);

    Ok(resolved)
}

/// Find the tileset containing a given tile index.
fn find_tileset_for<'a>(tilesets: &'a [ResolvedTileset], tile_ind: u32) -> Option<&'a ResolvedTileset> {
    tilesets.iter().find(|ts| {
        tile_ind >= ts.first_gid && tile_ind < ts.first_gid + ts.num_tiles as u32
    })
}

/// Get pixel from tileset image for a specific tile, applying flips.
fn get_tile_pixel(ts: &ResolvedTileset, local_ind: u32, px: usize, py: usize, hflip: bool, vflip: bool) -> u8 {
    let tile_col = local_ind as usize % ts.columns;
    let tile_row = local_ind as usize / ts.columns;

    let actual_px = if hflip { ts.tile_size - 1 - px } else { px };
    let actual_py = if vflip { ts.tile_size - 1 - py } else { py };

    let img_x = tile_col * ts.tile_size + actual_px;
    let img_y = tile_row * ts.tile_size + actual_py;

    if img_x < ts.image_width && img_y < ts.image_height {
        ts.image_8bpp[img_y * ts.image_width + img_x]
    } else {
        0
    }
}



/// Find a layer by name (case-insensitive).
fn find_layer<'a>(layers: &'a [xml_util::TmxLayer], name: &str) -> Option<&'a xml_util::TmxLayer> {
    layers.iter().find(|l| l.name.eq_ignore_ascii_case(name))
}

/// Process a TMX file into an 8bpp image for TILEMAP or MAP use.
///
/// The layer lookup follows Tiled's convention:
/// - Single layer mode: look for `layer_name` directly, with optional `layer_name priority` or `layer_name prio`
/// - Dual layer mode: look for `layer_name low` and `layer_name high`
pub fn process_tmx_to_image(tmx_path: &str, layer_name: &str) -> Result<TmxProcessedMap> {
    let tmx = xml_util::parse_tmx(tmx_path)?;

    // Validate TMX
    let tile_size = tmx.tile_width;
    if tmx.tile_width != tmx.tile_height {
        bail!("TMX tile dimensions must be square (got {}x{})", tmx.tile_width, tmx.tile_height);
    }
    if tile_size % 8 != 0 || tile_size < 8 || tile_size > 32 {
        bail!("TMX tile size must be 8, 16, 24, or 32 (got {})", tile_size);
    }

    let map_w = tmx.width;
    let map_h = tmx.height;
    let img_w = map_w * tile_size;
    let img_h = map_h * tile_size;

    // Resolve tilesets
    let tilesets = resolve_tilesets(tmx_path, &tmx.tilesets)?;
    let tileset_image_paths: Vec<String> = tilesets.iter().map(|t| t.image_path.clone()).collect();

    // Find layers
    let (map_data, prio_data) = find_map_and_prio_layers(&tmx.layers, layer_name)?;

    // Build the output 8bpp image
    let mut map_image = vec![0u8; img_w * img_h];

    for j in 0..map_h {
        for i in 0..map_w {
            let map_idx = j * map_w + i;
            let raw_value = map_data[map_idx];
            let prio_value = prio_data.map(|d| d[map_idx]).unwrap_or(0);

            // Decode tile index and flip flags
            let hflip = (raw_value & TMX_HFLIP) != 0;
            let vflip = (raw_value & TMX_VFLIP) != 0;
            let _axeflip = (raw_value & TMX_AXE_FLIP) != 0;
            let tile_ind = raw_value & TMX_TILE_IND_MASK;

            // Determine if priority should be set from the separate priority layer
            let has_prio = if let Some(_) = &prio_data {
                (prio_value & TMX_TILE_IND_MASK) != 0
            } else {
                false
            };

            if tile_ind == 0 {
                // Empty tile — leave as zero
                continue;
            }

            // Find the tileset containing this tile
            let ts = match find_tileset_for(&tilesets, tile_ind) {
                Some(ts) => ts,
                None => {
                    // Tile index doesn't match any tileset — skip
                    continue;
                }
            };

            let local_ind = tile_ind - ts.first_gid;

            // Copy tile pixels to the map image
            for py in 0..tile_size {
                for px in 0..tile_size {
                    let mut pixel = get_tile_pixel(ts, local_ind, px, py, hflip, vflip);

                    // Apply priority bit: set bit 7 of pixels in 8×8 sub-tiles that have priority
                    if has_prio {
                        pixel |= 0x80; // Set priority bit
                    }

                    let out_x = i * tile_size + px;
                    let out_y = j * tile_size + py;
                    map_image[out_y * img_w + out_x] = pixel;
                }
            }
        }
    }

    Ok(TmxProcessedMap {
        map_image,
        image_width: img_w,
        image_height: img_h,
        tile_size,
        map_w,
        map_h,
        tileset_image_paths,
    })
}

/// Find the map data and optional priority data layers.
fn find_map_and_prio_layers<'a>(
    layers: &'a [xml_util::TmxLayer],
    layer_name: &str,
) -> Result<(&'a [u32], Option<&'a [u32]>)> {
    // Try single layer mode first
    if let Some(main_layer) = find_layer(layers, layer_name) {
        // Look for priority layer
        let prio_layer = find_layer(layers, &format!("{} priority", layer_name))
            .or_else(|| find_layer(layers, &format!("{} prio", layer_name)));

        return Ok((
            &main_layer.data,
            prio_layer.map(|l| l.data.as_slice()),
        ));
    }

    // Try dual layer mode: "name low" + "name high"
    let low_name = format!("{} low", layer_name);
    let high_name = format!("{} high", layer_name);

    let low_layer = find_layer(layers, &low_name);
    let high_layer = find_layer(layers, &high_name);

    if let (Some(low), Some(high)) = (low_layer, high_layer) {
        // In dual layer mode, we need to merge: use high layer tiles (with priority)
        // where they're non-zero, otherwise use low layer tiles
        // The caller will merge these; return low as main and high as prio
        return Ok((&low.data, Some(&high.data)));
    }

    bail!(
        "Cannot find layer '{}' (or '{} low'/'{}  high') in TMX",
        layer_name, layer_name, layer_name
    )
}

/// Get tileset image paths from resolved tilesets (for resource file tracking).
pub fn get_tmx_tileset_image_paths(tmx_path: &str) -> Result<Vec<String>> {
    let tmx = xml_util::parse_tmx(tmx_path)?;
    let tilesets = resolve_tilesets(tmx_path, &tmx.tilesets)?;
    Ok(tilesets.iter().map(|t| t.image_path.clone()).collect())
}
