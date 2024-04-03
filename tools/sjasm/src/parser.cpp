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

// parser.cpp

#include "sjasm.h"

int replacedefineteller=0,comnxtlin;

int ParseExpPrim(char *&p, aint &nval) {
  int res=0;
  skipblanks(p);
  if (!*p) { return 0; }
  if (*p=='(') { ++p; res=ParseExpression(p,nval); if (!need(p,')')) { error("')' expected",0); return 0; }  }
  else if (isdigit(*p) || (*p=='#' && isalnum(*(p+1))) || (*p=='$' && isalnum(*(p+1))) || *p=='%' || (*p == '&' && isalnum(*(p + 1)))) 
	{ res=getConstant(p,nval); }
  else if (isalpha(*p) || *p=='_' || *p=='.' || *p=='@') { res=getLabelValue(p,nval); }
  else if (*p=='$') { ++p; nval=adres; return 1; }
  else if (!(res=getCharConst(p,nval))) { if(synerr) error("Syntax error",p,CATCHALL); return 0; }
  return res;
}

int ParseExpUnair(char *&p, aint &nval) {
  aint right;
  int oper;
  if ((oper=need(p,"! ~ + - ")) || (oper=needa(p,"not",'!',"low",'l',"high",'h'))) {
    switch (oper) {
    case '!': if(!ParseExpUnair(p,right)) return 0; nval=-!right; break;
    case '~': if(!ParseExpUnair(p,right)) return 0; nval=~right; break;
    case '+': if(!ParseExpUnair(p,right)) return 0; nval=right; break;
    case '-': if(!ParseExpUnair(p,right)) return 0; nval=~right+1; break;
    case 'l': if(!ParseExpUnair(p,right)) return 0; nval=right&255; break;
    case 'h': if(!ParseExpUnair(p,right)) return 0; nval=(right>>8)&255; break;
    default: error("Parser error",0); break;
    }
    return 1;
  } else return ParseExpPrim(p,nval);
}

int ParseExpMul(char *&p, aint &nval) {
  aint left,right;
  int oper;
  if (!ParseExpUnair(p,left)) return 0;
  while ((oper=need(p,"* / % ")) || (oper=needa(p,"mod",'%'))) {
    if (!ParseExpUnair(p,right)) return 0;
    switch (oper) {
    case '*': left*=right; break;
    case '/': if (right) left/=right; else { error("Division by zero",0); left=0; } break;
    case '%': if (right) left%=right; else { error("Division by zero",0); left=0; } break;
    default: error("Parser error",0); break;
    }
  }
  nval=left; return 1;
}

int ParseExpAdd(char *&p, aint &nval) {
  aint left,right;
  int oper;
  if (!ParseExpMul(p,left)) return 0;
  while (oper=need(p,"+ - ")) {
    if (!ParseExpMul(p,right)) return 0;
    switch (oper) {
    case '+': left+=right; break;
    case '-': left-=right; break;
    default: error("Parser error",0); break;
    }
  }
  nval=left; return 1;
}

int ParseExpShift(char *&p, aint &nval) {
  aint left,right;
  unsigned long l;
  int oper;
  if (!ParseExpAdd(p,left)) return 0;
  while ((oper=need(p,"<<>>")) || (oper=needa(p,"shl",'<'+'<',"shr",'>'))) {
    if (oper=='>'+'>' && *p=='>') { ++p; oper='>'+'@'; }
    if (!ParseExpAdd(p,right)) return 0;
    switch (oper) {
    case '<'+'<': left<<=right; break;
    case '>':
    case '>'+'>': left>>=right; break;
    case '>'+'@': l=left; l>>=right; left=l; break;
    default: error("Parser error",0); break;
    }
  }
  nval=left; return 1;
}

int ParseExpMinMax(char *&p, aint &nval) {
  aint left,right;
  int oper;
  if (!ParseExpShift(p,left)) return 0;
  while (oper=need(p,"<?>?")) {
    if (!ParseExpShift(p,right)) return 0;
    switch (oper) {
    case '<'+'?': left=left<right?left:right; break;
    case '>'+'?': left=left>right?left:right; break;
    default: error("Parser error",0); break;
    }
  }
  nval=left; return 1;
}

