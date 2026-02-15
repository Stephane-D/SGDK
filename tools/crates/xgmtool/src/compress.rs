use std::cell::RefCell;
use crate::lz77;

/// LZ77 stream compressor state
struct Lz77CompressState {
    out: Vec<u8>,
    /// Index of current flag byte
    flag_idx: usize,
    /// Next bit position within flag byte (0..7)
    next_bit: u8,
    /// Number of data bytes after the flag byte
    out_l: usize,
}

impl Lz77CompressState {
    fn new() -> Self {
        Lz77CompressState {
            out: vec![0], // first flag byte
            flag_idx: 0,
            next_bit: 0,
            out_l: 1,
        }
    }

    fn check_flush(&mut self) {
        self.next_bit = (self.next_bit + 1) & 7;
        if self.next_bit == 0 {
            self.flag_idx += self.out_l;
            while self.out.len() <= self.flag_idx {
                self.out.push(0);
            }
            self.out[self.flag_idx] = 0;
            self.out_l = 1;
        }
    }

    fn lit(&mut self, lit: u8) {
        self.out[self.flag_idx] >>= 1;
        let pos = self.flag_idx + self.out_l;
        if pos >= self.out.len() {
            self.out.resize(pos + 1, 0);
        }
        self.out[pos] = lit;
        self.out_l += 1;
        self.check_flush();
    }

    fn backref(&mut self, dist: usize, len: usize) {
        self.out[self.flag_idx] = (self.out[self.flag_idx] >> 1) | 0x80;
        let pos = self.flag_idx + self.out_l;
        if pos + 2 >= self.out.len() {
            self.out.resize(pos + 4, 0);
        }
        self.out[pos] = (((dist - 1) >> 8) & 0xFF) as u8;
        self.out[pos + 1] = ((dist - 1) & 0xFF) as u8;
        self.out[pos + 2] = ((len - 1) & 0xFF) as u8;
        self.out_l += 3;
        self.check_flush();
    }

    fn finish(&mut self) {
        self.out[self.flag_idx] = ((self.out[self.flag_idx] >> 1) | 0x80) >> (7 - self.next_bit);
        let pos = self.flag_idx + self.out_l;
        if pos + 2 >= self.out.len() {
            self.out.resize(pos + 3, 0);
        }
        self.out[pos] = 0;
        self.out[pos + 1] = 0;
        self.out[pos + 2] = 0;
        self.out_l += 3;
        self.out.truncate(self.flag_idx + self.out_l);
    }
}

/// Compress data using LZ77 with bit-stream encoding.
/// Returns compressed data or None on failure.
pub fn lz77c_compress_buf(input: &[u8]) -> Option<Vec<u8>> {
    let state = RefCell::new(Lz77CompressState::new());

    let res = lz77::lz77_compress(
        input,
        32 * 1024,  // max_dist
        256,        // max_len
        true,       // allow_overlap
        |lit| { state.borrow_mut().lit(lit); true },
        |dist, len| { state.borrow_mut().backref(dist, len); true },
    );

    if !res { return None; }

    state.borrow_mut().finish();
    Some(state.into_inner().out)
}
