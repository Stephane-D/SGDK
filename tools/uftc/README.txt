-----------------------------------------------------------------------------
 _   _ _____ _____ _____
| | | |  ___|_   _|  _  |
| | | | |__   | | | | |_|
| | | |  __|  | | | |  _
| |_| | |     | | | |_| |
|_____|_|     |_| |_____|

by Javier Degirolmo (Sik)
sik dot the dot hedgehog at gmail dot com

UFTC is a graphics compression format for the Mega Drive. Its purpose is to
compress all the sprites making the animations of an object and be able to
extract time in run-time (as the game is playing). It allows random access of
tiles and it's pretty fast.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Before version 1.2, UFTC was limited to 15-bit offsets, giving it a limit of
4096 dictionary entries and 4096 tiles. Since version 1.2, it uses 16-bit
offsets, giving it a limit of 8192 dictionary entries and 8192 tiles. These
formats are called UFTC15 and UFTC16, respectively.

If you have used UFTC before, take into account that UFTC15 is forwards
compatible with UFTC16. In other words, the new code should be able to read
data compressed for the older versions. You can still use your old files
without any modifications.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Provided in this package:

   * Compression tool source code
   * 68k asm decompression source code for Mega Drive
   * C decompression source code for Mega Drive

The source code for the tool is licensed under the GPL version 3 or later
(see tool/LICENSE). The source code for use in Mega Drive is under a
permissive license (do whatever you want with it as long as you don't claim
it as your own - no explicit credit needed however).

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

UFTC stands for "ultra fast tile compression".

-----------------------------------------------------------------------------

There's a command line tool that can be used to compress or decompress data.
You can build this tool by using the makefile or just compiling together all
the C files. This tool is licensed under the GPL version 3 or later.

To compress a file:

   uftc -c «infile» «outfile»

To decompress a file:

   uftc -d «infile» «outfile»

The tool will default to UFTC16 format. Since UFTC15 files are compatible
with it, this shouldn't be a problem. If for whatever reason you want to
enforce the old limitations (e.g. you can't replace the old code), pass the
-15 or --uftc15 parameter:

   uftc -15 -c «infile» «outfile»
   uftc -15 -d «infile» «outfile»

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

It's suggested to use the ".uftc" extension to denote UFTC-compressed files
(e.g. "example.uftc").

-----------------------------------------------------------------------------

A 68000 assembly routine is provided (md/uftc.68k) that can be used to
decompress UFTC data from assembly code. To do this, you should do the
following:

   lea (input), a6         ; Input address goes into a6
   lea (output), a5        ; Output address goes into a5
   move.w start, d7        ; ID of first tile goes into d7
   move.w count, d6        ; Number of tiles goes into d6
   bsr DecompressUftc      ; Call subroutine

Where «input» is the address of UFTC compressed data, and «output» is where
you want the decompressed tiles to be stored (must be 68000 RAM). The value
«start» is the ID of starting tile to decompress (0 for first tile, 1 for
second tile, etc.) and «count» is the amount of tiles to decompress.

The contents of registers d5 to d7 and a4 to a6 are clobbered after calling
this subroutine.

-----------------------------------------------------------------------------

Also a C function is provided (md/uftc.c and md/uftc.h) that can be called
from C code in the Mega Drive to decompress UFTC data. The usage of this
function is as follows:

   decompress_uftc(uint16_t *output, const uint16_t *input,
                   uint16_t start, uint16_t count)

Parameters are the same as for its assembly counterpart (see above). Make
sure to include "uftc.h" wherever you call this function.

You'll probably want to turn on full optimization (e.g. -O3 in GCC) when
compiling this function. Otherwise decompression is going to be very slow.

-----------------------------------------------------------------------------
