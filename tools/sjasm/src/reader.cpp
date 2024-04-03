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

// reader.cpp

#include "sjasm.h"

int cmphstr(char *&p1, char *p2) {
  int i=0;
  if (isupper(*p1)) 
    while (p2[i]) { 
      if (p1[i]!=toupper(p2[i])) return 0; 
      ++i; 
    }
  else 
    while (p2[i]) { 
      if (p1[i]!=p2[i]) return 0; 
      ++i; 
    }
  if (p1[i]>' ') return 0;
  p1+=i;
  return 1;
}

int white() {
  return (*lp && *lp<=' ');
}

int skipblanks() {
  while(*lp && *lp<=' ') ++lp;
  return (*lp==0);
}

void skipblanks(char *&p) {
  while (*p && *p<=' ') ++p;
}

int needequ() {
  char *olp=lp;
  skipblanks();
  if (*lp=='=') { ++lp; return 1; }
  if (*lp=='.') ++lp;
  if (cmphstr(lp,"equ")) return 1;
  lp=olp;
  return 0;
}

int needfield() {
  char *olp=lp;
  skipblanks();
  if (*lp=='#') { ++lp; return 1; }
  if (*lp=='.') ++lp;
  if (cmphstr(lp,"field")) return 1;
  lp=olp;
  return 0;
}

int comma(char *&p) {
  skipblanks(p);
  if (*p!=',') return 0;
  ++p; return 1;
}

int cpc='4';

int oparen(char *&p, char c) {
  skipblanks(p);
  if (*p!=c) return 0;
  if (c=='[') cpc=']';
  if (c=='(') cpc=')';
  if (c=='{') cpc='}';
  ++p; return 1;
}

int cparen(char *&p) {
  skipblanks(p);
  if (*p!=cpc) return 0;
  ++p; return 1;
}

char *getparen(char *p) {
  int teller=0;
  skipblanks(p);
  while (*p) {
    if (*p=='(') ++teller;
    else if (*p==')') 
      if (teller==1) { skipblanks(++p); return p; } else --teller;
    ++p;
  }
  return 0;
}

char *getid(char *&p) {
  char nid[LINEMAX],*np;
  np=nid;
  skipblanks(p);
  if (!isalpha(*p) && *p!='_') return 0;
  while(*p) {
    if (IsNotValidIdChar(*p)) break;
    *np=*p; ++p; ++np;
  }
  *np=0;
  return strdup(nid);
}

char *getinstr(char *&p) {
  char nid[LINEMAX],*np;
  np=nid;
  skipblanks(p);
  if (!isalpha(*p) && *p!='.') return 0; else { *np=*p; ++p; ++np; }
  while(*p) {
    if (!isalnum(*p)) break;
    *np=*p; ++p; ++np;
  }
  *np=0;
  return strdup(nid);
}

int check8(unsigned aint val) {
  if (val!=(val&255) && ~val>127) { error("Bytes lost",0); return 0; }
  return 1;
}

int check8o(long val) {
  if (val<-128 || val>127) { error("Offset out of range",0); return 0; }
  return 1;
}

int check16(unsigned aint val) {
  if (val!=(val&65535) && ~val>32767) { error("Bytes lost",0); return 0; }
  return 1;
}

int check24(unsigned aint val) {
  if (val!=(val&16777215) && ~val>8388607) { error("Bytes lost",0); return 0; }
  return 1;
}

int need(char *&p, char c) {
  skipblanks(p);
  if (*p!=c) return 0;
  ++p; return 1;
}

int needa(char *&p, char *c1, int r1, char *c2, int r2, char *c3, int r3) {
//  skipblanks(p);
  if (!isalpha(*p)) return 0;
  if (cmphstr(p,c1)) return r1;
  if (c2 && cmphstr(p,c2)) return r2;
  if (c3 && cmphstr(p,c3)) return r3;
  return 0;
}

int need(char *&p, char *c) {
  skipblanks(p);
  while (*c) {
    if (*p!=*c) { c+=2; continue; }
    ++c;
    if (*c==' ') { ++p; return *(c-1); }
    if (*c=='_' && *(p+1)!=*(c-1)) { ++p; return *(c-1); }
    if (*(p+1)==*c) { p+=2; return *(c-1)+*c; }
    ++c;
  }
  return 0;
}

int getval(int p) {
  switch (p) {
  case '0': case '1': case '2': case '3': case '4':
  case '5': case '6': case '7': case '8': case '9':
    return p-'0';
  default:
    if (isupper(p)) return p-'A'+10;
    if (islower(p)) return p-'a'+10;
    return 200;
  }
}

int isValidSpaceInsideConstant(char c) {
	return compassCompatibilityEnabled && (c == ' ' || c == '\t');
}

