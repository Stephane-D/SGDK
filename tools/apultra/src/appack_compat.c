/*
 * appack_compat.c - command line compression utility for the apultra library
 *
 * Copyright (C) 2019 Emmanuel Marty
 * Copyright (C) 2020 David Guillen Fandos
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

/*
 * Uses the libdivsufsort library Copyright (c) 2003-2008 Yuta Mori
 *
 * Inspired by cap by Sven-Åke Dahl. https://github.com/svendahl/cap
 * Also inspired by Charles Bloom's compression blog. http://cbloomrants.blogspot.com/
 * With ideas from LZ4 by Yann Collet. https://github.com/lz4/lz4
 * With help and support from spke <zxintrospec@gmail.com>
 *
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif
#include "libapultra.h"

#define OPT_STATS          2

#define TOOL_VERSION "1.3.0"

/*---------------------------------------------------------------------------*/

#ifdef _WIN32
LARGE_INTEGER hpc_frequency;
BOOL hpc_available = FALSE;
#endif

static void do_init_time() {
#ifdef _WIN32
   hpc_frequency.QuadPart = 0;
   hpc_available = QueryPerformanceFrequency(&hpc_frequency);
#endif
}

static long long do_get_time() {
   long long nTime;

#ifdef _WIN32
   if (hpc_available) {
      LARGE_INTEGER nCurTime;

      /* Use HPC hardware for best precision */
      QueryPerformanceCounter(&nCurTime);
      nTime = (long long)(nCurTime.QuadPart * 1000000LL / hpc_frequency.QuadPart);
   }
   else {
      struct _timeb tb;
      _ftime(&tb);

      nTime = ((long long)tb.time * 1000LL + (long long)tb.millitm) * 1000LL;
   }
#else
   struct timeval tm;
   gettimeofday(&tm, NULL);

   nTime = (long long)tm.tv_sec * 1000000LL + (long long)tm.tv_usec;
#endif
   return nTime;
}

/*---------------------------------------------------------------------------*/

static void compression_progress(long long nOriginalSize, long long nCompressedSize) {
   if (nOriginalSize >= 512 * 1024) {
      fprintf(stdout, "\r%lld => %lld (%g %%)     \b\b\b\b\b", nOriginalSize, nCompressedSize, (double)(nCompressedSize * 100.0 / nOriginalSize));
      fflush(stdout);
   }
}

