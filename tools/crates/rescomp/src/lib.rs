//! SGDK Resource Compiler (rescomp) - Rust implementation
//!
//! Compiles .res resource files into assembly (.s) and header (.h) files
//! for the Sega Mega Drive / Genesis.

pub mod types;
pub mod tile;
pub mod util;
pub mod image_util;
pub mod sound_util;
pub mod xml_util;
pub mod tmx;
pub mod sprite_cutter;
pub mod resource;
pub mod processor;
pub mod compiler;
