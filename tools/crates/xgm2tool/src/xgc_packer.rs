/// XGC Packer — LZ-style compression for XGM2 FM/PSG music data blocks

pub const LITERAL_MAX_SIZE: usize = 7;
pub const MATCH_MAX_SIZE: usize = 31;
pub const MATCH_OFFSET_MAX: usize = 0x100;

pub const FRAME_MIN_SIZE: usize = 32;
pub const FRAME_MAX_SIZE: usize = 256;

// --- Internal types ---

#[derive(Clone)]
struct ByteMatch {
    offset: usize,
    repeat: usize,
}

impl Ord for ByteMatch {
    fn cmp(&self, other: &Self) -> std::cmp::Ordering {
        self.offset.cmp(&other.offset)
    }
}
impl PartialOrd for ByteMatch {
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        Some(self.cmp(other))
    }
}
impl PartialEq for ByteMatch {
    fn eq(&self, other: &Self) -> bool {
        self.offset == other.offset
    }
}
impl Eq for ByteMatch {}

#[derive(Clone)]
struct Match {
    cur_offset: usize,
    ref_offset: usize,
    length: usize,
    saved: i32,
    cost: usize,
}

impl Match {
    const MAX_SAVED: i32 = MATCH_MAX_SIZE as i32 - 1;

    fn new(cur_off: usize, ref_off: usize, len: usize) -> Self {
        let cost = 2;
        Match {
            cur_offset: cur_off,
            ref_offset: ref_off,
            length: len,
            cost,
            saved: len as i32 - cost as i32,
        }
    }

    fn get_relative_offset(&self) -> usize {
        if self.cur_offset >= self.ref_offset {
            self.cur_offset - self.ref_offset
        } else {
            self.ref_offset - self.cur_offset
        }
    }

    fn _fix_for_extra_byte(&mut self, offset: usize) -> bool {
        let rel = self.get_relative_offset();
        if offset < rel {
            let max_len = rel - offset;
            if self.length >= max_len {
                if crate::is_verbose() {
                    println!(
                        "Info: cannot use match (len = {}) because of delay frame command.",
                        self.length
                    );
                }
                return false;
            }
            if self.ref_offset > 0 {
                self.ref_offset -= 1;
            }
        }
        true
    }
}

// --- Helper functions ---

fn get_repeat(data: &[u8], ind: usize) -> usize {
    let value = data[ind];
    let mut off = ind + 1;
    while off < data.len() && data[off] == value {
        off += 1;
    }
    off - ind - 1
}

fn find_best_match_internal(data: &[u8], from: usize, ind: usize) -> Match {
    let mut ref_offset = from;
    let mut cur_offset = ind;
    let mut len = 0;

    while cur_offset < data.len()
        && ref_offset < data.len()
        && data[ref_offset] == data[cur_offset]
        && len < MATCH_MAX_SIZE
    {
        ref_offset += 1;
        cur_offset += 1;
        len += 1;
    }

    Match::new(ind, from, len)
}

