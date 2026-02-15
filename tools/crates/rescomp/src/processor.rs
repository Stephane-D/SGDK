//! Resource processors for rescomp.
//!
//! Each processor handles a specific resource type keyword from the .res file.

use linked_hash_map::LinkedHashMap;
use std::path::Path;

use crate::compiler::Compiler;
use crate::image_util;
use crate::resource::*;
use crate::sound_util;
use crate::tile::{self, Tile, bytes_to_ints, ints_to_bytes, shorts_to_bytes, convert_to_4bpp, tile_attr_full};
use crate::tmx;
use crate::types::*;
use crate::util;
use crate::xml_util;
use crate::xml_util::TmxObject;

/// Execute a resource line and return the created resource ref (or pseudo-resource).
pub fn execute_line(compiler: &mut Compiler, fields: &[&str]) -> Result<Option<ResourceRef>, String> {
    if fields.is_empty() {
        return Err("Empty resource line".into());
    }

    let resource_type = fields[0].to_uppercase();
    match resource_type.as_str() {
        "ALIGN" => process_align(compiler, fields),
        "UNGROUP" => process_ungroup(compiler, fields),
        "NEAR" => process_near(compiler, fields),
        "BIN" => process_bin(compiler, fields),
        "PALETTE" => process_palette(compiler, fields),
        "BITMAP" => process_bitmap(compiler, fields),
        "TILESET" => process_tileset(compiler, fields),
        "TILEMAP" => process_tilemap(compiler, fields),
        "MAP" => process_map(compiler, fields),
        "OBJECTS" => process_objects(compiler, fields),
        "IMAGE" => process_image(compiler, fields),
        "SPRITE" => process_sprite(compiler, fields),
        "WAV" => process_wav(compiler, fields),
        "XGM" => process_xgm(compiler, fields),
        "XGM2" => process_xgm2(compiler, fields),
        _ => Err(format!("Unknown resource type '{}'", fields[0])),
    }
}

// ────────────────────────────────────────────
// ALIGN
// ────────────────────────────────────────────

fn process_align(compiler: &mut Compiler, fields: &[&str]) -> Result<Option<ResourceRef>, String> {
    let alignment = if fields.len() > 1 {
        util::parse_int(fields[1], 524288) as usize
    } else {
        524288
    };

    let res = Resource::Align(AlignResource { align: alignment });
    let idx = compiler.add_resource_raw("align", true, res);
    Ok(Some(idx))
}

// ────────────────────────────────────────────
// UNGROUP
// ────────────────────────────────────────────

fn process_ungroup(compiler: &mut Compiler, _fields: &[&str]) -> Result<Option<ResourceRef>, String> {
    let res = Resource::Ungroup(UngroupResource);
    let idx = compiler.add_resource_raw("ungroup", true, res);
    Ok(Some(idx))
}

// ────────────────────────────────────────────
// NEAR
// ────────────────────────────────────────────

fn process_near(compiler: &mut Compiler, _fields: &[&str]) -> Result<Option<ResourceRef>, String> {
    let res = Resource::Near(NearResource);
    let idx = compiler.add_resource_raw("near", true, res);
    Ok(Some(idx))
}

// ────────────────────────────────────────────
// BIN
// ────────────────────────────────────────────

fn process_bin(compiler: &mut Compiler, fields: &[&str]) -> Result<Option<ResourceRef>, String> {
    if fields.len() < 3 {
        return Err("Wrong BIN definition: BIN name file [align [size_align [fill [compression [far]]]]]".into());
    }

    let id = fields[1];
    let file_in = util::adjust_path(&compiler.res_dir, fields[2]);
    let align = if fields.len() >= 4 { util::parse_int(fields[3], 2) as usize } else { 2 };
    let salign = if fields.len() >= 5 { util::parse_int(fields[4], 2) as usize } else { 2 };
    let fill = if fields.len() >= 6 { util::parse_int(fields[5], 0) as u8 } else { 0 };
    let compression = if fields.len() >= 7 {
        util::get_compression(fields[6]).map_err(|e| e.to_string())?
    } else {
        Compression::None
    };
    if compression == Compression::Auto {
        return Err("Cannot use AUTO compression on BIN resource!".into());
    }
    let far = if fields.len() >= 8 { util::parse_bool(fields[7], true) } else { true };

    let data = std::fs::read(&file_in)
        .map_err(|e| format!("Cannot read file '{}': {}", file_in, e))?;

    compiler.add_resource_file(&file_in);

    let data = if salign > 0 { util::size_align(&data, salign, fill) } else { data };
    let hc = hash_bytes(&data) ^ ((align as u64) << 16) ^ (compression as u64);

    let res = Resource::Bin(BinResource {
        data,
        align,
        wanted_compression: compression,
        packed_data: None,
        done_compression: Compression::None,
        far,
        embedded: false,
        hc,
    });

    let idx = compiler.add_resource_raw(id, true, res);
    Ok(Some(idx))
}

// ────────────────────────────────────────────
// PALETTE
// ────────────────────────────────────────────

fn process_palette(compiler: &mut Compiler, fields: &[&str]) -> Result<Option<ResourceRef>, String> {
    if fields.len() < 3 {
        return Err("Wrong PALETTE definition: PALETTE name file".into());
    }

    let id = fields[1];
    let file_in = util::adjust_path(&compiler.res_dir, fields[2]);
    compiler.add_resource_file(&file_in);

    create_palette(compiler, id, &file_in, 64, true, false).map(Some)
}

// ────────────────────────────────────────────
// BITMAP
// ────────────────────────────────────────────

fn process_bitmap(compiler: &mut Compiler, fields: &[&str]) -> Result<Option<ResourceRef>, String> {
    if fields.len() < 3 {
        return Err("Wrong BITMAP definition: BITMAP name \"file\" [compression]".into());
    }

    let id = fields[1];
    let file_in = util::adjust_path(&compiler.res_dir, fields[2]);
    let compression = if fields.len() >= 4 {
        util::get_compression(fields[3]).map_err(|e| e.to_string())?
    } else {
        Compression::None
    };

    compiler.add_resource_file(&file_in);

    // Get basic info
    let info = image_util::get_basic_info(&file_in)
        .map_err(|e| format!("Cannot read image '{}': {}", file_in, e))?;

    let w = info.w;
    if (w & 1) == 1 {
        return Err(format!("'{}' width is {}, even width required.", file_in, w));
    }

    // Get 8bpp pixels
    let data = image_util::get_image_as_8bpp(&file_in, false, true)
        .map_err(|e| format!("Cannot convert image '{}': {}", file_in, e))?
        .ok_or_else(|| format!("Cannot convert image '{}' to 8bpp", file_in))?;

    let h = data.len() / w;

    // Check max color index
    let max_index = data.iter().map(|&b| b & 0xF).max().unwrap_or(0);
    if max_index >= 16 {
        return Err(format!("'{}' uses color index >= 16, BITMAP requires max 16 colors", file_in));
    }

    // Convert to 4bpp
    let data_4bpp = convert_to_4bpp(&data, 8);

    // Create BIN (image data)
    let bin_ref = create_bin_internal(compiler, &format!("{}_data", id), &data_4bpp, 2, compression, true, true);
    // Create PALETTE
    let pal_ref = create_palette(compiler, &format!("{}_palette", id), &file_in, 16, true, true)?;

    let bin_hc = compiler.resources[bin_ref].hash_code();
    let pal_hc: u64 = compiler.resources[pal_ref].hash_code();
    let hc = bin_hc ^ ((w as u64) << 8) ^ ((h as u64) << 16) ^ pal_hc;

    let res = Resource::Bitmap(BitmapResource { w, h, bin: bin_ref, palette: pal_ref, hc });
    let idx = compiler.add_resource_raw(id, true, res);
    Ok(Some(idx))
}

// ────────────────────────────────────────────
// TILESET
// ────────────────────────────────────────────

