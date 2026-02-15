//! Main compilation logic for rescomp.
//!
//! The Compiler struct holds all state (resources, file lists) and orchestrates
//! the .res → .s/.h compilation pipeline.

use std::collections::{HashMap, HashSet};
use std::path::Path;

use crate::processor;
use crate::resource::*;
use crate::types::*;

/// Main compiler state.
pub struct Compiler {
    /// All stored resources (indexed by ResourceRef).
    pub resources: Vec<StoredResource>,
    /// Dedup map: hash → list of indices.
    dedup_map: HashMap<u64, Vec<usize>>,
    /// All referenced resource files (for dependency generation).
    resource_files: HashSet<String>,
    /// Application directory.
    pub current_dir: String,
    /// Resource file directory.
    pub res_dir: String,
}

impl Compiler {
    pub fn new() -> Self {
        Self {
            resources: Vec::new(),
            dedup_map: HashMap::new(),
            resource_files: HashSet::new(),
            current_dir: String::new(),
            res_dir: String::new(),
        }
    }

    /// Add a resource file path for dependency tracking.
    pub fn add_resource_file(&mut self, file: &str) {
        self.resource_files.insert(file.to_string());
    }

    /// Add a resource (raw, no dedup). Returns its resource ref.
    pub fn add_resource_raw(&mut self, id: &str, global: bool, resource: Resource) -> ResourceRef {
        let idx = self.resources.len();
        let stored = StoredResource::new(id, global, resource);
        let hc = stored.hash_code();
        self.dedup_map.entry(hc).or_default().push(idx);
        self.resources.push(stored);
        idx
    }

    /// Add an internal resource with deduplication.
    /// If a resource with the same hash and equal content already exists, return the existing index.
    pub fn add_internal_resource(&mut self, id: &str, resource: Resource) -> ResourceRef {
        let stored = StoredResource::new(id, false, resource);
        let hc = stored.hash_code();

        // Check for existing duplicate
        if let Some(indices) = self.dedup_map.get(&hc) {
            for &idx in indices {
                if self.resources[idx].internal_equals(&stored, &self.resources) {
                    eprintln!("Info: '{}' has same content as '{}'", id, self.resources[idx].id);
                    return idx;
                }
            }
        }

        let idx = self.resources.len();
        self.dedup_map.entry(hc).or_default().push(idx);
        self.resources.push(stored);
        idx
    }

    /// Find a resource by ID.
    pub fn get_resource_by_id(&self, id: &str) -> Option<ResourceRef> {
        if id.eq_ignore_ascii_case("NULL") {
            return None;
        }
        self.resources.iter().position(|r| r.id == id)
    }