fn find_best_match(byte_matches: &[Vec<ByteMatch>], data: &[u8], ind: usize) -> Option<Match> {
    if ind < 1 {
        return None;
    }

    let bml = &byte_matches[data[ind] as usize];
    let off_min = if ind > MATCH_OFFSET_MAX {
        ind - MATCH_OFFSET_MAX
    } else {
        0
    };

    // find starting index
    let mut bml_ind = 0;
    while bml_ind < bml.len() {
        let wm = &bml[bml_ind];
        if wm.offset + wm.repeat >= off_min {
            break;
        }
        bml_ind += 1;
    }

    let cur_repeat = get_repeat(data, ind);
    let mut best: Option<Match> = None;
    let mut saved = 0i32;

    while bml_ind < bml.len() {
        let wm = &bml[bml_ind];
        let mut off = wm.offset;
        let mut repeat = wm.repeat as i64;

        if off >= ind {
            break;
        }

        if off < off_min {
            repeat -= (off_min - off) as i64;
            off = off_min;
        }

        if repeat >= 0 {
            if (repeat as usize) < cur_repeat {
                let m = Match::new(ind, off, std::cmp::min(MATCH_MAX_SIZE, repeat as usize + 1));
                if m.saved >= saved {
                    saved = m.saved;
                    best = Some(m);
                    if saved >= Match::MAX_SAVED {
                        return best;
                    }
                }
            } else {
                if (repeat as usize) > cur_repeat {
                    let mut delta = repeat as usize - cur_repeat;
                    if off + delta >= ind {
                        delta = if ind > off { ind - off - 1 } else { 0 };
                    }
                    off += delta;
                    repeat -= delta as i64;
                }

                let m;
                if repeat > 0 {
                    let rep = repeat as usize;
                    let adj_rep = if off + rep >= ind {
                        if ind > off { ind - off - 1 } else { 0 }
                    } else {
                        rep
                    };

                    let inner = find_best_match_internal(data, off + adj_rep, ind + adj_rep);
                    m = Match::new(
                        inner.cur_offset.saturating_sub(adj_rep),
                        inner.ref_offset.saturating_sub(adj_rep),
                        std::cmp::min(MATCH_MAX_SIZE, inner.length + adj_rep),
                    );
                } else {
                    m = find_best_match_internal(data, off, ind);
                }

                if m.saved >= saved {
                    saved = m.saved;
                    best = Some(m);
                    if saved == Match::MAX_SAVED {
                        return best;
                    }
                }
            }
        } else {
            let mut cur_off = off;
            let mut r = repeat;
            while r >= 0 && cur_off < ind {
                let m = find_best_match_internal(data, cur_off, ind);
                if m.saved >= saved {
                    saved = m.saved;
                    best = Some(m);
                    if saved == Match::MAX_SAVED {
                        return best;
                    }
                }
                r -= 1;
                cur_off += 1;
            }
        }

        bml_ind += 1;
    }

    best
}

fn get_max_frame_offset_for(frame_offsets: &[i32], start_ind: usize, cur_offset: usize) -> i32 {
    if frame_offsets.is_empty() || start_ind >= frame_offsets.len() {
        return (cur_offset + FRAME_MAX_SIZE) as i32;
    }

    let mut ind = start_ind;
    let max_off = (cur_offset + FRAME_MAX_SIZE) as i32;

    while ind < frame_offsets.len() && frame_offsets[ind] < max_off {
        ind += 1;
    }

    let idx = if ind > 0 { ind - 1 } else { 0 };
    let idx = std::cmp::max(idx, start_ind);
    frame_offsets[idx]
}

fn add_block(
    result: &mut Vec<u8>,
    literal: &[u8],
    m: Option<&Match>,
    base_align: i32,
) {
    let mut literal_data = literal.to_vec();
    let start = result.len();
    let block_size = literal_data.len() + if m.is_some() { 2 } else { 1 };
    let end = start + block_size - 1;

    // page crossing
    if ((start as i32 + base_align) & 0xFF00) != ((end as i32 + base_align) & 0xFF00) {
        let first_page_remain = 0x100 - ((start as i32 + base_align) & 0xFF) as usize;

        if first_page_remain > 1 {
            let len = first_page_remain - 1;
            result.push(((len << 5) | 1) as u8);
            result.extend_from_slice(&literal_data[..len]);
            literal_data = literal_data[len..].to_vec();
        } else {
            result.push(1);
        }
    }

    if let Some(mat) = m {
        let off = mat.get_relative_offset();
        result.push(((literal_data.len() << 5) | mat.length) as u8);
        result.extend_from_slice(&literal_data);
        result.push((-(off as i32) & 0xFF) as u8);
    } else {
        if !literal_data.is_empty() {
            result.push((literal_data.len() << 5) as u8);
            result.extend_from_slice(&literal_data);
        }
    }
}

// --- Public API ---

