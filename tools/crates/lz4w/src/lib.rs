//! LZ4W compression library for SGDK
//!
//! A word-oriented LZ4 variant optimized for the Mega Drive's 16-bit bus.
//! Faithful port of the original Java LZ4W implementation.

// --- Constants ---

const LITERAL_LENGTH_MASK: u16 = 0xF000;
const LITERAL_LENGTH_SFT: u16 = 12;
const MATCH_LENGTH_MASK: u16 = 0x0F00;
const MATCH_LENGTH_SFT: u16 = 8;
const MATCH_OFFSET_MASK: u16 = 0x00FF;
const MATCH_OFFSET_SFT: u16 = 0;
const MATCH_LONG_OFFSET_MASK: u16 = 0x7FFF;
const MATCH_LONG_OFFSET_ROM_SOURCE_MASK: u16 = 0x8000;

const LITERAL_MAX_SIZE: usize = 0xF;
const MATCH_MIN_SIZE: usize = 1;
const MATCH_LONG_MIN_SIZE: usize = 2;
const MATCH_MAX_SIZE: usize = 0xF + MATCH_MIN_SIZE;
const MATCH_LONG_MAX_SIZE: usize = 0xFF + MATCH_LONG_MIN_SIZE;
const MATCH_MIN_OFFSET: usize = 1;
const MATCH_LONG_MIN_OFFSET: usize = 1;
const MATCH_OFFSET_MAX: usize = 0x0FF + MATCH_MIN_OFFSET;
const MATCH_LONG_OFFSET_MAX: usize = 0x3FFF + MATCH_LONG_MIN_OFFSET;

// --- Types ---

#[derive(Clone)]
struct Match {
    cur_offset: usize,
    ref_offset: usize,
    length: usize,
    saved_word: i32,
    cost: usize,
    long_match: bool,
    rom: bool,
}

const MAX_SAVED_WORD: i32 = (MATCH_LONG_MAX_SIZE as i32) - 2;

impl Match {
    fn new(cur_off: usize, ref_off: usize, len: usize, from_rom: bool) -> Self {
        let rel_offset = if cur_off >= ref_off {
            cur_off - ref_off
        } else {
            ref_off - cur_off
        };

        let (long_match, cost, saved_word) =
            if from_rom || rel_offset > MATCH_OFFSET_MAX || len > MATCH_MAX_SIZE {
                let sw = if from_rom && rel_offset > (MATCH_LONG_OFFSET_MAX - 0x20) {
                    0
                } else {
                    std::cmp::max(0, len as i32 - MATCH_LONG_MIN_SIZE as i32)
                };
                (true, 2, sw)
            } else {
                (false, 1, len as i32 - MATCH_MIN_SIZE as i32)
            };

        Self {
            cur_offset: cur_off,
            ref_offset: ref_off,
            length: len,
            saved_word,
            cost,
            long_match,
            rom: from_rom,
        }
    }

    fn empty() -> Self {
        Self::new(0, 0, 0, false)
    }

    fn relative_offset(&self) -> usize {
        if self.cur_offset >= self.ref_offset {
            self.cur_offset - self.ref_offset
        } else {
            self.ref_offset - self.cur_offset
        }
    }
}

#[derive(Clone)]
struct WordMatch {
    offset: usize,
    repeat: usize,
}

// --- Helper functions ---

fn byte_to_short(data: &[u8]) -> Vec<u16> {
    data.chunks_exact(2)
        .map(|c| u16::from_le_bytes([c[0], c[1]]))
        .collect()
}

fn write_word_le(dst: &mut Vec<u8>, value: u16) {
    dst.push((value & 0xFF) as u8);
    dst.push(((value >> 8) & 0xFF) as u8);
}

fn get_repeat(wdata: &[u16], ind: usize) -> usize {
    let value = wdata[ind];
    let mut off = ind + 1;
    while off < wdata.len() && wdata[off] == value {
        off += 1;
    }
    off - ind - 1
}

fn find_best_match_internal(
    wdata: &[u16],
    from: usize,
    ind: usize,
    origin_start: usize,
) -> Match {
    let max_len = if from < origin_start {
        std::cmp::min(origin_start - from, MATCH_LONG_MAX_SIZE)
    } else {
        MATCH_LONG_MAX_SIZE
    };

    let mut ref_offset = from;
    let mut cur_offset = ind;
    let mut len = 0;
    while cur_offset < wdata.len()
        && wdata[ref_offset] == wdata[cur_offset]
        && len < max_len
    {
        ref_offset += 1;
        cur_offset += 1;
        len += 1;
    }

    Match::new(ind, from, len, from < origin_start)
}

