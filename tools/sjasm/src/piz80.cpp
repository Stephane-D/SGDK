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

// piz80.cpp

#include "sjasm.h"

enum Z80Reg { Z80_B=0,Z80_C,Z80_D,Z80_E,Z80_H,Z80_L,Z80_A=7,
              Z80_I,Z80_R,Z80_F,Z80_BC=0x10,Z80_DE=0x20,Z80_HL=0x30,
              Z80_IXH,Z80_IXL,Z80_IYH,Z80_IYL,
              Z80_SP=0x40,Z80_AF=0x50,Z80_IX=0xdd,Z80_IY=0xfd,Z80_UNK=-1 };
enum Z80Cond { Z80C_C,Z80C_M,Z80C_NC,Z80C_NZ,Z80C_P,Z80C_PE,Z80C_PO,Z80C_Z,Z80C_UNK };

funtabcls z80funtab;

void piZ80() {
  char *n;
  bp=lp;
  if (!(n=getinstr(lp))) 
    if (*lp=='#' && *(lp+1)=='#') {
      lp+=2;
      aint val;
      synerr=0; if (!ParseExpression(lp,val)) val=4; synerr=1;
      mapadr+=((~mapadr+1)&(val-1));
      return;
    } else {
      error ("Unrecognized instruction",lp); return;
    }
  if (!z80funtab.zoek(n)) { error ("Unrecognized instruction",bp); *lp=0; }
}

int z80getbyte(char *&p) {
  aint val;
  if (!ParseExpression(p,val)) { error("Operand expected",NULL); return 0; }
  check8(val);
  return val & 255;
}

int z80getword(char *&p) {
  aint val;
  if (!ParseExpression(p,val)) { error("Operand expected",NULL); return 0; }
  check16(val);
  return val & 65535;
}

int z80getidxoffset(char *&p) {
  aint val;
  char *pp=p;
  skipblanks(pp);
  if (*pp==')') return 0;
  if (*pp==']') return 0;
  if (!ParseExpression(p,val)) { error("Operand expected",NULL); return 0; }
  check8o(val);
  return val & 255;
}

int z80getadres(char *&p, aint &ad) {
  if (getLocaleLabelValue(p,ad)) return 1;
  if (ParseExpression(p,ad)) return 1;
  error("Operand expected",0,CATCHALL);
  return 0;
}

Z80Cond getz80cond(char *&p) {
  char *pp=p;
  skipblanks(p);
  switch (*(p++)) {
  case 'n':
    switch (*(p++)) {
    case 'z': if (!islabchar(*p)) return Z80C_NZ; break;
    case 'c': if (!islabchar(*p)) return Z80C_NC; break;
    case 's': if (!islabchar(*p)) return Z80C_P; break;
    default: break;
    }
    break;
  case 'N':
    switch (*(p++)) {
    case 'Z': if (!islabchar(*p)) return Z80C_NZ; break;
    case 'C': if (!islabchar(*p)) return Z80C_NC; break;
    case 'S': if (!islabchar(*p)) return Z80C_P; break;
    default: break;
    }
    break;
  case 'z': case 'Z': if (!islabchar(*p)) return Z80C_Z; break;
  case 'c': case 'C': if (!islabchar(*p)) return Z80C_C; break;
  case 'm': case 'M': case 's': case 'S': if (!islabchar(*p)) return Z80C_M; break;
  case 'p':
    if (!islabchar(*p)) return Z80C_P;
    switch (*(p++)) {
    case 'e': if (!islabchar(*p)) return Z80C_PE; break;
    case 'o': if (!islabchar(*p)) return Z80C_PO; break;
    default: break;
    }
    break;
  case 'P':
    if (!islabchar(*p)) return Z80C_P;
    switch (*(p++)) {
    case 'E': if (!islabchar(*p)) return Z80C_PE; break;
    case 'O': if (!islabchar(*p)) return Z80C_PO; break;
    default: break;
    }
    break;
  default: break;
  }
  p=pp;
  return Z80C_UNK;
}

Z80Reg getz80reg(char *&p) {
  char *pp=p;
  skipblanks(p);
  switch (*(p++)) {
  case 'a':
    if (!islabchar(*p)) return Z80_A;
    if (*p=='f' && !islabchar(*(p+1))) { ++p; return Z80_AF; }
    break;
  case 'b':
    if (!islabchar(*p)) return Z80_B;
    if (*p=='c' && !islabchar(*(p+1))) { ++p; return Z80_BC; }
    break;
  case 'c':
    if (!islabchar(*p)) return Z80_C;
    break;
  case 'd':
    if (!islabchar(*p)) return Z80_D;
    if (*p=='e' && !islabchar(*(p+1))) { ++p; return Z80_DE; }
    break;
  case 'e':
    if (!islabchar(*p)) return Z80_E;
    break;
  case 'f':
    if (!islabchar(*p)) return Z80_F;
    break;
  case 'h':
    if (!islabchar(*p)) return Z80_H;
    if (*p=='l' && !islabchar(*(p+1))) { ++p; return Z80_HL; }
    break;
  case 'i':
    if (*p=='x') {
      if (!islabchar(*(p+1))) { ++p; return Z80_IX; }
      if (*(p+1)=='h' && !islabchar(*(p+2))) { p+=2; return Z80_IXH; }
      if (*(p+1)=='l' && !islabchar(*(p+2))) { p+=2; return Z80_IXL; }
    }
    if (*p=='y') {
      if (!islabchar(*(p+1))) { ++p; return Z80_IY; }
      if (*(p+1)=='h' && !islabchar(*(p+2))) { p+=2; return Z80_IYH; }
      if (*(p+1)=='l' && !islabchar(*(p+2))) { p+=2; return Z80_IYL; }
    }
    if (!islabchar(*p)) return Z80_I;
    break;
  case 'l':
    if (!islabchar(*p)) return Z80_L;
    break;
  case 'r':
    if (!islabchar(*p)) return Z80_R;
    break;
  case 's':
    if (*p=='p' && !islabchar(*(p+1))) { ++p; return Z80_SP; }
    break;
  case 'A':
    if (!islabchar(*p)) return Z80_A;
    if (*p=='F' && !islabchar(*(p+1))) { ++p; return Z80_AF; }
    break;
  case 'B':
    if (!islabchar(*p)) return Z80_B;
    if (*p=='C' && !islabchar(*(p+1))) { ++p; return Z80_BC; }
    break;
  case 'C':
    if (!islabchar(*p)) return Z80_C;
    break;
  case 'D':
    if (!islabchar(*p)) return Z80_D;
    if (*p=='E' && !islabchar(*(p+1))) { ++p; return Z80_DE; }
    break;
  case 'E':
    if (!islabchar(*p)) return Z80_E;
    break;
  case 'F':
    if (!islabchar(*p)) return Z80_F;
    break;
  case 'H':
    if (!islabchar(*p)) return Z80_H;
    if (*p=='L' && !islabchar(*(p+1))) { ++p; return Z80_HL; }
    break;
  case 'I':
    if (*p=='X') {
      if (!islabchar(*(p+1))) { ++p; return Z80_IX; }
      if (*(p+1)=='H' && !islabchar(*(p+2))) { p+=2; return Z80_IXH; }
      if (*(p+1)=='L' && !islabchar(*(p+2))) { p+=2; return Z80_IXL; }
    }
    if (*p=='Y') {
      if (!islabchar(*(p+1))) { ++p; return Z80_IY; }
      if (*(p+1)=='H' && !islabchar(*(p+2))) { p+=2; return Z80_IYH; }
      if (*(p+1)=='L' && !islabchar(*(p+2))) { p+=2; return Z80_IYL; }
    }
    if (!islabchar(*p)) return Z80_I;
    break;
  case 'L':
    if (!islabchar(*p)) return Z80_L;
    break;
  case 'R':
    if (!islabchar(*p)) return Z80_R;
    break;
  case 'S':
    if (*p=='P' && !islabchar(*(p+1))) { ++p; return Z80_SP; }
    break;
  default: break;
  }
  p=pp;
  return Z80_UNK;
}

void pizADC() {
  Z80Reg reg;
  int e[4];
  e[0]=e[1]=e[2]=e[3]=-1;
  switch (reg=getz80reg(lp)) {
  case Z80_HL:
    if (!comma(lp)) { error("Comma expected",0); break; }
    switch (getz80reg(lp)) {
    case Z80_BC: e[0]=0xed; e[1]=0x4a; break;
    case Z80_DE: e[0]=0xed; e[1]=0x5a; break;
    case Z80_HL: e[0]=0xed; e[1]=0x6a; break;
    case Z80_SP: e[0]=0xed; e[1]=0x7a; break;
    default: ;
    }
    break;
  case Z80_A:
    if (!comma(lp)) { e[0]=0x8f; break; }
    reg=getz80reg(lp);
  default:
    switch (reg) {
    case Z80_IXH: e[0]=0xdd; e[1]=0x8c; break;
    case Z80_IXL: e[0]=0xdd; e[1]=0x8d; break;
    case Z80_IYH: e[0]=0xfd; e[1]=0x8c; break;
    case Z80_IYL: e[0]=0xfd; e[1]=0x8d; break;
    case Z80_B: case Z80_C: case Z80_D: case Z80_E:
    case Z80_H: case Z80_L: case Z80_A: e[0]=0x88+reg; break;
    case Z80_F: case Z80_I: case Z80_R:
    case Z80_AF: case Z80_BC: case Z80_DE: case Z80_HL: case Z80_SP:
    case Z80_IX: case Z80_IY:
      break;
    default:
      reg=Z80_UNK;
      if (oparen(lp,'[')) { 
        if ((reg=getz80reg(lp))==Z80_UNK) break;
      } else if (oparen(lp,'(')) {
        if ((reg=getz80reg(lp))==Z80_UNK) --lp;
      }
      switch (reg) {
      case Z80_HL:
        if (cparen(lp)) e[0]=0x8e;
        break;
      case Z80_IX: case Z80_IY:
        e[1]=0x8e; e[2]=z80getidxoffset(lp);
        if (cparen(lp)) e[0]=reg;
        break;
      default: e[0]=0xce; e[1]=z80getbyte(lp); break;
      }
    }
  }
  EmitBytes(e);
}

void pizADD() {
  Z80Reg reg;
  int e[4];
  e[0]=e[1]=e[2]=e[3]=-1;
  switch (reg=getz80reg(lp)) {
  case Z80_HL:
    if (!comma(lp)) { error("Comma expected",0); break; }
    switch (getz80reg(lp)) {
    case Z80_BC: e[0]=0x09; break;
    case Z80_DE: e[0]=0x19; break;
    case Z80_HL: e[0]=0x29; break;
    case Z80_SP: e[0]=0x39; break;
    default: ;
    }
    break;
  case Z80_IX:
    if (!comma(lp)) { error("Comma expected",0); break; }
    switch (getz80reg(lp)) {
    case Z80_BC: e[0]=0xdd; e[1]=0x09; break;
    case Z80_DE: e[0]=0xdd; e[1]=0x19; break;
    case Z80_IX: e[0]=0xdd; e[1]=0x29; break;
    case Z80_SP: e[0]=0xdd; e[1]=0x39; break;
    default: ;
    }
    break;
  case Z80_IY:
    if (!comma(lp)) { error("Comma expected",0); break; }
    switch (getz80reg(lp)) {
    case Z80_BC: e[0]=0xfd; e[1]=0x09; break;
    case Z80_DE: e[0]=0xfd; e[1]=0x19; break;
    case Z80_IY: e[0]=0xfd; e[1]=0x29; break;
    case Z80_SP: e[0]=0xfd; e[1]=0x39; break;
    default: ;
    }
    break;
  case Z80_A:
    if (!comma(lp)) { e[0]=0x87; break; }
    reg=getz80reg(lp);
  default:
    switch (reg) {
    case Z80_IXH: e[0]=0xdd; e[1]=0x84; break;
    case Z80_IXL: e[0]=0xdd; e[1]=0x85; break;
    case Z80_IYH: e[0]=0xfd; e[1]=0x84; break;
    case Z80_IYL: e[0]=0xfd; e[1]=0x85; break;
    case Z80_B: case Z80_C: case Z80_D: case Z80_E:
    case Z80_H: case Z80_L: case Z80_A: e[0]=0x80+reg; break;
    case Z80_F: case Z80_I: case Z80_R:
    case Z80_AF: case Z80_BC: case Z80_DE: case Z80_HL: case Z80_SP:
    case Z80_IX: case Z80_IY:
      break;
    default:
      reg=Z80_UNK;
      if (oparen(lp,'[')) { 
        if ((reg=getz80reg(lp))==Z80_UNK) break;
      } else if (oparen(lp,'(')) {
        if ((reg=getz80reg(lp))==Z80_UNK) --lp;
      }
      switch (reg) {
      case Z80_HL:
        if (cparen(lp)) e[0]=0x86;
        break;
      case Z80_IX: case Z80_IY:
        e[1]=0x86; e[2]=z80getidxoffset(lp);
        if (cparen(lp)) e[0]=reg;
        break;
      default: e[0]=0xc6; e[1]=z80getbyte(lp); break;
      }
    }
  }
  EmitBytes(e);
}

