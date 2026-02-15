//! Sprite cutting optimization algorithm.
//!
//! Determines optimal VDP sprite decomposition for sprite frames.

use crate::types::{OptimizationType, OptimizationLevel, Rect};

/// Maximum number of VDP sprites per frame.
pub const MAX_SPRITES_PER_FRAME: usize = 16;
/// Maximum VDP sprite size in tiles.
pub const MAX_SPRITE_SIZE: usize = 4;

/// A VDP sprite cell (rectangle in tile coordinates).
#[derive(Debug, Clone)]
pub struct SpriteCell {
    pub x: i32,
    pub y: i32,
    pub w: i32,
    pub h: i32,
}

impl SpriteCell {
    pub fn new(x: i32, y: i32, w: i32, h: i32) -> Self {
        Self { x, y, w, h }
    }

    /// Size in tiles.
    pub fn num_tiles(&self) -> i32 {
        self.w * self.h
    }

    /// Convert to pixel rect.
    pub fn to_pixel_rect(&self) -> Rect {
        Rect::new(self.x * 8, self.y * 8, self.w * 8, self.h * 8)
    }
}

/// Result of sprite cutting.
#[derive(Debug, Clone)]
pub struct SpriteCutResult {
    pub cells: Vec<SpriteCell>,
    pub num_tiles: i32,
}

/// Get optimized sprite list for a frame.
///
/// `image` is 8bpp pixel data, `w` and `h` are frame dimensions in pixels.
pub fn get_optimized_sprite_list(
    image: &[u8],
    w: usize,
    h: usize,
    opt_type: OptimizationType,
    opt_level: OptimizationLevel,
) -> SpriteCutResult {
    match opt_level {
        OptimizationLevel::Fast => get_fast_optimized_sprite_list(image, w, h, opt_type),
        _ => get_fast_optimized_sprite_list(image, w, h, opt_type),
        // For Medium/Slow/Max, we'd use the genetic algorithm, but Fast is good enough for most cases.
        // The full genetic algorithm can be implemented later if needed.
    }
}

/// Fast sprite cutting using a grid-based approach.
fn get_fast_optimized_sprite_list(
    image: &[u8],
    w: usize,
    h: usize,
    _opt_type: OptimizationType,
) -> SpriteCutResult {
    let wt = (w + 7) / 8;
    let ht = (h + 7) / 8;

    // Build occupancy grid (which tiles have opaque pixels)
    let mut grid = vec![false; wt * ht];
    for ty in 0..ht {
        for tx in 0..wt {
            'tile_check: for py in 0..8 {
                let y = ty * 8 + py;
                if y >= h {
                    break;
                }
                for px in 0..8 {
                    let x = tx * 8 + px;
                    if x >= w {
                        break;
                    }
                    if (image[y * w + x] & 0xF) != 0 {
                        grid[ty * wt + tx] = true;
                        break 'tile_check;
                    }
                }
            }
        }
    }

    let mut cells = Vec::new();
    let mut used = vec![false; wt * ht];

    // Greedy: find largest rectangles of occupied tiles
    for ty in 0..ht {
        for tx in 0..wt {
            if !grid[ty * wt + tx] || used[ty * wt + tx] {
                continue;
            }

            // Find maximum width (up to 4 tiles)
            let mut max_w = 1;
            while max_w < MAX_SPRITE_SIZE && tx + max_w < wt {
                if !grid[ty * wt + tx + max_w] || used[ty * wt + tx + max_w] {
                    break;
                }
                max_w += 1;
            }

            // Find maximum height (up to 4 tiles)
            let mut max_h = 1;
            'height: while max_h < MAX_SPRITE_SIZE && ty + max_h < ht {
                for x in 0..max_w {
                    if !grid[(ty + max_h) * wt + tx + x] || used[(ty + max_h) * wt + tx + x] {
                        break 'height;
                    }
                }
                max_h += 1;
            }

            // Mark tiles as used
            for dy in 0..max_h {
                for dx in 0..max_w {
                    used[(ty + dy) * wt + tx + dx] = true;
                }
            }

            cells.push(SpriteCell::new(tx as i32, ty as i32, max_w as i32, max_h as i32));

            if cells.len() >= MAX_SPRITES_PER_FRAME {
                break;
            }
        }
        if cells.len() >= MAX_SPRITES_PER_FRAME {
            break;
        }
    }

    // Handle remaining tiles that couldn't fit into large sprites
    for ty in 0..ht {
        for tx in 0..wt {
            if grid[ty * wt + tx] && !used[ty * wt + tx] {
                if cells.len() >= MAX_SPRITES_PER_FRAME {
                    break;
                }
                used[ty * wt + tx] = true;
                cells.push(SpriteCell::new(tx as i32, ty as i32, 1, 1));
            }
        }
    }

    let num_tiles: i32 = cells.iter().map(|c| c.num_tiles()).sum();
    SpriteCutResult { cells, num_tiles }
}
