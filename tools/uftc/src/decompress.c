//***************************************************************************
// "decompress.c"
// Decompresses a UFTC file into a raw blob
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

#include "../inc/main.h"

//***************************************************************************
// decompress
// Reads a file in UFTC format and outputs an uncompressed blob
//---------------------------------------------------------------------------
// param infile: input file
// param outfile: output file
// param format: which format to parse for
// return: error code
//***************************************************************************

int decompress(FILE *infile, FILE *outfile, int format) {
   // To store error codes
   int errcode;

   uint16_t dicsize;

   // read numer of tile but we can ignore it (not needed here)
   errcode = read_word(infile, &dicsize);
   if (errcode) return errcode;

   // read dictionary size
   errcode = read_word(infile, &dicsize);
   if (errcode) return errcode;

   // Dictionary size must be valid!
   if (dicsize & 0x0001)
      return ERR_CORRUPT;
   if ((dicsize & 0x8000) && format == FORMAT_UFTC15)
      return ERR_CORRUPT;
   if (dicsize < 0x0008)
      return ERR_CORRUPT;

   // Allocate memory for dictionary
   uint8_t *dictionary = (uint8_t *) malloc(sizeof(uint8_t) * dicsize);
   if (dictionary == NULL)
      return ERR_NOMEMORY;

   // Read dictionary into memory
   if (fread(dictionary, 1, dicsize, infile) < dicsize) {
      free(dictionary);
      return ferror(infile) ? ERR_CANTREAD : ERR_CORRUPT;
   }

   // Determine location of last entry in dictionary
   uint16_t limit = dicsize - 0x0008;

   // Read all tiles in the UFTC file
   for (;;) {
      // Read compressed tile
      uint8_t ctile[8];
      size_t numread = fread(ctile, 1, 8, infile);
      if (numread == 0 && feof(infile))
         break;
      if (numread < 8)
         return ferror(infile) ? ERR_CANTREAD : ERR_CORRUPT;

      // Get positions for each 4x4 block in the dictionary
      uint16_t pos[4];
      pos[0] = ctile[0] << 8 | ctile[1];
      pos[1] = ctile[2] << 8 | ctile[3];
      pos[2] = ctile[4] << 8 | ctile[5];
      pos[3] = ctile[6] << 8 | ctile[7];

      // Ensure positions are valid
      if (pos[0] & 0x0001 || pos[0] > limit) return ERR_CORRUPT;
      if (pos[1] & 0x0001 || pos[1] > limit) return ERR_CORRUPT;
      if (pos[2] & 0x0001 || pos[2] > limit) return ERR_CORRUPT;
      if (pos[3] & 0x0001 || pos[3] > limit) return ERR_CORRUPT;

      // To store the uncompressed tile
      uint8_t tile[0x20];
      uint8_t *ptr = tile;

      // Decompress tile
      *ptr++ = dictionary[pos[0] + 0]; *ptr++ = dictionary[pos[0] + 1];
      *ptr++ = dictionary[pos[1] + 0]; *ptr++ = dictionary[pos[1] + 1];
      *ptr++ = dictionary[pos[0] + 2]; *ptr++ = dictionary[pos[0] + 3];
      *ptr++ = dictionary[pos[1] + 2]; *ptr++ = dictionary[pos[1] + 3];
      *ptr++ = dictionary[pos[0] + 4]; *ptr++ = dictionary[pos[0] + 5];
      *ptr++ = dictionary[pos[1] + 4]; *ptr++ = dictionary[pos[1] + 5];
      *ptr++ = dictionary[pos[0] + 6]; *ptr++ = dictionary[pos[0] + 7];
      *ptr++ = dictionary[pos[1] + 6]; *ptr++ = dictionary[pos[1] + 7];
      *ptr++ = dictionary[pos[2] + 0]; *ptr++ = dictionary[pos[2] + 1];
      *ptr++ = dictionary[pos[3] + 0]; *ptr++ = dictionary[pos[3] + 1];
      *ptr++ = dictionary[pos[2] + 2]; *ptr++ = dictionary[pos[2] + 3];
      *ptr++ = dictionary[pos[3] + 2]; *ptr++ = dictionary[pos[3] + 3];
      *ptr++ = dictionary[pos[2] + 4]; *ptr++ = dictionary[pos[2] + 5];
      *ptr++ = dictionary[pos[3] + 4]; *ptr++ = dictionary[pos[3] + 5];
      *ptr++ = dictionary[pos[2] + 6]; *ptr++ = dictionary[pos[2] + 7];
      *ptr++ = dictionary[pos[3] + 6]; *ptr++ = dictionary[pos[3] + 7];

      // Write decompressed tile to blob
      if (fwrite(tile, 1, 0x20, outfile) < 0x20)
         return ERR_CANTWRITE;
   }

   // Success!
   return ERR_NONE;
}
