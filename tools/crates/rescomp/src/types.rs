//! Core type definitions for rescomp.

/// Sound driver type for WAV resources.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum SoundDriver {
    Pcm,
    Dpcm2,
    Pcm4,
    Xgm,
    Xgm2,
}

/// Compression algorithm.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum Compression {
    /// Automatic (best ratio)
    Auto = -1_isize as _,
    /// No compression
    None = 0,
    /// APlib compression (good ratio, slow)
    Aplib = 1,
    /// LZ4W compression (average ratio, fast)
    Lz4w = 2,
}

impl Compression {
    /// Ordinal value matching Java enum ordering: AUTO=0, NONE=1, APLIB=2, LZ4W=3
    /// Used for compression encoding in output structures.
    pub fn ordinal(&self) -> i32 {
        match self {
            Compression::Auto => 0,
            Compression::None => 1,
            Compression::Aplib => 2,
            Compression::Lz4w => 3,
        }
    }
}

/// Tile optimization mode.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum TileOptimization {
    /// No optimization (duplicate tiles preserved)
    None,
    /// Full optimization (deduplicate identical + flipped tiles)
    All,
    /// Deduplicate identical tiles only (no flip detection)
    DuplicateOnly,
}

/// Tile ordering direction.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum TileOrdering {
    Row,
    Column,
}

/// Tile equality result.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum TileEquality {
    None,
    Equal,
    VFlip,
    HFlip,
    HVFlip,
}

impl TileEquality {
    pub fn hflip(&self) -> bool {
        matches!(self, TileEquality::HFlip | TileEquality::HVFlip)
    }
    pub fn vflip(&self) -> bool {
        matches!(self, TileEquality::VFlip | TileEquality::HVFlip)
    }
}

/// Collision type.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum CollisionType {
    None,
    Circle,
    Box,
}

/// Box collision data.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct CollisionBox {
    pub x: i32,
    pub y: i32,
    pub w: i32,
    pub h: i32,
}

/// Circle collision data.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct CollisionCircle {
    pub x: i32,
    pub y: i32,
    pub ray: i32,
}

/// Packed (compressed) data result.
#[derive(Debug, Clone)]
pub struct PackedData {
    pub data: Vec<u8>,
    pub compression: Compression,
}

impl PackedData {
    pub fn new(data: Vec<u8>, compression: Compression) -> Self {
        Self { data, compression }
    }
}

/// Sprite optimization type.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum OptimizationType {
    Balanced,
    MinSprite,
    MinTile,
    None,
}

/// Sprite optimization level.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum OptimizationLevel {
    Fast,
    Medium,
    Slow,
    Max,
}

/// SGDK object field type definition.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum SGDKObjectType {
    U8,
    S8,
    U16,
    S16,
    U32,
    S32,
    F16,
    F32,
    Bool,
    String,
    Object,
}

impl SGDKObjectType {
    /// Size in bytes for this field type.
    pub fn size(&self) -> usize {
        match self {
            SGDKObjectType::U8 | SGDKObjectType::S8 | SGDKObjectType::Bool => 1,
            SGDKObjectType::U16 | SGDKObjectType::S16 | SGDKObjectType::F16 => 2,
            SGDKObjectType::U32 | SGDKObjectType::S32 | SGDKObjectType::F32
            | SGDKObjectType::String | SGDKObjectType::Object => 4,
        }
    }

    /// Parse from string.
    pub fn parse(s: &str) -> Option<Self> {
        match s.to_uppercase().as_str() {
            "U8" => Some(Self::U8),
            "S8" => Some(Self::S8),
            "U16" => Some(Self::U16),
            "S16" => Some(Self::S16),
            "U32" => Some(Self::U32),
            "S32" => Some(Self::S32),
            "F16" => Some(Self::F16),
            "F32" => Some(Self::F32),
            "BOOL" => Some(Self::Bool),
            "STRING" => Some(Self::String),
            "OBJECT" => Some(Self::Object),
            _ => None,
        }
    }
}

/// Tiled object field type.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum TiledObjectType {
    Int,
    Float,
    Bool,
    String,
    Color,
    File,
    Object,
    Enum,
}

/// Rectangle (x, y, w, h).
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct Rect {
    pub x: i32,
    pub y: i32,
    pub w: i32,
    pub h: i32,
}

impl Rect {
    pub fn new(x: i32, y: i32, w: i32, h: i32) -> Self {
        Self { x, y, w, h }
    }

    pub fn contains_point(&self, px: i32, py: i32) -> bool {
        px >= self.x && px < self.x + self.w && py >= self.y && py < self.y + self.h
    }

    pub fn area(&self) -> i32 {
        self.w * self.h
    }
}

/// Basic image info.
#[derive(Debug, Clone)]
pub struct BasicImageInfo {
    pub w: usize,
    pub h: usize,
    pub bpp: u8,
}
