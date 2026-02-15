//! File I/O utility functions.

use anyhow::{Context, Result};
use std::fs;
use std::path::{Path, PathBuf};

/// Read a file as raw bytes.
pub fn read_binary_file(path: &str) -> Result<Vec<u8>> {
    fs::read(path).with_context(|| format!("Failed to read file: {}", path))
}

/// Write raw bytes to a file.
pub fn write_binary_file(data: &[u8], path: &str) -> Result<()> {
    // Ensure parent directory exists
    if let Some(parent) = Path::new(path).parent() {
        if !parent.exists() {
            fs::create_dir_all(parent)
                .with_context(|| format!("Failed to create directory: {}", parent.display()))?;
        }
    }
    fs::write(path, data).with_context(|| format!("Failed to write file: {}", path))
}

/// Get file extension (lowercase).
pub fn get_extension(path: &str) -> String {
    Path::new(path)
        .extension()
        .and_then(|e| e.to_str())
        .unwrap_or("")
        .to_lowercase()
}

/// Get filename without extension.
pub fn get_filename_without_ext(path: &str) -> String {
    Path::new(path)
        .file_stem()
        .and_then(|s| s.to_str())
        .unwrap_or("")
        .to_string()
}

/// Get just the filename from a path.
pub fn get_filename(path: &str) -> String {
    Path::new(path)
        .file_name()
        .and_then(|s| s.to_str())
        .unwrap_or(path)
        .to_string()
}

/// Change extension of a path.
pub fn set_extension(path: &str, ext: &str) -> String {
    let mut pb = PathBuf::from(path);
    pb.set_extension(ext);
    pb.to_string_lossy().to_string()
}

/// Check if a file exists.
pub fn file_exists(path: &str) -> bool {
    Path::new(path).exists()
}

/// Get file size in bytes.
pub fn file_size(path: &str) -> Result<u64> {
    let metadata =
        fs::metadata(path).with_context(|| format!("Failed to get metadata for: {}", path))?;
    Ok(metadata.len())
}
