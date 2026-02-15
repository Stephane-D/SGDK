//! XML utilities for TMX (Tiled Map) and TSX (Tiled Tileset) parsing.

use anyhow::{bail, Result};
use std::path::Path;

/// Get an attribute value from an XML node.
pub fn get_attr(node: &roxmltree::Node, name: &str) -> Option<String> {
    node.attribute(name).map(|s| s.to_string())
}

/// Get an integer attribute value.
pub fn get_attr_int(node: &roxmltree::Node, name: &str, default: i32) -> i32 {
    node.attribute(name)
        .and_then(|s| s.parse().ok())
        .unwrap_or(default)
}

/// Get a float attribute value.
pub fn get_attr_float(node: &roxmltree::Node, name: &str, default: f64) -> f64 {
    node.attribute(name)
        .and_then(|s| s.parse().ok())
        .unwrap_or(default)
}

/// Parse a TMX file and return map information.
pub struct TmxMap {
    pub width: usize,
    pub height: usize,
    pub tile_width: usize,
    pub tile_height: usize,
    pub layers: Vec<TmxLayer>,
    pub tilesets: Vec<TmxTilesetRef>,
    pub object_groups: Vec<TmxObjectGroup>,
}

pub struct TmxLayer {
    pub name: String,
    pub width: usize,
    pub height: usize,
    pub data: Vec<u32>,
}

pub struct TmxTilesetRef {
    pub first_gid: u32,
    pub source: String,
}

pub struct TmxObjectGroup {
    pub name: String,
    pub objects: Vec<TmxObject>,
}

pub struct TmxObject {
    pub id: u32,
    pub name: String,
    pub type_name: String,
    pub x: f64,
    pub y: f64,
    pub width: f64,
    pub height: f64,
    pub properties: Vec<(String, String, String)>, // (name, type, value)
}

/// Parse a TMX file.
pub fn parse_tmx(path: &str) -> Result<TmxMap> {
    let content = std::fs::read_to_string(path)?;
    let doc = roxmltree::Document::parse(&content)?;
    let root = doc.root_element();

    if root.tag_name().name() != "map" {
        bail!("Not a valid TMX file: root element is not 'map'");
    }

    let width = get_attr_int(&root, "width", 0) as usize;
    let height = get_attr_int(&root, "height", 0) as usize;
    let tile_width = get_attr_int(&root, "tilewidth", 8) as usize;
    let tile_height = get_attr_int(&root, "tileheight", 8) as usize;

    let mut layers = Vec::new();
    let mut tilesets = Vec::new();
    let mut object_groups = Vec::new();

    for child in root.children() {
        match child.tag_name().name() {
            "tileset" => {
                let first_gid = get_attr_int(&child, "firstgid", 1) as u32;
                let source = get_attr(&child, "source").unwrap_or_default();
                tilesets.push(TmxTilesetRef { first_gid, source });
            }
            "layer" => {
                let layer_name = get_attr(&child, "name").unwrap_or_default();
                let lw = get_attr_int(&child, "width", width as i32) as usize;
                let lh = get_attr_int(&child, "height", height as i32) as usize;

                let mut data = Vec::new();
                for data_node in child.children() {
                    if data_node.tag_name().name() == "data" {
                        let encoding = get_attr(&data_node, "encoding").unwrap_or_default();
                        if encoding == "csv" || encoding.is_empty() {
                            if let Some(text) = data_node.text() {
                                for val in text.split(',') {
                                    if let Ok(v) = val.trim().parse::<u32>() {
                                        data.push(v);
                                    }
                                }
                            }
                        }
                    }
                }

                layers.push(TmxLayer {
                    name: layer_name,
                    width: lw,
                    height: lh,
                    data,
                });
            }
            "objectgroup" => {
                let group_name = get_attr(&child, "name").unwrap_or_default();
                let mut objects = Vec::new();

                for obj_node in child.children() {
                    if obj_node.tag_name().name() == "object" {
                        let id = get_attr_int(&obj_node, "id", 0) as u32;
                        let name = get_attr(&obj_node, "name").unwrap_or_default();
                        let type_name = get_attr(&obj_node, "type")
                            .or_else(|| get_attr(&obj_node, "class"))
                            .unwrap_or_default();
                        let x = get_attr_float(&obj_node, "x", 0.0);
                        let y = get_attr_float(&obj_node, "y", 0.0);
                        let obj_w = get_attr_float(&obj_node, "width", 0.0);
                        let obj_h = get_attr_float(&obj_node, "height", 0.0);

                        let mut properties = Vec::new();
                        for props_node in obj_node.children() {
                            if props_node.tag_name().name() == "properties" {
                                for prop in props_node.children() {
                                    if prop.tag_name().name() == "property" {
                                        let pname = get_attr(&prop, "name").unwrap_or_default();
                                        let ptype = get_attr(&prop, "type").unwrap_or_else(|| "string".to_string());
                                        let pvalue = get_attr(&prop, "value")
                                            .or_else(|| prop.text().map(|s| s.to_string()))
                                            .unwrap_or_default();
                                        properties.push((pname, ptype, pvalue));
                                    }
                                }
                            }
                        }

                        objects.push(TmxObject {
                            id,
                            name,
                            type_name,
                            x,
                            y,
                            width: obj_w,
                            height: obj_h,
                            properties,
                        });
                    }
                }

                object_groups.push(TmxObjectGroup {
                    name: group_name,
                    objects,
                });
            }
            _ => {}
        }
    }

    Ok(TmxMap {
        width,
        height,
        tile_width,
        tile_height,
        layers,
        tilesets,
        object_groups,
    })
}

