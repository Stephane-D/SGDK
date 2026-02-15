//! Resource types and management for rescomp.
//!
//! Each resource type corresponds to a Java resource class from the original implementation.
//! Resources are stored centrally in the Compiler and referenced by `ResourceRef` indices.

use std::collections::HashMap;

use crate::tile::{Tile, ints_to_bytes};
use crate::types::*;
use crate::util;

/// Reference to a resource (index into Compiler's resource list).
pub type ResourceRef = usize;

// ────────────────────────────────────────────
// Collision shapes (used by Collision resource)
// ────────────────────────────────────────────

#[derive(Debug, Clone)]
pub enum CollisionShape {
    Box { x: i32, y: i32, w: i32, h: i32 },
    Circle { x: i32, y: i32, ray: i32 },
}

// ────────────────────────────────────────────
// VDPSprite info (used by SpriteFrame)
// ────────────────────────────────────────────

#[derive(Debug, Clone)]
pub struct VDPSpriteInfo {
    pub offset_x: i32,
    pub offset_y: i32,
    pub wt: i32,
    pub ht: i32,
    pub offset_x_flip: i32,
    pub offset_y_flip: i32,
}

impl VDPSpriteInfo {
    pub fn new(off_x: i32, off_y: i32, w: i32, h: i32, wf: i32, hf: i32) -> Self {
        Self {
            offset_x: off_x,
            offset_y: off_y,
            wt: w,
            ht: h,
            offset_x_flip: (wf * 8) - (off_x + (w * 8)),
            offset_y_flip: (hf * 8) - (off_y + (h * 8)),
        }
    }

    pub fn formatted_size(&self) -> i32 {
        ((self.wt - 1) << 2) | (self.ht - 1)
    }

    /// Write VDP sprite data inline (within AnimationFrame).
    pub fn write_inline_asm(&self, out_s: &mut String) {
        // respect field order: offsetY, offsetYFlip, size, offsetX, offsetXFlip, numTile
        let w1 = (self.offset_y << 8) | (self.offset_y_flip & 0xFF);
        let w2 = (self.formatted_size() << 8) | (self.offset_x & 0xFF);
        let w3 = (self.offset_x_flip << 8) | ((self.ht * self.wt) & 0xFF);
        out_s.push_str(&format!("    dc.w    {}\n", w1));
        out_s.push_str(&format!("    dc.w    {}\n", w2));
        out_s.push_str(&format!("    dc.w    {}\n", w3));
    }
}

// ────────────────────────────────────────────
// Metatile / MapBlock types (used by Map)
// ────────────────────────────────────────────

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Metatile {
    pub data: Vec<i16>,
    hc: u64,
}

impl Metatile {
    pub fn new(size: usize) -> Self {
        Self {
            data: vec![0i16; size],
            hc: 0,
        }
    }

    pub fn set(&mut self, index: usize, value: i16) {
        self.data[index] = value;
    }