pub fn pack(data: &[u8], frame_offsets: &[i32], base_align: i32) -> Vec<u8> {
    if data.is_empty() {
        return Vec::new();
    }

    let mut result = Vec::new();
    let mut literal = Vec::new();

    // PASS 1: build byte matches table
    let mut byte_matches: Vec<Vec<ByteMatch>> = vec![Vec::new(); 0x100];
    let mut m_offset = 0usize;
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

    for bml in &mut byte_matches {
        bml.sort();
    }

    // PASS 2: find best match for each position
    let mut matches: Vec<Option<Match>> = Vec::with_capacity(data.len());
    for i in 0..data.len() {
        matches.push(find_best_match(&byte_matches, data, i));
    }

    // PASS 3: walk backward and optimize
    let mut costs = vec![0i32; data.len() + 1];
    costs[data.len()] = 0;

    for i in (0..data.len()).rev() {
        let literal_cost = costs[i + 1] + 1;
        let mut match_cost = i32::MAX;

        if let Some(ref m) = matches[i] {
            if i + m.length <= data.len() {
                match_cost = m.cost as i32 + costs[i + m.length];
            }
        }

        if literal_cost < match_cost {
            costs[i] = literal_cost;
            matches[i] = None;
        } else {
            costs[i] = match_cost;
        }
    }

    // PASS 4: build compressed output
    literal.clear();
    let mut next_frame_offset = if !frame_offsets.is_empty() {
        frame_offsets[0]
    } else {
        0
    };
    let mut max_next_frame_offset =
        std::cmp::max(get_max_frame_offset_for(frame_offsets, 0, 0), next_frame_offset);
    let mut ind = 0usize;
    let mut last_frame_start = 0usize;
    let mut frame_ind = 1usize;

    while ind < matches.len() {
        let frame_size = ind - last_frame_start;
        let m = matches[ind].clone();
        let block_size = if m.is_some() {
            m.as_ref().unwrap().length
        } else {
            1
        };

        // advance to current frame offset
        while (next_frame_offset as usize) < ind && frame_ind < frame_offsets.len() {
            next_frame_offset = frame_offsets[frame_ind];
            frame_ind += 1;
        }

        if frame_size >= FRAME_MAX_SIZE {
            eprintln!(
                "Error: max frame size reached at frame #{}",
                frame_ind - 1
            );
        }

        // aligned on end of frame?
        let mut accept = ind == next_frame_offset as usize;
        if accept {
            accept = frame_size >= FRAME_MIN_SIZE || ind == max_next_frame_offset as usize;

            if accept {
                if frame_size > 128 && crate::is_verbose() {
                    println!(
                        "Warning: large frame at #{} - size = {}",
                        frame_ind - 1,
                        frame_size
                    );
                }

                if !literal.is_empty() {
                    add_block(&mut result, &literal, None, base_align);
                    literal.clear();
                }

                result.push(0);
                last_frame_start = ind;

                max_next_frame_offset = get_max_frame_offset_for(frame_offsets, frame_ind, ind);
                while next_frame_offset as usize <= ind && frame_ind < frame_offsets.len() {
                    next_frame_offset = frame_offsets[frame_ind];
                    frame_ind += 1;
                }
            }
        }

        let use_match;
        if !accept {
            // cancel match if it would cross frame boundary
            if ind + block_size > max_next_frame_offset as usize {
                if crate::is_verbose() {
                    println!(
                        "Info: cannot use match at offset ={} (len = {}) because of frame end alignment",
                        ind, block_size
                    );
                }
                use_match = None;
            } else {
                use_match = m;
            }
        } else {
            use_match = m;
        }

        if let Some(ref mat) = use_match {
            add_block(&mut result, &literal, Some(mat), base_align);
            literal.clear();
            ind += mat.length;
        } else {
            if literal.len() >= LITERAL_MAX_SIZE {
                add_block(&mut result, &literal, None, base_align);
                literal.clear();
            }
            literal.push(data[ind]);
            ind += 1;
        }
    }

    if !literal.is_empty() {
        add_block(&mut result, &literal, None, base_align);
    }

    // end marker
    result.push(0);

    if crate::is_verbose() {
        println!(
            "XGC block compressed to {} bytes (original = {})",
            result.len(),
            data.len()
        );
    }

    result
}

pub fn unpack(data: &[u8]) -> Vec<u8> {
    let mut result: Vec<u8> = Vec::new();
    let mut ind = 0;

    while ind < data.len() {
        let block = data[ind] as i8;
        ind += 1;

        let lit_size = ((block >> 5) & 0x07) as usize;
        let mat_size = ((block as u8) & 0x1F) as usize;

        let mat_off = if mat_size > 1 {
            let o = data[ind] as usize;
            ind += 1;
            o
        } else {
            0
        };

        // write literal bytes
        for _ in 0..lit_size {
            if ind < data.len() {
                result.push(data[ind]);
                ind += 1;
            }
        }

        // copy match bytes
        if mat_size > 1 {
            let base = result.len();
            let mut off = if base >= mat_off { base - mat_off } else { 0 };
            for _ in 0..mat_size {
                let b = result[off];
                result.push(b);
                off += 1;
            }
        }
    }

    if crate::is_verbose() {
        println!(
            "Unpacked = {} - packed = {}",
            result.len(),
            data.len()
        );
    }

    result
}
