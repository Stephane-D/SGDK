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

// direct.cpp

#include "sjasm.h"

funtabcls dirtab;

int ParseDirective() {
  char *olp=lp;
  char *n;
  bp=lp;
  if (!(n=getinstr(lp))) 
    if (*lp=='#' && *(lp+1)=='#') {
      lp+=2;
      aint val;
      synerr=0; if (!ParseExpression(lp,val)) val=4; synerr=1;
      mapadr+=((~mapadr+1)&(val-1));
      return 1;
    } 
    else { lp=olp;  return 0; }
  if (dirtab.zoek(n)) { return 1; }
  lp=olp;
  return 0;
}

#ifdef SECTIONS
void dirPOOL() {
  sections osection=section;
  section=POOL;
  pooltab.emit();
  section=osection;
}

void dirTEXT() {
  section=TEXT;
}

void dirDATA() {
  section=DATA;
}
#endif

void dirBYTE() {
  int teller,e[129];
  teller=getBytes(lp,e,0,0);
  if (!teller) { error(".byte with no arguments",0); return; }
#ifdef SECTIONS
  switch (section) {
  case TEXT: case POOL: EmitBytes(e); break;
  case DATA: pooltab.add(bp); break;
  default: error ("Unknown section",0,FATAL); break;
  }
#else
  EmitBytes(e);
#endif
}

void dirDC() {
  int teller,e[129];
  teller=getBytes(lp,e,0,1);
  if (!teller) { error(".byte with no arguments",0); return; }
#ifdef SECTIONS
  switch (section) {
  case TEXT: case POOL: EmitBytes(e); break;
  case DATA: pooltab.add(bp); break;
  default: error ("Unknown section",0,FATAL); break;
  }
#else
  EmitBytes(e);
#endif
}

void dirDZ() {
  int teller,e[130];
  teller=getBytes(lp,e,0,0);
  if (!teller) { error(".byte with no arguments",0); return; }
  e[teller++]=0; e[teller]=-1;
#ifdef SECTIONS
  switch (section) {
  case TEXT: case POOL: EmitBytes(e); break;
  case DATA: pooltab.add(bp); break;
  default: error ("Unknown section",0,FATAL); break;
  }
#else
  EmitBytes(e);
#endif
}

void dirABYTE() {
  aint add;
  int teller=0,e[129];
  if (ParseExpression(lp,add)) {
    check8(add); add&=255;
    teller=getBytes(lp,e,add,0);
    if (!teller) { error(".abyte with no arguments",0); return; }
#ifdef SECTIONS
    switch (section) {
    case TEXT: case POOL: EmitBytes(e); break;
    case DATA: pooltab.add(bp); break;
    default: error ("Unknown section",0,FATAL); break;
    }
#else
    EmitBytes(e);
#endif
  } else error("Expression expected",0);
}

void dirABYTEC() {
  aint add;
  int teller=0,e[129];
  if (ParseExpression(lp,add)) {
    check8(add); add&=255;
    teller=getBytes(lp,e,add,1);
    if (!teller) { error(".abyte with no arguments",0); return; }
#ifdef SECTIONS
    switch (section) {
    case TEXT: case POOL: EmitBytes(e); break;
    case DATA: pooltab.add(bp); break;
    default: error ("Unknown section",0,FATAL); break;
    }
#else
    EmitBytes(e);
#endif
  } else error("Expression expected",0);
}

void dirABYTEZ() {
  aint add;
  int teller=0,e[129];
  if (ParseExpression(lp,add)) {
    check8(add); add&=255;
    teller=getBytes(lp,e,add,0);
    if (!teller) { error(".abyte with no arguments",0); return; }
    e[teller++]=0; e[teller]=-1;
#ifdef SECTIONS
    switch (section) {
    case TEXT: case POOL: EmitBytes(e); break;
    case DATA: pooltab.add(bp); break;
    default: error ("Unknown section",0,FATAL); break;
    }
#else
    EmitBytes(e);
#endif
  } else error("Expression expected",0);
}

