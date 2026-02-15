//! APJ - aPLib compression library for SGDK
//!
//! aPLib compression is an LZSS-based lossless algorithm by Jorgen Ibsen.
//! This is a faithful port of the SGDK Java APJ implementation.

// --- Bit I/O ---

/// Bit-level reader for decompression.
pub struct BitReader<'a> {
    data: &'a [u8],
    pub offset: usize,
    tag: u8,
    nbit: i32,
}

impl<'a> BitReader<'a> {
    pub fn new(data: &'a [u8]) -> Self {
        Self {
            data,
            offset: 0,
            tag: 0,
            nbit: 0,
        }
    }

    pub fn read_bit(&mut self) -> u8 {
        if self.nbit == 0 {
            self.tag = self.read_ubyte();
            self.nbit = 8;
        }
        self.nbit -= 1;
        let result = (self.tag >> 7) & 1;
        self.tag <<= 1;
        result
    }

    pub fn read_ubyte(&mut self) -> u8 {
        let b = self.data[self.offset];
        self.offset += 1;
        b
    }
}

/// Bit-level writer for compression.
pub struct BitWriter {
    buf: Vec<u8>,
    tag_offset: Option<usize>,
    tag: u8,
    nbit: i32,
}

impl BitWriter {
    pub fn new(capacity: usize) -> Self {
        Self {
            buf: Vec::with_capacity(capacity),
            tag_offset: None,
            tag: 0,
            nbit: 0,
        }
    }

    fn save_and_allocate_tag(&mut self) {
        if let Some(off) = self.tag_offset {
            self.buf[off] = self.tag;
        }
        self.tag_offset = Some(self.buf.len());
        self.buf.push(0); // allocate tag byte
        self.tag = 0;
    }

    pub fn write_bit(&mut self, bit: u8) {
        if self.nbit == 0 {
            self.save_and_allocate_tag();
            self.nbit = 8;
        }
        self.nbit -= 1;
        self.tag |= (bit & 1) << self.nbit;
    }

    pub fn write_ubyte(&mut self, value: u8) {
        self.buf.push(value);
    }

    pub fn to_bytes(&mut self) -> Vec<u8> {
        if let Some(off) = self.tag_offset {
            self.buf[off] = self.tag;
        }
        self.buf.clone()
    }
}

/// Push a byte to result, optionally checking against verification data.
#[inline]
fn write_check(result: &mut Vec<u8>, value: u8, verif: Option<&[u8]>) {
    if let Some(v) = verif {
        if result.len() < v.len() && value != v[result.len()] {
            eprintln!("Error at {}", result.len());
        }
    }
    result.push(value);
}

/// Read a byte from result at position (result.len() - off).
#[inline]
fn read_back_ubyte(result: &[u8], off: usize) -> u8 {
    result[result.len() - off]
}

// --- Match types ---

#[derive(Clone)]
struct ByteMatch {
    offset: usize,
    repeat: usize,
}

#[derive(Clone)]
struct Match {
    data_ref: Vec<u8>, // we store a reference to data slice for cost computation
    index: usize,
    offset: usize,
    length: usize,
    raw_cost: Option<i32>,
    cost: Option<i32>,
    saved: Option<i32>,
}

// Static mutable state for encoding (mirrors Java's static fields)
struct EncoderState {
    was_match: bool,
    last_offset: i32,
}

fn get_high_bit_num(value: i32) -> i32 {
    if value <= 0 {
        return 0;
    }
    let mut result = 0;
    let mut v = value;
    if v >= 0x10000 {
        result += 16;
        v >>= 16;
    }
    if v >= 0x100 {
        result += 8;
        v >>= 8;
    }
    if v >= 0x10 {
        result += 4;
        v >>= 4;
    }
    if v >= 4 {
        result += 2;
        v >>= 2;
    }
    if v >= 2 {
        result += 1;
    }
    result
}

fn get_length_delta(offset: usize) -> usize {
    if offset < 0x80 || offset >= 0x7D00 {
        2
    } else if offset >= 0x500 {
        1
    } else {
        0
    }
}