/// TSX tileset info.
pub struct TsxTileset {
    pub name: String,
    pub tile_width: usize,
    pub tile_height: usize,
    pub tile_count: usize,
    pub columns: usize,
    pub image_path: String,
    pub image_width: usize,
    pub image_height: usize,
    pub transparent_color: Option<String>,
}

/// Parse a TSX tileset file.
pub fn parse_tsx(path: &str) -> Result<TsxTileset> {
    let content = std::fs::read_to_string(path)?;
    let doc = roxmltree::Document::parse(&content)?;
    let root = doc.root_element();

    if root.tag_name().name() != "tileset" {
        bail!("Not a valid TSX file");
    }

    let name = get_attr(&root, "name").unwrap_or_default();
    let tile_width = get_attr_int(&root, "tilewidth", 8) as usize;
    let tile_height = get_attr_int(&root, "tileheight", 8) as usize;
    let tile_count = get_attr_int(&root, "tilecount", 0) as usize;
    let columns = get_attr_int(&root, "columns", 0) as usize;

    let mut image_path = String::new();
    let mut image_width = 0;
    let mut image_height = 0;
    let mut transparent_color = None;

    for child in root.children() {
        if child.tag_name().name() == "image" {
            image_path = get_attr(&child, "source").unwrap_or_default();
            image_width = get_attr_int(&child, "width", 0) as usize;
            image_height = get_attr_int(&child, "height", 0) as usize;
            transparent_color = get_attr(&child, "trans");
        }
    }

    // Make image path relative to TSX file location
    if !image_path.is_empty() && !Path::new(&image_path).is_absolute() {
        if let Some(parent) = Path::new(path).parent() {
            image_path = parent.join(&image_path).to_string_lossy().to_string();
        }
    }

    Ok(TsxTileset {
        name,
        tile_width,
        tile_height,
        tile_count,
        columns,
        image_path,
        image_width,
        image_height,
        transparent_color,
    })
}

/// Get the image path from a TSX tileset file.
pub fn get_tsx_tileset_path(tsx_path: &str) -> Result<String> {
    let ts = parse_tsx(tsx_path)?;
    Ok(ts.image_path)
}