static int do_compress(const char *pszInFilename, const char *pszOutFilename, const unsigned int nOptions, const unsigned int nMaxWindowSize) {
   long long nStartTime = 0LL, nEndTime = 0LL;
   size_t nOriginalSize = 0L, nCompressedSize = 0L, nMaxCompressedSize;
   int nFlags = 0;
   apultra_stats stats;
   unsigned char *pDecompressedData;
   unsigned char *pCompressedData;

   nStartTime = do_get_time();

   /* Read the whole original file in memory */

   FILE *f_in = fopen(pszInFilename, "rb");
   if (!f_in) {
      fprintf(stderr, "error opening '%s' for reading\n", pszInFilename);
      return 100;
   }

   fseek(f_in, 0, SEEK_END);
   nOriginalSize = (size_t)ftell(f_in);
   fseek(f_in, 0, SEEK_SET);

   pDecompressedData = (unsigned char*)malloc(nOriginalSize);
   if (!pDecompressedData) {
      fclose(f_in);
      fprintf(stderr, "out of memory for reading '%s', %zd bytes needed\n", pszInFilename, nOriginalSize);
      return 100;
   }

   if (fread(pDecompressedData, 1, nOriginalSize, f_in) != nOriginalSize) {
      free(pDecompressedData);
      fclose(f_in);
      fprintf(stderr, "I/O error while reading '%s'\n", pszInFilename);
      return 100;
   }

   fclose(f_in);

   /* Allocate max compressed size */

   nMaxCompressedSize = apultra_get_max_compressed_size(nOriginalSize);

   pCompressedData = (unsigned char*)malloc(nMaxCompressedSize);
   if (!pCompressedData) {
      free(pDecompressedData);
      fprintf(stderr, "out of memory for compressing '%s', %zd bytes needed\n", pszInFilename, nMaxCompressedSize);
      return 100;
   }

   memset(pCompressedData, 0, nMaxCompressedSize);

   nCompressedSize = apultra_compress(pDecompressedData, pCompressedData, nOriginalSize, nMaxCompressedSize, nFlags, nMaxWindowSize, compression_progress, &stats);

   nEndTime = do_get_time();

   if (nCompressedSize == -1) {
      free(pCompressedData);
      free(pDecompressedData);
      fprintf(stderr, "compression error for '%s'\n", pszInFilename);
      return 100;
   }

   if (pszOutFilename) {
      FILE *f_out;

      /* Write whole compressed file out */

      f_out = fopen(pszOutFilename, "wb");
      if (f_out) {
         fwrite(pCompressedData, 1, nCompressedSize, f_out);
         fclose(f_out);
      }
   }

   free(pCompressedData);
   free(pDecompressedData);

   if (nOptions & OPT_STATS) {
      double fDelta = ((double)(nEndTime - nStartTime)) / 1000000.0;
      double fSpeed = ((double)nOriginalSize / 1048576.0) / fDelta;
      fprintf(stdout, "\rCompressed '%s' in %g seconds, %.02g Mb/s, %d tokens (%g bytes/token), %d into %d bytes ==> %g %%\n",
         pszInFilename, fDelta, fSpeed, stats.commands_divisor, (double)nOriginalSize / (double)stats.commands_divisor,
         (int)nOriginalSize, (int)nCompressedSize, (double)(nCompressedSize * 100.0 / nOriginalSize));

      fprintf(stdout, "Tokens: literals: %d short matches: %d normal matches: %d large matches: %d rep matches: %d\n",
         stats.num_literals, stats.num_4bit_matches, stats.num_7bit_matches, stats.num_variable_matches, stats.num_rep_matches);
      if (stats.match_divisor > 0) {
         fprintf(stdout, "Offsets: min: %d avg: %d max: %d count: %d\n", stats.min_offset, (int)(stats.total_offsets / (long long)stats.match_divisor), stats.max_offset, stats.match_divisor);
         fprintf(stdout, "Match lens: min: %d avg: %d max: %d count: %d\n", stats.min_match_len, stats.total_match_lens / stats.match_divisor, stats.max_match_len, stats.match_divisor);
      }
      else {
         fprintf(stdout, "Offsets: none\n");
         fprintf(stdout, "Match lens: none\n");
      }
      if (stats.rle1_divisor > 0) {
         fprintf(stdout, "RLE1 lens: min: %d avg: %d max: %d count: %d\n", stats.min_rle1_len, stats.total_rle1_lens / stats.rle1_divisor, stats.max_rle1_len, stats.rle1_divisor);
      }
      else {
         fprintf(stdout, "RLE1 lens: none\n");
      }
      if (stats.rle2_divisor > 0) {
         fprintf(stdout, "RLE2 lens: min: %d avg: %d max: %d count: %d\n", stats.min_rle2_len, stats.total_rle2_lens / stats.rle2_divisor, stats.max_rle2_len, stats.rle2_divisor);
      }
      else {
         fprintf(stdout, "RLE2 lens: none\n");
      }
      fprintf(stdout, "Safe distance: %d (0x%X)\n", stats.safe_dist, stats.safe_dist);
   }
   return 0;
}

/*---------------------------------------------------------------------------*/