void dirWORD() {
  aint val;
  int teller=0,e[129];
  skipblanks();
  while (*lp) {
    if (ParseExpression(lp,val)) {
      check16(val);
      if (teller>127) error("Over 128 values in .word",0,FATAL);
      e[teller++]=val & 65535;
    } else { error("Syntax error",lp,CATCHALL); return; }
    skipblanks();
    if (*lp!=',') break;
    ++lp; skipblanks();
  }
  e[teller]=-1;
  if (!teller) { error(".word with no arguments",0); return; }
#ifdef SECTIONS
  switch (section) {
  case TEXT: case POOL: EmitWords(e); break;
  case DATA: pooltab.add(bp); break;
  default: error ("Unknown section",0,FATAL); break;
  }
#else
  EmitWords(e);
#endif
}

void dirDWORD() {
  aint val;
  int teller=0,e[129*2];
  skipblanks();
  while (*lp) {
    if (ParseExpression(lp,val)) {
      if (teller>127) error("Over 128 values in .dword",0,FATAL);
      e[teller*2]=val & 65535; e[teller*2+1]=val >> 16; ++teller;
    } else { error("Syntax error",lp,CATCHALL); return; }
    skipblanks();
    if (*lp!=',') break;
    ++lp; skipblanks();
  }
  e[teller*2]=-1;
  if (!teller) { error(".dword with no arguments",0); return; }
#ifdef SECTIONS
  switch (section) {
  case TEXT: case POOL: EmitWords(e); break;
  case DATA: pooltab.add(bp); break;
  default: error ("Unknown section",0,FATAL); break;
  }
#else
  EmitWords(e);
#endif
}

void dirD24() {
  aint val;
  int teller=0,e[129*3];
  skipblanks();
  while (*lp) {
    if (ParseExpression(lp,val)) {
      check24(val);
      if (teller>127) error("Over 128 values in .d24",0,FATAL);
      e[teller*3]=val & 255; e[teller*3+1]=(val>>8)&255; e[teller*3+2]=(val>>16)&255; ++teller;
    } else { error("Syntax error",lp,CATCHALL); return; }
    skipblanks();
    if (*lp!=',') break;
    ++lp; skipblanks();
  }
  e[teller*3]=-1;
  if (!teller) { error(".d24 with no arguments",0); return; }
#ifdef SECTIONS
  switch (section) {
  case TEXT: case POOL: EmitBytes(e); break;
  case DATA: pooltab.add(bp); break;
  default: error ("Unknown section",0,FATAL); break;
  }
#else
  EmitBytes(e);
#endif
}

void dirBLOCK() {
  aint teller,val=0;
  if (ParseExpression(lp,teller)) {
    if ((signed)teller<0) { error("block with negative size",0); teller=0; }
    if (comma(lp)) ParseExpression(lp,val);
#ifdef SECTIONS
    switch (section) {
    case TEXT: case POOL: EmitBlock(val,teller); break;
    case DATA: pooltab.add(bp); break;
    default: error ("Unknown section",0,FATAL); break;
    }
#else
  EmitBlock(val,teller);
#endif
  } else error("Syntax Error",lp,CATCHALL);
}

void dirORG() {
  aint val;
#ifdef SECTIONS
  if (section!=TEXT) { error(".org only allowed in text sections",0); *lp=0; return; }
#endif
  if (ParseExpression(lp,val)) adres=val; else error("Syntax error",0,CATCHALL);
}

void dirMAP() {
#ifdef SECTIONS
  if (section!=TEXT) { error(".map only allowed in text sections",0); *lp=0; return; }
#endif
  maplstp=new adrlst(mapadr,maplstp);
  aint val;
  labelnotfound=0;
  if (ParseExpression(lp,val)) mapadr=val; else error("Syntax error",0,CATCHALL);
  if (labelnotfound) error("Forward reference",0,ALL);
}