fn process_tileset(compiler: &mut Compiler, fields: &[&str]) -> Result<Option<ResourceRef>, String> {
    if fields.len() < 3 {
        return Err("Wrong TILESET definition: TILESET name \"file\" [compression [opt [ordering [export]]]]".into());
    }

    let id = fields[1];
    let file_in = util::adjust_path(&compiler.res_dir, fields[2]);

    let is_tsx = Path::new(&file_in).extension()
        .map(|e| e.to_string_lossy().to_lowercase() == "tsx")
        .unwrap_or(false);

    let compression = if fields.len() >= 4 {
        util::get_compression(fields[3]).map_err(|e| e.to_string())?
    } else {
        Compression::None
    };

    let opt = if !is_tsx && fields.len() >= 5 {
        util::get_tile_opt(fields[4]).map_err(|e| e.to_string())?
    } else {
        TileOptimization::All
    };

    let order = if fields.len() >= 6 {
        util::get_tile_ordering(fields[5]).map_err(|e| e.to_string())?
    } else {
        TileOrdering::Row
    };

    compiler.add_resource_file(&file_in);

    let img_file = if is_tsx {
        xml_util::get_tsx_tileset_path(&file_in)
            .map_err(|e| format!("Cannot parse TSX '{}': {}", file_in, e))?
    } else {
        file_in.clone()
    };

    create_tileset_from_image(compiler, id, &img_file, compression, opt, is_tsx, false, order, true).map(Some)
}

// ────────────────────────────────────────────
// TILEMAP
// ────────────────────────────────────────────

fn process_tilemap(compiler: &mut Compiler, fields: &[&str]) -> Result<Option<ResourceRef>, String> {
    if fields.len() < 4 {
        return Err("Wrong TILEMAP definition: TILEMAP name \"file\" tileset_id [compression [map_opt [map_base [ordering]]]]".into());
    }

    let id = fields[1];
    let file_in = util::adjust_path(&compiler.res_dir, fields[2]);
    compiler.add_resource_file(&file_in);

    let is_tmx = Path::new(&file_in).extension()
        .map(|e| e.to_string_lossy().to_lowercase() == "tmx")
        .unwrap_or(false);

    if is_tmx {
        let layer_name = fields[3];
        let ts_comp = if fields.len() >= 5 {
            util::get_compression(fields[4]).map_err(|e| e.to_string())?
        } else { Compression::None };
        let map_comp = if fields.len() >= 6 {
            util::get_compression(fields[5]).map_err(|e| e.to_string())?
        } else { Compression::None };
        let map_base = if fields.len() >= 7 { util::parse_int(fields[6], 0) } else { 0 };
        let order = if fields.len() >= 8 {
            util::get_tile_ordering(fields[7]).map_err(|e| e.to_string())?
        } else { TileOrdering::Row };

        // Process TMX into 8bpp image
        let tmx_result = tmx::process_tmx_to_image(&file_in, layer_name)
            .map_err(|e| format!("Cannot process TMX '{}': {}", file_in, e))?;

        // Track tileset image files
        for path in &tmx_result.tileset_image_paths {
            compiler.add_resource_file(path);
        }

        let img = &tmx_result.map_image;
        let img_w = tmx_result.image_width;
        let img_h = tmx_result.image_height;
        let wt = img_w / 8;
        let ht = img_h / 8;

        // Create tileset from reconstructed image
        let tileset_ref = create_tileset_data(
            compiler, &format!("{}_tileset", id),
            img, img_w, img_h, 0, 0, wt, ht,
            TileOptimization::All, ts_comp, true, false, order, false,
        )?;

        // Create tilemap from reconstructed image
        let tilemap_ref = create_tilemap_data(
            compiler, id,
            img, img_w, img_h, 0, 0, wt, ht,
            tileset_ref, map_base, TileOptimization::All, map_comp, order, false,
        )?;

        return Ok(Some(tilemap_ref));
    }

    // Image file mode
    let compression = if fields.len() >= 5 {
        util::get_compression(fields[4]).map_err(|e| e.to_string())?
    } else { Compression::None };
    let tile_opt = if fields.len() >= 6 {
        util::get_tile_opt(fields[5]).map_err(|e| e.to_string())?
    } else { TileOptimization::All };
    let map_base = if fields.len() >= 7 { util::parse_int(fields[6], 0) } else { 0 };
    let order = if fields.len() >= 8 {
        util::get_tile_ordering(fields[7]).map_err(|e| e.to_string())?
    } else { TileOrdering::Row };

    // Find tileset by id
    let tileset_ref = compiler.get_resource_by_id(fields[3])
        .ok_or_else(|| format!("TILEMAP: Tileset '{}' not found!", fields[3]))?;

    create_tilemap_from_image(compiler, id, &file_in, tileset_ref, map_base, tile_opt, compression, order).map(Some)
}

// ────────────────────────────────────────────
// IMAGE
// ────────────────────────────────────────────

fn process_image(compiler: &mut Compiler, fields: &[&str]) -> Result<Option<ResourceRef>, String> {
    if fields.len() < 3 {
        return Err("Wrong IMAGE definition: IMAGE name \"file\" [compression [map_opt [map_base]]]".into());
    }

    let id = fields[1];
    let file_in = util::adjust_path(&compiler.res_dir, fields[2]);
    let compression = if fields.len() >= 4 {
        util::get_compression(fields[3]).map_err(|e| e.to_string())?
    } else { Compression::None };
    let tile_opt = if fields.len() >= 5 {
        util::get_tile_opt(fields[4]).map_err(|e| e.to_string())?
    } else { TileOptimization::All };
    let map_base = if fields.len() >= 6 { util::parse_int(fields[5], 0) } else { 0 };

    compiler.add_resource_file(&file_in);

    // Get 8bpp pixels
    let image = image_util::get_image_as_8bpp(&file_in, true, true)
        .map_err(|e| format!("Cannot convert image '{}': {}", file_in, e))?
        .ok_or_else(|| format!("Cannot convert image '{}' to 8bpp", file_in))?;

    // Check for bit 6
    for &d in &image {
        if (d & 0x40) != 0 {
            return Err(format!("'{}' has color index in [64..127] range, IMAGE requires max 64 colors", file_in));
        }
    }

    let info = image_util::get_basic_info(&file_in)
        .map_err(|e| format!("Cannot get image info '{}': {}", file_in, e))?;
    let w = info.w;
    let h = image.len() / w;
    let wt = w / 8;
    let ht = h / 8;

    // Build TILESET with wanted compression
    let tileset_ref = create_tileset_data(compiler, &format!("{}_tileset", id), &image, w, h, 0, 0, wt, ht, tile_opt, compression, false, false, TileOrdering::Row, true)?;

    // Build TILEMAP with wanted compression
    let tilemap_ref = create_tilemap_data(compiler, &format!("{}_tilemap", id), &image, w, h, 0, 0, wt, ht, tileset_ref, map_base, tile_opt, compression, TileOrdering::Row, true)?;

    // Build PALETTE
    let palette_ref = create_palette(compiler, &format!("{}_palette", id), &file_in, 64, true, true)?;

    let ts_hc = compiler.resources[tileset_ref].hash_code();
    let tm_hc = compiler.resources[tilemap_ref].hash_code();
    let pal_hc = compiler.resources[palette_ref].hash_code();
    let hc = ts_hc ^ tm_hc ^ pal_hc;

    let res = Resource::Image(ImageResource { tileset: tileset_ref, tilemap: tilemap_ref, palette: palette_ref, hc });
    let idx = compiler.add_resource_raw(id, true, res);
    Ok(Some(idx))
}

// ────────────────────────────────────────────
// MAP
// ────────────────────────────────────────────