fn find_best_match(
    word_matches: &[Vec<WordMatch>],
    wdata: &[u16],
    ind: usize,
    origin_start_offset: usize,
) -> Option<Match> {
    if ind < 1 {
        return None;
    }

    let wml = &word_matches[wdata[ind] as usize];
    let off_min = if ind > MATCH_LONG_OFFSET_MAX {
        ind - MATCH_LONG_OFFSET_MAX
    } else {
        0
    };

    // Find starting index
    let mut wml_ind = 0;
    while wml_ind < wml.len() {
        let wm = &wml[wml_ind];
        if wm.offset + wm.repeat >= off_min {
            break;
        }
        wml_ind += 1;
    }

    let cur_repeat = get_repeat(wdata, ind);
    let mut best: Option<Match> = None;
    let mut saved_word: i32 = 1;

    while wml_ind < wml.len() {
        let wm = &wml[wml_ind];
        let mut off = wm.offset;
        let mut repeat = wm.repeat as i32;

        if off >= ind {
            break;
        }

        if off < off_min {
            repeat -= (off_min - off) as i32;
            off = off_min;
        }

        let can_optimize =
            off >= origin_start_offset || (off + repeat as usize) < origin_start_offset;

        if can_optimize {
            if (repeat as usize) < cur_repeat {
                let m = Match::new(
                    ind,
                    off,
                    std::cmp::min(MATCH_LONG_MAX_SIZE, repeat as usize + 1),
                    off < origin_start_offset,
                );
                if m.saved_word >= saved_word {
                    saved_word = m.saved_word;
                    best = Some(m);
                    if saved_word == MAX_SAVED_WORD {
                        return best;
                    }
                }
            } else {
                let mut adjusted_repeat = repeat as usize;
                if adjusted_repeat > cur_repeat {
                    let mut delta = adjusted_repeat - cur_repeat;
                    if off + delta >= ind {
                        delta = ind - off - 1;
                    }
                    off += delta;
                    adjusted_repeat -= delta;
                }

                let m = if adjusted_repeat > 0 {
                    if off + adjusted_repeat >= ind {
                        let adjusted_repeat2 = ind - off - 1;
                        let internal = find_best_match_internal(
                            wdata,
                            off + adjusted_repeat2,
                            ind + adjusted_repeat2,
                            origin_start_offset,
                        );
                        Match::new(
                            internal.cur_offset - adjusted_repeat2,
                            internal.ref_offset - adjusted_repeat2,
                            std::cmp::min(
                                MATCH_LONG_MAX_SIZE,
                                internal.length + adjusted_repeat2,
                            ),
                            (internal.ref_offset - adjusted_repeat2) < origin_start_offset,
                        )
                    } else {
                        let internal = find_best_match_internal(
                            wdata,
                            off + adjusted_repeat,
                            ind + adjusted_repeat,
                            origin_start_offset,
                        );
                        Match::new(
                            internal.cur_offset - adjusted_repeat,
                            internal.ref_offset - adjusted_repeat,
                            std::cmp::min(
                                MATCH_LONG_MAX_SIZE,
                                internal.length + adjusted_repeat,
                            ),
                            (internal.ref_offset - adjusted_repeat) < origin_start_offset,
                        )
                    }
                } else {
                    find_best_match_internal(wdata, off, ind, origin_start_offset)
                };

                if m.saved_word >= saved_word {
                    saved_word = m.saved_word;
                    best = Some(m);
                    if saved_word == MAX_SAVED_WORD {
                        return best;
                    }
                }
            }
        } else {
            // Cross-origin boundary case
            let mut r = repeat;
            let mut o = off;
            while r >= 0 && o < ind {
                let m = find_best_match_internal(wdata, o, ind, origin_start_offset);
                if m.saved_word >= saved_word {
                    saved_word = m.saved_word;
                    best = Some(m);
                    if saved_word == MAX_SAVED_WORD {
                        return best;
                    }
                }
                o += 1;
                r -= 1;
            }
        }

        wml_ind += 1;
    }

    best
}