void dirENDMAP() {
#ifdef SECTIONS
  if (section!=TEXT) { error(".endmap only allowed in text sections",0); *lp=0; return; }
#endif
  if (maplstp) { mapadr=maplstp->val; maplstp=maplstp->next; }
  else error(".endmodule without module",0);
}


void dirALIGN() {
  aint val;
  if (!ParseExpression(lp,val)) val=4;
  switch (val) {
  case 1: break;
  case 2: case 4: case 8: case 16: case 32: case 64: case 128: case 256:
  case 512: case 1024: case 2048: case 4096: case 8192: case 16384: case 32768:
    val=(~adres+1)&(val-1);
    EmitBlock(0,val);
    break;
  default:
    error("Illegal align",0); break;
  }
}

void dirMODULE() {
#ifdef SECTIONS
  if (section!=TEXT) { error(".module only allowed in text sections",0); *lp=0; return; }
#endif
  char *n;
  stringlst* old_modlstp;

  old_modlstp = modlstp;
  modlstp=new stringlst();
  modlstp->string=modlabp;
  modlstp->next=old_modlstp;

  skipblanks(lp); if (!*lp) { modlabp=0; return; }
  if (n=getid(lp)) modlabp=n; else error("Syntax error",0,CATCHALL);
}

void dirENDMODULE() {
#ifdef SECTIONS
  if (section!=TEXT) { error(".endmodule only allowed in text sections",0); *lp=0; return; }
#endif
  if (modlstp) { modlabp=modlstp->string; modlstp=modlstp->next; }
  else error(".endmodule without module",0);
}

void dirZ80() {
#ifdef SECTIONS
  dirPOOL();
  section=TEXT;
#endif
#ifdef METARM
  cpu=Z80;
#endif
  piCPUp=piZ80;
}

void dirARM() {
#ifdef METARM
#ifdef SECTIONS
  dirPOOL();
  section=TEXT;
#endif
  cpu=ARM; piCPUp=piARM;
#else
  error("No ARM support in this version",0,FATAL);
#endif
}

void dirTHUMB() {
#ifdef METARM
#ifdef SECTIONS
  dirPOOL();
  section=TEXT;
#endif
  cpu=THUMB; piCPUp=piTHUMB;
#else
  error("No ARM support in this version",0,FATAL);
#endif
}

void dirEND() {
#ifdef SECTIONS
  dirPOOL();
#endif
  running=0;
}

void dirSIZE() {
  aint val;
#ifdef SECTIONS
  if (section!=TEXT) error(".size is only allowed in text sections",0,FATAL);
#endif
  if (!ParseExpression(lp,val)) { error("Syntax error",bp,CATCHALL); return; }
  if (pass==2) return;
  if (size!=(aint)-1) { error("Multiple sizes?",0); return; }
  size=val;
}

void dirINCBIN() {
  aint val;
  char *fnaam;
  int offset=-1,length=-1;
#ifdef SECTIONS
  if (section!=TEXT) error(".include only allowed in text sections",0,FATAL);
#endif
  fnaam=getfilename(lp);
  if (comma(lp)) {
    if (!comma(lp)) { 
      if (!ParseExpression(lp,val)) { error("Syntax error",bp,CATCHALL); return; } 
      if (val<0) { error("Negative values are not allowed",bp); return; }
      offset=val;
    }
    if (comma(lp)) { 
      if (!ParseExpression(lp,val)) { error("Syntax error",bp,CATCHALL); return; } 
      if (val<0) { error("Negative values are not allowed",bp); return; }
      length=val;
    }
  }
  BinIncFile(fnaam,offset,length);
}

void dirTEXTAREA() {
#ifdef SECTIONS
  if (section!=TEXT) { error(".textarea only allowed in text sections",0); *lp=0; return; }
#endif
  aint oadres=adres,val;
  labelnotfound=0;
  if (!ParseExpression(lp,val)) { error("No adress given",0); return; }
  if (labelnotfound) error("Forward reference",0,ALL);
  ListFile();
  adres=val; if (ReadFile()!=ENDTEXTAREA) error("No end of textarea",0);
#ifdef SECTIONS
  dirPOOL();
#endif
  adres=oadres+adres-val;
}

