//***************************************************************************
// "compress.c"
// Compresses a raw blob into an UFTC file
//***************************************************************************
// Uftc compression tool
// Copyright 2011, 2012 Javier Degirolmo
//
// This file is part of the uftc tool.
//
// The uftc tool is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The uftc tool is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with the uftc tool.  If not, see <http://www.gnu.org/licenses/>.
//***************************************************************************

// Required headers
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../inc/main.h"

// Function prototypes
static int putblock(uint8_t **, size_t *, const uint8_t *, uint16_t *);

//***************************************************************************
// compress
// Reads an uncompressed blob from a file and outputs UFTC compressed data
//---------------------------------------------------------------------------
// param infile: input file
// param outfile: output file
// param format: format to output for
// return: error code
//***************************************************************************

int compress(FILE *infile, FILE *outfile, int format) {
   // To store error codes
   int errcode;

   // To store the dictionary
   uint8_t *dictionary = NULL;
   size_t dicsize = 0;

   // To store compressed tiles
   uint8_t *tiles = NULL;
   size_t tilesize = 0;

   // While there're tiles to read...
   for (;;) {
      // Read next raw tile, if any
      uint8_t rawtile[0x20];
      size_t numread = fread(rawtile, 1, 0x20, infile);
      if (numread == 0 && feof(infile))
         break;
      else if (numread < 0x20)
         return ferror(infile) ? ERR_CANTREAD : ERR_BADSIZE;

      // To store block contents
      uint8_t block[8];
      uint16_t blockid[4];

      // Look for first block
      block[0] = rawtile[0];  block[1] = rawtile[1];
      block[2] = rawtile[4];  block[3] = rawtile[5];
      block[4] = rawtile[8];  block[5] = rawtile[9];
      block[6] = rawtile[12]; block[7] = rawtile[13];

      errcode = putblock(&dictionary, &dicsize, block, &blockid[0]);
      if (errcode) {
         if (dictionary) free(dictionary);
         if (tiles) free(tiles);
         return errcode;
      }

      // Look for second block
      block[0] = rawtile[2];  block[1] = rawtile[3];
      block[2] = rawtile[6];  block[3] = rawtile[7];
      block[4] = rawtile[10]; block[5] = rawtile[11];
      block[6] = rawtile[14]; block[7] = rawtile[15];

      errcode = putblock(&dictionary, &dicsize, block, &blockid[1]);
      if (errcode) {
         if (dictionary) free(dictionary);
         if (tiles) free(tiles);
         return errcode;
      }

      // Look for third block
      block[0] = rawtile[16]; block[1] = rawtile[17];
      block[2] = rawtile[20]; block[3] = rawtile[21];
      block[4] = rawtile[24]; block[5] = rawtile[25];
      block[6] = rawtile[28]; block[7] = rawtile[29];

      errcode = putblock(&dictionary, &dicsize, block, &blockid[2]);
      if (errcode) {
         if (dictionary) free(dictionary);
         if (tiles) free(tiles);
         return errcode;
      }

      // Look for fourth block
      block[0] = rawtile[18]; block[1] = rawtile[19];
      block[2] = rawtile[22]; block[3] = rawtile[23];
      block[4] = rawtile[26]; block[5] = rawtile[27];
      block[6] = rawtile[30]; block[7] = rawtile[31];

      errcode = putblock(&dictionary, &dicsize, block, &blockid[3]);
      if (errcode) {
         if (dictionary) free(dictionary);
         if (tiles) free(tiles);
         return errcode;
      }

      // Put compressed tile into list
      tilesize += 8;
      uint8_t *temp = (uint8_t *) realloc(tiles, tilesize);
      if (temp == NULL) {
         if (dictionary) free(dictionary);
         if (tiles) free(tiles);
         return ERR_NOMEMORY;
      }
      tiles = temp;
      temp = tiles + tilesize - 8;

      *temp++ = blockid[0] >> 8; *temp++ = blockid[0] & 0xFF;
      *temp++ = blockid[1] >> 8; *temp++ = blockid[1] & 0xFF;
      *temp++ = blockid[2] >> 8; *temp++ = blockid[2] & 0xFF;
      *temp++ = blockid[3] >> 8; *temp++ = blockid[3] & 0xFF;
   }

   // Check that dictionary is OK
   if (dicsize == 0)
      return ERR_TOOSMALL;
   else if ((dicsize >= 0x8000 && format == FORMAT_UFTC15) ||
    (dicsize >= 0x10000 && format == FORMAT_UFTC16) ||
    ((tilesize / 8) >= 0x10000)) {
      free(dictionary);
      free(tiles);
      return ERR_TOOBIG;
   }

   // Write number of packed tile
   errcode = write_word(outfile, tilesize / 8);
   if (errcode) {
      free(dictionary);
      free(tiles);
      return errcode;
   }

   // Write dictionary size
   errcode = write_word(outfile, dicsize);
   if (errcode) {
      free(dictionary);
      free(tiles);
      return errcode;
   }

   // Write dictionary
   if (fwrite(dictionary, 1, dicsize, outfile) < dicsize) {
      free(dictionary);
      free(tiles);
      return ERR_CANTWRITE;
   }

   // Write compressed tiles
   if (fwrite(tiles, 1, tilesize, outfile) < tilesize) {
      free(dictionary);
      free(tiles);
      return ERR_CANTWRITE;
   }

   // We don't need this anymore
   free(dictionary);
   free(tiles);

   // Success!
   return ERR_NONE;
}

//***************************************************************************
// putblock [internal]
// Looks up for a 4x4 block in the dictionary (adding it if needed), and
// returns its ID unless something went wrong
//---------------------------------------------------------------------------
// param dictionary: pointer to dictionary
// param dicsize: pointer to dictionary size
// param block: pointer to 4x4 block data
// param id: where to store block ID
// return: error code
//***************************************************************************

static int putblock(uint8_t **dictionary, size_t *dicsize,
const uint8_t *block, uint16_t *id) {
   // Look for a match in the dictionary only if there's a dictionary yet...
   uint8_t *ptr = *dictionary;
   if (*dictionary != NULL) {
      // Look if block is in the dictionary already
      size_t limit = *dicsize;
      while (limit >= 8) {
         // Is it in the dictionary already?
         if (!memcmp(ptr, block, 8)) {
            *id = (uint16_t)(ptr - *dictionary);
            return ERR_NONE;
         }

         // Nope, keep looking
         ptr += 2;
         limit -= 2;
      };
   }

   // Block isn't in dictionary, so allocate memory for it
   *dicsize += 8;
   ptr = (uint8_t *) realloc(*dictionary, *dicsize);
   if (ptr == NULL) return ERR_NOMEMORY;
   *dictionary = ptr;

   // Store block in dictionary
   *id = *dicsize - 8;
   memcpy(*dictionary + *id, block, 8);

   // Success!
   return ERR_NONE;
}