fn add_segment(
    result: &mut Vec<u8>,
    literal: &Vec<u8>,
    m: &Match,
    offset_diff: i32,
) -> Result<i32, String> {
    let mut literal_length = literal.len() / 2;
    let mut literal_offset: usize = 0;
    let mut offset_adj: i32 = 0;

    // Handle literal overflow
    while literal_length > LITERAL_MAX_SIZE {
        offset_adj += 1;

        // Add literal-only segment
        result.push(((LITERAL_MAX_SIZE as u8 & 0xF) << 4) | 0);
        result.push(0);
        // Write literal data
        result.extend_from_slice(&literal[literal_offset..literal_offset + LITERAL_MAX_SIZE * 2]);

        literal_offset += LITERAL_MAX_SIZE * 2;
        literal_length -= LITERAL_MAX_SIZE;
    }

    let match_length = m.length;
    let mut match_offset = m.relative_offset();

    if match_length > 0 || literal_length > 0 {
        if m.long_match {
            offset_adj += 2;
            offset_adj -= match_length as i32;

            result.push(((literal_length as u8 & 0xF) << 4) | 0);
            result.push((match_length - MATCH_LONG_MIN_SIZE) as u8);
        } else {
            offset_adj += 1;

            if match_length != 0 {
                offset_adj -= match_length as i32;
                result.push(
                    ((literal_length as u8 & 0xF) << 4)
                        | ((match_length - MATCH_MIN_SIZE) as u8 & 0xF),
                );
                result.push((match_offset - MATCH_MIN_OFFSET) as u8);
            } else {
                result.push(((literal_length as u8 & 0xF) << 4) | 0);
                result.push(0);
            }
        }

        // Write literal data
        result.extend_from_slice(&literal[literal_offset..literal_offset + literal_length * 2]);

        // Long match extended offset
        if m.long_match {
            if m.rom {
                match_offset = (match_offset as i32 + offset_diff + offset_adj + match_length as i32) as usize;
                if match_offset > MATCH_LONG_OFFSET_MAX {
                    return Err(
                        "Can't encode long offset... retry without previous data block!".to_string(),
                    );
                }
            }

            let neg_offset =
                (-(match_offset as i32 - MATCH_LONG_MIN_OFFSET as i32) as u16)
                    & MATCH_LONG_OFFSET_MASK;
            let encoded = if m.rom {
                neg_offset | MATCH_LONG_OFFSET_ROM_SOURCE_MASK
            } else {
                neg_offset
            };

            result.push((encoded >> 8) as u8);
            result.push((encoded & 0xFF) as u8);
        }
    } else {
        // End marker
        result.push(0);
        result.push(0);
    }

    Ok(offset_adj)
}

/// Pack data using the LZ4W algorithm.
///
/// `data` is the full concatenated data (prev + input).
/// `start` is the byte offset where the actual input data begins.
pub fn pack(data: &[u8], start: usize, silent: bool) -> Result<Vec<u8>, String> {
    let mut result: Vec<u8> = Vec::with_capacity(data.len());
    let mut literal: Vec<u8> = Vec::with_capacity(1024);

    let len = data.len() - start;

    if len >= 2 {
        let wdata = byte_to_short(data);
        let offset = start / 2;

        // PASS 1: build word matches table
        let mut word_matches: Vec<Vec<WordMatch>> = Vec::with_capacity(0x10000);
        for _ in 0..0x10000 {
            word_matches.push(Vec::new());
        }

        let mut wm_offset = if offset > MATCH_LONG_OFFSET_MAX {
            offset - MATCH_LONG_OFFSET_MAX
        } else {
            0
        };

        while wm_offset < wdata.len() {
            let off = wm_offset;
            let val = wdata[wm_offset];
            wm_offset += 1;

            let mut repeat = 0usize;
            while wm_offset < wdata.len() && wdata[wm_offset] == val {
                repeat += 1;
                wm_offset += 1;
            }

            word_matches[val as usize].push(WordMatch { offset: off, repeat });
        }

        // PASS 2: find best match for each position
        let match_count = wdata.len() - offset;
        let mut matches: Vec<Option<Match>> = vec![None; match_count];

        for i in 0..match_count {
            matches[i] = find_best_match(&word_matches, &wdata, offset + i, start / 2);
        }

        // PASS 3: walk backward for optimal match length
        let mut costs = vec![0i32; match_count + 1];

        for i in (0..match_count).rev() {
            let literal_cost = costs[i + 1] + 1;
            let mut match_cost = i32::MAX;

            if let Some(ref m) = matches[i] {
                match_cost = m.cost as i32 + costs[i + m.length];
            }

            if literal_cost < match_cost {
                costs[i] = literal_cost;
                matches[i] = None;
            } else {
                costs[i] = match_cost;
            }
        }

        // PASS 4: build compressed data
        literal.clear();
        let mut ind = 0;
        let mut off_adj: i32 = 0;

        while ind < match_count {
            if let Some(ref m) = matches[ind].clone() {
                let adj = add_segment(&mut result, &literal, m, off_adj)?;
                off_adj += adj;
                ind += m.length;
                literal.clear();
            } else {
                write_word_le(&mut literal, wdata[offset + ind]);
                ind += 1;
            }
        }
    }

    // Remaining literal data
    if !literal.is_empty() {
        add_segment(&mut result, &literal, &Match::empty(), 0)?;
        literal.clear();
    }

    // End marker
    add_segment(&mut result, &literal, &Match::empty(), 0)?;

    // Handle last byte
    let len = data.len() - start;
    if (len & 1) != 0 {
        result.push(0x80);
        result.push(data[data.len() - 1]);
    } else {
        result.push(0x00);
        result.push(0x00);
    }

    if !silent {
        eprintln!("LZ4W packed {} -> {} bytes", len, result.len());
    }

    Ok(result)
}