void dirIFCOND(char* errorMessage) {
  aint val;
  labelnotfound=0;
  if (!ParseExpression(lp,val)) { error("Syntax error",0,CATCHALL); return; }
  if (labelnotfound) error("Forward reference",0,ALL);
  if (val) {
    ListFile();
    switch (ReadFile()) {
    case ELSE: if (SkipFile()!=ENDIF) error(errorMessage,0); break;
    case ENDIF: break;
    default: error(errorMessage,0); break;
    }
  } 
  else {
    ListFile();
    switch (SkipFile()) {
    case ELSE: if (ReadFile()!=ENDIF) error(errorMessage,0); break;
    case ENDIF: break;
    default: error(errorMessage,0); break;
    }
  }
  *lp=0;
}

void dirCOND() {
	dirIFCOND("No endc");
}

void dirIF() {
	dirIFCOND("No endif");
}

void dirELSE() {
  error("Else without if",0);
}

void dirENDC() {
	error("Endc without cond", 0);
}

void dirENDIF() {
  error("Endif without if",0);
}

void dirENDTEXTAREA() {
  error("Endt without textarea",0);
}

void dirNOOP() {
  while (*lp) lp++;
}

void dirINCLUDE() {
  char *fnaam;
#ifdef SECTIONS
  if (section!=TEXT) error(".include only allowed in text sections",0,FATAL);
#endif
  fnaam=getfilename(lp);
  ListFile(); OpenFile(fnaam); donotlist=1;
}

void dirOUTPUT() {
  char *fnaam;
#ifdef SECTIONS
  if (section!=TEXT) error(".output only allowed in text sections",0,FATAL);
#endif
  fnaam=getfilename(lp); if (fnaam[0]=='<') fnaam++;
  int mode=OUTPUT_TRUNCATE;
  if(comma(lp))
  {
    char modechar=(*lp) | 0x20;
    lp++;
    if(modechar=='t') mode=OUTPUT_TRUNCATE;
    else if(modechar=='r') mode=OUTPUT_REWIND;
    else if(modechar=='a') mode=OUTPUT_APPEND;
    else error("Syntax error",bp,CATCHALL);
  }
  if (pass==2) NewDest(fnaam,mode);
}

void dirDEFINE() {
  char *id;
  char *p=line;
#ifdef SECTIONS
  if (section!=TEXT) { error(".define only allowed in text sections",0); *lp=0; return; }
#endif
  while ('o') {
    if (!*p) error("define error",0,FATAL);
    if (*p=='.') { ++p; continue; }
    if (*p=='d' || *p=='D') break;
    ++p;
  }
  if (!cmphstr(p,"define")) error("define error",0,FATAL);
  if (!(id=getid(p))) { error("illegal define",0); return; }
  definetab.add(id,p);
  while (*lp) ++lp;
}

void dirIFDEF() {
  char *p=line,*id;
  while ('o') {
    if (!*p) error("ifdef error",0,FATAL);
    if (*p=='.') { ++p; continue; }
    if (*p=='i' || *p=='I') break;
    ++p;
  }
  if (!cmphstr(p,"ifdef")) error("ifdef error",0,FATAL);
  Ending res;
  if (!(id=getid(p))) { error("Illegal identifier",0,PASS1); return; }
  if (definetab.bestaat(id)) {
    ListFile();
    switch (res=ReadFile()) {
    case ELSE: if (SkipFile()!=ENDIF) error("No endif",0); break;
    case ENDIF: break;
    default: error("No endif!",0); break;
    }
  } 
  else {
    ListFile();
    switch (res=SkipFile()) {
    case ELSE: if (ReadFile()!=ENDIF) error("No endif",0); break;
    case ENDIF: break;
    default: error("No endif!",0); break;
    }
  }
  *lp=0;
}