void pizAND() {
  Z80Reg reg;
  int e[4];
  e[0]=e[1]=e[2]=e[3]=-1;
  switch (reg=getz80reg(lp)) {
  case Z80_A:
    if (!comma(lp)) { e[0]=0xa7; break; }
    reg=getz80reg(lp);
  default:
    switch (reg) {
    case Z80_IXH: e[0]=0xdd; e[1]=0xa4; break;
    case Z80_IXL: e[0]=0xdd; e[1]=0xa5; break;
    case Z80_IYH: e[0]=0xfd; e[1]=0xa4; break;
    case Z80_IYL: e[0]=0xfd; e[1]=0xa5; break;
    case Z80_B: case Z80_C: case Z80_D: case Z80_E:
    case Z80_H: case Z80_L: case Z80_A: e[0]=0xa0+reg; break;
    case Z80_F: case Z80_I: case Z80_R:
    case Z80_AF: case Z80_BC: case Z80_DE: case Z80_HL: case Z80_SP:
    case Z80_IX: case Z80_IY:
      break;
    default:
      reg=Z80_UNK;
      if (oparen(lp,'[')) { 
        if ((reg=getz80reg(lp))==Z80_UNK) break;
      } else if (oparen(lp,'(')) {
        if ((reg=getz80reg(lp))==Z80_UNK) --lp;
      }
      switch (reg) {
      case Z80_HL:
        if (cparen(lp)) e[0]=0xa6;
        break;
      case Z80_IX: case Z80_IY:
        e[1]=0xa6; e[2]=z80getidxoffset(lp);
        if (cparen(lp)) e[0]=reg;
        break;
      default: e[0]=0xe6; e[1]=z80getbyte(lp); break;
      }
    }
  }
  EmitBytes(e);
}

void pizBIT() {
  Z80Reg reg;
  int e[5],bit;
  e[0]=e[1]=e[2]=e[3]=e[4]=-1;
  bit=z80getbyte(lp);
  if (!comma(lp)) bit=-1;
  switch (reg=getz80reg(lp)) {
  case Z80_B: case Z80_C: case Z80_D: case Z80_E:
  case Z80_H: case Z80_L: case Z80_A:
    e[0]=0xcb; e[1]=8*bit+0x40+reg; break;
  default:
    if (!oparen(lp,'[') && !oparen(lp,'(')) break;
      switch (reg=getz80reg(lp)) {
      case Z80_HL:
        if (cparen(lp)) e[0]=0xcb;
        e[1]=8*bit+0x46; break;
      case Z80_IX: case Z80_IY:
        e[1]=0xcb; e[2]=z80getidxoffset(lp); e[3]=8*bit+0x46;
        if (cparen(lp)) e[0]=reg;
        break;
      default: ;
      }
  }
  if (bit<0 || bit>7) e[0]=-1;
  EmitBytes(e);
}

void pizCALL() {
  aint callad;
  int e[4],b;
  e[0]=e[1]=e[2]=e[3]=-1;
  switch (getz80cond(lp)) {
  case Z80C_C: if (comma(lp)) e[0]=0xdc; break;
  case Z80C_M: if (comma(lp)) e[0]=0xfc; break;
  case Z80C_NC: if (comma(lp)) e[0]=0xd4; break;
  case Z80C_NZ: if (comma(lp)) e[0]=0xc4; break;
  case Z80C_P: if (comma(lp)) e[0]=0xf4; break;
  case Z80C_PE: if (comma(lp)) e[0]=0xec; break;
  case Z80C_PO: if (comma(lp)) e[0]=0xe4; break;
  case Z80C_Z: if (comma(lp)) e[0]=0xcc; break;
  default: e[0]=0xcd; break;
  }
  if (!(z80getadres(lp,callad))) callad=0;
  b=(signed)callad;
  e[1]=callad&255; e[2]=(callad>>8)&255;
  if (b>65535) error("Bytes lost",0);
  EmitBytes(e);
}

void pizCCF() {
  EmitByte(0x3f);
}

void pizCP() {
  Z80Reg reg;
  int e[4];
  e[0]=e[1]=e[2]=e[3]=-1;
  switch (reg=getz80reg(lp)) {
  case Z80_A:
    if (!comma(lp)) { e[0]=0xbf; break; }
    reg=getz80reg(lp);
  default:
    switch (reg) {
    case Z80_IXH: e[0]=0xdd; e[1]=0xbc; break;
    case Z80_IXL: e[0]=0xdd; e[1]=0xbd; break;
    case Z80_IYH: e[0]=0xfd; e[1]=0xbc; break;
    case Z80_IYL: e[0]=0xfd; e[1]=0xbd; break;
    case Z80_B: case Z80_C: case Z80_D: case Z80_E:
    case Z80_H: case Z80_L: case Z80_A: e[0]=0xb8+reg; break;
    case Z80_F: case Z80_I: case Z80_R:
    case Z80_AF: case Z80_BC: case Z80_DE: case Z80_HL: case Z80_SP:
    case Z80_IX: case Z80_IY:
      break;
    default:
      reg=Z80_UNK;
      if (oparen(lp,'[')) { 
        if ((reg=getz80reg(lp))==Z80_UNK) break;
      } else if (oparen(lp,'(')) {
        if ((reg=getz80reg(lp))==Z80_UNK) --lp;
      }
      switch (reg) {
      case Z80_HL:
        if (cparen(lp)) e[0]=0xbe;
        break;
      case Z80_IX: case Z80_IY:
        e[1]=0xbe; e[2]=z80getidxoffset(lp);
        if (cparen(lp)) e[0]=reg;
        break;
      default: e[0]=0xfe; e[1]=z80getbyte(lp); break;
      }
    }
  }
  EmitBytes(e);
}

void pizCPD() {
  int e[3];
  e[0]=0xed;
  e[1]=0xa9;
  e[2]=-1;
  EmitBytes(e);
}

void pizCPDR() {
  int e[3];
  e[0]=0xed;
  e[1]=0xb9;
  e[2]=-1;
  EmitBytes(e);
}

void pizCPI() {
  int e[3];
  e[0]=0xed;
  e[1]=0xa1;
  e[2]=-1;
  EmitBytes(e);
}

void pizCPIR() {
  int e[3];
  e[0]=0xed;
  e[1]=0xb1;
  e[2]=-1;
  EmitBytes(e);
}

void pizCPL() {
  EmitByte(0x2f);
}

void pizDAA() {
  EmitByte(0x27);
}

void pizDEC() {
  Z80Reg reg;
  int e[4];
  e[0]=e[1]=e[2]=e[3]=-1;
  switch (getz80reg(lp)) {
  case Z80_A: e[0]=0x3d; break;
  case Z80_B: e[0]=0x05; break;
  case Z80_BC: e[0]=0x0b; break;
  case Z80_C: e[0]=0x0d; break;
  case Z80_D: e[0]=0x15; break;
  case Z80_DE: e[0]=0x1b; break;
  case Z80_E: e[0]=0x1d; break;
  case Z80_H: e[0]=0x25; break;
  case Z80_HL: e[0]=0x2b; break;
  case Z80_IX: e[0]=0xdd; e[1]=0x2b; break;
  case Z80_IY: e[0]=0xfd; e[1]=0x2b; break;
  case Z80_L: e[0]=0x2d; break;
  case Z80_SP: e[0]=0x3b; break;
  case Z80_IXH: e[0]=0xdd; e[1]=0x25; break;
  case Z80_IXL: e[0]=0xdd; e[1]=0x2d; break;
  case Z80_IYH: e[0]=0xfd; e[1]=0x25; break;
  case Z80_IYL: e[0]=0xfd; e[1]=0x2d; break;
  default: 
    if (!oparen(lp,'[') && !oparen(lp,'(')) break;
    switch (reg=getz80reg(lp)) {
    case Z80_HL: 
      if (cparen(lp)) e[0]=0x35; break;
    case Z80_IX: case Z80_IY:
      e[1]=0x35; e[2]=z80getidxoffset(lp);
      if (cparen(lp)) e[0]=reg;
      break;
    default: ;
    }
  }
  EmitBytes(e);
}

void pizDI() {
  EmitByte(0xf3);
}

void pizDJNZ() {
  int jmp;
  aint nad;
  int e[3];
  e[0]=e[1]=e[2]=-1;
  if (!z80getadres(lp,nad)) nad=adres+2;
  jmp=nad-adres-2;
  if (jmp<-128 || jmp>127) { 
    char el[LINEMAX];
    sprintf(el,"Target out of range (%i)",jmp);
   error(el,0); jmp=0;
  }
  e[0]=0x10; e[1]=jmp<0?256+jmp:jmp;
  EmitBytes(e);
}

void pizEI() {
  EmitByte(0xfb);
}

void pizEX() {
  Z80Reg reg;
  int e[4];
  e[0]=e[1]=e[2]=e[3]=-1;
  switch (getz80reg(lp)) {
  case Z80_AF:
    if (comma(lp)) {
      if (getz80reg(lp)==Z80_AF) {
        if (*lp=='\'') ++lp;
      } else break;
    }
    e[0]=0x08;
    break;
  case Z80_DE: 
    if (!comma(lp)) { error("Comma expected",0); break; }
    if (getz80reg(lp)!=Z80_HL) break;
    e[0]=0xeb;
    break;
  default: 
    if (!oparen(lp,'[') && !oparen(lp,'(')) break;
    if (getz80reg(lp)!=Z80_SP) break;
    if (!cparen(lp)) break;
    if (!comma(lp)) { error("Comma expected",0); break; }
    switch (reg=getz80reg(lp)) {
    case Z80_HL: e[0]=0xe3; break;
    case Z80_IX: case Z80_IY:
        e[0]=reg; e[1]=0xe3; break;
    default: ;
    }
  }
  EmitBytes(e);
}

void pizEXX() {
  EmitByte(0xd9);
}

void pizHALT() {
  EmitByte(0x76);
}

void pizIM() {
  int e[3];
  e[0]=0xed; e[2]=-1;
  switch(z80getbyte(lp)) {
  case 0: e[1]=0x46; break;
  case 1: e[1]=0x56; break;
  case 2: e[1]=0x5e; break;
  default: e[0]=-1;
  }
  EmitBytes(e);
}

void pizIN() {
  Z80Reg reg;
  int e[3];
  e[0]=e[1]=e[2]=-1;
  switch (reg=getz80reg(lp)) {
  case Z80_A:
    if (!comma(lp)) break;
    if (!oparen(lp,'[') && !oparen(lp,'(')) break;
    if (getz80reg(lp)==Z80_C) {
      e[1]=0x78; if (cparen(lp)) e[0]=0xed;
    } else {
      e[1]=z80getbyte(lp); if (cparen(lp)) e[0]=0xdb;
    }
    break;
  case Z80_B: case Z80_C: case Z80_D:
  case Z80_E: case Z80_H: case Z80_L: case Z80_F:
    if (!comma(lp)) break;
    if (!oparen(lp,'[') && !oparen(lp,'(')) break;
    if (getz80reg(lp)!=Z80_C) break;
    if (cparen(lp)) e[0]=0xed;
    switch (reg) {
    case Z80_B: e[1]=0x40; break;
    case Z80_C: e[1]=0x48; break;
    case Z80_D: e[1]=0x50; break;
    case Z80_E: e[1]=0x58; break;
    case Z80_H: e[1]=0x60; break;
    case Z80_L: e[1]=0x68; break;
    case Z80_F: e[1]=0x70; break;
    default: ;
    }
  default:
    if (!oparen(lp,'[') && !oparen(lp,'(')) break;
    if (getz80reg(lp)!=Z80_C) break;
    if (cparen(lp)) e[0]=0xed;
    e[1]=0x70;
  }
  EmitBytes(e);
}