    pub fn update_internals(&mut self) {
        self.hc = 0;
        for &v in &self.data {
            self.hc = self.hc.wrapping_add(v as u16 as u64);
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct MapBlock {
    pub data: Vec<i16>,
    hc: u64,
}

impl MapBlock {
    pub fn new() -> Self {
        Self {
            data: vec![0i16; 64], // 8x8
            hc: 0,
        }
    }

    pub fn set(&mut self, index: usize, value: i16) {
        self.data[index] = value;
    }

    pub fn compute_hash_code(&mut self) {
        self.hc = 0;
        for &v in &self.data {
            self.hc = self.hc.wrapping_add(v as u16 as u64);
        }
    }
}

// ────────────────────────────────────────────
// SObject for TMX objects export
// ────────────────────────────────────────────

#[derive(Debug, Clone)]
pub struct SFieldDef {
    pub name: String,
    pub sgdk_type: SGDKObjectType,
}

#[derive(Debug, Clone)]
pub struct SField {
    pub name: String,
    pub sgdk_type: SGDKObjectType,
    pub value: String,
    pub resolved: bool,
}

impl SField {
    pub fn new(name: &str, sgdk_type: SGDKObjectType, value: &str) -> Self {
        Self {
            name: name.to_string(),
            sgdk_type,
            value: value.to_string(),
            resolved: false,
        }
    }
}

#[derive(Debug, Clone)]
pub struct SObject {
    pub name: String,
    pub tiled_id: i32,
    pub fields: Vec<SField>,
    pub file_name: String,
}

impl SObject {
    pub fn new(name: &str, tiled_id: i32, fields: Vec<SField>, file_name: &str) -> Self {
        Self {
            name: name.to_string(),
            tiled_id,
            fields,
            file_name: file_name.to_string(),
        }
    }

    pub fn get_name(&self) -> &str {
        &self.name
    }

    pub fn get_file_name(&self) -> &str {
        &self.file_name
    }

    pub fn size(&self) -> usize {
        let mut s = 0;
        for f in &self.fields {
            s += f.sgdk_type.size();
        }
        // align to 2
        (s + 1) & !1
    }

    /// Update object field references for cross-linking.
    pub fn update_object_fields_references_to(&mut self, external: &SObject) {
        for field in &mut self.fields {
            if field.sgdk_type == SGDKObjectType::Object && !field.resolved {
                // value contains the tiled id of the referenced object
                if let Ok(ref_id) = field.value.parse::<i32>() {
                    if ref_id == external.tiled_id {
                        field.value = external.name.clone();
                        field.resolved = true;
                    }
                }
            }
        }
    }

    pub fn out(&self, _out_b: &mut Vec<u8>, out_s: &mut String, out_h: &mut String) {
        // declare object
        util::decl(out_s, out_h, "", &self.name, 2, false);

        for field in &self.fields {
            match field.sgdk_type {
                SGDKObjectType::U8 | SGDKObjectType::S8 | SGDKObjectType::Bool => {
                    let v: i32 = field.value.parse().unwrap_or(0);
                    out_s.push_str(&format!("    dc.b    {}\n", v));
                }
                SGDKObjectType::U16 | SGDKObjectType::S16 => {
                    let v: i32 = field.value.parse().unwrap_or(0);
                    out_s.push_str(&format!("    dc.w    {}\n", v));
                }
                SGDKObjectType::U32 | SGDKObjectType::S32 => {
                    let v: i32 = field.value.parse().unwrap_or(0);
                    out_s.push_str(&format!("    dc.l    {}\n", v));
                }
                SGDKObjectType::F16 => {
                    let fv: f64 = field.value.parse().unwrap_or(0.0);
                    let v = (fv * 256.0) as i32;
                    out_s.push_str(&format!("    dc.w    {}\n", v));
                }
                SGDKObjectType::F32 => {
                    let fv: f64 = field.value.parse().unwrap_or(0.0);
                    let v = (fv * 65536.0) as i32;
                    out_s.push_str(&format!("    dc.l    {}\n", v));
                }
                SGDKObjectType::String => {
                    out_s.push_str(&format!("    dc.l    {}_str\n", self.name));
                }
                SGDKObjectType::Object => {
                    if field.resolved {
                        out_s.push_str(&format!("    dc.l    {}\n", field.value));
                    } else {
                        out_s.push_str("    dc.l    0\n");
                    }
                }
            }
        }

        // align to word boundary
        let size: usize = self.fields.iter().map(|f| f.sgdk_type.size()).sum();
        if (size & 1) != 0 {
            out_s.push_str("    dc.b    0\n");
        }

        // output string data after the struct
        for field in &self.fields {
            if field.sgdk_type == SGDKObjectType::String {
                out_s.push_str(&format!("{}_str:\n", self.name));
                out_s.push_str(&format!("    .ascii  \"{}\"\n", field.value));
                out_s.push_str("    dc.b    0\n");
                // align
                if (field.value.len() + 1) & 1 != 0 {
                    out_s.push_str("    dc.b    0\n");
                }
            }
        }
        out_s.push('\n');
    }
}

// ────────────────────────────────────────────
// Internal Tileset data
// Used for tile indexing during resource creation
// ────────────────────────────────────────────

/// Tileset data used during resource creation.
/// Not stored in the Resource enum - used as a temporary structure.
pub struct TilesetData {
    pub tiles: Vec<Tile>,
    tile_indexes: HashMap<u64, Vec<usize>>,
}

impl TilesetData {
    pub fn new() -> Self {
        Self {
            tiles: Vec::new(),
            tile_indexes: HashMap::new(),
        }
    }

    pub fn add(&mut self, tile: Tile) {
        let hc = tile.hc;
        let index = self.tiles.len();
        self.tiles.push(tile);
        self.tile_indexes.entry(hc).or_insert_with(Vec::new).push(index);
    }

    pub fn num_tiles(&self) -> usize {
        self.tiles.len()
    }

    pub fn is_empty(&self) -> bool {
        self.tiles.is_empty()
    }

    pub fn get(&self, index: usize) -> &Tile {
        &self.tiles[index]
    }

    pub fn get_tile_index(&self, tile: &Tile, opt: TileOptimization) -> i32 {
        if opt == TileOptimization::None {
            return -1;
        }

        // Fast perfect match - check all tiles with same hash
        if let Some(indices) = self.tile_indexes.get(&tile.hc) {
            for &idx in indices {
                if self.tiles[idx].data == tile.data {
                    return idx as i32;
                }
            }
        }

        // Allow flip?
        if opt == TileOptimization::All {
            if let Some(indices) = self.tile_indexes.get(&tile.hc) {
                for &idx in indices {
                    if self.tiles[idx].get_flip_equality(tile) != TileEquality::None {
                        return idx as i32;
                    }
                }
            }
        }

        -1
    }

    /// Build binary data (4bpp tile data as int array -> bytes).
    pub fn build_binary(&self) -> Vec<u8> {
        let mut data = Vec::with_capacity(self.tiles.len() * 8 * 4);
        for t in &self.tiles {
            let bytes = ints_to_bytes(&t.data);
            data.extend_from_slice(&bytes);
        }
        data
    }

    /// Build from 8bpp image with tile optimization.
    pub fn build_from_image(
        image: &[u8], w: usize, h: usize,
        start_tx: usize, start_ty: usize,
        wt: usize, ht: usize,
        opt: TileOptimization, order: TileOrdering,
        add_blank: bool,
    ) -> Self {
        let mut ts = Self::new();
        let mut has_blank = false;

        match order {
            TileOrdering::Row => {
                for j in 0..ht {
                    for i in 0..wt {
                        let tile = Tile::get_tile(image, w, h, (i + start_tx) * 8, (j + start_ty) * 8, 8);
                        has_blank |= tile.is_blank();
                        if ts.get_tile_index(&tile, opt) == -1 {
                            ts.add(tile);
                        }
                    }
                }
            }
            TileOrdering::Column => {
                for i in 0..wt {
                    for j in 0..ht {
                        let tile = Tile::get_tile(image, w, h, (i + start_tx) * 8, (j + start_ty) * 8, 8);
                        has_blank |= tile.is_blank();
                        if ts.get_tile_index(&tile, opt) == -1 {
                            ts.add(tile);
                        }
                    }
                }
            }
        }

        if !has_blank && add_blank {
            ts.add(Tile::new(vec![0i32; 8], 8, 0, false, 0));
        }

        ts
    }

    /// Build from sprite cells (vertical ordering per cell for VDP sprites).
    pub fn build_from_sprites(
        image: &[u8], img_w: usize, img_h: usize,
        sprites: &[Rect],
    ) -> Self {
        let mut ts = Self::new();
        for rect in sprites {
            let wt = rect.w as usize / 8;
            let ht = rect.h as usize / 8;
            // vertical ordering within each sprite
            for i in 0..wt {
                for j in 0..ht {
                    let tile = Tile::get_tile(
                        image, img_w, img_h,
                        rect.x as usize + i * 8,
                        rect.y as usize + j * 8,
                        8,
                    );
                    ts.add(tile);
                }
            }
        }
        ts
    }

    /// Merge multiple tilesets (for TMX maps).
    pub fn merge(tilesets: &[TilesetData]) -> Self {
        let mut ts = Self::new();
        for tileset in tilesets {
            for tile in &tileset.tiles {
                ts.add(tile.clone());
            }
        }
        ts
    }
}

// ────────────────────────────────────────────
// Resource types
// ────────────────────────────────────────────

/// Binary data resource.
#[derive(Debug, Clone)]
pub struct BinResource {
    pub data: Vec<u8>,
    pub align: usize,
    pub wanted_compression: Compression,
    pub packed_data: Option<PackedData>,
    pub done_compression: Compression,
    pub far: bool,
    pub embedded: bool,
    pub hc: u64,
}

/// Palette resource.
#[derive(Debug, Clone)]
pub struct PaletteResource {
    pub bin: ResourceRef,
    pub hc: u64,
}

/// Bitmap resource (4bpp unstructured).
#[derive(Debug, Clone)]
pub struct BitmapResource {
    pub w: usize,
    pub h: usize,
    pub bin: ResourceRef,
    pub palette: ResourceRef,
    pub hc: u64,
}

/// Image resource (tiled).
#[derive(Debug, Clone)]
pub struct ImageResource {
    pub tileset: ResourceRef,
    pub tilemap: ResourceRef,
    pub palette: ResourceRef,
    pub hc: u64,
}

/// Tileset resource.
#[derive(Debug, Clone)]
pub struct TilesetResource {
    pub num_tile: usize,
    pub bin: ResourceRef,
    pub is_duplicate: bool,
    pub hc: u64,
}

/// Tilemap resource.
#[derive(Debug, Clone)]
pub struct TilemapResource {
    pub w: usize,
    pub h: usize,
    pub bin: ResourceRef,
    pub hc: u64,
}

/// Map resource.
#[derive(Debug, Clone)]
pub struct MapResource {
    pub wb: usize,
    pub hb: usize,
    pub compression: Compression,
    pub num_metatiles: usize,
    pub num_map_blocks: usize,
    pub num_block_index_rows: usize,
    pub tilesets: Vec<ResourceRef>,
    pub metatiles_bin: ResourceRef,
    pub map_blocks_bin: ResourceRef,
    pub map_block_indexes_bin: ResourceRef,
    pub map_block_row_offsets_bin: ResourceRef,
    pub hc: u64,
}

/// Objects resource (from TMX).
#[derive(Debug, Clone)]
pub struct ObjectsResource {
    pub type_name: String,
    pub objects: Vec<SObject>,
    pub hc: u64,
}

/// Sprite resource.
#[derive(Debug, Clone)]
pub struct SpriteResource {
    pub wf: usize,
    pub hf: usize,
    pub animations: Vec<ResourceRef>,
    pub max_num_tile: usize,
    pub max_num_sprite: usize,
    pub palette: ResourceRef,
    pub hc: u64,
}

/// Sprite animation resource.
#[derive(Debug, Clone)]
pub struct SpriteAnimationResource {
    pub frames: Vec<ResourceRef>,
    pub loop_index: usize,
    pub hc: u64,
}

/// Sprite frame resource.
#[derive(Debug, Clone)]
pub struct SpriteFrameResource {
    pub vdp_sprites: Vec<VDPSpriteInfo>,
    pub collision: Option<ResourceRef>,
    pub tileset: ResourceRef,
    pub timer: usize,
    pub num_tile: usize,
    pub is_optimisable: bool,
    pub hc: u64,
}

/// VDP Sprite resource (FrameVDPSprite struct).
#[derive(Debug, Clone)]
pub struct VDPSpriteResource {
    pub info: VDPSpriteInfo,
    pub hc: u64,
}

/// Collision resource.
#[derive(Debug, Clone)]
pub struct CollisionResource {
    pub type_hit: CollisionType,
    pub type_attack: CollisionType,
    pub hit: Option<CollisionShape>,
    pub attack: Option<CollisionShape>,
    pub hc: u64,
}

/// Align pseudo-resource.
#[derive(Debug, Clone)]
pub struct AlignResource {
    pub align: usize,
}

/// Near pseudo-resource.
#[derive(Debug, Clone)]
pub struct NearResource;

/// Ungroup pseudo-resource.
#[derive(Debug, Clone)]
pub struct UngroupResource;

// ────────────────────────────────────────────
// Resource enum
// ────────────────────────────────────────────

#[derive(Debug, Clone)]
pub enum Resource {
    Bin(BinResource),
    Palette(PaletteResource),
    Bitmap(BitmapResource),
    Image(ImageResource),
    Tileset(TilesetResource),
    Tilemap(TilemapResource),
    Map(MapResource),
    Objects(ObjectsResource),
    Sprite(SpriteResource),
    SpriteAnimation(SpriteAnimationResource),
    SpriteFrame(SpriteFrameResource),
    VDPSprite(VDPSpriteResource),
    Collision(CollisionResource),
    Align(AlignResource),
    Near(NearResource),
    Ungroup(UngroupResource),
}

/// Stored resource with ID and global flag.
#[derive(Debug, Clone)]
pub struct StoredResource {
    pub id: String,
    pub global: bool,
    pub resource: Resource,
}

impl StoredResource {
    pub fn new(id: &str, global: bool, resource: Resource) -> Self {
        Self {
            id: id.to_string(),
            global,
            resource,
        }
    }

    /// Get internal hash code for deduplication.
    pub fn hash_code(&self) -> u64 {
        match &self.resource {
            Resource::Bin(r) => r.hc,
            Resource::Palette(r) => r.hc,
            Resource::Bitmap(r) => r.hc,
            Resource::Image(r) => r.hc,
            Resource::Tileset(r) => r.hc,
            Resource::Tilemap(r) => r.hc,
            Resource::Map(r) => r.hc,
            Resource::Objects(r) => r.hc,
            Resource::Sprite(r) => r.hc,
            Resource::SpriteAnimation(r) => r.hc,
            Resource::SpriteFrame(r) => r.hc,
            Resource::VDPSprite(r) => r.hc,
            Resource::Collision(r) => r.hc,
            Resource::Align(_) => 0,
            Resource::Near(_) => 0,
            Resource::Ungroup(_) => 1,
        }
    }

    /// Check internal equality for deduplication.
    pub fn internal_equals(&self, other: &StoredResource, resources: &[StoredResource]) -> bool {
        match (&self.resource, &other.resource) {
            (Resource::Bin(a), Resource::Bin(b)) => {
                a.align == b.align && a.wanted_compression == b.wanted_compression && a.data == b.data
            }
            (Resource::Palette(a), Resource::Palette(b)) => {
                bins_equal(a.bin, b.bin, resources)
            }
            (Resource::Bitmap(a), Resource::Bitmap(b)) => {
                a.w == b.w && a.h == b.h
                    && bins_equal(a.bin, b.bin, resources)
                    && palettes_equal(a.palette, b.palette, resources)
            }
            (Resource::Image(a), Resource::Image(b)) => {
                tilesets_equal(a.tileset, b.tileset, resources)
                    && tilemaps_equal(a.tilemap, b.tilemap, resources)
                    && palettes_equal(a.palette, b.palette, resources)
            }
            (Resource::Tileset(a), Resource::Tileset(b)) => {
                bins_equal(a.bin, b.bin, resources)
            }
            (Resource::Tilemap(a), Resource::Tilemap(b)) => {
                a.w == b.w && a.h == b.h && bins_equal(a.bin, b.bin, resources)
            }
            (Resource::VDPSprite(a), Resource::VDPSprite(b)) => {
                a.info.offset_x == b.info.offset_x
                    && a.info.offset_y == b.info.offset_y
                    && a.info.wt == b.info.wt
                    && a.info.ht == b.info.ht
                    && a.info.offset_x_flip == b.info.offset_x_flip
                    && a.info.offset_y_flip == b.info.offset_y_flip
            }
            (Resource::Collision(a), Resource::Collision(b)) => {
                a.type_hit == b.type_hit && a.type_attack == b.type_attack
            }
            (Resource::SpriteFrame(a), Resource::SpriteFrame(b)) => {
                a.timer == b.timer && a.tileset == b.tileset
                    && a.vdp_sprites.len() == b.vdp_sprites.len()
                    && a.collision == b.collision
            }
            (Resource::SpriteAnimation(a), Resource::SpriteAnimation(b)) => {
                a.loop_index == b.loop_index && a.frames == b.frames
            }
            _ => false,
        }
    }

    /// Check if this resource is a Bin.
    pub fn is_bin(&self) -> bool {
        matches!(self.resource, Resource::Bin(_))
    }

    /// Get the Bin data if this is a Bin resource.
    pub fn as_bin(&self) -> Option<&BinResource> {
        if let Resource::Bin(b) = &self.resource { Some(b) } else { None }
    }

    pub fn as_bin_mut(&mut self) -> Option<&mut BinResource> {
        if let Resource::Bin(b) = &mut self.resource { Some(b) } else { None }
    }

    /// Get internal bin resource refs (for grouping optimization).
    pub fn get_internal_bin_resources(&self) -> Vec<ResourceRef> {
        match &self.resource {
            Resource::Bin(_) => vec![],
            Resource::Palette(r) => vec![r.bin],
            Resource::Bitmap(r) => vec![r.bin],
            Resource::Image(_) => vec![],
            Resource::Tileset(r) => vec![r.bin],
            Resource::Tilemap(r) => vec![r.bin],
            Resource::Map(r) => vec![r.metatiles_bin, r.map_blocks_bin, r.map_block_indexes_bin, r.map_block_row_offsets_bin],
            _ => vec![],
        }
    }

    /// Shallow size (just the metadata struct, no referenced objects).
    pub fn shallow_size(&self) -> usize {
        match &self.resource {
            Resource::Bin(r) => {
                if let Some(pd) = &r.packed_data {
                    pd.data.len()
                } else {
                    r.data.len()
                }
            }
            Resource::Palette(_) => 2 + 4,
            Resource::Bitmap(_) => 2 + 2 + 2 + 4 + 4,
            Resource::Image(_) => 4 + 4 + 4,
            Resource::Tileset(_) => 2 + 2 + 4,
            Resource::Tilemap(_) => 2 + 2 + 2 + 4,
            Resource::Map(_) => 2 + 2 + 2 + 2 + 2 + 2 + 4 + 4 + 4 + 4,
            Resource::Objects(r) => r.objects.len() * 4,
            Resource::Sprite(r) => (r.animations.len() * 4) + 2 + 2 + 4 + 2 + 4 + 2 + 2,
            Resource::SpriteAnimation(r) => (r.frames.len() * 4) + 1 + 1 + 4,
            Resource::SpriteFrame(r) => (r.vdp_sprites.len() * 6) + 1 + 1 + 4 + 4,
            Resource::VDPSprite(_) => 6,
            Resource::Collision(r) => {
                let mut s = 2 + 1 + 1;
                if r.type_attack != CollisionType::None {
                    s += 8;
                } else {
                    if r.hit.is_some() { s += 4; }
                    if r.attack.is_some() { s += 4; }
                }
                s
            }
            Resource::Align(_) | Resource::Near(_) | Resource::Ungroup(_) => 0,
        }
    }

    /// Output resource to ASM.
    pub fn out(&self, out_b: &mut Vec<u8>, out_s: &mut String, out_h: &mut String, resources: &[StoredResource]) {
        let id = &self.id;
        let global = self.global;

        match &self.resource {
            Resource::Bin(r) => self.out_bin(r, id, global, out_b, out_s, out_h),
            Resource::Palette(r) => self.out_palette(r, id, global, out_b, out_s, out_h, resources),
            Resource::Bitmap(r) => self.out_bitmap(r, id, global, out_b, out_s, out_h, resources),
            Resource::Image(r) => self.out_image(r, id, global, out_b, out_s, out_h, resources),
            Resource::Tileset(r) => self.out_tileset(r, id, global, out_b, out_s, out_h, resources),
            Resource::Tilemap(r) => self.out_tilemap(r, id, global, out_b, out_s, out_h, resources),
            Resource::Map(r) => self.out_map(r, id, global, out_b, out_s, out_h, resources),
            Resource::Objects(r) => self.out_objects(r, id, global, out_b, out_s, out_h),
            Resource::Sprite(r) => self.out_sprite(r, id, global, out_b, out_s, out_h, resources),
            Resource::SpriteAnimation(r) => self.out_sprite_animation(r, id, global, out_b, out_s, out_h, resources),
            Resource::SpriteFrame(r) => self.out_sprite_frame(r, id, global, out_b, out_s, out_h, resources),
            Resource::VDPSprite(r) => self.out_vdp_sprite(r, id, global, out_b, out_s, out_h),
            Resource::Collision(r) => self.out_collision(r, id, global, out_b, out_s, out_h),
            Resource::Align(r) => { out_b.clear(); out_s.push_str(&format!("    .align  {}\n\n", r.align)); }
            Resource::Near(_) | Resource::Ungroup(_) => {}
        }
    }

    // ── Output methods for each type ──

    fn out_bin(&self, r: &BinResource, id: &str, global: bool, out_b: &mut Vec<u8>, out_s: &mut String, out_h: &mut String) {
        // Align binary stream
        util::align_stream(out_b, r.align);

        // Pack data
        let packed = util::pack(&r.data, r.wanted_compression, out_b, !r.embedded);

        // Print compression info
        if r.wanted_compression != Compression::None {
            match packed.compression {
                Compression::None => {
                    println!("'{}' not packed (size = {})", id, r.data.len());
                }
                Compression::Aplib => {
                    println!("'{}' packed with APLIB, size = {} ({}% - origin size = {})",
                        id, packed.data.len(),
                        (packed.data.len() as f64 * 100.0 / r.data.len() as f64).round() as i32,
                        r.data.len());
                }
                Compression::Lz4w => {
                    println!("'{}' packed with LZ4W, size = {} ({}% - origin size = {})",
                        id, packed.data.len(),
                        (packed.data.len() as f64 * 100.0 / r.data.len() as f64).round() as i32,
                        r.data.len());
                }
                _ => {}
            }
        }

        // Output binary data
        out_b.extend_from_slice(&packed.data);

        // Declare array
        util::decl_array(out_s, out_h, "u8", id, packed.data.len(), r.align, global);
        util::out_s(out_s, &packed.data, 1);
        util::decl_array_end(out_s, out_h, "u8", id, packed.data.len(), r.align, global);
        out_s.push('\n');
    }

    fn out_palette(&self, r: &PaletteResource, id: &str, global: bool, out_b: &mut Vec<u8>, out_s: &mut String, out_h: &mut String, resources: &[StoredResource]) {
        out_b.clear();
        let bin = &resources[r.bin];
        let bin_data_len = bin.as_bin().map(|b| b.data.len()).unwrap_or(0);

        util::decl(out_s, out_h, "Palette", id, 2, global);
        out_s.push_str(&format!("    dc.w    {}\n", bin_data_len / 2));
        out_s.push_str(&format!("    dc.l    {}\n", bin.id));
        out_s.push('\n');
    }

    fn out_bitmap(&self, r: &BitmapResource, id: &str, global: bool, out_b: &mut Vec<u8>, out_s: &mut String, out_h: &mut String, resources: &[StoredResource]) {
        out_b.clear();
        let bin = &resources[r.bin];
        let pal = &resources[r.palette];
        let comp = bin.as_bin().map(|b| b.done_compression).unwrap_or(Compression::None);

        util::decl(out_s, out_h, "Bitmap", id, 2, global);
        out_s.push_str(&format!("    dc.w    {}\n", comp.ordinal() - 1));
        out_s.push_str(&format!("    dc.w    {}, {}\n", r.w, r.h));
        out_s.push_str(&format!("    dc.l    {}\n", pal.id));
        out_s.push_str(&format!("    dc.l    {}\n", bin.id));
        out_s.push('\n');
    }

    fn out_image(&self, r: &ImageResource, id: &str, global: bool, out_b: &mut Vec<u8>, out_s: &mut String, out_h: &mut String, resources: &[StoredResource]) {
        out_b.clear();
        util::decl(out_s, out_h, "Image", id, 2, global);
        out_s.push_str(&format!("    dc.l    {}\n", resources[r.palette].id));
        out_s.push_str(&format!("    dc.l    {}\n", resources[r.tileset].id));
        out_s.push_str(&format!("    dc.l    {}\n", resources[r.tilemap].id));
        out_s.push('\n');
    }

    fn out_tileset(&self, r: &TilesetResource, id: &str, global: bool, out_b: &mut Vec<u8>, out_s: &mut String, out_h: &mut String, resources: &[StoredResource]) {
        out_b.clear();
        let bin = &resources[r.bin];
        let comp = bin.as_bin().map(|b| b.done_compression).unwrap_or(Compression::None);

        util::decl(out_s, out_h, "TileSet", id, 2, global);
        out_s.push_str(&format!("    dc.w    {}\n", comp.ordinal() - 1));
        out_s.push_str(&format!("    dc.w    {}\n", r.num_tile));
        out_s.push_str(&format!("    dc.l    {}\n", bin.id));
        out_s.push('\n');
    }

    fn out_tilemap(&self, r: &TilemapResource, id: &str, global: bool, out_b: &mut Vec<u8>, out_s: &mut String, out_h: &mut String, resources: &[StoredResource]) {
        out_b.clear();
        let bin = &resources[r.bin];
        let comp = bin.as_bin().map(|b| b.done_compression).unwrap_or(Compression::None);

        util::decl(out_s, out_h, "TileMap", id, 2, global);
        out_s.push_str(&format!("    dc.w    {}\n", comp.ordinal() - 1));
        out_s.push_str(&format!("    dc.w    {}, {}\n", r.w, r.h));
        out_s.push_str(&format!("    dc.l    {}\n", bin.id));
        out_s.push('\n');
    }

    fn out_map(&self, r: &MapResource, id: &str, global: bool, out_b: &mut Vec<u8>, out_s: &mut String, out_h: &mut String, resources: &[StoredResource]) {
        out_b.clear();

        // Compute compression value
        let mt_comp = resources[r.metatiles_bin].as_bin()
            .and_then(|b| b.packed_data.as_ref())
            .map(|p| p.compression.ordinal() - 1)
            .unwrap_or(0);
        let mb_comp = resources[r.map_blocks_bin].as_bin()
            .and_then(|b| b.packed_data.as_ref())
            .map(|p| p.compression.ordinal() - 1)
            .unwrap_or(0);
        let mbi_comp = resources[r.map_block_indexes_bin].as_bin()
            .and_then(|b| b.packed_data.as_ref())
            .map(|p| p.compression.ordinal() - 1)
            .unwrap_or(0);
        let comp_val = (mbi_comp << 8) | (mb_comp << 4) | mt_comp;

        util::decl(out_s, out_h, "MapDefinition", id, 2, global);
        out_s.push_str(&format!("    dc.w    {}, {}\n", r.wb, r.hb));
        out_s.push_str(&format!("    dc.w    {}\n", r.num_block_index_rows));
        out_s.push_str(&format!("    dc.w    {}\n", comp_val));
        out_s.push_str(&format!("    dc.w    {}\n", r.num_metatiles));
        out_s.push_str(&format!("    dc.w    {}\n", r.num_map_blocks));
        out_s.push_str(&format!("    dc.l    {}\n", resources[r.metatiles_bin].id));
        out_s.push_str(&format!("    dc.l    {}\n", resources[r.map_blocks_bin].id));
        out_s.push_str(&format!("    dc.l    {}\n", resources[r.map_block_indexes_bin].id));
        out_s.push_str(&format!("    dc.l    {}\n", resources[r.map_block_row_offsets_bin].id));
        out_s.push('\n');
    }

    fn out_objects(&self, r: &ObjectsResource, id: &str, global: bool, out_b: &mut Vec<u8>, out_s: &mut String, out_h: &mut String) {
        out_b.clear();

        // Export individual objects first
        for obj in &r.objects {
            obj.out(out_b, out_s, out_h);
        }

        let type_name = if r.type_name.is_empty() { "void" } else { &r.type_name };
        util::decl_array(out_s, out_h, &format!("{}*", type_name), id, r.objects.len(), 2, global);
        for obj in &r.objects {
            out_s.push_str(&format!("    dc.l    {}\n", obj.get_name()));
        }
        out_s.push('\n');
    }

    fn out_sprite(&self, r: &SpriteResource, id: &str, global: bool, out_b: &mut Vec<u8>, out_s: &mut String, out_h: &mut String, resources: &[StoredResource]) {
        out_b.clear();

        // Animations pointer table
        util::decl(out_s, out_h, "", &format!("{}_animations", id), 2, false);
        for &anim_ref in &r.animations {
            out_s.push_str(&format!("    dc.l    {}\n", resources[anim_ref].id));
        }
        out_s.push('\n');

        // SpriteDefinition structure
        util::decl(out_s, out_h, "SpriteDefinition", id, 2, global);
        out_s.push_str(&format!("    dc.w    {}\n", r.wf * 8));
        out_s.push_str(&format!("    dc.w    {}\n", r.hf * 8));
        out_s.push_str(&format!("    dc.l    {}\n", resources[r.palette].id));
        out_s.push_str(&format!("    dc.w    {}\n", r.animations.len()));
        out_s.push_str(&format!("    dc.l    {}_animations\n", id));
        out_s.push_str(&format!("    dc.w    {}\n", r.max_num_tile));
        out_s.push_str(&format!("    dc.w    {}\n", r.max_num_sprite));
        out_s.push('\n');
    }

    fn out_sprite_animation(&self, r: &SpriteAnimationResource, id: &str, global: bool, out_b: &mut Vec<u8>, out_s: &mut String, out_h: &mut String, resources: &[StoredResource]) {
        out_b.clear();

        // Frames pointer table
        util::decl(out_s, out_h, "", &format!("{}_frames", id), 2, false);
        for &frame_ref in &r.frames {
            out_s.push_str(&format!("    dc.l    {}\n", resources[frame_ref].id));
        }
        out_s.push('\n');

        // Animation structure
        util::decl(out_s, out_h, "Animation", id, 2, global);
        let num_and_loop = (r.frames.len() << 8) | (r.loop_index & 0xFF);
        out_s.push_str(&format!("    dc.w    {}\n", num_and_loop));
        out_s.push_str(&format!("    dc.l    {}_frames\n", id));
        out_s.push('\n');
    }

    fn out_sprite_frame(&self, r: &SpriteFrameResource, id: &str, global: bool, out_b: &mut Vec<u8>, out_s: &mut String, out_h: &mut String, resources: &[StoredResource]) {
        out_b.clear();

        // AnimationFrame structure
        util::decl(out_s, out_h, "AnimationFrame", id, 2, global);
        let num_sprite = if r.is_optimisable { 0x81 } else { r.vdp_sprites.len() };
        let sprite_timer = ((num_sprite << 8) & 0xFF00) | (r.timer & 0xFF);
        out_s.push_str(&format!("    dc.w    {}\n", sprite_timer));
        out_s.push_str(&format!("    dc.l    {}\n", resources[r.tileset].id));
        match r.collision {
            Some(coll_ref) => out_s.push_str(&format!("    dc.l    {}\n", resources[coll_ref].id)),
            None => out_s.push_str("    dc.l    0\n"),
        }

        // Inline VDP sprites
        for spr in &r.vdp_sprites {
            spr.write_inline_asm(out_s);
        }
        out_s.push('\n');
    }

    fn out_vdp_sprite(&self, r: &VDPSpriteResource, id: &str, global: bool, out_b: &mut Vec<u8>, out_s: &mut String, out_h: &mut String) {
        util::decl(out_s, out_h, "FrameVDPSprite", id, 2, global);
        r.info.write_inline_asm(out_s);

        // Write binary data
        out_b.push(r.info.offset_y as u8);
        out_b.push(r.info.offset_y_flip as u8);
        out_b.push(r.info.formatted_size() as u8);
        out_b.push(r.info.offset_x as u8);
        out_b.push(r.info.offset_x_flip as u8);
        out_b.push((r.info.ht * r.info.wt) as u8);
        out_s.push('\n');
    }

    fn out_collision(&self, r: &CollisionResource, id: &str, global: bool, out_b: &mut Vec<u8>, out_s: &mut String, out_h: &mut String) {
        util::decl(out_s, out_h, "Collision", id, 2, global);

        // Collision types
        let types_word = ((r.type_hit as i32) << 8) | (r.type_attack as i32 & 0xFF);
        out_s.push_str(&format!("    dc.w    {}\n", types_word));
        out_b.push(r.type_hit as u8);
        out_b.push(r.type_attack as u8);

        // Hit collision
        Self::out_collision_shape(&r.hit, r.type_attack != CollisionType::None, out_b, out_s);
        // Attack collision
        Self::out_collision_shape(&r.attack, false, out_b, out_s);
        out_s.push('\n');
    }

    fn out_collision_shape(shape: &Option<CollisionShape>, force_export: bool, out_b: &mut Vec<u8>, out_s: &mut String) {
        match shape {
            Some(CollisionShape::Box { x, y, w, h }) => {
                let w1 = ((*x as i32) << 8) | ((*y as i32) & 0xFF);
                let w2 = ((*w as i32) << 8) | ((*h as i32) & 0xFF);
                out_s.push_str(&format!("    dc.w    {}\n", w1));
                out_s.push_str(&format!("    dc.w    {}\n", w2));
                out_b.push(*x as u8);
                out_b.push(*y as u8);
                out_b.push(*w as u8);
                out_b.push(*h as u8);
            }
            Some(CollisionShape::Circle { x, y, ray }) => {
                let w1 = ((*x as i32) << 8) | ((*y as i32) & 0xFF);
                out_s.push_str(&format!("    dc.w    {}\n", w1));
                out_s.push_str(&format!("    dc.w    {}\n", ray));
                out_b.push(*x as u8);
                out_b.push(*y as u8);
                let rv = *ray as u16;
                out_b.push((rv >> 8) as u8);
                out_b.push(rv as u8);
            }
            None => {
                if force_export {
                    out_s.push_str("    dc.w    0\n");
                    out_s.push_str("    dc.w    0\n");
                    out_b.extend_from_slice(&[0, 0, 0, 0]);
                }
            }
        }
    }
}

// ── Equality helpers for resource deduplication ──

fn bins_equal(a: ResourceRef, b: ResourceRef, resources: &[StoredResource]) -> bool {
    let ra = &resources[a];
    let rb = &resources[b];
    ra.hash_code() == rb.hash_code() && ra.internal_equals(rb, resources)
}

fn palettes_equal(a: ResourceRef, b: ResourceRef, resources: &[StoredResource]) -> bool {
    bins_equal(a, b, resources) // Palette equality is based on bin equality
}

fn tilesets_equal(a: ResourceRef, b: ResourceRef, resources: &[StoredResource]) -> bool {
    let ra = &resources[a];
    let rb = &resources[b];
    ra.hash_code() == rb.hash_code() && ra.internal_equals(rb, resources)
}

fn tilemaps_equal(a: ResourceRef, b: ResourceRef, resources: &[StoredResource]) -> bool {
    let ra = &resources[a];
    let rb = &resources[b];
    ra.hash_code() == rb.hash_code() && ra.internal_equals(rb, resources)
}