    /// Compile a .res file.
    pub fn compile(
        &mut self,
        file_name: &str,
        file_name_out: &str,
        do_asm: bool,
        do_header: bool,
        dep_target: Option<&str>,
    ) -> bool {
        // Get current dir
        self.current_dir = std::env::current_dir()
            .map(|p| p.to_string_lossy().to_string())
            .unwrap_or_default();

        // Get res dir
        self.res_dir = Path::new(file_name)
            .parent()
            .map(|p| p.to_string_lossy().to_string())
            .unwrap_or_default();

        // Reset state
        self.resources.clear();
        self.dedup_map.clear();
        self.resource_files.clear();

        // Read input file
        let content = match std::fs::read_to_string(file_name) {
            Ok(c) => c,
            Err(e) => {
                eprintln!("Couldn't open input file {}:", file_name);
                eprintln!("{}", e);
                return false;
            }
        };

        let mut align: i32 = -1;
        let mut group = true;
        let mut near = false;

        // Process input line by line
        for (line_num, raw_line) in content.lines().enumerate() {
            let line = raw_line.trim().replace('\t', " ");
            // Collapse multiple spaces
            let line = collapse_spaces(&line);

            if line.is_empty() || line.starts_with("//") || line.starts_with('#') {
                continue;
            }

            // Parse fields (split on spaces, preserve quoted strings)
            let fields = split_fields(&line);
            let field_refs: Vec<&str> = fields.iter().map(|s| s.as_str()).collect();

            // Execute
            match processor::execute_line(self, &field_refs) {
                Ok(Some(idx)) => {
                    let res = &self.resources[idx];
                    match &res.resource {
                        Resource::Align(a) => {
                            align = a.align as i32;
                            println!();
                        }
                        Resource::Ungroup(_) => {
                            group = false;
                            println!();
                        }
                        Resource::Near(_) => {
                            near = true;
                            println!();
                        }
                        _ => {
                            let total = res.shallow_size();
                            println!(" '{}' raw size: {} bytes", res.id, total);
                        }
                    }
                }
                Ok(None) => {}
                Err(e) => {
                    eprintln!("{}: error on line {}", file_name, line_num + 1);
                    eprintln!("{}", e);
                    return false;
                }
            }
        }

        // Resolve object field references
        self.resolve_object_references();

        println!();

        // --- Build output ---
        let mut out_s = String::with_capacity(8192);
        let mut out_h = String::with_capacity(4096);
        let mut out_b: Vec<u8> = Vec::new();

        // Build header guard name
        let mut header_name = Path::new(&self.res_dir)
            .file_name()
            .map(|n| n.to_string_lossy().to_string())
            .unwrap_or_else(|| "RES".to_string());

        let out_stem = Path::new(file_name_out)
            .file_stem()
            .map(|n| n.to_string_lossy().to_string())
            .unwrap_or_default();
        header_name = format!("{}_{}", header_name, out_stem).to_uppercase();

        out_h.push_str("#include <genesis.h>\n\n");
        out_h.push_str(&format!("#ifndef _{}_H_\n", header_name));
        out_h.push_str(&format!("#define _{}_H_\n\n", header_name));

        // --- BINARY SECTION ---

        // Near BIN resources
        let bin_resources_near = self.get_bin_resources(false);
        // Far BIN resources
        let bin_resources_far = self.get_bin_resources(true);

        // Palette BIN resources (always near, always first)
        let pal_bins = self.get_internal_bin_resources_of_type(ResourceType::Palette, false);
        // Grouped internal bins
        let mut grouped_near_bins = Vec::new();
        let mut grouped_far_bins = Vec::new();

        if group {
            // Near: Tilemap, Tileset, Bitmap, Map
            self.collect_internal_bin_resources_of_type(ResourceType::Tilemap, false, &mut grouped_near_bins);
            self.collect_internal_bin_resources_of_type(ResourceType::Tileset, false, &mut grouped_near_bins);
            self.collect_internal_bin_resources_of_type(ResourceType::Bitmap, false, &mut grouped_near_bins);
            self.collect_map_internal_bins(false, &mut grouped_near_bins);
            // Remove palette bins from grouped
            grouped_near_bins.retain(|idx| !pal_bins.contains(idx));

            // Far: same types
            self.collect_internal_bin_resources_of_type(ResourceType::Tilemap, true, &mut grouped_far_bins);
            self.collect_internal_bin_resources_of_type(ResourceType::Tileset, true, &mut grouped_far_bins);
            self.collect_internal_bin_resources_of_type(ResourceType::Bitmap, true, &mut grouped_far_bins);
            self.collect_map_internal_bins(true, &mut grouped_far_bins);
        }

        // Remove grouped and palette bins from regular bin lists
        let bin_near_filtered: Vec<ResourceRef> = bin_resources_near.into_iter()
            .filter(|idx| !pal_bins.contains(idx) && !grouped_near_bins.contains(idx))
            .collect();
        let bin_far_filtered: Vec<ResourceRef> = bin_resources_far.into_iter()
            .filter(|idx| !grouped_far_bins.contains(idx))
            .collect();

        // Non-BIN resources
        let mut non_bin_resources = self.get_non_bin_resources();

        // VDPSprite and Collision resources (can go in .rodata directly)
        let vdp_sprite_resources = self.get_resources_of_type(ResourceType::VDPSprite);
        let collision_resources = self.get_resources_of_type(ResourceType::Collision);

        // Remove VDPSprite and Collision from non-bin
        non_bin_resources.retain(|idx| !vdp_sprite_resources.contains(idx) && !collision_resources.contains(idx));

        // --- Export ---
        // .rodata section
        out_s.push_str(".section .rodata\n\n");
        out_b.clear();

        self.export_resources(&vdp_sprite_resources, &mut out_b, &mut out_s, &mut out_h);
        self.export_resources(&collision_resources, &mut out_b, &mut out_s, &mut out_h);

        // .rodata_bin section
        out_s.push_str(".section .rodata_bin\n\n");
        out_b.clear();

        self.export_resources(&pal_bins, &mut out_b, &mut out_s, &mut out_h);
        self.export_resources(&bin_near_filtered, &mut out_b, &mut out_s, &mut out_h);
        self.export_resources(&grouped_near_bins, &mut out_b, &mut out_s, &mut out_h);

        // FAR BIN section
        if !near {
            out_s.push_str(".section .rodata_binf\n\n");
            out_b.clear();
        }

        if align != -1 {
            out_s.push_str(&format!("    .align  {}\n\n", align));
            out_b.clear();
        }

        self.export_resources(&bin_far_filtered, &mut out_b, &mut out_s, &mut out_h);
        self.export_resources(&grouped_far_bins, &mut out_b, &mut out_s, &mut out_h);

        // Non-BIN metadata
        out_s.push_str(".section .rodata\n\n");
        out_b.clear();

        self.export_resources(&non_bin_resources, &mut out_b, &mut out_s, &mut out_h);

        out_h.push('\n');
        out_h.push_str(&format!("#endif // _{}_H_\n", header_name));

        // --- Summary ---
        let (mut unpacked_size, mut packed_raw_size, mut packed_size) = (0usize, 0usize, 0usize);
        for res in &self.resources {
            if let Resource::Bin(bin) = &res.resource {
                if bin.done_compression != Compression::None {
                    packed_raw_size += bin.data.len() + (bin.data.len() & 1);
                    packed_size += bin.packed_data.as_ref().map(|p| p.data.len() + (p.data.len() & 1)).unwrap_or(0);
                } else {
                    unpacked_size += bin.data.len() + (bin.data.len() & 1);
                }
            }
        }

        println!();
        println!("{} summary:", file_name);
        println!("-------------");
        println!("Binary data: {} bytes", unpacked_size + packed_size);
        if unpacked_size > 0 {
            println!("  Unpacked: {} bytes", unpacked_size);
        }
        if packed_size > 0 && packed_raw_size > 0 {
            let pct = (packed_size as f64 * 100.0 / packed_raw_size as f64).round() as i32;
            println!("  Packed: {} bytes ({}% - origin size: {} bytes)", packed_size, pct, packed_raw_size);
        }

        let mut sprite_meta_size = 0usize;
        for res in &self.resources {
            match &res.resource {
                Resource::Sprite(_) | Resource::VDPSprite(_) | Resource::Collision(_)
                | Resource::SpriteFrame(_) | Resource::SpriteAnimation(_) => {
                    sprite_meta_size += res.shallow_size();
                }
                _ => {}
            }
        }
        if sprite_meta_size > 0 {
            println!("Sprite metadata (all but tiles and palette data): {} bytes", sprite_meta_size);
        }

        let mut misc_meta_size = 0usize;
        for idx in &non_bin_resources {
            let res = &self.resources[*idx];
            match &res.resource {
                Resource::Sprite(_) | Resource::VDPSprite(_) | Resource::Collision(_)
                | Resource::SpriteFrame(_) | Resource::SpriteAnimation(_) => {}
                _ => { misc_meta_size += res.shallow_size(); }
            }
        }
        if misc_meta_size > 0 {
            println!("Misc metadata (map, bitmap, image, tilemap, tileset, palette..): {} bytes", misc_meta_size);
        }

        let total_size = unpacked_size + packed_size + sprite_meta_size + misc_meta_size;
        println!("Total: {} bytes ({} KB)", total_size, (total_size as f64 / 1024.0).round() as i32);

        // --- Write output files ---
        if do_asm {
            if let Err(e) = std::fs::write(file_name_out, &out_s) {
                eprintln!("Couldn't create output file: {}", e);
                return false;
            }
        }

        if do_header {
            let h_file = set_extension(file_name_out, ".h");
            if let Err(e) = std::fs::write(&h_file, &out_h) {
                eprintln!("Couldn't create header file: {}", e);
                return false;
            }
        }

        if let Some(target) = dep_target {
            let d_file = set_extension(file_name_out, ".d");
            let dep_content = self.generate_dependency(file_name, target);
            if let Err(e) = std::fs::write(&d_file, &dep_content) {
                eprintln!("Couldn't create dependency file: {}", e);
                return false;
            }
        }

        true
    }