fn process_map(compiler: &mut Compiler, fields: &[&str]) -> Result<Option<ResourceRef>, String> {
    if fields.len() < 4 {
        return Err("Wrong MAP definition: MAP name \"file\" tileset_id [compression [map_base]]".into());
    }

    let id = fields[1];
    let file_in = util::adjust_path(&compiler.res_dir, fields[2]);
    compiler.add_resource_file(&file_in);

    let is_tmx = Path::new(&file_in).extension()
        .map(|e| e.to_string_lossy().to_lowercase() == "tmx")
        .unwrap_or(false);

    if is_tmx {
        // TMX mode
        let layer_name = fields[3];
        let ts_comp = if fields.len() >= 5 {
            util::get_compression(fields[4]).map_err(|e| e.to_string())?
        } else { Compression::None };
        let map_comp = if fields.len() >= 6 {
            util::get_compression(fields[5]).map_err(|e| e.to_string())?
        } else { Compression::None };
        let map_base = if fields.len() >= 7 { util::parse_int(fields[6], 0) } else { 0 };

        // Process TMX into 8bpp image
        let tmx_result = tmx::process_tmx_to_image(&file_in, layer_name)
            .map_err(|e| format!("Cannot process TMX '{}': {}", file_in, e))?;

        // Track tileset image files
        for path in &tmx_result.tileset_image_paths {
            compiler.add_resource_file(path);
        }

        let img = &tmx_result.map_image;
        let img_w = tmx_result.image_width;
        let img_h = tmx_result.image_height;
        let wt = img_w / 8;
        let ht = img_h / 8;

        // Create tileset from the TMX image
        let tileset_ref = create_tileset_data(
            compiler, &format!("{}_tileset", id),
            img, img_w, img_h, 0, 0, wt, ht,
            TileOptimization::All, ts_comp, true, false, TileOrdering::Row, false,
        )?;

        // Make tileset global (since it won't be referenced by a TILESET command)
        compiler.resources[tileset_ref].global = true;

        // Build MAP from the reconstructed image data
        return create_map_from_8bpp(
            compiler, id, img, img_w, img_h,
            map_base, 2, vec![tileset_ref], map_comp,
        ).map(Some);
    }

    // Image file mode
    let tileset_ref = compiler.get_resource_by_id(fields[3])
        .ok_or_else(|| format!("MAP: Tileset '{}' not found!", fields[3]))?;
    let compression = if fields.len() >= 5 {
        util::get_compression(fields[4]).map_err(|e| e.to_string())?
    } else { Compression::None };
    let map_base = if fields.len() >= 6 { util::parse_int(fields[5], 0) } else { 0 };

    create_map_from_image(compiler, id, &file_in, map_base, 2, vec![tileset_ref], compression, true).map(Some)
}

// ────────────────────────────────────────────
// OBJECTS
// ────────────────────────────────────────────

fn process_objects(compiler: &mut Compiler, fields: &[&str]) -> Result<Option<ResourceRef>, String> {
    if fields.len() < 5 {
        return Err("Wrong OBJECTS definition: OBJECTS name tmx_file layer_id field_defs [sortby:<field>] [decl_type [type_filter]]".into());
    }

    let id = fields[1];
    let file_in = util::adjust_path(&compiler.res_dir, fields[2]);
    let layer_name = fields[3];
    let field_defs_str = fields[4];

    let mut idx = 5;
    let _sort_by = if idx < fields.len() && fields[idx].starts_with("sortby:") {
        let sb = fields[idx][7..].to_string();
        idx += 1;
        sb
    } else {
        String::new()
    };

    let decl_type = if idx < fields.len() {
        let dt = fields[idx].to_string();
        idx += 1;
        dt
    } else {
        String::new()
    };

    let _type_filter = if idx < fields.len() {
        fields[idx].to_string()
    } else {
        String::new()
    };

    // Parse field definitions
    let field_defs = parse_field_defs(field_defs_str)?;

    compiler.add_resource_file(&file_in);

    // Parse TMX and extract objects
    let tmx_map = xml_util::parse_tmx(&file_in)
        .map_err(|e| format!("Cannot parse TMX '{}': {}", file_in, e))?;

    let mut objects = Vec::new();
    for og in &tmx_map.object_groups {
        if og.name == layer_name {
            for (obj_idx, obj) in og.objects.iter().enumerate() {
                let name = format!("{}_{}", id, obj_idx);
                let mut fields_vec = Vec::new();

                for (field_name, field_type) in &field_defs {
                    let value = get_object_field_value(obj, field_name);
                    fields_vec.push(SField::new(field_name, *field_type, &value));
                }

                objects.push(SObject::new(&name, obj.id as i32, fields_vec, &file_in));
            }
        }
    }

    let hc = hash_string(&decl_type) ^ hash_objects(&objects);
    let res = Resource::Objects(ObjectsResource {
        type_name: decl_type,
        objects,
        hc,
    });
    let idx_res = compiler.add_resource_raw(id, true, res);
    Ok(Some(idx_res))
}

// ────────────────────────────────────────────
// SPRITE
// ────────────────────────────────────────────

fn process_sprite(compiler: &mut Compiler, fields: &[&str]) -> Result<Option<ResourceRef>, String> {
    if fields.len() < 5 {
        return Err("Wrong SPRITE definition: SPRITE name \"file\" width height [compression [time [collision [opt_type [opt_level [opt_duplicate]]]]]]".into());
    }

    let id = fields[1];
    let file_in = util::adjust_path(&compiler.res_dir, fields[2]);

    let info = image_util::get_basic_info(&file_in)
        .map_err(|e| format!("Cannot get image info '{}': {}", file_in, e))?;

    let wf = parse_frame_dimension(fields[3], info.w, "width")?;
    let hf = parse_frame_dimension(fields[4], info.h, "height")?;

    if wf < 1 || hf < 1 {
        return Err("SPRITE width and height should be > 0".into());
    }
    if wf >= 32 || hf >= 32 {
        return Err("SPRITE width and height should be < 32 tiles".into());
    }

    let compression = if fields.len() >= 6 {
        util::get_compression(fields[5]).map_err(|e| e.to_string())?
    } else { Compression::None };

    let time = if fields.len() >= 7 {
        parse_time_array(fields[6])
    } else {
        vec![vec![0]]
    };

    let collision = if fields.len() >= 8 {
        util::get_collision(fields[7]).map_err(|e| e.to_string())?
    } else { CollisionType::None };

    let opt_type = if fields.len() >= 9 {
        util::get_sprite_opt_type(fields[8]).map_err(|e| e.to_string())?
    } else { OptimizationType::Balanced };

    let opt_level = if fields.len() >= 10 {
        util::get_sprite_opt_level(fields[9]).map_err(|e| e.to_string())?
    } else { OptimizationLevel::Fast };

    let opt_duplicate = if fields.len() >= 11 {
        util::parse_bool(fields[10], false)
    } else { false };

    compiler.add_resource_file(&file_in);

    create_sprite(compiler, id, &file_in, wf, hf, compression, &time, collision, opt_type, opt_level, opt_duplicate)
}

// ────────────────────────────────────────────
// WAV
// ────────────────────────────────────────────

fn process_wav(compiler: &mut Compiler, fields: &[&str]) -> Result<Option<ResourceRef>, String> {
    if fields.len() < 4 {
        return Err("Wrong WAV definition: WAV name \"file\" driver [out_rate [far]]".into());
    }

    let id = fields[1];
    let file_in = util::adjust_path(&compiler.res_dir, fields[2]);
    let driver = util::get_sound_driver(fields[3]).map_err(|e| e.to_string())?;

    let mut out_rate = match driver {
        SoundDriver::Pcm => 16000,
        SoundDriver::Dpcm2 => 22050,
        SoundDriver::Pcm4 => 16000,
        SoundDriver::Xgm => 14000,
        SoundDriver::Xgm2 => 13300,
    };

    if fields.len() >= 5 {
        out_rate = util::parse_int(fields[4], out_rate as i32) as u32;
    }

    let far = if fields.len() >= 6 { util::parse_bool(fields[5], false) } else { false };

    let mut pcm_data = sound_util::get_raw_data_from_wav(&file_in, 8, out_rate, true, true, false)
        .map_err(|e| format!("Error converting WAV '{}': {}", file_in, e))?;

    if driver == SoundDriver::Dpcm2 {
        pcm_data = util::dpcm_pack(&pcm_data);
    }

    compiler.add_resource_file(&file_in);

    let (align, salign, fill) = if driver == SoundDriver::Dpcm2 {
        (128, 128, 136u8)
    } else {
        (256, 256, 0u8)
    };

    let data = util::size_align(&pcm_data, salign, fill);
    let hc = hash_bytes(&data) ^ ((align as u64) << 16);

    let res = Resource::Bin(BinResource {
        data,
        align,
        wanted_compression: Compression::None,
        packed_data: None,
        done_compression: Compression::None,
        far,
        embedded: false,
        hc,
    });

    let idx = compiler.add_resource_raw(id, true, res);
    Ok(Some(idx))
}

// ────────────────────────────────────────────
// XGM
// ────────────────────────────────────────────