void dirIFNDEF() {
  char *p=line,*id;
  while ('o') {
    if (!*p) error("ifndef error",0,FATAL);
    if (*p=='.') { ++p; continue; }
    if (*p=='i' || *p=='I') break;
    ++p;
  }
  if (!cmphstr(p,"ifndef")) error("ifndef error",0,FATAL);
  Ending res;
  if (!(id=getid(p))) { error("Illegal identifier",0,PASS1); return; }
  if (!definetab.bestaat(id)) {
    ListFile();
    switch (res=ReadFile()) {
    case ELSE: if (SkipFile()!=ENDIF) error("No endif",0); break;
    case ENDIF: break;
    default: error("No endif!",0); break;
    }
  } 
  else {
    ListFile();
    switch (res=SkipFile()) {
    case ELSE: if (ReadFile()!=ENDIF) error("No endif",0); break;
    case ENDIF: break;
    default: error("No endif!",0); break;
    }
  }
  *lp=0;
}

void dirEXPORT() {
  aint val;
  char *n,*p;
  if (pass==1) return;
  if (!(n=p=getid(lp))) { error("Syntax error",lp,CATCHALL); return; }
  labelnotfound=0;
    getLabelValue(n,val); if (labelnotfound) { error("Label not found",p,SUPPRES); return; }
    WriteExp(p,val);
}

void dirMACRO() {
#ifdef SECTIONS
  if (section!=TEXT) error("macro definitions only allowed in text sections",0,FATAL);
#endif

  if (lijst) error("No macro definitions allowed here",0,FATAL);
  char *n;
  if (!(n=getid(lp))) { error("Illegal macroname",0,PASS1); return; }
  macrotab.add(n,lp);
}

void dirENDM() {
  insideCompassStyleMacroDefinition = 0;
  error("End macro without macro",0);
}

void dirENDS() {
  error("End structre without structure",0);
}

void dirASSERT() {
  char *p=lp;
  aint val;
  if (!ParseExpression(lp,val)) { error("Syntax error",0,CATCHALL); return; }
  if (pass==2 && !val) error("Assertion failed",p);
  *lp=0;
}

void dirSTRUCT() {
#ifdef SECTIONS
  if (section!=TEXT) error("structure definitions only allowed in text sections",0,FATAL);
#endif
  structcls *st;
  int global=0;
  aint offset=0,bind=0;
  char *naam;
  skipblanks();
  if (*lp=='@') { ++lp; global=1; }
  if (!(naam=getid(lp))) { error("Illegal structurename",0,PASS1); return; }
  if (comma(lp)) {
    labelnotfound=0;
    if (!ParseExpression(lp,offset)) { error("Syntax error",0,CATCHALL); return; }
    if (labelnotfound) error("Forward reference",0,ALL);
  }
  st=structtab.add(naam,offset,bind,global);
  ListFile();
  while ('o') {
    if (!ReadLine()) { error("Unexpected end of structure",0,PASS1); break; }
    lp=line; if (white()) { skipblanks(lp); if (*lp=='.') ++lp; if (cmphstr(lp,"ends")) break; }
    ParseStructLine(st);
    ListFileSkip(line);
  }
  st->deflab();
}

void dirFORG() {
  aint val;
  int method=SEEK_SET;
  skipblanks(lp);
  if((*lp=='+') || (*lp=='-')) method=SEEK_CUR;
  if (!ParseExpression(lp,val)) error("Syntax error",0,CATCHALL);
  if (pass==2) SeekDest(val,method);
}

/*
void dirBIND() {
}
*/

void dirREPT() {
  aint val;
  char *ml;
  int olistmacro;
  stringlst *s,*f;
  labelnotfound=0;
  if (!ParseExpression(lp,val)) { error("Syntax error",0,CATCHALL); return; }
  if (labelnotfound) error("Forward reference",0,ALL);
  if ((int)val<0) { error("Illegal repeat value",0,CATCHALL); return; }
  ListFile();
  if (!ReadFileToStringLst(f,"endm")) error("Unexpected end of repeat",0,PASS1);
  insideCompassStyleMacroDefinition = 0;
  ListFile();
  olistmacro=listmacro; listmacro=1; ml=strdup(line);
  while (val--) { s=f; while (s) { strcpy(line,s->string); s=s->next; ParseLine(); } }
  strcpy(line,ml); listmacro=olistmacro; donotlist=1;
}