int getConstant(char *&op, aint &val) {
  aint base,pb=1,v,oval;
  char *p=op,*p2,*p3;
  skipblanks(p); p3=p;
  val=0;
  char prefix;

  if (*p == '&') {
	  p++;
	  prefix = tolower(*p);
	  if(prefix != 'h' && prefix != 'b') {
		  error("Syntax error", op, CATCHALL); return 0;
	  }
	  if(prefix == 'h')
		  *p = '#';
	  else
		  *p = '%';
  }

  switch (*p) {
  case '#':
  case '$': 
    ++p;
    while (isalnum(*p) || isValidSpaceInsideConstant(*p)) {
		if (isValidSpaceInsideConstant(*p)) {
			p++;
			continue;
		}
      if ((v=getval(*p))>=16) { error("Digit not in base",op); return 0; }
      oval=val; val=val*16+v; ++p; if (oval>val) error("Overflow",0,SUPPRES);
    }
    if (p-p3<2) { error("Syntax error",op,CATCHALL); return 0; }
    op=p; return 1;
  case '%':
    ++p;
    while (isdigit(*p) || isValidSpaceInsideConstant(*p)) {
	  if (isValidSpaceInsideConstant(*p)) {
		p++;
		continue;
	  }
    if ((v=getval(*p))>=2) { error("Digit not in base",op); return 0; }
    oval=val; val=val*2+v; ++p; if (oval>val) error("Overflow",0,SUPPRES);
    }
    if (p-p3<2) { error("Syntax error",op,CATCHALL); return 0; }
    op=p; return 1;
  case '0':
    ++p;
    if (*p=='x' || *p=='X') {
      ++p;
      while (isalnum(*p) || isValidSpaceInsideConstant(*p)) {
		  if (isValidSpaceInsideConstant(*p)) {
			  p++;
			  continue;
		  }
        if ((v=getval(*p))>=16) { error("Digit not in base",op); return 0; }
        oval=val; val=val*16+v; ++p; if (oval>val) error("Overflow",0,SUPPRES);
      }
      if (p-p3<3) { error("Syntax error",op,CATCHALL); return 0; }
      op=p; return 1;
    }
  default:
    while(isalnum(*p) || isValidSpaceInsideConstant(*p)) ++p;
    p2=p--;
	
	while(isValidSpaceInsideConstant(*p)) p--;

    if (isdigit(*p)) base=10;
    else if (*p=='b') { base=2; --p; }
    else if (*p=='h') { base=16; --p; }
    else if (*p=='B') { base=2; --p; }
    else if (*p=='H') { base=16; --p; }
    else if (*p=='o') { base=8; --p; }
    else if (*p=='q') { base=8; --p; }
    else if (*p=='d') { base=10; --p; }
    else if (*p=='O') { base=8; --p; }
    else if (*p=='Q') { base=8; --p; }
    else if (*p=='D') { base=10; --p; }
    else return 0;
    do {
	  if (isValidSpaceInsideConstant(*p)) continue;
	  if ((v=getval(*p))>=base) { error("Digit not in base",op); return 0; }
      oval=val; val+=v*pb; if (oval>val) error("Overflow",0,SUPPRES);
      pb*=base;
    } while (p--!=p3);
    op=p2;
    return 1;
  }
}

int getCharConstChar(char *&op, aint &val) {
  if ((val=*op++)!='\\') return 1;
  switch (val=*op++) {
  case '\\': case '\'': case '\"': case '\?':
    return 1;
  case 'n': case 'N': val=10; return 1;
  case 't': case 'T': val=9; return 1;
  case 'v': case 'V': val=11; return 1;
  case 'b': case 'B': val=8; return 1;
  case 'r': case 'R': val=13; return 1;
  case 'f': case 'F': val=12; return 1;
  case 'a': case 'A': val=7; return 1;
  case 'e': case 'E': val=27; return 1;
  case 'd': case 'D': val=127; return 1;
  default: --op; val='\\'; error("Unknown escape",op); return 1;
  }
  return 0;
}

int getCharConst(char *&p, aint &val) {
  aint s=24,r,t=0; val=0;
  char *op=p,q;
  if (*p!='\'' && *p!='"') return 0;
  if (compassCompatibilityEnabled && ((p[0] == '"' && p[1] == '"') || (p[0] == '\'' && p[1] == '\''))) {
	  val = 0;
	  p += 2;
	  return 1;
  }
  if (compassCompatibilityEnabled && (p[0] == '"' || p[0] == '\'') && p[1] == '\\' && (p[2] == p[0])) {
      val = '\\';
      p += 3;
      return 1;
  }
  q=*p++;
  do {
    if (!*p || *p==q) { p=op; return 0; }
    getCharConstChar(p,r);
    val+=r<<s; s-=8; ++t;
  } while (*p!=q);
  if (t>4) error("Overflow",0,SUPPRES);
  val=val>>(s+8);
  ++p;
  return 1;
}