void pizINC() {
  Z80Reg reg;
  int e[4];
  e[0]=e[1]=e[2]=e[3]=-1;
  switch (getz80reg(lp)) {
  case Z80_A: e[0]=0x3c; break;
  case Z80_B: e[0]=0x04; break;
  case Z80_BC: e[0]=0x03; break;
  case Z80_C: e[0]=0x0c; break;
  case Z80_D: e[0]=0x14; break;
  case Z80_DE: e[0]=0x13; break;
  case Z80_E: e[0]=0x1c; break;
  case Z80_H: e[0]=0x24; break;
  case Z80_HL: e[0]=0x23; break;
  case Z80_IX: e[0]=0xdd; e[1]=0x23; break;
  case Z80_IY: e[0]=0xfd; e[1]=0x23; break;
  case Z80_L: e[0]=0x2c; break;
  case Z80_SP: e[0]=0x33; break;
  case Z80_IXH: e[0]=0xdd; e[1]=0x24; break;
  case Z80_IXL: e[0]=0xdd; e[1]=0x2c; break;
  case Z80_IYH: e[0]=0xfd; e[1]=0x24; break;
  case Z80_IYL: e[0]=0xfd; e[1]=0x2c; break;
  default: 
    if (!oparen(lp,'[') && !oparen(lp,'(')) break;
    switch (reg=getz80reg(lp)) {
    case Z80_HL: 
      if (cparen(lp)) e[0]=0x34; break;
    case Z80_IX: case Z80_IY:
      e[1]=0x34; e[2]=z80getidxoffset(lp);
      if (cparen(lp)) e[0]=reg;
      break;
    default: ;
    }
  }
  EmitBytes(e);
}

void pizIND() {
  int e[3];
  e[0]=0xed;
  e[1]=0xaa;
  e[2]=-1;
  EmitBytes(e);
}

void pizINDR() {
  int e[3];
  e[0]=0xed;
  e[1]=0xba;
  e[2]=-1;
  EmitBytes(e);
}

void pizINI() {
  int e[3];
  e[0]=0xed;
  e[1]=0xa2;
  e[2]=-1;
  EmitBytes(e);
}

void pizINIR() {
  int e[3];
  e[0]=0xed;
  e[1]=0xb2;
  e[2]=-1;
  EmitBytes(e);
}

void pizJP() {
  Z80Reg reg;
  int haakjes=0;
  aint jpad;
  int e[4],b,k=0;
  e[0]=e[1]=e[2]=e[3]=-1;
  switch (getz80cond(lp)) {
  case Z80C_C: if (comma(lp)) e[0]=0xda; break;
  case Z80C_M: if (comma(lp)) e[0]=0xfa; break;
  case Z80C_NC: if (comma(lp)) e[0]=0xd2; break;
  case Z80C_NZ: if (comma(lp)) e[0]=0xc2; break;
  case Z80C_P: if (comma(lp)) e[0]=0xf2; break;
  case Z80C_PE: if (comma(lp)) e[0]=0xea; break;
  case Z80C_PO: if (comma(lp)) e[0]=0xe2; break;
  case Z80C_Z: if (comma(lp)) e[0]=0xca; break;
  default: 
    reg=Z80_UNK;
    if (oparen(lp,'[')) {
      if ((reg=getz80reg(lp))==Z80_UNK) break;
      haakjes=1;
    } else if (oparen(lp,'(')) {
      if ((reg=getz80reg(lp))==Z80_UNK) --lp; else haakjes=1;
    }
    if (reg==Z80_UNK) reg=getz80reg(lp);
    switch (reg) {
    case Z80_HL: if (haakjes && !cparen(lp)) break; e[0]=0xe9; k=1; break;
    case Z80_IX: case Z80_IY: e[1]=0xe9; if (haakjes && !cparen(lp)) break; e[0]=reg; k=1; break;
    default: e[0]=0xc3;
    }
  }
  if (!k) {
    if (!(z80getadres(lp,jpad))) jpad=0;
    b=(signed)jpad;
    e[1]=jpad&255; e[2]=(jpad>>8)&255;
    if (b>65535) error("Bytes lost",0);
  }
  EmitBytes(e);
}

void pizJR() {
  aint jrad;
  int e[4],jmp;
  e[0]=e[1]=e[2]=e[3]=-1;
  switch (getz80cond(lp)) {
  case Z80C_C: if (comma(lp)) e[0]=0x38; break;
  case Z80C_NC: if (comma(lp)) e[0]=0x30; break;
  case Z80C_NZ: if (comma(lp)) e[0]=0x20; break;
  case Z80C_Z: if (comma(lp)) e[0]=0x28; break;
  case Z80C_M: case Z80C_P: case Z80C_PE: case Z80C_PO: error("Illegal condition",0); break;
  default: e[0]=0x18; break;
  }
  if (!(z80getadres(lp,jrad))) jrad=adres+2;
  jmp=jrad-adres-2;
  if (jmp<-128 || jmp>127) { char el[LINEMAX]; sprintf(el,"Target out of range (%i)",jmp); error(el,0); jmp=0; }
  e[1]=jmp<0?256+jmp:jmp;
  EmitBytes(e);
}