impl Match {
    fn new(data: &[u8], index: usize, offset: usize, length: usize) -> Self {
        let start = index;
        let end = std::cmp::min(index + length, data.len());
        Self {
            data_ref: data[start..end].to_vec(),
            index,
            offset,
            length,
            raw_cost: None,
            cost: None,
            saved: None,
        }
    }

    fn inc_length(&mut self, data: &[u8], value: usize) {
        self.index -= value;
        self.length += value;
        let start = self.index;
        let end = std::cmp::min(self.index + self.length, data.len());
        self.data_ref = data[start..end].to_vec();
        self.raw_cost = None;
        self.cost = None;
        self.saved = None;
    }

    fn is_short_or_long(&self) -> bool {
        if self.length >= 2 && self.length <= 3 && self.offset > 0 && self.offset < 128 {
            return true;
        }
        if self.length >= 3 {
            return true;
        }
        false
    }

    fn compute_raw_cost(&self) -> i32 {
        let mut result = 0;
        for &b in &self.data_ref {
            if b == 0 {
                result += 7;
            } else {
                result += 9;
            }
        }
        result
    }

    fn compute_cost(&self, state: &EncoderState) -> i32 {
        if self.length == 1 && self.offset > 0 && self.offset < 16 {
            return 3 + 4;
        }
        if self.length >= 2 && self.length <= 3 && self.offset > 0 && self.offset < 128 {
            return 3 + 8;
        }

        let is_long_match = (self.length >= 2 && self.offset >= 0x80 && self.offset < 0x500)
            || (self.length >= 3 && self.offset >= 0x500 && self.offset < 0x7D00)
            || self.length >= 4;

        if is_long_match {
            let mut c: i32 = 2;

            if !state.was_match && state.last_offset == self.offset as i32 {
                c += 2;
            } else {
                c += (get_high_bit_num((self.offset >> 8) as i32)) * 2 + 2;
                c += 8;
            }

            c += get_high_bit_num(self.length as i32) * 2;
            return c;
        }

        self.get_raw_cost()
    }

    fn get_raw_cost(&self) -> i32 {
        self.raw_cost.unwrap_or_else(|| self.compute_raw_cost())
    }

    fn get_cost(&self, state: &EncoderState) -> i32 {
        self.cost.unwrap_or_else(|| self.compute_cost(state))
    }

    fn get_saved(&self, state: &EncoderState) -> i32 {
        self.saved
            .unwrap_or_else(|| self.get_raw_cost() - self.get_cost(state))
    }
}

fn get_repeat(data: &[u8], ind: usize) -> usize {
    let value = data[ind];
    let mut off = ind + 1;
    while off < data.len() && data[off] == value {
        off += 1;
    }
    off - ind - 1
}

fn get_match(data: &[u8], from: usize, ind: usize) -> Match {
    let mut ref_offset = from;
    let mut cur_offset = ind;
    let mut len = 0;
    while cur_offset < data.len() && data[ref_offset] == data[cur_offset] {
        ref_offset += 1;
        cur_offset += 1;
        len += 1;
    }
    Match::new(data, ind, ind - from, len)
}

fn find_best_match(
    byte_matches: &[Vec<ByteMatch>],
    data: &[u8],
    ind: usize,
    state: &EncoderState,
) -> Option<Match> {
    if ind < 1 {
        return None;
    }

    let bml = &byte_matches[data[ind] as usize];
    let cur_repeat = get_repeat(data, ind);

    let mut best: Option<Match> = None;
    let mut saved: i32 = 1;

    for bm in bml {
        let mut off = bm.offset;
        if off >= ind {
            break;
        }

        let mut repeat = bm.repeat as i32;

        if (repeat as usize) < cur_repeat {
            let m = Match::new(data, ind, ind - off, repeat as usize + 1);
            if m.get_saved(state) >= saved {
                saved = m.get_saved(state);
                best = Some(m);
            }
        } else {
            if repeat as usize > cur_repeat {
                let mut delta = repeat as usize - cur_repeat;
                if off + delta >= ind {
                    delta = ind - off - 1;
                }
                off += delta;
                repeat -= delta as i32;
            }

            let m = if repeat > 0 {
                let adj_repeat = if off + repeat as usize >= ind {
                    ind - off - 1
                } else {
                    repeat as usize
                };

                let mut m = get_match(data, off + adj_repeat, ind + adj_repeat);
                m.inc_length(data, adj_repeat);
                m
            } else {
                get_match(data, off, ind)
            };

            if m.get_saved(state) >= saved {
                saved = m.get_saved(state);
                best = Some(m);
            }
        }
    }

    best
}