static int do_decompress(const char *pszInFilename, const char *pszOutFilename, const unsigned int nOptions) {
   long long nStartTime = 0LL, nEndTime = 0LL;
   size_t nCompressedSize, nMaxDecompressedSize, nOriginalSize;
   unsigned char *pCompressedData;
   unsigned char *pDecompressedData;
   int nFlags = 0;

   /* Read the whole compressed file in memory */

   FILE *f_in = fopen(pszInFilename, "rb");
   if (!f_in) {
      fprintf(stderr, "error opening '%s' for reading\n", pszInFilename);
      return 100;
   }

   fseek(f_in, 0, SEEK_END);
   nCompressedSize = (size_t)ftell(f_in);
   fseek(f_in, 0, SEEK_SET);

   pCompressedData = (unsigned char*)malloc(nCompressedSize);
   if (!pCompressedData) {
      fclose(f_in);
      fprintf(stderr, "out of memory for reading '%s', %zd bytes needed\n", pszInFilename, nCompressedSize);
      return 100;
   }

   if (fread(pCompressedData, 1, nCompressedSize, f_in) != nCompressedSize) {
      free(pCompressedData);
      fclose(f_in);
      fprintf(stderr, "I/O error while reading '%s'\n", pszInFilename);
      return 100;
   }

   fclose(f_in);

   /* Allocate max decompressed size */

   nMaxDecompressedSize = apultra_get_max_decompressed_size(pCompressedData, nCompressedSize, nFlags);
   if (nMaxDecompressedSize == -1) {
      free(pCompressedData);
      fprintf(stderr, "invalid compressed format for file '%s'\n", pszInFilename);
      return 100;
   }

   pDecompressedData = (unsigned char*)malloc(nMaxDecompressedSize);
   if (!pDecompressedData) {
      free(pCompressedData);
      fprintf(stderr, "out of memory for decompressing '%s', %zd bytes needed\n", pszInFilename, nMaxDecompressedSize);
      return 100;
   }

   memset(pDecompressedData, 0, nMaxDecompressedSize);

   nStartTime = do_get_time();

   nOriginalSize = apultra_decompress(pCompressedData, pDecompressedData, nCompressedSize, nMaxDecompressedSize, nFlags);
   if (nOriginalSize == -1) {
      free(pDecompressedData);
      free(pCompressedData);

      fprintf(stderr, "decompression error for '%s'\n", pszInFilename);
      return 100;
   }

   if (pszOutFilename) {
      FILE *f_out;

      /* Write whole decompressed file out */

      f_out = fopen(pszOutFilename, "wb");
      if (f_out) {
         fwrite(pDecompressedData, 1, nOriginalSize, f_out);
         fclose(f_out);
      }
   }

   free(pDecompressedData);
   free(pCompressedData);

   if (nOptions & OPT_STATS) {
      nEndTime = do_get_time();
      double fDelta = ((double)(nEndTime - nStartTime)) / 1000000.0;
      double fSpeed = ((double)nOriginalSize / 1048576.0) / fDelta;
      fprintf(stdout, "Decompressed '%s' in %g seconds, %g Mb/s\n",
         pszInFilename, fDelta, fSpeed);
   }

   return 0;
}

/*---------------------------------------------------------------------------*/