int getBytes(char *&p, int e[], int add, int dc) {
  aint val;
  int t=0;
  while ('o') {
    skipblanks(p);
    if (!*p) { error("Expression expected",0,SUPPRES); break; }
    if (t==128) { error("Too many arguments",p,SUPPRES); break; }
    if (*p=='"') {
      p++;
      do {
        if (!*p || *p=='"') { error("Syntax error",p,SUPPRES); e[t]=-1; return t; }
        if (t==128) { error("Too many arguments",p,SUPPRES); e[t]=-1; return t; }
        if (compassCompatibilityEnabled && *p == '\\') {
            e[t++] = (92 + add) & 255;
            p++;
        }
        else {
            getCharConstChar(p, val); check8(val); e[t++] = (val + add) & 255;
        }
      } while (*p!='"');
      ++p; if (dc && t) e[t-1]|=128;
    } else {
      if (ParseExpression(p,val)) { check8(val); e[t++]=(val+add)&255; }
      else { error("Syntax error",p,SUPPRES); break; }
    }
    skipblanks(p); if (*p!=',') break; ++p;
  }
  e[t]=-1; return t;
}

char *getfilename(char *&p) {
  int o=0;
  char *fn,*np;
  np=fn=new char[LINEMAX];
  skipblanks(p);

  switch(*p) {
  case '"':
    ++p;
    while (*p && *p!='"') { *np=*p; ++np; ++p; }
    if (*p=='"') ++p; else error("No closing '\"'",0);
    break;
  case '<':
    while (*p && *p!='>') { *np=*p; ++np; ++p; }
    if (*p=='>') ++p; else error("No closing '>'",0);
    break;
  default:
    while (!white() && *p!=',') { *np=*p; ++np; ++p; }
    break;
  }
/*  
  if (*p=='"') { o=1; ++p; }
  else if (*p=='<') { o=2; }
  while (!white() && *p!='"' && *p!='>') { *np=*p; ++np; ++p; }
  if (o==1) if (*p=='"') ++p; else error("No closing '\"'",0);
  else if (o==2 && *p!='>') error("No closing '>'",0);
*/
  *np=0; 
  for(np=fn;*np;++np) {
#ifdef WIN32
     if (*np=='/') *np='\\';
#else
     if (*np=='\\') *np='/';
#endif
  }
  return fn;
}


#ifdef METARM
char *getid3(char *&p) {
  int tel=3;
  char nid[4],*np;
  np=nid;
  skipblanks(p);
  if (!isalpha(*p) && *p!='_') return NULL;
  while(*p && tel--) {
    if (!isalnum(*p) && *p!='_') break;
    *np=*p; ++p; ++np;
  }
  *np=0;
  np=new char[strlen(nid)];
  strcpy(np,nid);
  return np;
}
#endif

int needcomma(char *&p) {
  skipblanks(p);
  if (*p!=',') error("Comma expected",0);
  return (*(p++)==',');
}

int needbparen(char *&p) {
  skipblanks(p);
  if (*p!=']') error("']' expected",0);
  return (*(p++)==']');
}

int islabchar(char p) {
  if (isalnum(p) || p=='_' || p=='.' || p=='?' || p=='!' || p=='#' || p=='@') return 1;
  return 0;
}

structmembs GetStructMemberId(char *&p) {
  if (*p=='#') { ++p; if (*p=='#') { ++p; return SMEMBALIGN; } return SMEMBBLOCK; }
//  if (*p=='.') ++p;
  switch (*p*2+*(p+1)) {
  case 'b'*2+'y': case 'B'*2+'Y': if (cmphstr(p,"byte")) return SMEMBBYTE; break;
  case 'w'*2+'o': case 'W'*2+'O': if (cmphstr(p,"word")) return SMEMBWORD; break;
  case 'b'*2+'l': case 'B'*2+'L': if (cmphstr(p,"block")) return SMEMBBLOCK; break;
  case 'd'*2+'b': case 'D'*2+'B': if (cmphstr(p,"db")) return SMEMBBYTE; break;
  case 'd'*2+'w': case 'D'*2+'W': 
    if (cmphstr(p,"dw")) return SMEMBWORD;
    if (cmphstr(p,"dword")) return SMEMBDWORD;
    break;
  case 'd'*2+'s': case 'D'*2+'S': if (cmphstr(p,"ds")) return SMEMBBLOCK; break;
  case 'd'*2+'d': case 'D'*2+'D': if (cmphstr(p,"dd")) return SMEMBDWORD; break;
  case 'a'*2+'l': case 'A'*2+'L': if (cmphstr(p,"align")) return SMEMBALIGN; break;
  case 'd'*2+'e': case 'D'*2+'E':
    if (cmphstr(p,"defs")) return SMEMBBLOCK;
    if (cmphstr(p,"defb")) return SMEMBBYTE;
    if (cmphstr(p,"defw")) return SMEMBWORD;
    if (cmphstr(p,"defd")) return SMEMBDWORD;
    break;
  case 'd'*2+'2': case 'D'*2+'2':
    if (cmphstr(p,"d24")) return SMEMBD24;
    break;
  default:
    break;
  }
  return SMEMBUNKNOWN;
}
//eof reader.cpp