void InsertDirectives() {
  dirtab.insertd("assert",dirASSERT);
  dirtab.insertd("byte",dirBYTE);
  dirtab.insertd("abyte",dirABYTE);
  dirtab.insertd("abytec",dirABYTEC);
  dirtab.insertd("abytez",dirABYTEZ);
  dirtab.insertd("word",dirWORD);
  dirtab.insertd("block",dirBLOCK);
  dirtab.insertd("dword",dirDWORD);
  dirtab.insertd("d24",dirD24);
  dirtab.insertd("org",dirORG);
  dirtab.insertd("map",dirMAP);
  dirtab.insertd("align",dirALIGN);
  dirtab.insertd("module",dirMODULE);
  dirtab.insertd("z80",dirZ80);
  dirtab.insertd("arm",dirARM);
  dirtab.insertd("thumb",dirTHUMB);
  dirtab.insertd("size",dirSIZE);
  dirtab.insertd("textarea",dirTEXTAREA);
  dirtab.insertd("phase",dirTEXTAREA);
  dirtab.insertd("msx",dirZ80);
  dirtab.insertd("else",dirELSE);
  dirtab.insertd("export",dirEXPORT);
  dirtab.insertd("end",dirEND);
  dirtab.insertd("include",dirINCLUDE);
  dirtab.insertd("incbin",dirINCBIN);
  dirtab.insertd("if",dirIF);
  dirtab.insertd("output",dirOUTPUT);
  dirtab.insertd("define",dirDEFINE);
  dirtab.insertd("ifdef",dirIFDEF);
  dirtab.insertd("ifndef",dirIFNDEF);
  dirtab.insertd("macro",dirMACRO);
  dirtab.insertd("struct",dirSTRUCT);
  dirtab.insertd("dc",dirDC);
  dirtab.insertd("dz",dirDZ);
  dirtab.insertd("db",dirBYTE);
  dirtab.insertd("dw",dirWORD);
  dirtab.insertd("ds",dirBLOCK);
  dirtab.insertd("dd",dirDWORD);
  dirtab.insertd("dm",dirBYTE);
  dirtab.insertd("defb",dirBYTE);
  dirtab.insertd("defw",dirWORD);
  dirtab.insertd("defs",dirBLOCK);
  dirtab.insertd("defd",dirDWORD);
  dirtab.insertd("defm",dirBYTE);
  dirtab.insertd("endmod",dirENDMODULE);
  dirtab.insertd("endmodule",dirENDMODULE);
  dirtab.insertd("endmap",dirENDMAP);
  dirtab.insertd("rept",dirREPT);
  dirtab.insertd("fpos",dirFORG);
//  dirtab.insertd("bind",dirBIND);
  dirtab.insertd("endif",dirENDIF);
  dirtab.insertd("endt",dirENDTEXTAREA);
  dirtab.insertd("dephase",dirENDTEXTAREA);
  dirtab.insertd("endm",dirENDM);
  dirtab.insertd("ends",dirENDS);
#ifdef SECTIONS
  dirtab.insertd("code",dirTEXT);
  dirtab.insertd("data",dirDATA);
  dirtab.insertd("text",dirTEXT);
  dirtab.insertd("pool",dirPOOL);
#endif

  if (compassCompatibilityEnabled) {
	  dirtab.insertd("cond", dirCOND);
	  dirtab.insertd("endc", dirENDC);
      dirtab.insertd(".label", dirNOOP);
      dirtab.insertd(".upper", dirNOOP);
      dirtab.insertd("tsrhooks", dirNOOP);
      dirtab.insertd("breakp", dirNOOP);
  }
}
//eof direct.cpp