int ParseExpCmp(char *&p, aint &nval) {
  aint left,right;
  int oper;
  if (!ParseExpMinMax(p,left)) return 0;
  while (oper=need(p,"<=>=< > ")) {
    if (!ParseExpMinMax(p,right)) return 0;
    switch (oper) {
    case '<': left=-(left<right); break;
    case '>': left=-(left>right); break;
    case '<'+'=': left=-(left<=right); break;
    case '>'+'=': left=-(left>=right); break;
    default: error("Parser error",0); break;
    }
  }
  nval=left; return 1;
}

int ParseExpEqu(char *&p, aint &nval) {
  aint left,right;
  int oper;
  if (!ParseExpCmp(p,left)) return 0;
  while (oper=need(p,"=_==!=")) {
    if (!ParseExpCmp(p,right)) return 0;
    switch (oper) {
    case '=':
    case '='+'=': left=-(left==right); break;
    case '!'+'=': left=-(left!=right); break;
    default: error("Parser error",0); break;
    }
  }
  nval=left; return 1;
}

int ParseExpBitAnd(char *&p, aint &nval) {
  aint left,right;
  if (!ParseExpEqu(p,left)) return 0;
  while (need(p,"&_") || needa(p,"and",'&')) {
    if (!ParseExpEqu(p,right)) return 0;
    left&=right;
  }
  nval=left; return 1;
}

int ParseExpBitXor(char *&p, aint &nval) {
  aint left,right;
  if (!ParseExpBitAnd(p,left)) return 0;
  while (need(p,"^ ") || needa(p,"xor",'^')) {
    if (!ParseExpBitAnd(p,right)) return 0;
    left^=right;
  }
  nval=left; return 1;
}

int ParseExpBitOr(char *&p, aint &nval) {
  aint left,right;
  if (!ParseExpBitXor(p,left)) return 0;
  while (need(p,"|_") || needa(p,"or",'|')) {
    if (!ParseExpBitXor(p,right)) return 0;
    left|=right;
  }
  nval=left; return 1;
}

int ParseExpLogAnd(char *&p, aint &nval) {
  aint left,right;
  if (!ParseExpBitOr(p,left)) return 0;
  while (need(p,"&&")) {
    if (!ParseExpBitOr(p,right)) return 0;
    left=-(left&&right);
  }
  nval=left; return 1;
}

int ParseExpLogOr(char *&p, aint &nval) {
  aint left,right;
  if (!ParseExpLogAnd(p,left)) return 0;
  while (need(p,"||")) {
    if (!ParseExpLogAnd(p,right)) return 0;
    left=-(left||right);
  }
  nval=left; return 1;
}

int ParseExpDot(char *&p, aint &nval) {
  aint left,right;
  if (!ParseExpLogOr(p,left)) return 0;
  while (need(p,": ")) {
    if (!ParseExpLogOr(p,right)) return 0;
    left=left*256+right;
  }
  nval=left; return 1;
}

int ParseExpression(char *&p, aint &nval) {
  if (ParseExpDot(p,nval)) return 1;
  nval=0; 
  return 0;
}

char *ReplaceDefine(char*lp) {
  int definegereplaced=0,dr;
  char *nl=new char[LINEMAX*2];
  char *rp=nl,*nid,*ver,a;
  if (++replacedefineteller>20) error("Over 20 defines nested",0,FATAL);
  while ('o') {
    if (comlin || comnxtlin)
      if (*lp=='*' && *(lp+1)=='/') {
        *rp=' '; ++rp;
        lp+=2; if (comnxtlin) --comnxtlin; else --comlin; continue;
      }

    if (*lp==';' && !comlin && !comnxtlin) { *rp=0; return nl; }
    if (*lp=='/' && *(lp+1)=='/' && !comlin && !comnxtlin) { *rp=0; return nl; }
    if (*lp=='/' && *(lp+1)=='*') { lp+=2; ++comnxtlin; continue; }

    if (*lp=='"' || ((!compassCompatibilityEnabled) && *lp == '\'')) {
      a=*lp; if (!comlin && !comnxtlin) { *rp=*lp; ++rp; } ++lp;
      if (a!='\'' || (*(lp-2)!='f' || *(lp-3)!='a') && (*(lp-2)!='F' && *(lp-3)!='A'))
        while ('o') {
          if (!*lp) { *rp=0; return nl; }
          if (!comlin && !comnxtlin) *rp=*lp;
          if (*lp==a) { if (!comlin && !comnxtlin) ++rp; ++lp; break; }
          if (*lp=='\\' && !compassCompatibilityEnabled) { ++lp; if (!comlin && !comnxtlin) { ++rp; *rp=*lp; } }
          if (!comlin && !comnxtlin) ++rp; ++lp;
        }
      continue;
    }

    if (comlin || comnxtlin) { if (!*lp) { *rp=0; break; } ++lp; continue; }
    if (!isalpha(*lp) && *lp!='_') { if (!(*rp=*lp)) break; ++rp; ++lp; continue; }
    nid=getid(lp); dr=1;
    if (!(ver=definetab.getverv(nid))) if (!macrolabp || !(ver=macdeftab.getverv(nid))) { dr=0; ver=nid; }
    if (dr) definegereplaced=1;
    while (*rp=*ver) { ++rp; ++ver; }
  }
  if (strlen(nl)>LINEMAX-1) error("line too long after macro expansion",0,FATAL);
  if (definegereplaced) return ReplaceDefine(nl);
  return nl;
}