fn process_xgm(compiler: &mut Compiler, fields: &[&str]) -> Result<Option<ResourceRef>, String> {
    if fields.len() < 3 {
        return Err("Wrong XGM definition: XGM name file [timing [options]]".into());
    }

    let id = fields[1];
    let file_in = util::adjust_path(&compiler.res_dir, fields[2]);

    let timing = if fields.len() >= 4 { util::parse_int(fields[3], -1) } else { -1 };
    let options = if fields.len() >= 5 { fields[4] } else { "" };

    let file_out = set_extension(&file_in, ".bin");

    if !util::xgmtool(&compiler.current_dir, &file_in, &file_out, timing, options) {
        return Err(format!("Error compiling '{}' to BIN format", file_in));
    }

    let data = std::fs::read(&file_out)
        .map_err(|e| format!("Can't read data from '{}': {}", file_out, e))?;
    let _ = std::fs::remove_file(&file_out);

    compiler.add_resource_file(&file_in);

    let aligned = util::size_align(&data, 256, 0);
    let hc = hash_bytes(&aligned) ^ ((256u64) << 16);

    let res = Resource::Bin(BinResource {
        data: aligned,
        align: 256,
        wanted_compression: Compression::None,
        packed_data: None,
        done_compression: Compression::None,
        far: true,
        embedded: false,
        hc,
    });

    let idx = compiler.add_resource_raw(id, true, res);
    Ok(Some(idx))
}

// ────────────────────────────────────────────
// XGM2
// ────────────────────────────────────────────

fn process_xgm2(compiler: &mut Compiler, fields: &[&str]) -> Result<Option<ResourceRef>, String> {
    if fields.len() < 3 {
        return Err("Wrong XGM2 definition: XGM2 name file(s) [options]".into());
    }

    let id = fields[1];
    let mut file_ins = Vec::new();
    let mut options = String::new();

    let mut f = 2;
    while f < fields.len() {
        let field = fields[f];
        f += 1;
        if f == fields.len() && field.starts_with("-") {
            options = field.to_string();
        } else {
            file_ins.push(util::adjust_path(&compiler.res_dir, field));
        }
    }

    if !util::xgm2tool(&compiler.current_dir, &file_ins.iter().map(|s| s.as_str()).collect::<Vec<_>>().iter().map(|s| s.to_string()).collect::<Vec<_>>(), "out.xgc", &options) {
        return Err(format!("Error compiling file(s) to XGM2 format"));
    }

    let data = std::fs::read("out.xgc")
        .map_err(|e| format!("Can't read data from 'out.xgc': {}", e))?;
    let _ = std::fs::remove_file("out.xgc");

    for f in &file_ins {
        compiler.add_resource_file(f);
    }

    let aligned = util::size_align(&data, 256, 0);
    let hc = hash_bytes(&aligned) ^ ((256u64) << 16);

    let res = Resource::Bin(BinResource {
        data: aligned,
        align: 256,
        wanted_compression: Compression::None,
        packed_data: None,
        done_compression: Compression::None,
        far: true,
        embedded: false,
        hc,
    });

    let idx = compiler.add_resource_raw(id, true, res);
    Ok(Some(idx))
}

// ────────────────────────────────────────────
// Internal resource creation helpers
// ────────────────────────────────────────────

/// Create an internal BIN resource (with deduplication).
pub fn create_bin_internal(compiler: &mut Compiler, id: &str, data: &[u8], align: usize, compression: Compression, far: bool, internal: bool) -> ResourceRef {
    let hc = hash_bytes(data) ^ ((align as u64) << 16) ^ (compression as u64);

    let res = Resource::Bin(BinResource {
        data: data.to_vec(),
        align,
        wanted_compression: compression,
        packed_data: None,
        done_compression: Compression::None,
        far,
        embedded: true,
        hc,
    });

    if internal {
        compiler.add_internal_resource(id, res)
    } else {
        compiler.add_resource_raw(id, false, res)
    }
}

/// Create a BIN from int array data.
pub fn create_bin_from_ints(compiler: &mut Compiler, id: &str, data: &[i32], compression: Compression, internal: bool) -> ResourceRef {
    let bytes = ints_to_bytes(data);
    create_bin_internal(compiler, id, &bytes, 2, compression, true, internal)
}

/// Create a BIN from short array data.
pub fn create_bin_from_shorts(compiler: &mut Compiler, id: &str, data: &[i16], compression: Compression, far: bool, internal: bool) -> ResourceRef {
    let bytes = shorts_to_bytes(data);
    create_bin_internal(compiler, id, &bytes, 2, compression, far, internal)
}

/// Create a PALETTE resource.
pub fn create_palette(compiler: &mut Compiler, id: &str, file: &str, max_size: usize, align16: bool, internal: bool) -> Result<ResourceRef, String> {
    let ext = Path::new(file).extension()
        .map(|e| e.to_string_lossy().to_lowercase())
        .unwrap_or_default();

    let mut palette = if ext == "pal" {
        image_util::get_rgba4444_palette_from_pal_file(file, 0)
            .map_err(|e| format!("Cannot read palette '{}': {}", file, e))?
    } else {
        let is_tsx = ext == "tsx";
        let img_file = if is_tsx {
            xml_util::get_tsx_tileset_path(file)
                .map_err(|e| format!("Cannot parse TSX '{}': {}", file, e))?
        } else {
            file.to_string()
        };

        let info = image_util::get_basic_info(&img_file)
            .map_err(|e| format!("Cannot read image info '{}': {}", img_file, e))?;

        if info.bpp > 8 {
            image_util::get_rgba4444_palette_from_tiles(&img_file, 0)
                .map_err(|e| format!("Cannot extract palette from RGB image '{}': {}", img_file, e))?
                .ok_or_else(|| format!("Too many unique colors in '{}'", img_file))?
        } else {
            image_util::get_rgba4444_palette_from_image(&img_file, 0)
                .map_err(|e| format!("Cannot extract palette from indexed image '{}': {}", img_file, e))?
        }
    };

    let adj_max = if align16 { (max_size + 15) & 0xFFF0 } else { max_size };
    if palette.len() > adj_max {
        palette.truncate(adj_max);
    }

    // Convert shorts to bytes (big-endian)
    let bytes = shorts_to_bytes(&palette.iter().map(|&v| v as i16).collect::<Vec<_>>());

    let bin_ref = create_bin_internal(compiler, &format!("{}_data", id), &bytes, 2, Compression::None, false, true);
    let bin_hc = compiler.resources[bin_ref].hash_code();

    let res = Resource::Palette(PaletteResource { bin: bin_ref, hc: bin_hc });

    if internal {
        Ok(compiler.add_internal_resource(id, res))
    } else {
        Ok(compiler.add_resource_raw(id, true, res))
    }
}

/// Create a TILESET from image data.
pub fn create_tileset_data(
    compiler: &mut Compiler, id: &str,
    image: &[u8], img_w: usize, img_h: usize,
    start_tx: usize, start_ty: usize,
    wt: usize, ht: usize,
    opt: TileOptimization, compression: Compression,
    add_blank: bool, temp: bool, order: TileOrdering,
    internal: bool,
) -> Result<ResourceRef, String> {
    let ts_data = TilesetData::build_from_image(image, img_w, img_h, start_tx, start_ty, wt, ht, opt, order, add_blank);
    let num_tile = ts_data.num_tiles();
    let bin_data = ts_data.build_binary();

    let bin_ref = if temp {
        // Temporary tileset - don't add as internal resource
        let hc = hash_bytes(&bin_data) ^ ((2u64) << 16) ^ (compression as u64);
        compiler.add_resource_raw(&format!("{}_data", id), false, Resource::Bin(BinResource {
            data: bin_data,
            align: 2,
            wanted_compression: compression,
            packed_data: None,
            done_compression: Compression::None,
            far: true,
            embedded: true,
            hc,
        }))
    } else {
        create_bin_from_ints(compiler, &format!("{}_data", id), &bytes_to_ints(&bin_data).iter().map(|&v| v).collect::<Vec<_>>(), compression, true)
    };

    // Mark bin as not global
    compiler.resources[bin_ref].global = false;

    let is_duplicate = false; // Simplified
    let bin_hc = compiler.resources[bin_ref].hash_code();

    let res = Resource::Tileset(TilesetResource {
        num_tile,
        bin: bin_ref,
        is_duplicate,
        hc: bin_hc,
    });

    if internal {
        Ok(compiler.add_internal_resource(id, res))
    } else {
        Ok(compiler.add_resource_raw(id, true, res))
    }
}

