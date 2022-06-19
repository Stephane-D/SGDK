/* This file is part of hwzip from https://www.hanshq.net/zip.html
   It is put in the public domain; see the LICENSE file for details. */

#ifndef LZ77_H
#define LZ77_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define LZ_WND_SIZE 32768

/* Perform LZ77 compression on the src_len bytes in src, with back references
   limited to a certain maximum distance and length, and with or without
   self-overlap. Returns false as soon as either of the callback functions
   returns false, otherwise returns true when all bytes have been processed. */
bool lz77_compress(const uint8_t *src, size_t src_len,
                   size_t max_dist, size_t max_len, bool allow_overlap,
                   bool (*lit_callback)(uint8_t lit, void *aux),
                   bool (*backref_callback)(size_t dist, size_t len, void *aux),
                   void *aux);

#endif