static int do_compare(const char *pszInFilename, const char *pszOutFilename, const unsigned int nOptions) {
   long long nStartTime = 0LL, nEndTime = 0LL;
   size_t nCompressedSize, nMaxDecompressedSize, nOriginalSize, nDecompressedSize;
   unsigned char *pCompressedData = NULL;
   unsigned char *pOriginalData = NULL;
   unsigned char *pDecompressedData = NULL;
   int nFlags = 0;

   /* Read the whole compressed file in memory */

   FILE *f_in = fopen(pszInFilename, "rb");
   if (!f_in) {
      fprintf(stderr, "error opening '%s' for reading\n", pszInFilename);
      return 100;
   }

   fseek(f_in, 0, SEEK_END);
   nCompressedSize = (size_t)ftell(f_in);
   fseek(f_in, 0, SEEK_SET);

   pCompressedData = (unsigned char*)malloc(nCompressedSize);
   if (!pCompressedData) {
      fclose(f_in);
      fprintf(stderr, "out of memory for reading '%s', %zd bytes needed\n", pszInFilename, nCompressedSize);
      return 100;
   }

   if (fread(pCompressedData, 1, nCompressedSize, f_in) != nCompressedSize) {
      free(pCompressedData);
      fclose(f_in);
      fprintf(stderr, "I/O error while reading '%s'\n", pszInFilename);
      return 100;
   }

   fclose(f_in);

   /* Read the whole original file in memory */

   f_in = fopen(pszOutFilename, "rb");
   if (!f_in) {
      free(pCompressedData);
      fprintf(stderr, "error opening '%s' for reading\n", pszInFilename);
      return 100;
   }

   fseek(f_in, 0, SEEK_END);
   nOriginalSize = (size_t)ftell(f_in);
   fseek(f_in, 0, SEEK_SET);

   pOriginalData = (unsigned char*)malloc(nOriginalSize);
   if (!pOriginalData) {
      fclose(f_in);
      free(pCompressedData);
      fprintf(stderr, "out of memory for reading '%s', %zd bytes needed\n", pszInFilename, nOriginalSize);
      return 100;
   }

   if (fread(pOriginalData, 1, nOriginalSize, f_in) != nOriginalSize) {
      free(pOriginalData);
      fclose(f_in);
      free(pCompressedData);
      fprintf(stderr, "I/O error while reading '%s'\n", pszInFilename);
      return 100;
   }

   fclose(f_in);

   /* Allocate max decompressed size */

   nMaxDecompressedSize = apultra_get_max_decompressed_size(pCompressedData, nCompressedSize, nFlags);
   if (nMaxDecompressedSize == -1) {
      free(pOriginalData);
      free(pCompressedData);
      fprintf(stderr, "invalid compressed format for file '%s'\n", pszInFilename);
      return 100;
   }

   pDecompressedData = (unsigned char*)malloc(nMaxDecompressedSize);
   if (!pDecompressedData) {
      free(pOriginalData);
      free(pCompressedData);
      fprintf(stderr, "out of memory for decompressing '%s', %zd bytes needed\n", pszInFilename, nMaxDecompressedSize);
      return 100;
   }

   memset(pDecompressedData, 0, nMaxDecompressedSize);

   nStartTime = do_get_time();

   nDecompressedSize = apultra_decompress(pCompressedData, pDecompressedData, nCompressedSize, nMaxDecompressedSize, nFlags);
   if (nDecompressedSize == -1) {
      free(pDecompressedData);
      free(pOriginalData);
      free(pCompressedData);

      fprintf(stderr, "decompression error for '%s'\n", pszInFilename);
      return 100;
   }

   if (nDecompressedSize != nOriginalSize || memcmp(pDecompressedData, pOriginalData, nOriginalSize)) {
      fprintf(stderr, "error comparing compressed file '%s' with original '%s'\n", pszInFilename, pszOutFilename);
      return 100;
   }

   free(pDecompressedData);
   free(pOriginalData);
   free(pCompressedData);

   if (nOptions & OPT_STATS) {
      nEndTime = do_get_time();
      double fDelta = ((double)(nEndTime - nStartTime)) / 1000000.0;
      double fSpeed = ((double)nOriginalSize / 1048576.0) / fDelta;
      fprintf(stdout, "Compared '%s' in %g seconds, %g Mb/s\n",
         pszInFilename, fDelta, fSpeed);
   }

   return 0;
}

/*---------------------------------------------------------------------------*/

int main(int argc, char **argv) {
   unsigned int nOptions = 0;
   unsigned int nMaxWindowSize = 0;

   if (argc < 4) {
      fprintf(stderr, "apultra command-line tool v" TOOL_VERSION " by Emmanuel Marty and spke\n");
      fprintf(stderr, "usage: %s c|d <infile> <outfile>\n", argv[0]);
      fprintf(stderr, " c: check resulting stream after compressing\n");
      fprintf(stderr, " d: decompress\n");
      return 100;
   }

   do_init_time();

   int silent = (argc >= 5) && !strcmp(argv[4], "-s");
   if (!silent)
     nOptions |= OPT_STATS;

   if (!strcmp(argv[1], "c")) {
      int nResult = do_compress(argv[2], argv[3], nOptions, nMaxWindowSize);
      if (nResult == 0) {
         return do_compare(argv[3], argv[2], nOptions);
      } else {
         return nResult;
      }
   }
   else if (!strcmp(argv[1], "d")) {
      return do_decompress(argv[2], argv[3], nOptions);
   }
   else {
      return 100;
   }
}