/// Create a TILESET from an image file.
pub fn create_tileset_from_image(
    compiler: &mut Compiler, id: &str, img_file: &str,
    compression: Compression, opt: TileOptimization,
    add_blank: bool, temp: bool, order: TileOrdering, global: bool,
) -> Result<ResourceRef, String> {
    let image = image_util::get_image_as_8bpp(img_file, true, true)
        .map_err(|e| format!("Cannot convert image '{}': {}", img_file, e))?
        .ok_or_else(|| format!("Cannot convert image '{}' to 8bpp", img_file))?;

    let info = image_util::get_basic_info(img_file)
        .map_err(|e| format!("Cannot get image info '{}': {}", img_file, e))?;
    let w = info.w;
    let h = image.len() / w;

    create_tileset_data(compiler, id, &image, w, h, 0, 0, w / 8, h / 8, opt, compression, add_blank, temp, order, !global)
}

/// Create a TILEMAP from image data.
pub fn create_tilemap_data(
    compiler: &mut Compiler, id: &str,
    image: &[u8], img_w: usize, img_h: usize,
    start_tx: usize, start_ty: usize,
    wt: usize, ht: usize,
    _tileset_ref: ResourceRef,
    map_base: i32,
    opt: TileOptimization, compression: Compression,
    order: TileOrdering,
    internal: bool,
) -> Result<ResourceRef, String> {
    let map_base_prio = (map_base as u16 & tile::TILE_PRIORITY_MASK) != 0;
    let map_base_pal = ((map_base as u16 & tile::TILE_PALETTE_MASK) >> tile::TILE_PALETTE_SFT) as u16;
    let map_base_tile_ind = (map_base as u16 & tile::TILE_INDEX_MASK) as u16;
    let use_system_tiles = map_base_tile_ind != 0;

    // We need to rebuild the tileset data to look up tile indices
    // The tileset stores its tiles in the TilesetData which we need to reconstruct
    // For simplicity, rebuild from the image
    let ts_data = TilesetData::build_from_image(image, img_w, img_h, start_tx, start_ty, wt, ht, opt, order, false);

    let mut data = vec![0i16; wt * ht];
    let mut offset = 0;

    for j in 0..ht {
        for i in 0..wt {
            let ti = i + start_tx;
            let tj = j + start_ty;
            let tile_obj = Tile::get_tile(image, img_w, img_h, ti * 8, tj * 8, 8);
            let index;
            let mut equality = TileEquality::None;

            if opt == TileOptimization::None {
                index = if order == TileOrdering::Row {
                    ((j * wt) + i) as u16 + map_base_tile_ind
                } else {
                    ((i * ht) + j) as u16 + map_base_tile_ind
                };
            } else {
                if use_system_tiles && tile_obj.is_plain() {
                    index = tile_obj.get_plain_value() as u16;
                } else {
                    let idx = ts_data.get_tile_index(&tile_obj, opt);
                    if idx == -1 {
                        return Err(format!("Can't find tile [{},{}] in tileset", ti, tj));
                    }
                    equality = tile_obj.get_equality(ts_data.get(idx as usize));
                    index = idx as u16 + map_base_tile_ind;
                }
            }

            data[offset] = tile_attr_full(
                map_base_pal + tile_obj.pal as u16,
                map_base_prio || tile_obj.prio,
                equality.vflip(),
                equality.hflip(),
                index,
            ) as i16;
            offset += 1;
        }
    }

    let bin_ref = create_bin_from_shorts(compiler, &format!("{}_data", id), &data, compression, true, true);
    let bin_hc = compiler.resources[bin_ref].hash_code();
    let hc = bin_hc ^ ((wt as u64) << 8) ^ ((ht as u64) << 16);

    let res = Resource::Tilemap(TilemapResource { w: wt, h: ht, bin: bin_ref, hc });

    if internal {
        Ok(compiler.add_internal_resource(id, res))
    } else {
        Ok(compiler.add_resource_raw(id, true, res))
    }
}

/// Create a TILEMAP from an image file.
pub fn create_tilemap_from_image(
    compiler: &mut Compiler, id: &str, img_file: &str,
    tileset_ref: ResourceRef, map_base: i32,
    opt: TileOptimization, compression: Compression, order: TileOrdering,
) -> Result<ResourceRef, String> {
    let image = image_util::get_image_as_8bpp(img_file, true, true)
        .map_err(|e| format!("Cannot convert image '{}': {}", img_file, e))?
        .ok_or_else(|| format!("Cannot convert image '{}' to 8bpp", img_file))?;

    let info = image_util::get_basic_info(img_file)
        .map_err(|e| format!("Cannot get image info '{}': {}", img_file, e))?;
    let w = info.w;
    let h = image.len() / w;

    create_tilemap_data(compiler, id, &image, w, h, 0, 0, w / 8, h / 8, tileset_ref, map_base, opt, compression, order, false)
}

