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

// reader.h

int white();
int skipblanks();
void skipblanks(char *&p);
int needequ();
int needfield();
char *getid(char *&p);
char *getinstr(char *&p);
int comma(char *&p);
int oparen(char *&p,char c);
int cparen(char *&p);
char *getparen(char *p);
int check8(unsigned aint val);
int check8o(long val);
int check16(unsigned aint val);
int check24(unsigned aint val);
int need(char *&p, char c);
int need(char *&p, char *c);
int needa(char *&p, char *c1, int r1, char *c2=0, int r2=0, char *c3=0, int r3=0);
int getConstant(char *&op, aint &val);
int getCharConst(char *&p, aint &val);
int getCharConstChar(char *&op, aint &val);
int getBytes(char *&p, int e[], int add, int dc);
int cmphstr(char *&p1, char *p2);
char *getfilename(char *&p);
int needcomma(char *&p);
int needbparen(char *&p);
int islabchar(char p);
structmembs GetStructMemberId(char *&p);
#ifdef METARM
char *getid3(char *&p);
#endif
//eof reader.h

