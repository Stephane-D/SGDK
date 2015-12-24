//***************************************************************************
// "main.h"
// Some common definitions and such
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

#ifndef MAIN_H
#define MAIN_H

// Required headers
#include <stdint.h>

// Error codes
enum {
   ERR_NONE,            // No error
   ERR_CANTREAD,        // Can't read from input file
   ERR_CANTWRITE,       // Can't write into output file
   ERR_BADSIZE,         // Input file isn't suitable
   ERR_TOOSMALL,        // Input file must contain at least one tile
   ERR_TOOBIG,          // Dictionary has become too big
   ERR_CORRUPT,         // File is corrupt?
   ERR_NOMEMORY,        // Ran out of memory
   ERR_UNKNOWN          // Unknown error
};

// Possible formats
enum {
   FORMAT_DEFAULT,      // No format specified
   FORMAT_UFTC15,       // UFTC15 (15-bit offsets)
   FORMAT_UFTC16,       // UFTC16 (16-bit offsets)
   FORMAT_TOOMANY       // Too many formats specified
};

// Function prototypes
int read_word(FILE *, uint16_t *);
int write_word(FILE *, const uint16_t);

#endif