/// Create a MAP from image data.
pub fn create_map_from_image(
    compiler: &mut Compiler, id: &str, img_file: &str,
    map_base: i32, metatile_size: usize,
    tilesets: Vec<ResourceRef>,
    compression: Compression, _add_tileset: bool,
) -> Result<ResourceRef, String> {
    let image = image_util::get_image_as_8bpp(img_file, true, true)
        .map_err(|e| format!("Cannot convert image '{}': {}", img_file, e))?
        .ok_or_else(|| format!("Cannot convert image '{}' to 8bpp", img_file))?;

    // Check for bit 6
    for &d in &image {
        if (d & 0x40) != 0 {
            return Err(format!("'{}' has color index in [64..127] range", img_file));
        }
    }

    let info = image_util::get_basic_info(img_file)
        .map_err(|e| format!("Cannot get image info '{}': {}", img_file, e))?;
    let w = info.w;
    let h = image.len() / w;
    let wt = w / 8;
    let ht = h / 8;

    let map_base_prio = (map_base as u16 & tile::TILE_PRIORITY_MASK) != 0;
    let map_base_pal = ((map_base as u16 & tile::TILE_PALETTE_MASK) >> tile::TILE_PALETTE_SFT) as u16;
    let map_base_tile_ind = (map_base as u16 & tile::TILE_INDEX_MASK) as u16;
    let has_base_tile_index = map_base_tile_ind != 0;

    // Build global tileset from all provided tilesets
    let tileset = TilesetData::build_from_image(&image, w, h, 0, 0, wt, ht, TileOptimization::All, TileOrdering::Row, false);

    // Build metatiles, map blocks, etc.
    let wb = (wt + 15) / 16;
    let hb = (ht + 15) / 16;

    let mut metatiles: Vec<Metatile> = Vec::new();
    let mut map_blocks: Vec<MapBlock> = Vec::new();
    let mut map_block_indexes: Vec<Vec<i16>> = Vec::new();
    let mut map_block_row_offsets = vec![-1i16; hb];

    for j in 0..hb {
        let mut mb_row_indexes = vec![0i16; wb];

        for i in 0..wb {
            let mut mb = MapBlock::new();
            let mut mbi = 0;

            for bj in 0..8 {
                for bi in 0..8 {
                    let mut mt = Metatile::new(metatile_size * metatile_size);
                    let mut mtsi = 0;

                    for mj in 0..metatile_size {
                        for mi in 0..metatile_size {
                            let ti = (i * 16) + (bi * 2) + mi;
                            let tj = (j * 16) + (bj * 2) + mj;

                            let (index, eq) = if ti >= wt || tj >= ht {
                                (0u16, TileEquality::None)
                            } else {
                                let tile_obj = Tile::get_tile(&image, w, h, ti * 8, tj * 8, 8);
                                if has_base_tile_index && tile_obj.is_plain() {
                                    (tile_obj.get_plain_value() as u16, TileEquality::None)
                                } else {
                                    let idx = tileset.get_tile_index(&tile_obj, TileOptimization::All);
                                    if idx == -1 {
                                        return Err(format!("Can't find tile [{},{}] in tileset", ti, tj));
                                    }
                                    if idx > 2047 {
                                        return Err("Can't have more than 2048 different tiles".into());
                                    }
                                    let eq = tile_obj.get_equality(tileset.get(idx as usize));
                                    (idx as u16 + map_base_tile_ind, eq)
                                }
                            };

                            let tile_obj_for_attr = if ti < wt && tj < ht {
                                Tile::get_tile(&image, w, h, ti * 8, tj * 8, 8)
                            } else {
                                Tile::new(vec![0i32; 8], 8, 0, false, 0)
                            };

                            mt.set(mtsi, tile_attr_full(
                                map_base_pal + tile_obj_for_attr.pal as u16,
                                map_base_prio || tile_obj_for_attr.prio,
                                eq.vflip(), eq.hflip(), index,
                            ) as i16);
                            mtsi += 1;
                        }
                    }

                    mt.update_internals();

                    // Find existing metatile
                    let mt_index = metatiles.iter().position(|m| *m == mt)
                        .unwrap_or_else(|| {
                            let idx = metatiles.len();
                            metatiles.push(mt);
                            idx
                        });

                    mb.set(mbi, mt_index as i16);
                    mbi += 1;
                }
            }

            mb.compute_hash_code();

            let mb_index = map_blocks.iter().position(|m| *m == mb)
                .unwrap_or_else(|| {
                    let idx = map_blocks.len();
                    map_blocks.push(mb);
                    idx
                });

            mb_row_indexes[i] = mb_index as i16;
        }

        // Check for duplicate row
        let mut found_dup = false;
        for (ri, existing_row) in map_block_indexes.iter().enumerate() {
            if *existing_row == mb_row_indexes {
                map_block_row_offsets[j] = (ri * wb) as i16;
                found_dup = true;
                break;
            }
        }

        if !found_dup {
            map_block_row_offsets[j] = (map_block_indexes.len() * wb) as i16;
            map_block_indexes.push(mb_row_indexes);
        }
    }

    // Build binary data
    let mt_size = metatile_size * metatile_size;
    let mut mt_data: Vec<i16> = Vec::with_capacity(metatiles.len() * mt_size);
    for mt in &metatiles {
        mt_data.extend_from_slice(&mt.data);
    }
    let metatiles_bin = create_bin_from_shorts(compiler, &format!("{}_metatiles", id), &mt_data, compression, true, true);

    // Map blocks
    let map_blocks_bin = if metatiles.len() > 256 {
        let mut mb_data: Vec<i16> = Vec::with_capacity(map_blocks.len() * 64);
        for mb in &map_blocks {
            mb_data.extend_from_slice(&mb.data);
        }
        create_bin_from_shorts(compiler, &format!("{}_mapBlocks", id), &mb_data, compression, true, true)
    } else {
        let mut mb_data: Vec<u8> = Vec::with_capacity(map_blocks.len() * 64);
        for mb in &map_blocks {
            for &ind in &mb.data {
                mb_data.push(ind as u8);
            }
        }
        create_bin_internal(compiler, &format!("{}_mapBlocks", id), &mb_data, 2, compression, true, true)
    };

    // Map block indexes
    let map_block_indexes_bin = if map_blocks.len() > 256 {
        let mut mbi_data: Vec<i16> = Vec::new();
        for row in &map_block_indexes {
            mbi_data.extend_from_slice(row);
        }
        create_bin_from_shorts(compiler, &format!("{}_mapBlockIndexes", id), &mbi_data, compression, true, true)
    } else {
        let mut mbi_data: Vec<u8> = Vec::new();
        for row in &map_block_indexes {
            for &ind in row {
                mbi_data.push(ind as u8);
            }
        }
        create_bin_internal(compiler, &format!("{}_mapBlockIndexes", id), &mbi_data, 2, compression, true, true)
    };

    // Map block row offsets (never compressed)
    let map_block_row_offsets_bin = create_bin_from_shorts(
        compiler, &format!("{}_mapBlockRowOffsets", id),
        &map_block_row_offsets, Compression::None, true, true,
    );

    let hc = compiler.resources[metatiles_bin].hash_code()
        ^ compiler.resources[map_blocks_bin].hash_code()
        ^ compiler.resources[map_block_indexes_bin].hash_code()
        ^ compiler.resources[map_block_row_offsets_bin].hash_code();

    let num_block_index_rows = map_block_indexes.len();
    let num_metatiles = metatiles.len();
    let num_map_blocks = map_blocks.len();

    let res = Resource::Map(MapResource {
        wb, hb, compression,
        num_metatiles, num_map_blocks, num_block_index_rows,
        tilesets,
        metatiles_bin, map_blocks_bin, map_block_indexes_bin, map_block_row_offsets_bin,
        hc,
    });

    let idx = compiler.add_resource_raw(id, true, res);
    Ok(idx)
}

/// Create a MAP from pre-loaded 8bpp image data (used by TMX processor).
pub fn create_map_from_8bpp(
    compiler: &mut Compiler, id: &str,
    image: &[u8], w: usize, h: usize,
    map_base: i32, metatile_size: usize,
    tilesets: Vec<ResourceRef>,
    compression: Compression,
) -> Result<ResourceRef, String> {
    let wt = w / 8;
    let ht = h / 8;

    let map_base_prio = (map_base as u16 & tile::TILE_PRIORITY_MASK) != 0;
    let map_base_pal = ((map_base as u16 & tile::TILE_PALETTE_MASK) >> tile::TILE_PALETTE_SFT) as u16;
    let map_base_tile_ind = (map_base as u16 & tile::TILE_INDEX_MASK) as u16;
    let has_base_tile_index = map_base_tile_ind != 0;

    let tileset = TilesetData::build_from_image(image, w, h, 0, 0, wt, ht, TileOptimization::All, TileOrdering::Row, false);

    let wb = (wt + 15) / 16;
    let hb = (ht + 15) / 16;

    let mut metatiles: Vec<Metatile> = Vec::new();
    let mut map_blocks: Vec<MapBlock> = Vec::new();
    let mut map_block_indexes: Vec<Vec<i16>> = Vec::new();
    let mut map_block_row_offsets = vec![-1i16; hb];

    for j in 0..hb {
        let mut mb_row_indexes = vec![0i16; wb];
        for i in 0..wb {
            let mut mb = MapBlock::new();
            let mut mbi = 0;
            for bj in 0..8 {
                for bi in 0..8 {
                    let mut mt = Metatile::new(metatile_size * metatile_size);
                    let mut mtsi = 0;
                    for mj in 0..metatile_size {
                        for mi in 0..metatile_size {
                            let ti = (i * 16) + (bi * 2) + mi;
                            let tj = (j * 16) + (bj * 2) + mj;
                            let (index, eq) = if ti >= wt || tj >= ht {
                                (0u16, TileEquality::None)
                            } else {
                                let tile_obj = Tile::get_tile(image, w, h, ti * 8, tj * 8, 8);
                                if has_base_tile_index && tile_obj.is_plain() {
                                    (tile_obj.get_plain_value() as u16, TileEquality::None)
                                } else {
                                    let idx = tileset.get_tile_index(&tile_obj, TileOptimization::All);
                                    if idx == -1 {
                                        return Err(format!("Can't find tile [{},{}] in tileset", ti, tj));
                                    }
                                    if idx > 2047 {
                                        return Err("Can't have more than 2048 different tiles".into());
                                    }
                                    let eq = tile_obj.get_equality(tileset.get(idx as usize));
                                    (idx as u16 + map_base_tile_ind, eq)
                                }
                            };
                            let tile_obj_for_attr = if ti < wt && tj < ht {
                                Tile::get_tile(image, w, h, ti * 8, tj * 8, 8)
                            } else {
                                Tile::new(vec![0i32; 8], 8, 0, false, 0)
                            };
                            mt.set(mtsi, tile_attr_full(
                                map_base_pal + tile_obj_for_attr.pal as u16,
                                map_base_prio || tile_obj_for_attr.prio,
                                eq.vflip(), eq.hflip(), index,
                            ) as i16);
                            mtsi += 1;
                        }
                    }
                    mt.update_internals();
                    let mt_index = metatiles.iter().position(|m| *m == mt)
                        .unwrap_or_else(|| {
                            let idx = metatiles.len();
                            metatiles.push(mt);
                            idx
                        });
                    mb.set(mbi, mt_index as i16);
                    mbi += 1;
                }
            }
            mb.compute_hash_code();
            let mb_index = map_blocks.iter().position(|m| *m == mb)
                .unwrap_or_else(|| {
                    let idx = map_blocks.len();
                    map_blocks.push(mb);
                    idx
                });
            mb_row_indexes[i] = mb_index as i16;
        }

        let mut found_dup = false;
        for (ri, existing_row) in map_block_indexes.iter().enumerate() {
            if *existing_row == mb_row_indexes {
                map_block_row_offsets[j] = (ri * wb) as i16;
                found_dup = true;
                break;
            }
        }
        if !found_dup {
            map_block_row_offsets[j] = (map_block_indexes.len() * wb) as i16;
            map_block_indexes.push(mb_row_indexes);
        }
    }

    let mt_size = metatile_size * metatile_size;
    let mut mt_data: Vec<i16> = Vec::with_capacity(metatiles.len() * mt_size);
    for mt in &metatiles {
        mt_data.extend_from_slice(&mt.data);
    }
    let metatiles_bin = create_bin_from_shorts(compiler, &format!("{}_metatiles", id), &mt_data, compression, true, true);

    let map_blocks_bin = if metatiles.len() > 256 {
        let mut mb_data: Vec<i16> = Vec::with_capacity(map_blocks.len() * 64);
        for mb in &map_blocks { mb_data.extend_from_slice(&mb.data); }
        create_bin_from_shorts(compiler, &format!("{}_mapBlocks", id), &mb_data, compression, true, true)
    } else {
        let mut mb_data: Vec<u8> = Vec::with_capacity(map_blocks.len() * 64);
        for mb in &map_blocks { for &ind in &mb.data { mb_data.push(ind as u8); } }
        create_bin_internal(compiler, &format!("{}_mapBlocks", id), &mb_data, 2, compression, true, true)
    };

    let map_block_indexes_bin = if map_blocks.len() > 256 {
        let mut mbi_data: Vec<i16> = Vec::new();
        for row in &map_block_indexes { mbi_data.extend_from_slice(row); }
        create_bin_from_shorts(compiler, &format!("{}_mapBlockIndexes", id), &mbi_data, compression, true, true)
    } else {
        let mut mbi_data: Vec<u8> = Vec::new();
        for row in &map_block_indexes { for &ind in row { mbi_data.push(ind as u8); } }
        create_bin_internal(compiler, &format!("{}_mapBlockIndexes", id), &mbi_data, 2, compression, true, true)
    };

    let map_block_row_offsets_bin = create_bin_from_shorts(
        compiler, &format!("{}_mapBlockRowOffsets", id),
        &map_block_row_offsets, Compression::None, true, true,
    );

    let hc = compiler.resources[metatiles_bin].hash_code()
        ^ compiler.resources[map_blocks_bin].hash_code()
        ^ compiler.resources[map_block_indexes_bin].hash_code()
        ^ compiler.resources[map_block_row_offsets_bin].hash_code();

    let res = Resource::Map(MapResource {
        wb, hb, compression,
        num_metatiles: metatiles.len(),
        num_map_blocks: map_blocks.len(),
        num_block_index_rows: map_block_indexes.len(),
        tilesets,
        metatiles_bin, map_blocks_bin, map_block_indexes_bin, map_block_row_offsets_bin,
        hc,
    });

    let idx = compiler.add_resource_raw(id, true, res);
    Ok(idx)
}

