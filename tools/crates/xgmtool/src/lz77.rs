/// LZ77 compression (from hwzip, public domain)

const HASH_SIZE: usize = 15;
const NO_POS: usize = usize::MAX;
const MIN_LEN: usize = 4;
pub const LZ_WND_SIZE: usize = 32768;

fn cmp_prefix(src: &[u8], i: usize, j: usize, prev_match_len: usize, max_match_len: usize) -> usize {
    // Check first prev_match_len+1 bytes backwards
    for l in 0..=prev_match_len {
        if src[i + prev_match_len - l] != src[j + prev_match_len - l] {
            return 0;
        }
    }
    // Extend match
    let mut l = prev_match_len + 1;
    while l < max_match_len {
        if src[i + l] != src[j + l] {
            break;
        }
        l += 1;
    }
    l
}

fn find_match(
    src: &[u8], pos: usize, hash: u32, max_dist: usize,
    mut prev_match_len: usize, max_match_len: usize, allow_overlap: bool,
    head: &[usize], prev: &[usize], match_pos: &mut usize,
) -> usize {
    let mut max_match_steps: usize = 4096;

    if prev_match_len == 0 {
        prev_match_len = MIN_LEN - 1;
    }
    if prev_match_len >= max_match_len {
        return 0;
    }
    if prev_match_len >= 32 {
        max_match_steps /= 4;
    }

    let mut found = false;
    let mut i = head[hash as usize];

    while i != NO_POS {
        if max_match_steps == 0 { break; }
        max_match_steps -= 1;

        if pos - i > max_dist { break; }

        let max_cmp = if !allow_overlap {
            let mc = max_match_len.min(pos - i);
            if mc <= prev_match_len {
                i = prev[i % LZ_WND_SIZE];
                continue;
            }
            mc
        } else {
            max_match_len
        };

        let l = cmp_prefix(src, i, pos, prev_match_len, max_cmp);
        if l != 0 {
            found = true;
            *match_pos = i;
            prev_match_len = l;
            if l == max_match_len { return l; }
        }

        i = prev[i % LZ_WND_SIZE];
    }

    if !found { 0 } else { prev_match_len }
}

fn hash4(p: &[u8]) -> u32 {
    const HASH_MUL: u32 = 2654435761;
    let val = (p[0] as u32)
        | ((p[1] as u32) << 8)
        | ((p[2] as u32) << 16)
        | ((p[3] as u32) << 24);
    val.wrapping_mul(HASH_MUL) >> (32 - HASH_SIZE)
}

fn insert_hash(hash: u32, pos: usize, head: &mut [usize], prev: &mut [usize]) {
    prev[pos % LZ_WND_SIZE] = head[hash as usize];
    head[hash as usize] = pos;
}

/// Callback-based LZ77 compression.
/// Returns false if any callback returns false.
pub fn lz77_compress<F1, F2>(
    src: &[u8], max_dist: usize, max_len: usize, allow_overlap: bool,
    mut lit_callback: F1, mut backref_callback: F2,
) -> bool
where
    F1: FnMut(u8) -> bool,
    F2: FnMut(usize, usize) -> bool,
{
    let src_len = src.len();
    let mut head = vec![NO_POS; 1 << HASH_SIZE];
    let mut prev = vec![0usize; LZ_WND_SIZE];

    let mut prev_match_len: usize = 0;
    let mut prev_match_pos: usize = NO_POS;
    let mut match_pos: usize = 0;

    let mut i: usize = 0;
    while i + MIN_LEN - 1 < src_len {
        let h = hash4(&src[i..]);
        let match_len = find_match(
            src, i, h, max_dist, prev_match_len,
            max_len.min(src_len - i), allow_overlap,
            &head, &prev, &mut match_pos,
        );

        insert_hash(h, i, &mut head, &mut prev);

        if prev_match_len != 0 && prev_match_len >= match_len {
            let dist = (i - 1) - prev_match_pos;
            if !backref_callback(dist, prev_match_len) { return false; }
            let end = ((i - 1) + prev_match_len).min(src_len - (MIN_LEN - 1));
            for j in (i + 1)..end {
                let h2 = hash4(&src[j..]);
                insert_hash(h2, j, &mut head, &mut prev);
            }
            i = (i - 1) + prev_match_len;
            prev_match_len = 0;
            continue;
        }

        if match_len == 0 {
            if !lit_callback(src[i]) { return false; }
            i += 1;
            continue;
        }

        if prev_match_len != 0 {
            if !lit_callback(src[i - 1]) { return false; }
        }

        prev_match_len = match_len;
        prev_match_pos = match_pos;
        i += 1;
    }

    if prev_match_len != 0 {
        let dist = (i - 1) - prev_match_pos;
        if !backref_callback(dist, prev_match_len) { return false; }
        i = (i - 1) + prev_match_len;
    }

    while i < src_len {
        if !lit_callback(src[i]) { return false; }
        i += 1;
    }

    true
}