void pizLD() {
  Z80Reg reg;
  int e[7],beginhaakje;
  aint b;
  char *olp;
  e[0]=e[1]=e[2]=e[3]=e[4]=e[5]=e[6]=-1;
  switch (getz80reg(lp)) {
  case Z80_F: case Z80_AF: break;

  case Z80_A:
    if (!comma(lp)) break;
    switch (reg=getz80reg(lp)) {
    case Z80_F: case Z80_BC: case Z80_DE: case Z80_HL: 
    case Z80_SP: case Z80_AF: case Z80_IX: case Z80_IY:
      break;
    case Z80_A: case Z80_B: case Z80_C: case Z80_D: case Z80_E: case Z80_H: case Z80_L:
      e[0]=0x78+reg; break;
    case Z80_I: e[0]=0xed; e[1]=0x57; break;
    case Z80_R: e[0]=0xed; e[1]=0x5f; break;
    case Z80_IXL: e[0]=0xdd; e[1]=0x7d; break;
    case Z80_IXH: e[0]=0xdd; e[1]=0x7c; break;
    case Z80_IYL: e[0]=0xfd; e[1]=0x7d; break;
    case Z80_IYH: e[0]=0xfd; e[1]=0x7c; break;
    default:
      if (oparen(lp,'[')) { 
        if ((reg=getz80reg(lp))==Z80_UNK) {
          b=z80getword(lp); e[1]=b&255; e[2]=(b>>8)&255; if (cparen(lp)) e[0]=0x3a; break;
        }
      } else {
        if (oparen(lp,'(')) { 
          if ((reg=getz80reg(lp))==Z80_UNK) {
            olp=--lp;
            if (!ParseExpression(lp,b)) break;
            if (getparen(olp)==lp) { check16(b); e[0]=0x3a; e[1]=b&255; e[2]=(b>>8)&255; } 
            else { check8(b); e[0]=0x3e; e[1]=b&255; }
          }
        } else { e[0]=0x3e; e[1]=z80getbyte(lp); break; }
      }
      switch (reg) {
      case Z80_BC: if (cparen(lp)) e[0]=0x0a; break;
      case Z80_DE: if (cparen(lp)) e[0]=0x1a; break;
      case Z80_HL: if (cparen(lp)) e[0]=0x7e; break;
      case Z80_IX: case Z80_IY:
        e[1]=0x7e; e[2]=z80getidxoffset(lp); if (cparen(lp)) e[0]=reg; break;
      default: break;
      }
    }
    break;
  
  case Z80_B:
    if (!comma(lp)) break;
    switch (reg=getz80reg(lp)) {
    case Z80_F: case Z80_BC: case Z80_DE: case Z80_HL: case Z80_I: case Z80_R: 
    case Z80_SP: case Z80_AF: case Z80_IX: case Z80_IY:
      break;
    case Z80_A: case Z80_B: case Z80_C: case Z80_D: case Z80_E: case Z80_H: case Z80_L:
      e[0]=0x40+reg; break;
    case Z80_IXL: e[0]=0xdd; e[1]=0x45; break;
    case Z80_IXH: e[0]=0xdd; e[1]=0x44; break;
    case Z80_IYL: e[0]=0xfd; e[1]=0x45; break;
    case Z80_IYH: e[0]=0xfd; e[1]=0x44; break;
    default:
      if (oparen(lp,'[')) { if ((reg=getz80reg(lp))==Z80_UNK) break; } 
      else if (oparen(lp,'(')) { if ((reg=getz80reg(lp))==Z80_UNK) { --lp; e[0]=0x06; e[1]=z80getbyte(lp); break; } } 
      else { e[0]=0x06; e[1]=z80getbyte(lp); break; }
      switch (reg) {
      case Z80_HL: if (cparen(lp)) e[0]=0x46; break;
      case Z80_IX: case Z80_IY:
        e[1]=0x46; e[2]=z80getidxoffset(lp); if (cparen(lp)) e[0]=reg; break;
      default: break;
      }
    }
    break;

  case Z80_C:
    if (!comma(lp)) break;
    switch (reg=getz80reg(lp)) {
    case Z80_F: case Z80_BC: case Z80_DE: case Z80_HL: case Z80_I: case Z80_R: 
    case Z80_SP: case Z80_AF: case Z80_IX: case Z80_IY:
      break;
    case Z80_A: case Z80_B: case Z80_C: case Z80_D: case Z80_E: case Z80_H: case Z80_L:
      e[0]=0x48+reg; break;
    case Z80_IXL: e[0]=0xdd; e[1]=0x4d; break;
    case Z80_IXH: e[0]=0xdd; e[1]=0x4c; break;
    case Z80_IYL: e[0]=0xfd; e[1]=0x4d; break;
    case Z80_IYH: e[0]=0xfd; e[1]=0x4c; break;
    default:
      if (oparen(lp,'[')) { if ((reg=getz80reg(lp))==Z80_UNK) break; } 
      else if (oparen(lp,'(')) { if ((reg=getz80reg(lp))==Z80_UNK) { --lp; e[0]=0x0e; e[1]=z80getbyte(lp); break; } } 
      else { e[0]=0x0e; e[1]=z80getbyte(lp); break; }
      switch (reg) {
      case Z80_HL: if (cparen(lp)) e[0]=0x4e; break;
      case Z80_IX: case Z80_IY:
        e[1]=0x4e; e[2]=z80getidxoffset(lp); if (cparen(lp)) e[0]=reg; break;
      default: break;
      }
    }
    break;
  
  case Z80_D:
    if (!comma(lp)) break;
    switch (reg=getz80reg(lp)) {
    case Z80_F: case Z80_BC: case Z80_DE: case Z80_HL: case Z80_I: case Z80_R: 
    case Z80_SP: case Z80_AF: case Z80_IX: case Z80_IY:
      break;
    case Z80_A: case Z80_B: case Z80_C: case Z80_D: case Z80_E: case Z80_H: case Z80_L:
      e[0]=0x50+reg; break;
    case Z80_IXL: e[0]=0xdd; e[1]=0x55; break;
    case Z80_IXH: e[0]=0xdd; e[1]=0x54; break;
    case Z80_IYL: e[0]=0xfd; e[1]=0x55; break;
    case Z80_IYH: e[0]=0xfd; e[1]=0x54; break;
    default:
      if (oparen(lp,'[')) { if ((reg=getz80reg(lp))==Z80_UNK) break; } 
      else if (oparen(lp,'(')) { if ((reg=getz80reg(lp))==Z80_UNK) { --lp; e[0]=0x16; e[1]=z80getbyte(lp); break; } } 
      else { e[0]=0x16; e[1]=z80getbyte(lp); break; }
      switch (reg) {
      case Z80_HL: if (cparen(lp)) e[0]=0x56; break;
      case Z80_IX: case Z80_IY:
        e[1]=0x56; e[2]=z80getidxoffset(lp); if (cparen(lp)) e[0]=reg; break;
      default: break;
      }
    }
    break;
  
  case Z80_E:
    if (!comma(lp)) break;
    switch (reg=getz80reg(lp)) {
    case Z80_F: case Z80_BC: case Z80_DE: case Z80_HL: case Z80_I: case Z80_R: 
    case Z80_SP: case Z80_AF: case Z80_IX: case Z80_IY:
      break;
    case Z80_A: case Z80_B: case Z80_C: case Z80_D: case Z80_E: case Z80_H: case Z80_L:
      e[0]=0x58+reg; break;
    case Z80_IXL: e[0]=0xdd; e[1]=0x5d; break;
    case Z80_IXH: e[0]=0xdd; e[1]=0x5c; break;
    case Z80_IYL: e[0]=0xfd; e[1]=0x5d; break;
    case Z80_IYH: e[0]=0xfd; e[1]=0x5c; break;
    default:
      if (oparen(lp,'[')) { if ((reg=getz80reg(lp))==Z80_UNK) break; } 
      else if (oparen(lp,'(')) { if ((reg=getz80reg(lp))==Z80_UNK) { --lp; e[0]=0x1e; e[1]=z80getbyte(lp); break; } } 
      else { e[0]=0x1e; e[1]=z80getbyte(lp); break; }
      switch (reg) {
      case Z80_HL: if (cparen(lp)) e[0]=0x5e; break;
      case Z80_IX: case Z80_IY:
        e[1]=0x5e; e[2]=z80getidxoffset(lp); if (cparen(lp)) e[0]=reg; break;
      default: break;
      }
    }
    break;
  
  case Z80_H:
    if (!comma(lp)) break;
    switch (reg=getz80reg(lp)) {
    case Z80_F: case Z80_BC: case Z80_DE: case Z80_HL: case Z80_I: case Z80_R: 
    case Z80_SP: case Z80_AF: case Z80_IX: case Z80_IY:
    case Z80_IXL: case Z80_IXH: case Z80_IYL: case Z80_IYH:
      break;
    case Z80_A: case Z80_B: case Z80_C: case Z80_D: case Z80_E: case Z80_H: case Z80_L:
      e[0]=0x60+reg; break;
    default:
      if (oparen(lp,'[')) { if ((reg=getz80reg(lp))==Z80_UNK) break; } 
      else if (oparen(lp,'(')) { if ((reg=getz80reg(lp))==Z80_UNK) { --lp; e[0]=0x26; e[1]=z80getbyte(lp); break; } } 
      else { e[0]=0x26; e[1]=z80getbyte(lp); break; }
      switch (reg) {
      case Z80_HL: if (cparen(lp)) e[0]=0x66; break;
      case Z80_IX: case Z80_IY:
        e[1]=0x66; e[2]=z80getidxoffset(lp); if (cparen(lp)) e[0]=reg; break;
      default: break;
      }
    }
    break;
  
  case Z80_L:
    if (!comma(lp)) break;
    switch (reg=getz80reg(lp)) {
    case Z80_F: case Z80_BC: case Z80_DE: case Z80_HL: case Z80_I: case Z80_R: 
    case Z80_SP: case Z80_AF: case Z80_IX: case Z80_IY:
    case Z80_IXL: case Z80_IXH: case Z80_IYL: case Z80_IYH:
      break;
    case Z80_A: case Z80_B: case Z80_C: case Z80_D: case Z80_E: case Z80_H: case Z80_L:
      e[0]=0x68+reg; break;
    default:
      if (oparen(lp,'[')) { if ((reg=getz80reg(lp))==Z80_UNK) break; } 
      else if (oparen(lp,'(')) { if ((reg=getz80reg(lp))==Z80_UNK) { --lp; e[0]=0x2e; e[1]=z80getbyte(lp); break; } } 
      else { e[0]=0x2e; e[1]=z80getbyte(lp); break; }
      switch (reg) {
      case Z80_HL: if (cparen(lp)) e[0]=0x6e; break;
      case Z80_IX: case Z80_IY:
        e[1]=0x6e; e[2]=z80getidxoffset(lp); if (cparen(lp)) e[0]=reg; break;
      default: break;
      }
    }
    break;
  
  case Z80_I:
    if (!comma(lp)) break;
    if (getz80reg(lp)==Z80_A) e[0]=0xed; 
    e[1]=0x47; break;
    break;
  
  case Z80_R:
    if (!comma(lp)) break;
    if (getz80reg(lp)==Z80_A) e[0]=0xed; 
    e[1]=0x4f; break;
    break;
  
  case Z80_IXL:
    if (!comma(lp)) break;
    switch (reg=getz80reg(lp)) {
    case Z80_F: case Z80_BC: case Z80_DE: case Z80_HL: case Z80_I: case Z80_R: 
    case Z80_SP: case Z80_AF: case Z80_IX: case Z80_IY: case Z80_H: case Z80_L:
    case Z80_IYL: case Z80_IYH:
      break;
    case Z80_A: case Z80_B: case Z80_C: case Z80_D: case Z80_E:
      e[0]=0xdd; e[1]=0x68+reg; break;
    case Z80_IXL: e[0]=0xdd; e[1]=0x6d; break;
    case Z80_IXH: e[0]=0xdd; e[1]=0x6c; break;
    default:
      e[0]=0xdd; e[1]=0x2e; e[2]=z80getbyte(lp); break;
    }
    break;
  
  case Z80_IXH:
    if (!comma(lp)) break;
    switch (reg=getz80reg(lp)) {
    case Z80_F: case Z80_BC: case Z80_DE: case Z80_HL: case Z80_I: case Z80_R: 
    case Z80_SP: case Z80_AF: case Z80_IX: case Z80_IY: case Z80_H: case Z80_L:
    case Z80_IYL: case Z80_IYH:
      break;
    case Z80_A: case Z80_B: case Z80_C: case Z80_D: case Z80_E:
      e[0]=0xdd; e[1]=0x60+reg; break;
    case Z80_IXL: e[0]=0xdd; e[1]=0x65; break;
    case Z80_IXH: e[0]=0xdd; e[1]=0x64; break;
    default:
      e[0]=0xdd; e[1]=0x26; e[2]=z80getbyte(lp); break;
    }
    break;
  
  case Z80_IYL:
    if (!comma(lp)) break;
    switch (reg=getz80reg(lp)) {
    case Z80_F: case Z80_BC: case Z80_DE: case Z80_HL: case Z80_I: case Z80_R: 
    case Z80_SP: case Z80_AF: case Z80_IX: case Z80_IY: case Z80_H: case Z80_L:
    case Z80_IXL: case Z80_IXH:
      break;
    case Z80_A: case Z80_B: case Z80_C: case Z80_D: case Z80_E:
      e[0]=0xfd; e[1]=0x68+reg; break;
    case Z80_IYL: e[0]=0xfd; e[1]=0x6d; break;
    case Z80_IYH: e[0]=0xfd; e[1]=0x6c; break;
    default:
      e[0]=0xfd; e[1]=0x2e; e[2]=z80getbyte(lp); break;
    }
    break;
  
  case Z80_IYH:
    if (!comma(lp)) break;
    switch (reg=getz80reg(lp)) {
    case Z80_F: case Z80_BC: case Z80_DE: case Z80_HL: case Z80_I: case Z80_R: 
    case Z80_SP: case Z80_AF: case Z80_IX: case Z80_IY: case Z80_H: case Z80_L:
    case Z80_IXL: case Z80_IXH:
      break;
    case Z80_A: case Z80_B: case Z80_C: case Z80_D: case Z80_E:
      e[0]=0xfd; e[1]=0x60+reg; break;
    case Z80_IYL: e[0]=0xfd; e[1]=0x65; break;
    case Z80_IYH: e[0]=0xfd; e[1]=0x64; break;
    default:
      e[0]=0xfd; e[1]=0x26; e[2]=z80getbyte(lp); break;
    }
    break;
  
  case Z80_BC:
    if (!comma(lp)) break;
    switch (getz80reg(lp)) {
    case Z80_BC: e[0]=0x40; e[1]=0x49; break;
    case Z80_DE: e[0]=0x42; e[1]=0x4b; break;
    case Z80_HL: e[0]=0x44; e[1]=0x4d; break;
    case Z80_IX: e[0]=e[2]=0xdd; e[1]=0x44; e[3]=0x4d; break;
    case Z80_IY: e[0]=e[2]=0xfd; e[1]=0x44; e[3]=0x4d; break;
    default:
      if (oparen(lp,'[')) { 
        if ((reg=getz80reg(lp))==Z80_UNK) {
          b=z80getword(lp); e[1]=0x4b; e[2]=b&255; e[3]=(b>>8)&255; if (cparen(lp)) e[0]=0xed; break; 
        }
      } else {
        if (oparen(lp,'(')) {
          if ((reg=getz80reg(lp))==Z80_UNK) {
            olp=--lp;
            b=z80getword(lp);
            if (getparen(olp)==lp) { e[0]=0xed; e[1]=0x4b; e[2]=b&255; e[3]=(b>>8)&255; }
            else { e[0]=0x01; e[1]=b&255; e[2]=(b>>8)&255; }
          }
        } else { e[0]=0x01; b=z80getword(lp); e[1]=b&255; e[2]=(b>>8)&255; break; }
      }
      switch (reg) {
      case Z80_HL: if (cparen(lp)) e[0]=0x4e; e[1]=0x23; e[2]=0x46; e[3]=0x2b; break;
      case Z80_IX: case Z80_IY:
        if ((b=z80getidxoffset(lp))==127) error("Offset out of range",0); 
        if (cparen(lp)) e[0]=e[3]=reg; e[1]=0x4e; e[4]=0x46; e[2]=b; e[5]=b+1; break;
      default: break;
      }
    }
    break;
 
  case Z80_DE:
    if (!comma(lp)) break;
    switch (getz80reg(lp)) {
    case Z80_BC: e[0]=0x50; e[1]=0x59; break;
    case Z80_DE: e[0]=0x52; e[1]=0x5b; break;
    case Z80_HL: e[0]=0x54; e[1]=0x5d; break;
    case Z80_IX: e[0]=e[2]=0xdd; e[1]=0x54; e[3]=0x5d; break;
    case Z80_IY: e[0]=e[2]=0xfd; e[1]=0x54; e[3]=0x5d; break;
    default:
      if (oparen(lp,'[')) { 
        if ((reg=getz80reg(lp))==Z80_UNK) {
          b=z80getword(lp); e[1]=0x5b; e[2]=b&255; e[3]=(b>>8)&255; if (cparen(lp)) e[0]=0xed; break; 
        }
      } else {
        if (oparen(lp,'(')) {
          if ((reg=getz80reg(lp))==Z80_UNK) {
            olp=--lp;
            b=z80getword(lp);
            if (getparen(olp)==lp) { e[0]=0xed; e[1]=0x5b; e[2]=b&255; e[3]=(b>>8)&255; }
            else { e[0]=0x11; e[1]=b&255; e[2]=(b>>8)&255; }
          }
        } else { e[0]=0x11; b=z80getword(lp); e[1]=b&255; e[2]=(b>>8)&255; break; }
      }
      switch (reg) {
      case Z80_HL: if (cparen(lp)) e[0]=0x5e; e[1]=0x23; e[2]=0x56; e[3]=0x2b; break;
      case Z80_IX: case Z80_IY:
        if ((b=z80getidxoffset(lp))==127) error("Offset out of range",0); 
        if (cparen(lp)) e[0]=e[3]=reg; e[1]=0x5e; e[4]=0x56; e[2]=b; e[5]=b+1; break;
      default: break;
      }
    }
    break;

  case Z80_HL:
    if (!comma(lp)) break;
    switch (getz80reg(lp)) {
    case Z80_BC: e[0]=0x60; e[1]=0x69; break;
    case Z80_DE: e[0]=0x62; e[1]=0x6b; break;
    case Z80_HL: e[0]=0x64; e[1]=0x6d; break;
    case Z80_IX: e[0]=0xdd; e[1]=0xe5; e[2]=0xe1; break;
    case Z80_IY: e[0]=0xfd; e[1]=0xe5; e[2]=0xe1; break;
    default:
      if (oparen(lp,'[')) { 
        if ((reg=getz80reg(lp))==Z80_UNK) {
          b=z80getword(lp); e[1]=b&255; e[2]=(b>>8)&255; if (cparen(lp)) e[0]=0x2a; break; 
        }
      } else {
        if (oparen(lp,'(')) {
          if ((reg=getz80reg(lp))==Z80_UNK) {
            olp=--lp;
            b=z80getword(lp);
            if (getparen(olp)==lp) { e[0]=0x2a; e[1]=b&255; e[2]=(b>>8)&255; }
            else { e[0]=0x21; e[1]=b&255; e[2]=(b>>8)&255; }
          }
        } else { e[0]=0x21; b=z80getword(lp); e[1]=b&255; e[2]=(b>>8)&255; break; }
      }
      switch (reg) {
      case Z80_IX: case Z80_IY:
        if ((b=z80getidxoffset(lp))==127) error("Offset out of range",0); 
        if (cparen(lp)) e[0]=e[3]=reg; e[1]=0x6e; e[4]=0x66; e[2]=b; e[5]=b+1; break;
      default: break;
      }
    }
    break;

  case Z80_SP:
    if (!comma(lp)) break;
    switch (reg=getz80reg(lp)) {
    case Z80_HL: e[0]=0xf9; break;
    case Z80_IX: case Z80_IY: e[0]=reg; e[1]=0xf9; break;
    default:
    if (oparen(lp,'(') || oparen(lp,'[')) {
        b=z80getword(lp); e[1]=0x7b; e[2]=b&255; e[3]=(b>>8)&255; if (cparen(lp)) e[0]=0xed;
      } else { b=z80getword(lp); e[0]=0x31; e[1]=b&255; e[2]=(b>>8)&255; }
    }
    break;
  
  case Z80_IX:
    if (!comma(lp)) break;
    switch(reg=getz80reg(lp)) {
    case Z80_BC: e[0]=e[2]=0xdd; e[1]=0x69; e[3]=0x60; break;
    case Z80_DE: e[0]=e[2]=0xdd; e[1]=0x6b; e[3]=0x62; break;
    case Z80_HL: e[0]=0xe5; e[1]=0xdd; e[2]=0xe1; break;
    case Z80_IX: e[0]=e[2]=0xdd; e[1]=0x6d; e[3]=0x64; break;
    case Z80_IY: e[0]=0xfd; e[1]=0xe5; e[2]=0xdd; e[3]=0xe1; break;
    default:
      if (oparen(lp,'[')) { 
        b=z80getword(lp); e[1]=0x2a; e[2]=b&255; e[3]=(b>>8)&255; if (cparen(lp)) e[0]=0xdd; break;
      }
      if (beginhaakje=oparen(lp,'(')) olp=--lp;
      b=z80getword(lp);
      if (beginhaakje && getparen(olp)==lp) { e[0]=0xdd; e[1]=0x2a; e[2]=b&255; e[3]=(b>>8)&255; }
      else { e[0]=0xdd; e[1]=0x21; e[2]=b&255; e[3]=(b>>8)&255; }
      break;
    }
    break;
  
  case Z80_IY:
    if (!comma(lp)) break;
    switch(reg=getz80reg(lp)) {
    case Z80_BC: e[0]=e[2]=0xfd; e[1]=0x69; e[3]=0x60; break;
    case Z80_DE: e[0]=e[2]=0xfd; e[1]=0x6b; e[3]=0x62; break;
    case Z80_HL: e[0]=0xe5; e[1]=0xfd; e[2]=0xe1; break;
    case Z80_IX: e[0]=0xdd; e[1]=0xe5; e[2]=0xfd; e[3]=0xe1; break;
    case Z80_IY: e[0]=e[2]=0xfd; e[1]=0x6d; e[3]=0x64; break;
    default:
      if (oparen(lp,'[')) { 
        b=z80getword(lp); e[1]=0x2a; e[2]=b&255; e[3]=(b>>8)&255; if (cparen(lp)) e[0]=0xfd; break;
      }
      if (beginhaakje=oparen(lp,'(')) olp=--lp;
      b=z80getword(lp);
      if (beginhaakje && getparen(olp)==lp) { e[0]=0xfd; e[1]=0x2a; e[2]=b&255; e[3]=(b>>8)&255; }
      else { e[0]=0xfd; e[1]=0x21; e[2]=b&255; e[3]=(b>>8)&255; }
      break;
    }
    break;

  default:
    if (!oparen(lp,'(') && !oparen(lp,'[')) break;
    switch(getz80reg(lp)) {
    case Z80_BC:
      if (!cparen(lp)) break;
      if (!comma(lp)) break;
      if (getz80reg(lp)!=Z80_A) break;
      e[0]=0x02; break;
    case Z80_DE:
      if (!cparen(lp)) break;
      if (!comma(lp)) break;
      if (getz80reg(lp)!=Z80_A) break;
      e[0]=0x12; break;
    case Z80_HL:
      if (!cparen(lp)) break;
      if (!comma(lp)) break;
      switch (reg=getz80reg(lp)) {
      case Z80_A: case Z80_B: case Z80_C: case Z80_D: case Z80_E: case Z80_H: case Z80_L:
        e[0]=0x70+reg; break;
	    case Z80_I: case Z80_R: case Z80_F:
		    break;
      case Z80_BC: e[0]=0x71; e[1]=0x23; e[2]=0x70; e[3]=0x2b; break;
      case Z80_DE: e[0]=0x73; e[1]=0x23; e[2]=0x72; e[3]=0x2b; break;
      case Z80_HL: case Z80_IX: case Z80_IY: break;
      default:
        e[0]=0x36; e[1]=z80getbyte(lp);
        break;
      }
      break;
    case Z80_IX:
      e[2]=z80getidxoffset(lp);
      if (!cparen(lp)) break;
      if (!comma(lp)) break;
      switch (reg=getz80reg(lp)) {
      case Z80_A: case Z80_B: case Z80_C: case Z80_D: case Z80_E: case Z80_H: case Z80_L:
        e[0]=0xdd; e[1]=0x70+reg; break;
      case Z80_F: case Z80_I: case Z80_R: 
      case Z80_SP: case Z80_AF: case Z80_IX: case Z80_IY:
      case Z80_IXL: case Z80_IXH: case Z80_IYL: case Z80_IYH:
		    break;
      case Z80_BC: 
        if (e[2]==127) error("Offset out of range",0); 
        e[0]=e[3]=0xdd; e[1]=0x71; e[4]=0x70; e[5]=e[2]+1; break;
      case Z80_DE: 
        if (e[2]==127) error("Offset out of range",0); 
        e[0]=e[3]=0xdd; e[1]=0x73; e[4]=0x72; e[5]=e[2]+1; break;
      case Z80_HL: 
        if (e[2]==127) error("Offset out of range",0); 
        e[0]=e[3]=0xdd; e[1]=0x75; e[4]=0x74; e[5]=e[2]+1; break;
      default:
        e[0]=0xdd; e[1]=0x36; e[3]=z80getbyte(lp);
        break;
      }
      break;
    case Z80_IY:
      e[2]=z80getidxoffset(lp);
      if (!cparen(lp)) break;
      if (!comma(lp)) break;
      switch (reg=getz80reg(lp)) {
      case Z80_A: case Z80_B: case Z80_C: case Z80_D: case Z80_E: case Z80_H: case Z80_L:
        e[0]=0xfd; e[1]=0x70+reg; break;
      case Z80_F: case Z80_I: case Z80_R: 
      case Z80_SP: case Z80_AF: case Z80_IX: case Z80_IY:
      case Z80_IXL: case Z80_IXH: case Z80_IYL: case Z80_IYH:
		    break;
      case Z80_BC: 
        if (e[2]==127) error("Offset out of range",0); 
        e[0]=e[3]=0xfd; e[1]=0x71; e[4]=0x70; e[5]=e[2]+1; break;
      case Z80_DE: 
        if (e[2]==127) error("Offset out of range",0); 
        e[0]=e[3]=0xfd; e[1]=0x73; e[4]=0x72; e[5]=e[2]+1; break;
      case Z80_HL: 
        if (e[2]==127) error("Offset out of range",0); 
        e[0]=e[3]=0xfd; e[1]=0x75; e[4]=0x74; e[5]=e[2]+1; break;
      default:
        e[0]=0xfd; e[1]=0x36; e[3]=z80getbyte(lp);
        break;
      }
      break;
    default:
      b=z80getword(lp);
      if (!cparen(lp)) break;
      if (!comma(lp)) break;
      switch (getz80reg(lp)) {
      case Z80_A: e[0]=0x32; e[1]=b&255; e[2]=(b>>8)&255; break;
      case Z80_BC: e[0]=0xed; e[1]=0x43; e[2]=b&255; e[3]=(b>>8)&255; break;
      case Z80_DE: e[0]=0xed; e[1]=0x53; e[2]=b&255; e[3]=(b>>8)&255; break;
      case Z80_HL: e[0]=0x22; e[1]=b&255; e[2]=(b>>8)&255; break;
      case Z80_IX: e[0]=0xdd; e[1]=0x22; e[2]=b&255; e[3]=(b>>8)&255; break;
      case Z80_IY: e[0]=0xfd; e[1]=0x22; e[2]=b&255; e[3]=(b>>8)&255; break;
      case Z80_SP: e[0]=0xed; e[1]=0x73; e[2]=b&255; e[3]=(b>>8)&255; break;
      default: break;
      }
      break;
    }
    break;
  }
  EmitBytes(e);
}