/// Create a SPRITE resource.
pub fn create_sprite(
    compiler: &mut Compiler, id: &str, img_file: &str,
    wf: usize, hf: usize,
    compression: Compression, time: &[Vec<i32>],
    collision: CollisionType,
    opt_type: OptimizationType, opt_level: OptimizationLevel,
    _opt_duplicate: bool,
) -> Result<Option<ResourceRef>, String> {
    // Get 8bpp pixels
    let image = image_util::get_image_as_8bpp(img_file, true, true)
        .map_err(|e| format!("Cannot convert image '{}': {}", img_file, e))?
        .ok_or_else(|| format!("Cannot convert image '{}' to 8bpp", img_file))?;

    let info = image_util::get_basic_info(img_file)
        .map_err(|e| format!("Cannot get image info '{}': {}", img_file, e))?;
    let w = info.w;
    let h = image.len() / w;
    let wt = w / 8;
    let ht = h / 8;

    // Check max color index
    let max_index = image.iter().map(|&b| b & 0x3F).max().unwrap_or(0);
    if max_index >= 64 {
        return Err(format!("'{}' uses color index >= 64, SPRITE requires max 64 colors", img_file));
    }

    // Build PALETTE
    let palette_ref = create_palette(compiler, &format!("{}_palette", id), img_file, 16, true, true)?;

    let num_anim = ht / hf;
    let mut animations = Vec::new();
    let mut max_num_tile = 0;
    let mut max_num_sprite = 0;

    for anim_idx in 0..num_anim {
        let anim_time = &time[std::cmp::min(time.len() - 1, anim_idx)];
        let max_frame = wt / wf;

        // Find last non-transparent frame
        let mut last_frame = max_frame as i32 - 1;
        while last_frame >= 0 {
            let frame_x = (last_frame as usize * wf) * 8;
            let frame_y = (anim_idx * hf) * 8;
            if image_util::has_opaque_pixel_region(&image, w, frame_x, frame_y, wf * 8, hf * 8) {
                break;
            }
            last_frame -= 1;
        }

        let num_frame = (last_frame + 1) as usize;
        if num_frame == 0 {
            continue;
        }

        let mut frames = Vec::new();
        let mut anim_max_tile = 0;
        let mut anim_max_sprite = 0;

        let mut fi = 0;
        while fi < num_frame {
            let frame_x = (fi * wf) * 8;
            let frame_y = (anim_idx * hf) * 8;

            // Extract frame image
            let frame_image = image_util::get_sub_image(&image, w, frame_x, frame_y, wf * 8, hf * 8);

            let timer = anim_time[std::cmp::min(anim_time.len() - 1, fi)];

            // Use sprite cutter for VDP sprite decomposition
            let cut = crate::sprite_cutter::get_optimized_sprite_list(
                &frame_image, wf * 8, hf * 8, opt_type, opt_level,
            );

            if cut.cells.len() > 16 {
                return Err(format!("Sprite frame '{}' uses {} VDP sprites (max 16)", id, cut.cells.len()));
            }

            // Build VDP sprites
            let mut vdp_sprites = Vec::new();
            let mut sprites_for_tileset = Vec::new();

            for cell in &cut.cells {
                let spr = VDPSpriteInfo::new(
                    cell.x * 8, cell.y * 8,
                    cell.w, cell.h,
                    wf as i32, hf as i32,
                );
                sprites_for_tileset.push(Rect::new(cell.x * 8, cell.y * 8, cell.w * 8, cell.h * 8));
                vdp_sprites.push(spr);
            }

            // Build tileset for this frame
            let ts_data = TilesetData::build_from_sprites(&frame_image, wf * 8, hf * 8, &sprites_for_tileset);
            let frame_num_tile = ts_data.num_tiles();
            let bin_data = ts_data.build_binary();

            let ts_bin_ref = create_bin_internal(
                compiler,
                &format!("{}_animation{}_frame{}_tileset_data", id, anim_idx, fi),
                &bin_data, 2, compression, true, true,
            );
            compiler.resources[ts_bin_ref].global = false;

            let ts_hc = compiler.resources[ts_bin_ref].hash_code();
            let ts_ref = compiler.add_internal_resource(
                &format!("{}_animation{}_frame{}_tileset", id, anim_idx, fi),
                Resource::Tileset(TilesetResource {
                    num_tile: frame_num_tile,
                    bin: ts_bin_ref,
                    is_duplicate: false,
                    hc: ts_hc,
                }),
            );

            // Build collision
            let coll_ref = if collision != CollisionType::None {
                let shape = match collision {
                    CollisionType::Box => Some(CollisionShape::Box {
                        x: ((wf * 8) * 1 / 4) as i32,
                        y: ((hf * 8) * 1 / 4) as i32,
                        w: ((wf * 8) * 3 / 4) as i32,
                        h: ((hf * 8) * 3 / 4) as i32,
                    }),
                    CollisionType::Circle => Some(CollisionShape::Circle {
                        x: ((wf * 8) / 2) as i32,
                        y: ((hf * 8) / 2) as i32,
                        ray: ((wf * 8) * 3 / 8) as i32,
                    }),
                    _ => None,
                };

                if let Some(s) = shape {
                    let shape_hc = match &s {
                        CollisionShape::Box { x, y, w, h } => (*x as u64) ^ ((*y as u64) << 8) ^ ((*w as u64) << 16) ^ ((*h as u64) << 24),
                        CollisionShape::Circle { x, y, ray } => (*x as u64) ^ ((*y as u64) << 8) ^ ((*ray as u64) << 16),
                    };
                    let coll_type = match &s {
                        CollisionShape::Box { .. } => CollisionType::Box,
                        CollisionShape::Circle { .. } => CollisionType::Circle,
                    };
                    let hcc = (coll_type as u64) ^ shape_hc;

                    let coll = compiler.add_internal_resource(
                        &format!("{}_animation{}_frame{}_collision", id, anim_idx, fi),
                        Resource::Collision(CollisionResource {
                            type_hit: coll_type,
                            type_attack: CollisionType::None,
                            hit: Some(s),
                            attack: None,
                            hc: hcc,
                        }),
                    );
                    Some(coll)
                } else {
                    None
                }
            } else {
                None
            };

            let is_optimisable = vdp_sprites.len() == 1
                && (vdp_sprites[0].wt * 8) as usize == wf * 8
                && (vdp_sprites[0].ht * 8) as usize == hf * 8
                && vdp_sprites[0].offset_x == 0
                && vdp_sprites[0].offset_y == 0;

            let frame_hc = (timer as u64) << 16
                ^ ts_hc
                ^ vdp_sprites.len() as u64
                ^ coll_ref.map(|r| compiler.resources[r].hash_code()).unwrap_or(0);

            let frame_ref = compiler.add_internal_resource(
                &format!("{}_animation{}_frame{}", id, anim_idx, fi),
                Resource::SpriteFrame(SpriteFrameResource {
                    vdp_sprites,
                    collision: coll_ref,
                    tileset: ts_ref,
                    timer: timer as usize,
                    num_tile: frame_num_tile,
                    is_optimisable,
                    hc: frame_hc,
                }),
            );

            anim_max_tile = std::cmp::max(anim_max_tile, frame_num_tile);
            anim_max_sprite = std::cmp::max(anim_max_sprite, cut.cells.len());
            frames.push(frame_ref);

            fi += 1;
        }

        if frames.is_empty() {
            continue;
        }

        let anim_hc = frames.iter().fold(0u64, |acc, &f| acc ^ compiler.resources[f].hash_code());

        let anim_ref = compiler.add_internal_resource(
            &format!("{}_animation{}", id, anim_idx),
            Resource::SpriteAnimation(SpriteAnimationResource {
                frames,
                loop_index: 0,
                hc: anim_hc,
            }),
        );

        max_num_tile = std::cmp::max(max_num_tile, anim_max_tile);
        max_num_sprite = std::cmp::max(max_num_sprite, anim_max_sprite);
        animations.push(anim_ref);
    }

    let sprite_hc = (wf as u64) ^ ((hf as u64) << 8)
        ^ ((max_num_tile as u64) << 16) ^ ((max_num_sprite as u64) << 24)
        ^ animations.iter().fold(0u64, |acc, &a| acc ^ compiler.resources[a].hash_code())
        ^ compiler.resources[palette_ref].hash_code();

    let res = Resource::Sprite(SpriteResource {
        wf, hf, animations,
        max_num_tile, max_num_sprite,
        palette: palette_ref,
        hc: sprite_hc,
    });

    let idx = compiler.add_resource_raw(id, true, res);
    Ok(Some(idx))
}