// --- Encoding helpers ---

fn read_variable_number(stream: &mut BitReader) -> i32 {
    let mut result: i32 = 1;
    loop {
        result = (result << 1) + stream.read_bit() as i32;
        if stream.read_bit() == 0 {
            break;
        }
    }
    result
}

fn read_fixed_number(stream: &mut BitReader, nbit: i32) -> i32 {
    let mut result: i32 = 0;
    for _ in 0..nbit {
        result <<= 1;
        result |= stream.read_bit() as i32;
    }
    result
}

fn write_variable_number(stream: &mut BitWriter, value: i32) {
    if value < 2 {
        eprintln!("Encoding error - writeVariableNumber");
        return;
    }

    let mut nbit = get_high_bit_num(value);
    nbit -= 1;

    stream.write_bit(((value >> nbit) & 1) as u8);
    while nbit > 0 {
        nbit -= 1;
        stream.write_bit(1);
        stream.write_bit(((value >> nbit) & 1) as u8);
    }
    stream.write_bit(0);
}

fn write_fixed_number(stream: &mut BitWriter, value: i32, nbit: i32) {
    for i in (0..nbit).rev() {
        stream.write_bit(((value >> i) & 1) as u8);
    }
}

fn write_literal(stream: &mut BitWriter, data: u8, state: &mut EncoderState) {
    stream.write_bit(0);
    stream.write_ubyte(data);
    state.was_match = false;
}

fn write_tiny_block(stream: &mut BitWriter, offset: i32, state: &mut EncoderState) {
    stream.write_bit(1);
    stream.write_bit(1);
    stream.write_bit(1);
    write_fixed_number(stream, offset, 4);
    state.was_match = false;
}

fn write_short_block(
    stream: &mut BitWriter,
    offset: i32,
    length: i32,
    state: &mut EncoderState,
) {
    stream.write_bit(1);
    stream.write_bit(1);
    stream.write_bit(0);
    stream.write_ubyte(((offset << 1) | (length - 2)) as u8);
    state.last_offset = offset;
    state.was_match = true;
}

fn write_block(stream: &mut BitWriter, offset: i32, length: i32, state: &mut EncoderState) {
    stream.write_bit(1);
    stream.write_bit(0);

    if !state.was_match && state.last_offset == offset {
        write_variable_number(stream, 2);
        write_variable_number(stream, length);
    } else {
        let mut high_offset = (offset >> 8) + 2;
        let low_offset = offset & 0xFF;

        if !state.was_match {
            high_offset += 1;
        }

        write_variable_number(stream, high_offset);
        stream.write_ubyte(low_offset as u8);
        write_variable_number(stream, length - get_length_delta(offset as usize) as i32);
    }

    state.last_offset = offset;
    state.was_match = true;
}

fn write_end_block(stream: &mut BitWriter) {
    stream.write_bit(1);
    stream.write_bit(1);
    stream.write_bit(0);
    stream.write_ubyte(0);
}

