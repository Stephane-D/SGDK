/* This file is part of hwzip from https://www.hanshq.net/zip.html
   It is put in the public domain; see the LICENSE file for details. */

#include "../inc/lz77.h"

#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define HASH_SIZE 15
#define NO_POS    SIZE_MAX
#define MIN_LEN   4

/* Compare the substrings starting at src[i] and src[j], and return the length
 * of the common prefix if it is strictly longer than prev_match_len
 * and shorter or equal to max_match_len, otherwise return zero. */
static size_t cmp(const uint8_t* src, size_t i, size_t j,
    size_t prev_match_len, size_t max_match_len)
{
    size_t l;

    assert(prev_match_len < max_match_len);

    /* Check whether the first prev_match_len + 1 characters match. Do this
     * backwards for a higher chance of finding a mismatch quickly. */
    for (l = 0; l < prev_match_len + 1; l++) {
        if (src[i + prev_match_len - l] !=
            src[j + prev_match_len - l]) {
            return 0;
        }
    }

    assert(l == prev_match_len + 1);

    /* Now check how long the full match is. */
    for (; l < max_match_len; l++) {
        if (src[i + l] != src[j + l]) {
            break;
        }
    }

    assert(l > prev_match_len);
    assert(l <= max_match_len);
    assert(memcmp(&src[i], &src[j], l) == 0);

    return l;
}

static inline size_t min(size_t a, size_t b)
{
    return a < b ? a : b;
}

/* Find the longest most recent string which matches the string starting
 * at src[pos]. The match must be strictly longer than prev_match_len and
 * shorter or equal to max_match_len. Returns the length of the match if found
 * and stores the match position in *match_pos, otherwise returns zero. */
static size_t find_match(const uint8_t* src, size_t pos, uint32_t hash,
    size_t max_dist, size_t prev_match_len, size_t max_match_len, bool allow_overlap,
    const size_t* head, const size_t* prev, size_t* match_pos)
{
    size_t max_match_steps = 4096;
    size_t i, l;
    bool found;
    size_t max_cmp;

    if (prev_match_len == 0) {
        /* We want backrefs of length MIN_LEN or longer. */
        prev_match_len = MIN_LEN - 1;
    }

    if (prev_match_len >= max_match_len) {
        /* A longer match would be too long. */
        return 0;
    }

    if (prev_match_len >= 32) {
        /* Do not try too hard if there is already a good match. */
        max_match_steps /= 4;
    }

    found = false;
    i = head[hash];
    max_cmp = max_match_len;

    /* Walk the linked list of prefix positions. */
    for (i = head[hash]; i != NO_POS; i = prev[i % LZ_WND_SIZE]) {
        if (max_match_steps == 0) {
            break;
        }
        max_match_steps--;

        assert(i < pos && "Matches should precede pos.");
        if (pos - i > max_dist) {
            /* The match is too far back. */
            break;
        }

        if (!allow_overlap) {
            max_cmp = min(max_match_len, pos - i);
            if (max_cmp <= prev_match_len) {
                continue;
            }
        }

        l = cmp(src, i, pos, prev_match_len, max_cmp);

        if (l != 0) {
            assert(l > prev_match_len);
            assert(l <= max_match_len);

            found = true;
            *match_pos = i;
            prev_match_len = l;

            if (l == max_match_len) {
                /* A longer match is not possible. */
                return l;
            }
        }
    }

    if (!found) {
        return 0;
    }

    return prev_match_len;
}

/* Compute a hash value based on four bytes pointed to by ptr. */
static inline uint32_t read32le(const uint8_t* p)
{
    return ((uint32_t)p[0] << 0) |
        ((uint32_t)p[1] << 8) |
        ((uint32_t)p[2] << 16) |
        ((uint32_t)p[3] << 24);
}

static uint32_t hash4(const uint8_t* ptr)
{
    static const uint32_t HASH_MUL = 2654435761U;

    /* Knuth's multiplicative hash. */
    return (read32le(ptr) * HASH_MUL) >> (32 - HASH_SIZE);
}

static void insert_hash(uint32_t hash, size_t pos, size_t* head, size_t* prev)
{
    assert(pos != NO_POS && "Invalid pos!");
    prev[pos % LZ_WND_SIZE] = head[hash];
    head[hash] = pos;
}

bool lz77_compress(const uint8_t* src, size_t src_len, size_t max_dist,
    size_t max_len, bool allow_overlap,
    bool (*lit_callback)(uint8_t lit, void* aux),
    bool (*backref_callback)(size_t dist, size_t len, void* aux),
    void* aux)
{
    size_t head[1U << HASH_SIZE];
    size_t prev[LZ_WND_SIZE];

    uint32_t h;
    size_t i, j, dist;
    size_t match_len, match_pos;
    size_t prev_match_len, prev_match_pos;

    /* Initialize the hash table. */
    for (i = 0; i < sizeof(head) / sizeof(head[0]); i++) {
        head[i] = NO_POS;
    }

    prev_match_len = 0;
    prev_match_pos = NO_POS;

    for (i = 0; i + MIN_LEN - 1 < src_len; i++) {
        /* Search for a match using the hash table. */
        h = hash4(&src[i]);
        match_len = find_match(src, i, h, max_dist, prev_match_len,
            min(max_len, src_len - i), allow_overlap,
            head, prev, &match_pos);

        /* Insert the current hash for future searches. */
        insert_hash(h, i, head, prev);

        /* If the previous match is at least as good as the current. */
        if (prev_match_len != 0 && prev_match_len >= match_len) {
            /* Output the previous match. */
            dist = (i - 1) - prev_match_pos;
            if (!backref_callback(dist, prev_match_len, aux)) {
                return false;
            }
            /* Move past the match. */
            for (j = i + 1; j < min((i - 1) + prev_match_len,
                src_len - (MIN_LEN - 1)); j++) {
                h = hash4(&src[j]);
                insert_hash(h, j, head, prev);
            }
            i = (i - 1) + prev_match_len - 1;
            prev_match_len = 0;
            continue;
        }

        /* If no match (and no previous match), output literal. */
        if (match_len == 0) {
            assert(prev_match_len == 0);
            if (!lit_callback(src[i], aux)) {
                return false;
            }
            continue;
        }

        /* Otherwise the current match is better than the previous. */

        if (prev_match_len != 0) {
            /* Output a literal instead of the previous match. */
            if (!lit_callback(src[i - 1], aux)) {
                return false;
            }
        }

        /* Defer this match and see if the next is even better. */
        prev_match_len = match_len;
        prev_match_pos = match_pos;
    }

    /* Output any previous match. */
    if (prev_match_len != 0) {
        dist = (i - 1) - prev_match_pos;
        if (!backref_callback(dist, prev_match_len, aux)) {
            return false;
        }
        i = (i - 1) + prev_match_len;
    }

    /* Output any remaining literals. */
    for (; i < src_len; i++) {
        if (!lit_callback(src[i], aux)) {
            return false;
        }
    }

    return true;
}