void pizLDD() {
  Z80Reg reg, reg2;
  int e[7],b;
  e[0]=e[1]=e[2]=e[3]=e[4]=e[5]=e[6]=-1;
  switch(reg=getz80reg(lp)) {
  case Z80_A:
    if (!comma(lp)) break;
    if (!oparen(lp,'[') && !oparen(lp,'(')) break;
    switch(reg=getz80reg(lp)) {
    case Z80_BC: if (cparen(lp)) e[0]=0x0a; e[1]=0x0b; break;
    case Z80_DE: if (cparen(lp)) e[0]=0x1a; e[1]=0x1b; break;
    case Z80_HL: if (cparen(lp)) e[0]=0x7e; e[1]=0x2b; break;
    case Z80_IX:
    case Z80_IY: e[1]=0x7e; e[2]=z80getidxoffset(lp); if (cparen(lp)) e[0]=e[3]=reg; e[4]=0x2b; break;
    default: break;
    }
    break;
  case Z80_B: case Z80_C: case Z80_D: case Z80_E: case Z80_H: case Z80_L:
    if (!comma(lp)) break;
    if (!oparen(lp,'[') && !oparen(lp,'(')) break;
    switch(reg2=getz80reg(lp)) {
    case Z80_HL: if (cparen(lp)) e[0]=0x46+reg*8; e[1]=0x2b; break;
    case Z80_IX: 
    case Z80_IY: e[2]=z80getidxoffset(lp); if (cparen(lp)) e[0]=e[3]=reg2; e[1]=0x46+reg*8; e[4]=0x2b; break;
    default: break;
    }
    break;
  default:
    if (oparen(lp,'[') || oparen(lp,'(')) {
      reg=getz80reg(lp);
      if (reg==Z80_IX || reg==Z80_IY) b=z80getidxoffset(lp);
      if (!cparen(lp) || !comma(lp)) break;
      switch(reg) {
      case Z80_BC: case Z80_DE:
        if (getz80reg(lp)==Z80_A) e[0]=reg-14; e[1]=reg-5;
        break;
      case Z80_HL:
        switch (reg=getz80reg(lp)) {
        case Z80_A: case Z80_B: case Z80_C: case Z80_D: case Z80_E: case Z80_H: case Z80_L:
          e[0]=0x70+reg; e[1]=0x2b; break;
        case Z80_UNK:
          e[0]=0x36; e[1]=z80getbyte(lp); e[2]=0x2b; break;
        default: break;
        }
        break;
      case Z80_IX: case Z80_IY:
        switch (reg2=getz80reg(lp)) {
        case Z80_A: case Z80_B: case Z80_C: case Z80_D: case Z80_E: case Z80_H: case Z80_L:
          e[0]=e[3]=reg; e[2]=b; e[1]=0x70+reg2; e[4]=0x2b; break;
        case Z80_UNK:
          e[0]=e[4]=reg; e[1]=0x36; e[2]=b; e[3]=z80getbyte(lp); e[5]=0x2b; break;
        default: break;
        }
        break;
      default: break;
      }
    } else {
      e[0]=0xed; e[1]=0xa8; break;
    }
  }
  EmitBytes(e);
}

void pizLDDR() {
  int e[3];
  e[0]=0xed;
  e[1]=0xb8;
  e[2]=-1;
  EmitBytes(e);
}