void ParseLabel() {
  char *tp,temp[LINEMAX],*ttp;
  aint val,oval;
  if (white()) return;
  tp=temp;
  while (*lp && !white() && *lp!=':' && *lp!='=') { *tp=*lp; ++tp; ++lp; }
  *tp=0; if (*lp==':') ++lp;
  tp=temp; skipblanks();
  labelnotfound=0;
#ifdef SECTIONS
  switch (section) {
  case TEXT:
#endif
	if (isdigit(*tp)) {
    if (needequ() || needfield()) { 
      error("Numberlabels only allowed as adresslabels",0); 
#ifdef SECTIONS
      break;
#else
      return;
#endif
    }
    val=atoi(tp); if (pass==1) loklabtab.insert(val,adres);
	} else {
	  if (needequ()) {
  		if (!ParseExpression(lp,val)) { error("Expression error",lp); val=0; }
  		if (labelnotfound) { error("Forward reference",0,PASS1); }
	  } else if (needfield()) {
		  aint nv;
		  val=mapadr;
      synerr=0; if (ParseExpression(lp,nv)) mapadr+=nv; synerr=1;
      if (labelnotfound) error("Forward reference",0,PASS1);
    } else {
      int gl=0;
      char *p=lp,*n; 
      skipblanks(p);
      if (*p=='@') { ++p; gl=1; }
      if ((n=getid(p)) && structtab.emit(n,tp,p,gl)) { lp=p; return; }
      val=adres;
    }
    ttp=tp;
	  if (!(tp=MaakLabNaam(tp)))
#ifdef SECTIONS
      break;
#else
      return;
#endif
    if (pass==2) {
      if (!getLabelValue(ttp,oval)) error("Internal error. ParseLabel()",0,FATAL);
      if (val!=oval) error("Label has different value in pass 2",temp);
    } else
      if (!labtab.insert(tp,val)) error("Duplicate label",tp,PASS1);
	}
#ifdef SECTIONS
  break;
  case DATA:
    if (isdigit(*tp)) { error("Number labels not allowed in data sections",tp); break; }
    if (needequ()) { error("Equ not allowed in data sections",0); break; }
    if (needfield()) { error("Field not allowed in data sections",0); break; }
    if (!(tp=MaakLabNaam(tp))) break; 
    pooltab.addlabel(tp);
    break;
  default:
    error("internal error parselabel",0,FATAL);
    break;
  }
#endif
}

void ParseMacro() {
  int gl=0;
  char *p=lp,*n;
  skipblanks(p); if (*p=='@') { gl=1; ++p; }
  if (!(n=getid(p))) return;
  if (structtab.emit(n,0,p,gl) || !gl && macrotab.emit(n,p)) *lp=0;
}

void ParseInstruction() {
  if (ParseDirective()) return;
#ifdef SECTIONS
  if (section!=TEXT) { error("No instructions allowed outside text sections",lp); return; }
#endif
#ifdef METARM
  (*piCPUp)();
#else
  piZ80();
#endif
}

int insideCompassStyleMacroDefinition = 0;