// ────────────────────────────────────────────
// Helper functions
// ────────────────────────────────────────────

fn hash_bytes(data: &[u8]) -> u64 {
    use std::collections::hash_map::DefaultHasher;
    use std::hash::{Hash, Hasher};
    let mut hasher = DefaultHasher::new();
    data.hash(&mut hasher);
    hasher.finish()
}

fn hash_string(s: &str) -> u64 {
    use std::collections::hash_map::DefaultHasher;
    use std::hash::{Hash, Hasher};
    let mut hasher = DefaultHasher::new();
    s.hash(&mut hasher);
    hasher.finish()
}

fn hash_objects(objects: &[SObject]) -> u64 {
    use std::collections::hash_map::DefaultHasher;
    use std::hash::{Hash, Hasher};
    let mut hasher = DefaultHasher::new();
    objects.len().hash(&mut hasher);
    hasher.finish()
}

/// Parse field definitions string ("name:type;name:type;...")
fn parse_field_defs(defs_str: &str) -> Result<LinkedHashMap<String, SGDKObjectType>, String> {
    let mut result = LinkedHashMap::new();
    for def in defs_str.split(';') {
        let parts: Vec<&str> = def.split(':').collect();
        if parts.len() != 2 {
            return Err(format!("Invalid field definition '{}': expected name:type", def));
        }
        let name = parts[0].trim().to_lowercase();
        let type_str = parts[1].trim();
        let sgdk_type = SGDKObjectType::parse(type_str)
            .ok_or_else(|| format!("Unknown field type '{}' in definition '{}'", type_str, def))?;
        result.insert(name, sgdk_type);
    }
    if result.is_empty() {
        return Err("Field definition cannot be empty".into());
    }
    Ok(result)
}

/// Get a field value from a TMX object.
fn get_object_field_value(obj: &TmxObject, field_name: &str) -> String {
    match field_name {
        "x" => format!("{}", obj.x),
        "y" => format!("{}", obj.y),
        "width" | "w" => format!("{}", obj.width),
        "height" | "h" => format!("{}", obj.height),
        "id" => format!("{}", obj.id),
        "name" => obj.name.clone(),
        "type" => obj.type_name.clone(),
        _ => {
            // Look in custom properties
            obj.properties.iter()
                .find(|(name, _, _)| name == field_name)
                .map(|(_, _, value)| value.clone())
                .unwrap_or_default()
        }
    }
}

/// Parse sprite frame dimension (supports tile count, pixel 'P' suffix, frame count 'F' suffix).
fn parse_frame_dimension(arg: &str, total_pixels: usize, dim_name: &str) -> Result<usize, String> {
    let upper = arg.to_uppercase();
    if upper.ends_with('P') {
        let pixels: usize = upper[..upper.len()-1].parse()
            .map_err(|_| format!("Invalid {} pixel value: {}", dim_name, arg))?;
        if (pixels % 8) != 0 {
            return Err(format!("Sprite {} ({} pixels) is not a multiple of 8", dim_name, pixels));
        }
        Ok(pixels / 8)
    } else if upper.ends_with('F') {
        let frames: usize = upper[..upper.len()-1].parse()
            .map_err(|_| format!("Invalid {} frame count: {}", dim_name, arg))?;
        if (total_pixels % frames) != 0 {
            return Err(format!("Image {} ({}) is not a multiple of frame count ({})", dim_name, total_pixels, frames));
        }
        Ok(total_pixels / frames / 8)
    } else {
        arg.parse()
            .map_err(|_| format!("Invalid {} value: {}", dim_name, arg))
    }
}

/// Parse time array for sprite animations.
/// Supports: "0", "5", "[[3,3,3][4,5,5]]"
fn parse_time_array(text: &str) -> Vec<Vec<i32>> {
    let text = text.trim();

    // Simple single value
    if let Ok(v) = text.parse::<i32>() {
        return vec![vec![v]];
    }

    // Complex array format
    let text = text.replace('[', "").replace(']', " ");
    let mut result = Vec::new();
    for part in text.split_whitespace() {
        let mut anim = Vec::new();
        for val in part.split(',') {
            if let Ok(v) = val.trim().parse::<i32>() {
                anim.push(v);
            }
        }
        if !anim.is_empty() {
            result.push(anim);
        }
    }

    if result.is_empty() {
        vec![vec![0]]
    } else {
        result
    }
}

/// Set file extension.
fn set_extension(path: &str, ext: &str) -> String {
    if let Some(dot) = path.rfind('.') {
        format!("{}{}", &path[..dot], ext)
    } else {
        format!("{}{}", path, ext)
    }
}