void pizLDI() {
  Z80Reg reg, reg2;
  int e[11],b;
  e[0]=e[1]=e[2]=e[3]=e[4]=e[5]=e[6]=e[10]=-1;
  switch(reg=getz80reg(lp)) {
  case Z80_A:
    if (!comma(lp)) break;
    if (!oparen(lp,'[') && !oparen(lp,'(')) break;
    switch(reg=getz80reg(lp)) {
    case Z80_BC: if (cparen(lp)) e[0]=0x0a; e[1]=0x03; break;
    case Z80_DE: if (cparen(lp)) e[0]=0x1a; e[1]=0x13; break;
    case Z80_HL: if (cparen(lp)) e[0]=0x7e; e[1]=0x23; break;
    case Z80_IX:
    case Z80_IY: e[1]=0x7e; e[2]=z80getidxoffset(lp); if (cparen(lp)) e[0]=e[3]=reg; e[4]=0x23; break;
    default: break;
    }
    break;
  case Z80_B: case Z80_C: case Z80_D: case Z80_E: case Z80_H: case Z80_L:
    if (!comma(lp)) break;
    if (!oparen(lp,'[') && !oparen(lp,'(')) break;
    switch(reg2=getz80reg(lp)) {
    case Z80_HL: if (cparen(lp)) e[0]=0x46+reg*8; e[1]=0x23; break;
    case Z80_IX: 
    case Z80_IY: e[2]=z80getidxoffset(lp); if (cparen(lp)) e[0]=e[3]=reg2; e[1]=0x46+reg*8; e[4]=0x23; break;
    default: break;
    }
    break;
  case Z80_BC:
    if (!comma(lp)) break;
    if (!oparen(lp,'[') && !oparen(lp,'(')) break;
    switch(reg=getz80reg(lp)) {
    case Z80_HL: if(cparen(lp)) e[0]=0x4e; e[1]=e[3]=0x23; e[2]=0x46; break;
    case Z80_IX: case Z80_IY:
      e[2]=e[7]=z80getidxoffset(lp); if (cparen(lp)) e[0]=e[3]=e[5]=e[8]=reg;
      e[1]=0x4e; e[6]=0x46; e[4]=e[9]=0x23; break;
    default: break;
    }
    break;
  case Z80_DE:
    if (!comma(lp)) break;
    if (!oparen(lp,'[') && !oparen(lp,'(')) break;
    switch(reg=getz80reg(lp)) {
    case Z80_HL: if(cparen(lp)) e[0]=0x5e; e[1]=e[3]=0x23; e[2]=0x56; break;
    case Z80_IX: case Z80_IY:
      e[2]=e[7]=z80getidxoffset(lp); if (cparen(lp)) e[0]=e[3]=e[5]=e[8]=reg;
      e[1]=0x5e; e[6]=0x56; e[4]=e[9]=0x23; break;
    default: break;
    }
    break;
  case Z80_HL:
    if (!comma(lp)) break;
    if (!oparen(lp,'[') && !oparen(lp,'(')) break;
    switch(reg=getz80reg(lp)) {
    case Z80_IX: case Z80_IY:
      e[2]=e[7]=z80getidxoffset(lp); if (cparen(lp)) e[0]=e[3]=e[5]=e[8]=reg;
      e[1]=0x6e; e[6]=0x66; e[4]=e[9]=0x23; break;
    default: break;
    }
    break;
  default:
    if (oparen(lp,'[') || oparen(lp,'(')) {
      reg=getz80reg(lp);
      if (reg==Z80_IX || reg==Z80_IY) b=z80getidxoffset(lp);
      if (!cparen(lp) || !comma(lp)) break;
      switch(reg) {
      case Z80_BC: case Z80_DE:
        if (getz80reg(lp)==Z80_A) e[0]=reg-14; e[1]=reg-13;
        break;
      case Z80_HL:
        switch (reg=getz80reg(lp)) {
        case Z80_A: case Z80_B: case Z80_C: case Z80_D: case Z80_E: case Z80_H: case Z80_L:
          e[0]=0x70+reg; e[1]=0x23; break;
        case Z80_BC: e[0]=0x71; e[1]=e[3]=0x23; e[2]=0x70; break;
        case Z80_DE: e[0]=0x73; e[1]=e[3]=0x23; e[2]=0x72; break;
        case Z80_UNK:
          e[0]=0x36; e[1]=z80getbyte(lp); e[2]=0x23; break;
        default: break;
        }
        break;
      case Z80_IX: case Z80_IY:
        switch (reg2=getz80reg(lp)) {
        case Z80_A: case Z80_B: case Z80_C: case Z80_D: case Z80_E: case Z80_H: case Z80_L:
          e[0]=e[3]=reg; e[2]=b; e[1]=0x70+reg2; e[4]=0x23; break;
        case Z80_BC: e[0]=e[3]=e[5]=e[8]=reg; e[1]=0x71; e[6]=0x70; e[4]=e[9]=0x23; e[2]=e[7]=b; break;
        case Z80_DE: e[0]=e[3]=e[5]=e[8]=reg; e[1]=0x73; e[6]=0x72; e[4]=e[9]=0x23; e[2]=e[7]=b; break;
        case Z80_HL: e[0]=e[3]=e[5]=e[8]=reg; e[1]=0x75; e[6]=0x74; e[4]=e[9]=0x23; e[2]=e[7]=b; break;
        case Z80_UNK:
          e[0]=e[4]=reg; e[1]=0x36; e[2]=b; e[3]=z80getbyte(lp); e[5]=0x23; break;
        default: break;
        }
        break;
      default: break;
      }
    } else {
      e[0]=0xed; e[1]=0xa0; break;
    }
  }
  EmitBytes(e);
}

void pizLDIR() {
  int e[3];
  e[0]=0xed;
  e[1]=0xb0;
  e[2]=-1;
  EmitBytes(e);
}

void pizMULUB() {
  Z80Reg reg;
  int e[3];
  e[0]=e[1]=e[2]=-1;
  if ((reg=getz80reg(lp))==Z80_A && comma(lp)) reg=getz80reg(lp);
  switch (reg) {
    case Z80_B: case Z80_C: case Z80_D: case Z80_E:
    case Z80_H: case Z80_L: case Z80_A: e[0]=0xed; e[1]=0xc1+reg*8; break;
    default: ;
  }
  EmitBytes(e);
}

void pizMULUW() {
  Z80Reg reg;
  int e[3];
  e[0]=e[1]=e[2]=-1;
  if ((reg=getz80reg(lp))==Z80_HL && comma(lp)) reg=getz80reg(lp);
  switch (reg) {
    case Z80_BC: case Z80_SP: e[0]=0xed; e[1]=0xb3+reg; break;
    default: ;
  }
  EmitBytes(e);
}

void pizNEG() {
  int e[3];
  e[0]=0xed;
  e[1]=0x44;
  e[2]=-1;
  EmitBytes(e);
}

void pizNOP() {
  EmitByte(0x0);
}

void pizOR() {
  Z80Reg reg;
  int e[4];
  e[0]=e[1]=e[2]=e[3]=-1;
  switch (reg=getz80reg(lp)) {
  case Z80_A:
    if (!comma(lp)) { e[0]=0xb7; break; }
    reg=getz80reg(lp);
  default:
    switch (reg) {
    case Z80_IXH: e[0]=0xdd; e[1]=0xb4; break;
    case Z80_IXL: e[0]=0xdd; e[1]=0xb5; break;
    case Z80_IYH: e[0]=0xfd; e[1]=0xb4; break;
    case Z80_IYL: e[0]=0xfd; e[1]=0xb5; break;
    case Z80_B: case Z80_C: case Z80_D: case Z80_E:
    case Z80_H: case Z80_L: case Z80_A: e[0]=0xb0+reg; break;
    case Z80_F: case Z80_I: case Z80_R:
    case Z80_AF: case Z80_BC: case Z80_DE: case Z80_HL: case Z80_SP:
    case Z80_IX: case Z80_IY:
      break;
    default:
      reg=Z80_UNK;
      if (oparen(lp,'[')) { 
        if ((reg=getz80reg(lp))==Z80_UNK) break;
      } else if (oparen(lp,'(')) {
        if ((reg=getz80reg(lp))==Z80_UNK) --lp;
      }
      switch (reg) {
      case Z80_HL:
        if (cparen(lp)) e[0]=0xb6;
        break;
      case Z80_IX: case Z80_IY:
        e[1]=0xb6; e[2]=z80getidxoffset(lp);
        if (cparen(lp)) e[0]=reg;
        break;
      default: e[0]=0xf6; e[1]=z80getbyte(lp); break;
      }
    }
  }
  EmitBytes(e);
}

void pizOTDR() {
  int e[3];
  e[0]=0xed;
  e[1]=0xbb;
  e[2]=-1;
  EmitBytes(e);
}

void pizOTIR() {
  int e[3];
  e[0]=0xed;
  e[1]=0xb3;
  e[2]=-1;
  EmitBytes(e);
}

void pizOUT() {
  Z80Reg reg;
  int e[3];
  e[0]=e[1]=e[2]=-1;
  if (oparen(lp,'[') || oparen(lp,'(')) {
    if (getz80reg(lp)==Z80_C) {
      if (cparen(lp))
        if (comma(lp))
          switch (reg=getz80reg(lp)) {
          case Z80_A: e[0]=0xed; e[1]=0x79; break;
          case Z80_B: e[0]=0xed; e[1]=0x41; break;
          case Z80_C: e[0]=0xed; e[1]=0x49; break;
          case Z80_D: e[0]=0xed; e[1]=0x51; break;
          case Z80_E: e[0]=0xed; e[1]=0x59; break;
          case Z80_H: e[0]=0xed; e[1]=0x61; break;
          case Z80_L: e[0]=0xed; e[1]=0x69; break;
          default:
            if (!z80getbyte(lp)) e[0]=0xed; e[1]=0x71; break;
        }
    } 
    else {
      e[1]=z80getbyte(lp);
      if (cparen(lp))
        if (comma(lp))
          if (getz80reg(lp)==Z80_A) e[0]=0xd3;
    }
  }
  EmitBytes(e);
}

void pizOUTD() {
  int e[3];
  e[0]=0xed;
  e[1]=0xab;
  e[2]=-1;
  EmitBytes(e);
}

void pizOUTI() {
  int e[3];
  e[0]=0xed;
  e[1]=0xa3;
  e[2]=-1;
  EmitBytes(e);
}

void pizPOPoriginal() {
  int e[30],t=29,c=1;
  e[t]=-1;
  do {
    switch (getz80reg(lp)) {
    case Z80_AF: e[--t]=0xf1; break;
    case Z80_BC: e[--t]=0xc1; break;
    case Z80_DE: e[--t]=0xd1; break;
    case Z80_HL: e[--t]=0xe1; break;
    case Z80_IX: e[--t]=0xe1; e[--t]=0xdd; break;
    case Z80_IY: e[--t]=0xe1; e[--t]=0xfd; break;
    default: c=0; break;
    }
    if (!comma(lp) || t<2) c=0;
  } while (c);
  EmitBytes(&e[t]);
}

void pizPOPreversed() {
	int e[30], t = 0, c = 1;
	do {
		switch (getz80reg(lp)) {
		case Z80_AF: e[t++] = 0xf1; break;
		case Z80_BC: e[t++] = 0xc1; break;
		case Z80_DE: e[t++] = 0xd1; break;
		case Z80_HL: e[t++] = 0xe1; break;
		case Z80_IX: e[t++] = 0xdd; e[t++] = 0xe1; break;
		case Z80_IY: e[t++] = 0xfd; e[t++] = 0xe1; break;
		default: c = 0; break;
		}
		if (!comma(lp) || t>27) c = 0;
	} while (c);
	e[t] = -1;
	EmitBytes(e);
}

void pizPOP() {
	if(compassCompatibilityEnabled)
		pizPOPreversed();
	else
		pizPOPoriginal();
}

void pizPUSH() {
  int e[30],t=0,c=1;
  do {
    switch (getz80reg(lp)) {
    case Z80_AF: e[t++]=0xf5; break;
    case Z80_BC: e[t++]=0xc5; break;
    case Z80_DE: e[t++]=0xd5; break;
    case Z80_HL: e[t++]=0xe5; break;
    case Z80_IX: e[t++]=0xdd; e[t++]=0xe5; break;
    case Z80_IY: e[t++]=0xfd; e[t++]=0xe5; break;
    default: c=0; break;
    }
    if (!comma(lp) || t>27) c=0;
  } while (c);
  e[t]=-1;
  EmitBytes(e);
}

void pizRES() {
  Z80Reg reg;
  int e[5],bit;
  e[0]=e[1]=e[2]=e[3]=e[4]=-1;
  bit=z80getbyte(lp);
  if (!comma(lp)) bit=-1;
  switch (reg=getz80reg(lp)) {
  case Z80_B: case Z80_C: case Z80_D: case Z80_E:
  case Z80_H: case Z80_L: case Z80_A:
    e[0]=0xcb; e[1]=8*bit+0x80+reg ; break;
  default:
    if (!oparen(lp,'[') && !oparen(lp,'(')) break;
      switch (reg=getz80reg(lp)) {
      case Z80_HL:
        if (cparen(lp)) e[0]=0xcb;
        e[1]=8*bit+0x86; break;
      case Z80_IX: case Z80_IY:
        e[1]=0xcb; e[2]=z80getidxoffset(lp); e[3]=8*bit+0x86;
        if (cparen(lp)) e[0]=reg;
        if (comma(lp)) {
          switch(reg=getz80reg(lp)) {
          case Z80_B: case Z80_C: case Z80_D: case Z80_E:
          case Z80_H: case Z80_L: case Z80_A:
            e[3]=8*bit+0x80+reg;
            break;
          default: error("Illegal operand",lp,SUPPRES);
          }
        }
        break;
      default: ;
      }
  }
  if (bit<0 || bit>7) e[0]=-1;
  EmitBytes(e);
}