    // ── Resource query helpers ──

    fn get_bin_resources(&self, far: bool) -> Vec<ResourceRef> {
        self.resources.iter().enumerate()
            .filter(|(_, r)| {
                if let Resource::Bin(b) = &r.resource {
                    b.far == far
                } else {
                    false
                }
            })
            .map(|(i, _)| i)
            .collect()
    }

    fn get_non_bin_resources(&self) -> Vec<ResourceRef> {
        self.resources.iter().enumerate()
            .filter(|(_, r)| !matches!(&r.resource, Resource::Bin(_)))
            .map(|(i, _)| i)
            .collect()
    }

    fn get_resources_of_type(&self, rt: ResourceType) -> Vec<ResourceRef> {
        self.resources.iter().enumerate()
            .filter(|(_, r)| r.resource_type() == rt)
            .map(|(i, _)| i)
            .collect()
    }

    fn get_internal_bin_resources_of_type(&self, rt: ResourceType, far: bool) -> Vec<ResourceRef> {
        let mut result = Vec::new();
        self.collect_internal_bin_resources_of_type(rt, far, &mut result);
        result
    }

    fn collect_internal_bin_resources_of_type(&self, rt: ResourceType, far: bool, result: &mut Vec<ResourceRef>) {
        for (_i, res) in self.resources.iter().enumerate() {
            if res.resource_type() == rt {
                for bin_ref in res.get_internal_bin_resources() {
                    if let Resource::Bin(b) = &self.resources[bin_ref].resource {
                        if b.far == far && !result.contains(&bin_ref) {
                            result.push(bin_ref);
                        }
                    }
                }
            }
        }
    }