/// Unpack LZ4W compressed data.
///
/// `data` is the full concatenated data (prev + compressed).
/// `start` is the byte offset where compressed data begins.
pub fn unpack(data: &[u8], start: usize) -> Vec<u8> {
    let mut result: Vec<u8> = Vec::with_capacity(32 * 1024);

    // Copy previous buffer
    result.extend_from_slice(&data[..start]);

    let wdata = byte_to_short(data);
    let mut ind = start / 2;
    let mut offset_adj: i32 = 0;

    while ind < wdata.len() - 1 {
        let seg = wdata[ind].swap_bytes();
        ind += 1;
        offset_adj += 1;

        let literal_length = ((seg & LITERAL_LENGTH_MASK) >> LITERAL_LENGTH_SFT) as usize;
        let mut match_length = ((seg & MATCH_LENGTH_MASK) >> MATCH_LENGTH_SFT) as usize;
        let mut match_offset = ((seg & MATCH_OFFSET_MASK) >> MATCH_OFFSET_SFT) as usize;

        // Empty data → end
        if literal_length == 0 && match_length == 0 && match_offset == 0 {
            break;
        }

        // Write literal data
        for _ in 0..literal_length {
            if ind < wdata.len() {
                write_word_le(&mut result, wdata[ind]);
                ind += 1;
            }
        }

        // No match?
        if match_length == 0 {
            // Long match mode
            if match_offset > 0 {
                let value = wdata[ind].swap_bytes();
                ind += 1;
                offset_adj += 1;

                match_length = match_offset + MATCH_LONG_MIN_SIZE;
                let neg_val = (-(value as i16)) as u16;
                match_offset = (neg_val & MATCH_LONG_OFFSET_MASK) as usize + MATCH_LONG_MIN_OFFSET;

                if (value & MATCH_LONG_OFFSET_ROM_SOURCE_MASK) != 0 {
                    match_offset -= offset_adj as usize;
                }
            }
        } else {
            match_length += MATCH_MIN_SIZE;
            match_offset += MATCH_MIN_OFFSET;
        }

        // Copy match data
        if match_length != 0 {
            let mut off = result.len() - match_offset * 2;
            for _ in 0..match_length {
                let lo = result[off];
                let hi = result[off + 1];
                let value = (lo as u16) | ((hi as u16) << 8);
                write_word_le(&mut result, value);
                off += 2;
            }
            offset_adj -= match_length as i32;
        }
    }

    // Last byte
    if ind < wdata.len() {
        let value = wdata[ind].swap_bytes();
        if (value & 0x8000) != 0 {
            result.push((value & 0xFF) as u8);
        }
    }

    result[start..].to_vec()
}

/// Pad data to word alignment (2 bytes).
pub fn pad_to_word(data: &[u8]) -> Vec<u8> {
    if data.len() & 1 == 1 {
        let mut result = Vec::with_capacity(data.len() + 1);
        result.extend_from_slice(data);
        result.push(0);
        result
    } else {
        data.to_vec()
    }
}