void pizRET() {
  int e;
  switch (getz80cond(lp)) {
  case Z80C_C: e=0xd8; break;
  case Z80C_M: e=0xf8; break;
  case Z80C_NC: e=0xd0; break;
  case Z80C_NZ: e=0xc0; break;
  case Z80C_P: e=0xf0; break;
  case Z80C_PE: e=0xe8; break;
  case Z80C_PO: e=0xe0; break;
  case Z80C_Z: e=0xc8; break;
  default: e=0xc9; break;
  }
  EmitByte(e);
}

void pizRETI() {
  int e[3];
  e[0]=0xed;
  e[1]=0x4d;
  e[2]=-1;
  EmitBytes(e);
}

void pizRETN() {
  int e[3];
  e[0]=0xed;
  e[1]=0x45;
  e[2]=-1;
  EmitBytes(e);
}

void pizRL() {
  Z80Reg reg;
  int e[5];
  e[0]=e[1]=e[2]=e[3]=e[4]=-1;
  switch (reg=getz80reg(lp)) {
  case Z80_B: case Z80_C: case Z80_D: case Z80_E:
  case Z80_H: case Z80_L: case Z80_A:
    e[0]=0xcb; e[1]=0x10+reg ; break;
  case Z80_BC:
    e[0]=e[2]=0xcb; e[1]=0x11; e[3]=0x10; break;
  case Z80_DE:
    e[0]=e[2]=0xcb; e[1]=0x13; e[3]=0x12; break;
  case Z80_HL:
    e[0]=e[2]=0xcb; e[1]=0x15; e[3]=0x14; break;
  default:
    if (!oparen(lp,'[') && !oparen(lp,'(')) break;
    switch (reg=getz80reg(lp)) {
    case Z80_HL:
      if (cparen(lp)) e[0]=0xcb; 
      e[1]=0x16; break;
    case Z80_IX: case Z80_IY:
      e[1]=0xcb; e[2]=z80getidxoffset(lp); e[3]=0x16;
      if (cparen(lp)) e[0]=reg;
      if (comma(lp)) {
        switch(reg=getz80reg(lp)) {
        case Z80_B: case Z80_C: case Z80_D: case Z80_E:
        case Z80_H: case Z80_L: case Z80_A:
          e[3]=0x10+reg;
          break;
        default: error("Illegal operand",lp,SUPPRES);
        }
      }
      break;
    default: ;
    }
  }
  EmitBytes(e);
}

void pizRLA() {
  EmitByte(0x17);
}

void pizRLC() {
  Z80Reg reg;
  int e[5];
  e[0]=e[1]=e[2]=e[3]=e[4]=-1;
  switch (reg=getz80reg(lp)) {
  case Z80_B: case Z80_C: case Z80_D: case Z80_E:
  case Z80_H: case Z80_L: case Z80_A:
      e[0]=0xcb; e[1]=0x0+reg ; break;
  default:
    if (!oparen(lp,'[') && !oparen(lp,'(')) break;
    switch (reg=getz80reg(lp)) {
    case Z80_HL:
      if (cparen(lp)) e[0]=0xcb; 
      e[1]=0x6; break;
    case Z80_IX: case Z80_IY:
      e[1]=0xcb; e[2]=z80getidxoffset(lp); e[3]=0x6;
      if (cparen(lp)) e[0]=reg;
      if (comma(lp)) {
        switch(reg=getz80reg(lp)) {
        case Z80_B: case Z80_C: case Z80_D: case Z80_E:
        case Z80_H: case Z80_L: case Z80_A:
          e[3]=reg;
          break;
        default: error("Illegal operand",lp,SUPPRES);
        }
      }
      break;
    default: ;
    }
  }
  EmitBytes(e);
}

void pizRLCA() {
  EmitByte(0x7);
}

void pizRLD() {
  int e[3];
  e[0]=0xed;
  e[1]=0x6f;
  e[2]=-1;
  EmitBytes(e);
}

void pizRR() {
  Z80Reg reg;
  int e[5];
  e[0]=e[1]=e[2]=e[3]=e[4]=-1;
  switch (reg=getz80reg(lp)) {
  case Z80_B: case Z80_C: case Z80_D: case Z80_E:
  case Z80_H: case Z80_L: case Z80_A:
    e[0]=0xcb; e[1]=0x18+reg ; break;
  case Z80_BC:
    e[0]=e[2]=0xcb; e[1]=0x18; e[3]=0x19; break;
  case Z80_DE:
    e[0]=e[2]=0xcb; e[1]=0x1a; e[3]=0x1b; break;
  case Z80_HL:
    e[0]=e[2]=0xcb; e[1]=0x1c; e[3]=0x1d; break;
  default:
    if (!oparen(lp,'[') && !oparen(lp,'(')) break;
    switch (reg=getz80reg(lp)) {
    case Z80_HL:
      if (cparen(lp)) e[0]=0xcb; 
      e[1]=0x1e; break;
    case Z80_IX: case Z80_IY:
      e[1]=0xcb; e[2]=z80getidxoffset(lp); e[3]=0x1e;
      if (cparen(lp)) e[0]=reg;
      if (comma(lp)) {
        switch(reg=getz80reg(lp)) {
        case Z80_B: case Z80_C: case Z80_D: case Z80_E:
        case Z80_H: case Z80_L: case Z80_A:
          e[3]=0x18+reg;
          break;
        default: error("Illegal operand",lp,SUPPRES);
        }
      }
      break;
    default: ;
    }
  }
  EmitBytes(e);
}

void pizRRA() {
  EmitByte(0x1f);
}

void pizRRC() {
  Z80Reg reg;
  int e[5];
  e[0]=e[1]=e[2]=e[3]=e[4]=-1;
  switch (reg=getz80reg(lp)) {
  case Z80_B: case Z80_C: case Z80_D: case Z80_E:
  case Z80_H: case Z80_L: case Z80_A:
      e[0]=0xcb; e[1]=0x8+reg ; break;
  default:
    if (!oparen(lp,'[') && !oparen(lp,'(')) break;
    switch (reg=getz80reg(lp)) {
    case Z80_HL:
      if (cparen(lp)) e[0]=0xcb; 
      e[1]=0xe; break;
    case Z80_IX: case Z80_IY:
      e[1]=0xcb; e[2]=z80getidxoffset(lp); e[3]=0xe;
      if (cparen(lp)) e[0]=reg;
      if (comma(lp)) {
        switch(reg=getz80reg(lp)) {
        case Z80_B: case Z80_C: case Z80_D: case Z80_E:
        case Z80_H: case Z80_L: case Z80_A:
          e[3]=0x8+reg;
          break;
        default: error("Illegal operand",lp,SUPPRES);
        }
      }
      break;
    default: ;
    }
  }
  EmitBytes(e);
}

void pizRRCA() {
  EmitByte(0xf);
}

void pizRRD() {
  int e[3];
  e[0]=0xed;
  e[1]=0x67;
  e[2]=-1;
  EmitBytes(e);
}

void pizRST() {
  int e;
  switch(z80getbyte(lp)) {
  case 0x00: e=0xc7; break;
  case 0x08: e=0xcf; break;
  case 0x10: e=0xd7; break;
  case 0x18: e=0xdf; break;
  case 0x20: e=0xe7; break;
  case 0x28: e=0xef; break;
  case 0x30: e=0xf7; break;
  case 0x38: e=0xff; break;
  default: error("Illegal operand",line); *lp=0; return;
  }
  EmitByte(e);
}

void pizSBC() {
  Z80Reg reg;
  int e[4];
  e[0]=e[1]=e[2]=e[3]=-1;
  switch (reg=getz80reg(lp)) {
  case Z80_HL:
    if (!comma(lp)) { error("Comma expected",0); break; }
    switch (getz80reg(lp)) {
    case Z80_BC: e[0]=0xed; e[1]=0x42; break;
    case Z80_DE: e[0]=0xed; e[1]=0x52; break;
    case Z80_HL: e[0]=0xed; e[1]=0x62; break;
    case Z80_SP: e[0]=0xed; e[1]=0x72; break;
    default: ;
    }
    break;
  case Z80_A:
    if (!comma(lp)) { e[0]=0x9f; break; }
    reg=getz80reg(lp);
  default:
    switch (reg) {
    case Z80_IXH: e[0]=0xdd; e[1]=0x9c; break;
    case Z80_IXL: e[0]=0xdd; e[1]=0x9d; break;
    case Z80_IYH: e[0]=0xfd; e[1]=0x9c; break;
    case Z80_IYL: e[0]=0xfd; e[1]=0x9d; break;
    case Z80_B: case Z80_C: case Z80_D: case Z80_E:
    case Z80_H: case Z80_L: case Z80_A: e[0]=0x98+reg; break;
    case Z80_F: case Z80_I: case Z80_R:
    case Z80_AF: case Z80_BC: case Z80_DE: case Z80_HL: case Z80_SP:
    case Z80_IX: case Z80_IY:
      break;
    default:
      reg=Z80_UNK;
      if (oparen(lp,'[')) { 
        if ((reg=getz80reg(lp))==Z80_UNK) break;
      } else if (oparen(lp,'(')) {
        if ((reg=getz80reg(lp))==Z80_UNK) --lp;
      }
      switch (reg) {
      case Z80_HL:
        if (cparen(lp)) e[0]=0x9e;
        break;
      case Z80_IX: case Z80_IY:
        e[1]=0x9e; e[2]=z80getidxoffset(lp);
        if (cparen(lp)) e[0]=reg;
        break;
      default: e[0]=0xde; e[1]=z80getbyte(lp); break;
      }
    }
  }
  EmitBytes(e);
}

void pizSCF() {
  EmitByte(0x37);
}

void pizSET() {
  Z80Reg reg;
  int e[5],bit;
  e[0]=e[1]=e[2]=e[3]=e[4]=-1;
  bit=z80getbyte(lp);
  if (!comma(lp)) bit=-1;
  switch (reg=getz80reg(lp)) {
  case Z80_B: case Z80_C: case Z80_D: case Z80_E:
  case Z80_H: case Z80_L: case Z80_A:
    e[0]=0xcb; e[1]=8*bit+0xc0+reg ; break;
  default:
    if (!oparen(lp,'[') && !oparen(lp,'(')) break;
      switch (reg=getz80reg(lp)) {
      case Z80_HL:
        if (cparen(lp)) e[0]=0xcb;
        e[1]=8*bit+0xc6; break;
      case Z80_IX: case Z80_IY:
        e[1]=0xcb; e[2]=z80getidxoffset(lp); e[3]=8*bit+0xc6;
        if (cparen(lp)) e[0]=reg;
        if (comma(lp)) {
          switch(reg=getz80reg(lp)) {
          case Z80_B: case Z80_C: case Z80_D: case Z80_E:
          case Z80_H: case Z80_L: case Z80_A:
            e[3]=8*bit+0xc0+reg;
            break;
          default: error("Illegal operand",lp,SUPPRES);
          }
        }
        break;
      default: ;
      }
  }
  if (bit<0 || bit>7) e[0]=-1;
  EmitBytes(e);
}

void pizSLA() {
  Z80Reg reg;
  int e[5];
  e[0]=e[1]=e[2]=e[3]=e[4]=-1;
  switch (reg=getz80reg(lp)) {
  case Z80_B: case Z80_C: case Z80_D: case Z80_E:
  case Z80_H: case Z80_L: case Z80_A:
    e[0]=0xcb; e[1]=0x20+reg ; break;
  case Z80_BC:
    e[0]=e[2]=0xcb; e[1]=0x21; e[3]=0x10; break;
  case Z80_DE:
    e[0]=e[2]=0xcb; e[1]=0x23; e[3]=0x12; break;
  case Z80_HL:
    e[0]=0x29; break;
  default:
    if (!oparen(lp,'[') && !oparen(lp,'(')) break;
    switch (reg=getz80reg(lp)) {
    case Z80_HL:
      if (cparen(lp)) e[0]=0xcb; 
      e[1]=0x26; break;
    case Z80_IX: case Z80_IY:
      e[1]=0xcb; e[2]=z80getidxoffset(lp); e[3]=0x26;
      if (cparen(lp)) e[0]=reg;
      if (comma(lp)) {
        switch(reg=getz80reg(lp)) {
        case Z80_B: case Z80_C: case Z80_D: case Z80_E:
        case Z80_H: case Z80_L: case Z80_A:
          e[3]=0x20+reg;
          break;
        default: error("Illegal operand",lp,SUPPRES);
        }
      }
      break;
    default: ;
    }
  }
  EmitBytes(e);
}