    fn collect_map_internal_bins(&self, far: bool, result: &mut Vec<ResourceRef>) {
        for res in &self.resources {
            if let Resource::Map(m) = &res.resource {
                // Order matters for grouping: metatiles, mapBlockIndexes, mapBlockRowOffsets, mapBlocks
                let refs = [m.metatiles_bin, m.map_block_indexes_bin, m.map_block_row_offsets_bin, m.map_blocks_bin];
                for bin_ref in refs {
                    if let Resource::Bin(b) = &self.resources[bin_ref].resource {
                        if b.far == far && !result.contains(&bin_ref) {
                            result.push(bin_ref);
                        }
                    }
                }
            }
        }
    }

    fn export_resources(&self, indices: &[ResourceRef], out_b: &mut Vec<u8>, out_s: &mut String, out_h: &mut String) {
        for &idx in indices {
            self.resources[idx].out(out_b, out_s, out_h, &self.resources);
        }
    }

    fn resolve_object_references(&mut self) {
        // Collect all Objects resources
        let obj_indices: Vec<usize> = self.resources.iter().enumerate()
            .filter(|(_, r)| matches!(&r.resource, Resource::Objects(_)))
            .map(|(i, _)| i)
            .collect();

        // For each pair of objects, resolve cross-references
        for i in 0..obj_indices.len() {
            for j in 0..obj_indices.len() {
                if i == j { continue; }
                // Get immutable reference to other objects
                let other_objects: Vec<SObject> = if let Resource::Objects(o) = &self.resources[obj_indices[j]].resource {
                    o.objects.clone()
                } else {
                    continue;
                };

                // Update fields in source
                if let Resource::Objects(ref mut o) = self.resources[obj_indices[i]].resource {
                    for src_obj in &mut o.objects {
                        for ext_obj in &other_objects {
                            src_obj.update_object_fields_references_to(ext_obj);
                        }
                    }
                }
            }
        }
    }