/// Pack data using the aPLib algorithm.
///
/// `ultra` enables optimal (slow) compression mode.
pub fn pack(data: &[u8], ultra: bool, silent: bool) -> Vec<u8> {
    let len = data.len();
    if len < 2 {
        return data.to_vec();
    }

    let mut result = BitWriter::new(len);
    let mut state = EncoderState {
        was_match: false,
        last_offset: 0,
    };

    // PASS 1: build byte matches table
    let mut byte_matches: Vec<Vec<ByteMatch>> = Vec::with_capacity(256);
    for _ in 0..256 {
        byte_matches.push(Vec::new());
    }

    {
        let mut m_offset = 0;
        while m_offset < data.len() {
            let off = m_offset;
            let val = data[m_offset];
            m_offset += 1;

            let mut repeat = 0usize;
            while m_offset < data.len() && data[m_offset] == val {
                repeat += 1;
                m_offset += 1;
            }

            byte_matches[val as usize].push(ByteMatch { offset: off, repeat });
        }

        // Sort all ByteMatch lists by offset
        for list in byte_matches.iter_mut() {
            list.sort_by_key(|bm| bm.offset);
        }
    }

    // PASS 2: find best match for each position
    state.was_match = false;
    state.last_offset = -1;
    let mut matches: Vec<Option<Match>> = vec![None; data.len()];

    if ultra {
        for i in 1..data.len() {
            matches[i] = find_best_match(&byte_matches, data, i, &state);
        }
    } else {
        let mut i = 1;
        while i < data.len() {
            let m = find_best_match(&byte_matches, data, i, &state);
            matches[i] = m.clone();

            if let Some(ref mat) = m {
                i += mat.length;
                if mat.is_short_or_long() {
                    state.last_offset = mat.offset as i32;
                    state.was_match = true;
                } else {
                    state.was_match = false;
                }
            } else {
                i += 1;
                state.was_match = false;
            }
        }
    }

    // PASS 3: optimal parsing (ultra mode only)
    if ultra {
        let mut costs = vec![0i32; data.len() + 1];

        for i in (0..data.len()).rev() {
            let literal_cost = costs[i + 1] + if data[i] == 0 { 7 } else { 9 };
            let mut match_cost = i32::MAX;

            if let Some(ref m) = matches[i] {
                let mc = m.get_cost(&state);
                if mc < i32::MAX - costs[i + m.length] {
                    match_cost = mc + costs[i + m.length];
                }
            }

            if literal_cost < match_cost {
                costs[i] = literal_cost;
                matches[i] = None;
            } else {
                costs[i] = match_cost;
            }
        }
    }

    // PASS 4: encode
    state.last_offset = 0;
    state.was_match = false;

    // First byte always literal
    result.write_ubyte(data[0]);

    let mut ind = 1;
    while ind < data.len() {
        let mut done = false;

        if let Some(ref m) = matches[ind].clone() {
            let off = m.offset as i32;
            let l = m.length as i32;
            done = true;

            if l == 1 && off > 0 && off < 16 {
                write_tiny_block(&mut result, off, &mut state);
            } else if l >= 2 && l <= 3 && off > 0 && off < 0x80 {
                write_short_block(&mut result, off, l, &mut state);
            } else if l >= 2 && off >= 0x80 && off < 0x500 {
                write_block(&mut result, off, l, &mut state);
            } else if l >= 3 && off >= 0x500 && off < 0x7D00 {
                write_block(&mut result, off, l, &mut state);
            } else if l >= 4 {
                write_block(&mut result, off, l, &mut state);
            } else {
                done = false;
            }

            if done {
                ind += l as usize;
            }
        }

        if !done {
            let v = data[ind];
            ind += 1;

            if v == 0 {
                write_tiny_block(&mut result, 0, &mut state);
            } else {
                write_literal(&mut result, v, &mut state);
            }
        }
    }

    write_end_block(&mut result);

    if !silent {
        eprintln!("APJ packed {} -> {} bytes", data.len(), result.buf.len());
    }

    result.to_bytes()
}