void pizSLL() {
  Z80Reg reg;
  int e[5];
  e[0]=e[1]=e[2]=e[3]=e[4]=-1;
  switch (reg=getz80reg(lp)) {
  case Z80_B: case Z80_C: case Z80_D: case Z80_E:
  case Z80_H: case Z80_L: case Z80_A:
    e[0]=0xcb; e[1]=0x30+reg ; break;
  case Z80_BC:
    e[0]=e[2]=0xcb; e[1]=0x31; e[3]=0x10; break;
  case Z80_DE:
    e[0]=e[2]=0xcb; e[1]=0x33; e[3]=0x12; break;
  case Z80_HL:
    e[0]=e[2]=0xcb; e[1]=0x35; e[3]=0x14; break;
  default:
    if (!oparen(lp,'[') && !oparen(lp,'(')) break;
    switch (reg=getz80reg(lp)) {
    case Z80_HL:
      if (cparen(lp)) e[0]=0xcb; 
      e[1]=0x36; break;
    case Z80_IX: case Z80_IY:
      e[1]=0xcb; e[2]=z80getidxoffset(lp); e[3]=0x36;
      if (cparen(lp)) e[0]=reg;
      if (comma(lp)) {
        switch(reg=getz80reg(lp)) {
        case Z80_B: case Z80_C: case Z80_D: case Z80_E:
        case Z80_H: case Z80_L: case Z80_A:
          e[3]=0x30+reg;
          break;
        default: error("Illegal operand",lp,SUPPRES);
        }
      }
      break;
    default: ;
    }
  }
  EmitBytes(e);
}

void pizSRA() {
  Z80Reg reg;
  int e[5];
  e[0]=e[1]=e[2]=e[3]=e[4]=-1;
  switch (reg=getz80reg(lp)) {
  case Z80_B: case Z80_C: case Z80_D: case Z80_E:
  case Z80_H: case Z80_L: case Z80_A:
    e[0]=0xcb; e[1]=0x28+reg ; break;
  case Z80_BC:
    e[0]=e[2]=0xcb; e[1]=0x28; e[3]=0x19; break;
  case Z80_DE:
    e[0]=e[2]=0xcb; e[1]=0x2a; e[3]=0x1b; break;
  case Z80_HL:
    e[0]=e[2]=0xcb; e[1]=0x2c; e[3]=0x1d; break;
  default:
    if (!oparen(lp,'[') && !oparen(lp,'(')) break;
    switch (reg=getz80reg(lp)) {
    case Z80_HL:
      if (cparen(lp)) e[0]=0xcb; 
      e[1]=0x2e; break;
    case Z80_IX: case Z80_IY:
      e[1]=0xcb; e[2]=z80getidxoffset(lp); e[3]=0x2e;
      if (cparen(lp)) e[0]=reg;
      if (comma(lp)) {
        switch(reg=getz80reg(lp)) {
        case Z80_B: case Z80_C: case Z80_D: case Z80_E:
        case Z80_H: case Z80_L: case Z80_A:
          e[3]=0x28+reg;
          break;
        default: error("Illegal operand",lp,SUPPRES);
        }
      }
      break;
    default: ;
    }
  }
  EmitBytes(e);
}

void pizSRL() {
  Z80Reg reg;
  int e[5];
  e[0]=e[1]=e[2]=e[3]=e[4]=-1;
  switch (reg=getz80reg(lp)) {
  case Z80_B: case Z80_C: case Z80_D: case Z80_E:
  case Z80_H: case Z80_L: case Z80_A:
    e[0]=0xcb; e[1]=0x38+reg ; break;
  case Z80_BC:
    e[0]=e[2]=0xcb; e[1]=0x38; e[3]=0x19; break;
  case Z80_DE:
    e[0]=e[2]=0xcb; e[1]=0x3a; e[3]=0x1b; break;
  case Z80_HL:
    e[0]=e[2]=0xcb; e[1]=0x3c; e[3]=0x1d; break;
  default:
    if (!oparen(lp,'[') && !oparen(lp,'(')) break;
    switch (reg=getz80reg(lp)) {
    case Z80_HL:
      if (cparen(lp)) e[0]=0xcb; 
      e[1]=0x3e; break;
    case Z80_IX: case Z80_IY:
      e[1]=0xcb; e[2]=z80getidxoffset(lp); e[3]=0x3e;
      if (cparen(lp)) e[0]=reg;
      if (comma(lp)) {
        switch(reg=getz80reg(lp)) {
        case Z80_B: case Z80_C: case Z80_D: case Z80_E:
        case Z80_H: case Z80_L: case Z80_A:
          e[3]=0x38+reg;
          break;
        default: error("Illegal operand",lp,SUPPRES);
        }
      }
      break;
    default: ;
    }
  }
  EmitBytes(e);
}

void pizSUB() {
  Z80Reg reg;
  int e[4];
  e[0]=e[1]=e[2]=e[3]=-1;
  switch (reg=getz80reg(lp)) {
  case Z80_HL:
    if (!needcomma(lp)) break;
    switch (getz80reg(lp)) {
      case Z80_BC: e[0]=0xb7; e[1]=0xed; e[2]=0x42; break;
      case Z80_DE: e[0]=0xb7; e[1]=0xed; e[2]=0x52; break;
      case Z80_HL: e[0]=0xb7; e[1]=0xed; e[2]=0x62; break;
      case Z80_SP: e[0]=0xb7; e[1]=0xed; e[2]=0x72; break;
    }
    break;
  case Z80_A:
    if (!comma(lp)) { e[0]=0x97; break; }
    reg=getz80reg(lp);
  default:
    switch (reg) {
    case Z80_IXH: e[0]=0xdd; e[1]=0x94; break;
    case Z80_IXL: e[0]=0xdd; e[1]=0x95; break;
    case Z80_IYH: e[0]=0xfd; e[1]=0x94; break;
    case Z80_IYL: e[0]=0xfd; e[1]=0x95; break;
    case Z80_B: case Z80_C: case Z80_D: case Z80_E:
    case Z80_H: case Z80_L: case Z80_A: e[0]=0x90+reg; break;
    case Z80_F: case Z80_I: case Z80_R:
    case Z80_AF: case Z80_BC: case Z80_DE: case Z80_HL: case Z80_SP:
    case Z80_IX: case Z80_IY:
      break;
    default:
      reg=Z80_UNK;
      if (oparen(lp,'[')) { 
        if ((reg=getz80reg(lp))==Z80_UNK) break;
      } else if (oparen(lp,'(')) {
        if ((reg=getz80reg(lp))==Z80_UNK) --lp;
      }
      switch (reg) {
      case Z80_HL:
        if (cparen(lp)) e[0]=0x96;
        break;
      case Z80_IX: case Z80_IY:
        e[1]=0x96; e[2]=z80getidxoffset(lp);
        if (cparen(lp)) e[0]=reg;
        break;
      default: e[0]=0xd6; e[1]=z80getbyte(lp); break;
      }
    }
  }
  EmitBytes(e);
}

void pizXOR() {
  Z80Reg reg;
  int e[4];
  e[0]=e[1]=e[2]=e[3]=-1;
  switch (reg=getz80reg(lp)) {
  case Z80_A:
    if (!comma(lp)) { e[0]=0xaf; break; }
    reg=getz80reg(lp);
  default:
    switch (reg) {
    case Z80_IXH: e[0]=0xdd; e[1]=0xac; break;
    case Z80_IXL: e[0]=0xdd; e[1]=0xad; break;
    case Z80_IYH: e[0]=0xfd; e[1]=0xac; break;
    case Z80_IYL: e[0]=0xfd; e[1]=0xad; break;
    case Z80_B: case Z80_C: case Z80_D: case Z80_E:
    case Z80_H: case Z80_L: case Z80_A: e[0]=0xa8+reg; break;
    case Z80_F: case Z80_I: case Z80_R:
    case Z80_AF: case Z80_BC: case Z80_DE: case Z80_HL: case Z80_SP:
    case Z80_IX: case Z80_IY:
      break;
    default:
      reg=Z80_UNK;
      if (oparen(lp,'[')) { 
        if ((reg=getz80reg(lp))==Z80_UNK) break;
      } else if (oparen(lp,'(')) {
        if ((reg=getz80reg(lp))==Z80_UNK) --lp;
      }
      switch (reg) {
      case Z80_HL:
        if (cparen(lp)) e[0]=0xae;
        break;
      case Z80_IX: case Z80_IY:
        e[1]=0xae; e[2]=z80getidxoffset(lp);
        if (cparen(lp)) e[0]=reg;
        break;
      default: e[0]=0xee; e[1]=z80getbyte(lp); break;
      }
    }
  }
  EmitBytes(e);
}

void InitpiZ80() {
  z80funtab.insert("adc",pizADC);
  z80funtab.insert("add",pizADD);
  z80funtab.insert("and",pizAND);
  z80funtab.insert("bit",pizBIT);
  z80funtab.insert("call",pizCALL);
  z80funtab.insert("ccf",pizCCF);
  z80funtab.insert("cp",pizCP);
  z80funtab.insert("cpd",pizCPD);
  z80funtab.insert("cpdr",pizCPDR);
  z80funtab.insert("cpi",pizCPI);
  z80funtab.insert("cpir",pizCPIR);
  z80funtab.insert("cpl",pizCPL);
  z80funtab.insert("daa",pizDAA);
  z80funtab.insert("dec",pizDEC);
  z80funtab.insert("di",pizDI);
  z80funtab.insert("djnz",pizDJNZ);
  z80funtab.insert("ei",pizEI);
  z80funtab.insert("ex",pizEX);
  z80funtab.insert("exx",pizEXX);
  z80funtab.insert("halt",pizHALT);
  z80funtab.insert("im",pizIM);
  z80funtab.insert("in",pizIN);
  z80funtab.insert("inc",pizINC);
  z80funtab.insert("ind",pizIND);
  z80funtab.insert("indr",pizINDR);
  z80funtab.insert("ini",pizINI);
  z80funtab.insert("inir",pizINIR);
  z80funtab.insert("jp",pizJP);
  z80funtab.insert("jr",pizJR);
  z80funtab.insert("ld",pizLD);
  z80funtab.insert("ldd",pizLDD);
  z80funtab.insert("lddr",pizLDDR);
  z80funtab.insert("ldi",pizLDI);
  z80funtab.insert("ldir",pizLDIR);
  z80funtab.insert("mulub",pizMULUB);
  z80funtab.insert("muluw",pizMULUW);
  z80funtab.insert("neg",pizNEG);
  z80funtab.insert("nop",pizNOP);
  z80funtab.insert("or",pizOR);
  z80funtab.insert("otdr",pizOTDR);
  z80funtab.insert("otir",pizOTIR);
  z80funtab.insert("out",pizOUT);
  z80funtab.insert("outd",pizOUTD);
  z80funtab.insert("outi",pizOUTI);
  z80funtab.insert("pop",pizPOP);
  z80funtab.insert("push",pizPUSH);
  z80funtab.insert("res",pizRES);
  z80funtab.insert("ret",pizRET);
  z80funtab.insert("reti",pizRETI);
  z80funtab.insert("retn",pizRETN);
  z80funtab.insert("rl",pizRL);
  z80funtab.insert("rla",pizRLA);
  z80funtab.insert("rlc",pizRLC);
  z80funtab.insert("rlca",pizRLCA);
  z80funtab.insert("rld",pizRLD);
  z80funtab.insert("rr",pizRR);
  z80funtab.insert("rra",pizRRA);
  z80funtab.insert("rrc",pizRRC);
  z80funtab.insert("rrca",pizRRCA);
  z80funtab.insert("rrd",pizRRD);
  z80funtab.insert("rst",pizRST);
  z80funtab.insert("sbc",pizSBC);
  z80funtab.insert("scf",pizSCF);
  z80funtab.insert("set",pizSET);
  z80funtab.insert("sla",pizSLA);
  z80funtab.insert("sli",pizSLL);
  z80funtab.insert("sll",pizSLL);
  z80funtab.insert("sra",pizSRA);
  z80funtab.insert("srl",pizSRL);
  z80funtab.insert("sub",pizSUB);
  z80funtab.insert("xor",pizXOR);
}

void Initpi() {
  InitpiZ80();
#ifdef METARM
  InitpiARM();
  InitpiTHUMB();
#endif
  InsertDirectives();
}
//eof piz80.cpp