    fn generate_dependency(&self, res_file_name: &str, target_file_name: &str) -> String {
        let mut result = fix_path(res_file_name);

        for file in &self.resource_files {
            result.push_str(&format!(" \\\n{}", fix_path(file)));
        }

        format!("{}: {}", fix_path(target_file_name), result)
    }
}

// ── Resource type discriminant (for query helpers) ──

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ResourceType {
    Bin, Palette, Bitmap, Image, Tileset, Tilemap, Map,
    Objects, Sprite, SpriteAnimation, SpriteFrame,
    VDPSprite, Collision, Align, Near, Ungroup,
}

impl StoredResource {
    pub fn resource_type(&self) -> ResourceType {
        match &self.resource {
            Resource::Bin(_) => ResourceType::Bin,
            Resource::Palette(_) => ResourceType::Palette,
            Resource::Bitmap(_) => ResourceType::Bitmap,
            Resource::Image(_) => ResourceType::Image,
            Resource::Tileset(_) => ResourceType::Tileset,
            Resource::Tilemap(_) => ResourceType::Tilemap,
            Resource::Map(_) => ResourceType::Map,
            Resource::Objects(_) => ResourceType::Objects,
            Resource::Sprite(_) => ResourceType::Sprite,
            Resource::SpriteAnimation(_) => ResourceType::SpriteAnimation,
            Resource::SpriteFrame(_) => ResourceType::SpriteFrame,
            Resource::VDPSprite(_) => ResourceType::VDPSprite,
            Resource::Collision(_) => ResourceType::Collision,
            Resource::Align(_) => ResourceType::Align,
            Resource::Near(_) => ResourceType::Near,
            Resource::Ungroup(_) => ResourceType::Ungroup,
        }
    }
}

// ── Utility functions ──

fn collapse_spaces(s: &str) -> String {
    let mut result = String::with_capacity(s.len());
    let mut prev_space = false;
    for ch in s.chars() {
        if ch == ' ' {
            if !prev_space {
                result.push(' ');
            }
            prev_space = true;
        } else {
            result.push(ch);
            prev_space = false;
        }
    }
    result
}

/// Split a resource line on spaces, preserving quoted strings.
fn split_fields(input: &str) -> Vec<String> {
    let mut fields = Vec::new();
    let mut current = String::new();
    let mut in_quotes = false;

    for ch in input.chars() {
        match ch {
            '"' => {
                in_quotes = !in_quotes;
                // Don't include the quote character
            }
            ' ' if !in_quotes => {
                if !current.is_empty() {
                    fields.push(current.clone());
                    current.clear();
                }
            }
            _ => {
                current.push(ch);
            }
        }
    }
    if !current.is_empty() {
        fields.push(current);
    }
    fields
}

fn set_extension(path: &str, ext: &str) -> String {
    if let Some(dot) = path.rfind('.') {
        format!("{}{}", &path[..dot], ext)
    } else {
        format!("{}{}", path, ext)
    }
}

fn fix_path(path: &str) -> String {
    path.replace('\\', "/").replace(' ', "\\ ")
}