/// Unpack aPLib compressed data.
pub fn unpack(data: &[u8], verif: Option<&[u8]>) -> Vec<u8> {
    let mut source = BitReader::new(data);
    let mut result: Vec<u8> = Vec::with_capacity(32 * 1024);
    let mut last_offset: i32 = 0;
    let mut was_match = false;

    // First byte always literal
    let first = source.read_ubyte();
    result.push(first);

    loop {
        // 1xx
        if source.read_bit() == 1 {
            // 11x
            if source.read_bit() == 1 {
                // 111: tiny match (1 byte, 4-bit offset)
                if source.read_bit() == 1 {
                    let offset = read_fixed_number(&mut source, 4);

                    if offset != 0 {
                        let b = read_back_ubyte(&result, offset as usize);
                        write_check(&mut result, b, verif);
                    } else {
                        write_check(&mut result, 0, verif);
                    }
                    was_match = false;
                } else {
                    // 110: short match (2-3 bytes, 7-bit offset)
                    let byte_val = source.read_ubyte();
                    let len = 2 + (byte_val & 1) as usize;
                    let offset = (byte_val >> 1) as i32;

                    if offset != 0 {
                        for _ in 0..len {
                            let b = read_back_ubyte(&result, offset as usize);
                            write_check(&mut result, b, verif);
                        }
                    } else {
                        // End of stream
                        break;
                    }

                    last_offset = offset;
                    was_match = true;
                }
            } else {
                // 10x: long match (variable offset/length)
                let mut offset = read_variable_number(&mut source);

                if !was_match && offset == 2 {
                    // Repeat last offset
                    offset = last_offset;
                    let len = read_variable_number(&mut source);

                    for _ in 0..len {
                        let b = read_back_ubyte(&result, offset as usize);
                        write_check(&mut result, b, verif);
                    }
                } else {
                    if !was_match {
                        offset -= 3;
                    } else {
                        offset -= 2;
                    }

                    offset <<= 8;
                    offset |= source.read_ubyte() as i32;

                    let mut len = read_variable_number(&mut source);

                    if offset >= 32000 {
                        len += 1;
                    }
                    if offset >= 1280 {
                        len += 1;
                    }
                    if offset < 128 {
                        len += 2;
                    }

                    for _ in 0..len {
                        let b = read_back_ubyte(&result, offset as usize);
                        write_check(&mut result, b, verif);
                    }

                    last_offset = offset;
                }

                was_match = true;
            }
        } else {
            // 0xx: literal byte
            let b = source.read_ubyte();
            write_check(&mut result, b, verif);
            was_match = false;
        }
    }

    result
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_roundtrip_simple() {
        let data = b"Hello, World! Hello, World! Hello, World! This is a test of APJ compression.";
        let packed = pack(data, false, true);
        let unpacked = unpack(&packed, None);
        assert_eq!(data.to_vec(), unpacked);
    }

    #[test]
    fn test_roundtrip_zeros() {
        let data = vec![0u8; 1024];
        let packed = pack(&data, false, true);
        let unpacked = unpack(&packed, None);
        assert_eq!(data, unpacked);
    }

    #[test]
    fn test_roundtrip_ultra() {
        let data = b"ABCABCABCABCABCDEFDEFDEFDEFDEF12341234123412341234";
        let packed = pack(data, true, true);
        let unpacked = unpack(&packed, None);
        assert_eq!(data.to_vec(), unpacked);
    }

    #[test]
    fn test_tiny_data() {
        let data = vec![42u8];
        let packed = pack(&data, false, true);
        // Data smaller than 2 bytes is returned as-is
        assert_eq!(packed, data);
    }

    #[test]
    fn test_bit_writer_reader_roundtrip() {
        let mut writer = BitWriter::new(16);
        writer.write_bit(1);
        writer.write_bit(0);
        writer.write_bit(1);
        writer.write_bit(1);
        writer.write_ubyte(0xAB);
        writer.write_bit(0);
        writer.write_bit(1);

        let bytes = writer.to_bytes();
        let mut reader = BitReader::new(&bytes);
        assert_eq!(reader.read_bit(), 1);
        assert_eq!(reader.read_bit(), 0);
        assert_eq!(reader.read_bit(), 1);
        assert_eq!(reader.read_bit(), 1);
        assert_eq!(reader.read_ubyte(), 0xAB);
        assert_eq!(reader.read_bit(), 0);
        assert_eq!(reader.read_bit(), 1);
    }
}