void ReformatCompassStyleMacro(char* line)
{
	char* labelStart;
	char* labelEnd;
	char* paramsStart;
	int i;
	int labelLength;

	char* lp = line;
	if (!*lp || *lp <= ' ' || *lp == ';')
		return;

	labelStart = line;
	while(*lp && *lp > ' ' && *lp != ':' && *lp != ';') lp++;
	if (!*lp || *lp == ';')
		return;

	labelEnd = lp;
	labelLength = labelEnd - labelStart;

	while (*lp == ':' || *lp == ' ' || *lp == '\t') lp++;
	if (!*lp)
		return;

	if (tolower(lp[0]) != 'm') return;
	if (tolower(lp[1]) != 'a') return;
	if (tolower(lp[2]) != 'c') return;
	if (tolower(lp[3]) != 'r') return;
	if (tolower(lp[4]) != 'o') return;
	if (lp[5] > ' ') return;

	lp += 5;
	while (*lp && *lp <= ' ') ++lp;
	paramsStart = lp;
	
	//cout << "!!1 " << paramsStart << endl;

	/* It's a Compass style macro */

    strcpy(temp, " macro ");
    memcpy(temp + 7, labelStart, labelLength);
    strcpy(temp + 7 + labelLength, paramsStart - 1);

    strcpy(line, temp);

    insideCompassStyleMacroDefinition = 1;
}

void ParseLine() {
  char* tempLp;

  if(compassCompatibilityEnabled)
	ReformatCompassStyleMacro(line);

  if(insideCompassStyleMacroDefinition)
	  ReplaceAtToUnderscore(line);
  
  ++gcurlin;
  replacedefineteller=comnxtlin=0;
  lp = ReplaceDefine(line);
  if (comlin) { comlin+=comnxtlin; ListFileSkip(line); return; }
  comlin+=comnxtlin; if (!*lp) { ListFile(); return; }
  ParseLabel(); if (skipblanks()) { ListFile(); return; }
  ParseMacro(); if (skipblanks()) { ListFile(); return; }
  ParseInstruction(); if (skipblanks()) { ListFile(); return; }
  if (*lp) error("Unexpected",lp); ListFile();
}

void ParseStructLabel(structcls *st) {
  char *tp,temp[LINEMAX];
  prevlab=0;
  if (white()) return;
  tp=temp; if (*lp=='.') ++lp;
  while (*lp && islabchar(*lp)) { *tp=*lp; ++tp; ++lp; }
  *tp=0; if (*lp==':') ++lp;
  tp=temp; skipblanks();
	if (isdigit(*tp)) { error("Numberlabels not allowed within structs",0); return; }
  prevlab=strdup(tp); st->addlabel(tp);
}

void ParseInSTRUCTion(structcls *st) {
  structmembicls *smp;
  aint val,len;
  bp=lp;
  switch (GetStructMemberId(lp)) {
  case SMEMBBLOCK:
    if (!ParseExpression(lp,len)) { len=1; error("Expression expected",0,PASS1); }
    if (comma(lp)) {
      if (!ParseExpression(lp,val)) { val=0; error("Expression expected",0,PASS1); }
    } else val=0;
    check8(val);
    smp=new structmembicls(st->noffset,len,val&255,SMEMBBLOCK);
    st->addmemb(smp);
    break;
  case SMEMBBYTE:
    if (!ParseExpression(lp,val)) val=0; check8(val);
    smp=new structmembicls(st->noffset,1,val,SMEMBBYTE);
    st->addmemb(smp);
    break;
  case SMEMBWORD:
    if (!ParseExpression(lp,val)) val=0; check16(val);
    smp=new structmembicls(st->noffset,2,val,SMEMBWORD);
    st->addmemb(smp);
    break;
  case SMEMBD24:
    if (!ParseExpression(lp,val)) val=0; check24(val);
    smp=new structmembicls(st->noffset,3,val,SMEMBD24);
    st->addmemb(smp);
    break;
  case SMEMBDWORD:
    if (!ParseExpression(lp,val)) val=0;
    smp=new structmembicls(st->noffset,4,val,SMEMBDWORD);
    st->addmemb(smp);
    break;
  case SMEMBALIGN:
    if (!ParseExpression(lp,val)) val=4;
    st->noffset+=((~st->noffset+1)&(val-1));
    break;
  default:
    char *pp=lp,*n;
    int gl=0;
    structcls *s;
    skipblanks(pp); if (*pp=='@') { ++pp; gl=1; }
    if ((n=getid(pp)) && (s=structtab.zoek(n,gl))) { lp=pp; st->cpylabels(s); st->cpymembs(s,lp); }
    break;
  }
}

void ParseStructLine(structcls *st) {
  replacedefineteller=comnxtlin=0;
  lp=ReplaceDefine(line);
  if (comlin) { comlin+=comnxtlin; return; }
  comlin+=comnxtlin; if (!*lp) return;
  ParseStructLabel(st); if (skipblanks()) return;
  ParseInSTRUCTion(st); if (skipblanks()) return;
  if (*lp) error("Unexpected",lp);
}
//eof parser.cpp
