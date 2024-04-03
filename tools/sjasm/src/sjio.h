/*

  SjASM Z80 Assembler

  Copyright (c) 2015 Konamiman
  Based on Sjasm 0.39g6 - Copyright (c) 2006 Sjoerd Mastijn

  This software is provided 'as-is', without any express or implied warranty.
  In no event will the authors be held liable for any damages arising from the
  use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it freely,
  subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not claim
     that you wrote the original software. If you use this software in a product,
     an acknowledgment in the product documentation would be appreciated but is
     not required.

  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

  3. This notice may not be removed or altered from any source distribution.

*/

// sjio.h

enum _fouten { ALL, PASS1, PASS2, FATAL, CATCHALL, SUPPRES };
enum Ending { END, ELSE, ENDIF, ENDTEXTAREA, ENDM };

#define str(x) #x

extern aint eadres,epadres;

#define OUTPUT_TRUNCATE 0
#define OUTPUT_REWIND 1
#define OUTPUT_APPEND 2

void OpenDest(int);
void NewDest(char *ndestfilename, int mode);
int FileExists(char* filename);
void error(char*,char*,int=PASS2);
void ListFile();
void ListFileSkip(char*);
void EmitByte(int byte);
void EmitBytes(int *bytes);
void EmitWords(int *words);
void EmitBlock(aint byte, aint lengte);
void OpenFile(char *nfilename);
void Close();
void OpenList();
void OpenDest();
void printhex32(char *&p, aint h);
void BinIncFile(char *fname,int offset,int length);
int ReadLine();
Ending ReadFile();
Ending SkipFile();
void NewDest(char *ndestfilename);
void SeekDest(long,int);
int ReadFileToStringLst(stringlst *&f,char *end);
void WriteExp(char *n, aint v);
void emitarm(aint data);
#ifdef METARM
void emitarmdataproc(int cond, int I,int opcode,int S,int Rn,int Rd,int Op2);
void emitthumb(aint data);
#endif
//eof sjio.h

