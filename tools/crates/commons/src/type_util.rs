//! Type utilities for Mega Drive data types.

/// Mega Drive VDP color: 9-bit RGB (3 bits per channel, stored in u16).
///
/// Format: `0000BBB0GGG0RRR0` (MSB to LSB)
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct VdpColor(pub u16);

impl VdpColor {
    /// Create from individual RGB components (0-7 each).
    pub fn from_rgb(r: u8, g: u8, b: u8) -> Self {
        Self(((b as u16 & 7) << 9) | ((g as u16 & 7) << 5) | ((r as u16 & 7) << 1))
    }

    /// Create from a 24-bit RGB color, converting to Mega Drive format.
    pub fn from_rgb24(r: u8, g: u8, b: u8) -> Self {
        // Round to nearest Mega Drive color level (0-7)
        let r_md = ((r as u16 + 18) / 36).min(7) as u8;
        let g_md = ((g as u16 + 18) / 36).min(7) as u8;
        let b_md = ((b as u16 + 18) / 36).min(7) as u8;
        Self::from_rgb(r_md, g_md, b_md)
    }

    /// Get red component (0-7).
    pub fn r(&self) -> u8 {
        ((self.0 >> 1) & 7) as u8
    }

    /// Get green component (0-7).
    pub fn g(&self) -> u8 {
        ((self.0 >> 5) & 7) as u8
    }

    /// Get blue component (0-7).
    pub fn b(&self) -> u8 {
        ((self.0 >> 9) & 7) as u8
    }

    /// Convert to 24-bit RGB.
    pub fn to_rgb24(&self) -> (u8, u8, u8) {
        let scale = |v: u8| (v as u16 * 255 / 7) as u8;
        (scale(self.r()), scale(self.g()), scale(self.b()))
    }
}

/// Tile attribute for VDP tilemap entries.
///
/// Format: `PCCVHNNNNNNNNNNN`
/// - P: Priority
/// - CC: Palette (0-3)
/// - V: Vertical flip
/// - H: Horizontal flip
/// - N: Tile index (0-2047)
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct TileAttr(pub u16);

impl TileAttr {
    pub fn new(priority: bool, palette: u8, v_flip: bool, h_flip: bool, index: u16) -> Self {
        let mut val: u16 = index & 0x7FF;
        if h_flip {
            val |= 0x0800;
        }
        if v_flip {
            val |= 0x1000;
        }
        val |= ((palette as u16) & 3) << 13;
        if priority {
            val |= 0x8000;
        }
        Self(val)
    }

    pub fn priority(&self) -> bool {
        (self.0 & 0x8000) != 0
    }

    pub fn palette(&self) -> u8 {
        ((self.0 >> 13) & 3) as u8
    }

    pub fn v_flip(&self) -> bool {
        (self.0 & 0x1000) != 0
    }

    pub fn h_flip(&self) -> bool {
        (self.0 & 0x0800) != 0
    }

    pub fn index(&self) -> u16 {
        self.0 & 0x7FF
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_vdp_color_roundtrip() {
        let c = VdpColor::from_rgb(3, 5, 7);
        assert_eq!(c.r(), 3);
        assert_eq!(c.g(), 5);
        assert_eq!(c.b(), 7);
    }

    #[test]
    fn test_vdp_color_from_rgb24() {
        // Pure white
        let c = VdpColor::from_rgb24(255, 255, 255);
        assert_eq!(c.r(), 7);
        assert_eq!(c.g(), 7);
        assert_eq!(c.b(), 7);

        // Pure black
        let c = VdpColor::from_rgb24(0, 0, 0);
        assert_eq!(c.r(), 0);
        assert_eq!(c.g(), 0);
        assert_eq!(c.b(), 0);
    }

    #[test]
    fn test_tile_attr() {
        let attr = TileAttr::new(true, 2, true, false, 100);
        assert!(attr.priority());
        assert_eq!(attr.palette(), 2);
        assert!(attr.v_flip());
        assert!(!attr.h_flip());
        assert_eq!(attr.index(), 100);
    }
}
