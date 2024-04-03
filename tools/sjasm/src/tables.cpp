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

// tables.cpp

#include "sjasm.h"

char *prevlab;

char *MaakLabNaam(char *naam) {
  char *np=naam,*lp,*label,*mlp=macrolabp;
  int p=0,l=0;
  label=new char[LINEMAX];
  lp=label;
  label[0]=0;
  if (mlp && *np=='@') { ++np; mlp=0; }
  switch (*np) {
  case '@': p=1; ++np; break;
  case '.': l=1; ++np; break;
  default: break;
  }
  naam=np;
  if (!isalpha(*np) && *np!='_') { error("Invalid labelname",naam); return 0; }
  while (*np) {
    if (isalnum(*np) || *np=='_' || *np=='.' || *np=='?' || *np=='!' || *np=='#' || *np=='@') ++np;
    else { error("Invalid labelname",naam); return 0; }
  }
  if (strlen(naam)>LABMAX) {
    error("1Label too long",naam,PASS1);
    naam[LABMAX]=0;
  }
  if (mlp && l) {
    strcat(lp,macrolabp); strcat(lp,">");
  } else {
    if (!p && modlabp) {
      int len1=strlen(lp);
      int len2=strlen(modlabp);
      strcat(lp,modlabp); 
      strcat(lp,"."); 
    }
    if (l) { 
      strcat(lp,vorlabp); strcat(lp,"."); 
    } else 
      vorlabp=strdup(naam);
  }
  strcat(lp,naam);
  return label;
}

int getLabelValue(char *&p, aint &val) {
  char *mlp=macrolabp,*op=p;
  int g=0,l=0,len,olabelnotfound=labelnotfound,plen;
  char *np;
  if (mlp && *p=='@') { ++op; mlp=0; }
  if (mlp) {
    switch (*p) {
    case '@': g=1; ++p; break;
    case '.': l=1; ++p; break;
    default: break;
    }
    temp[0]=0;
    if (l) {
      strcat(temp,macrolabp); strcat(temp,">");
      len=strlen(temp); np=temp+len; plen=0;
      if (!isalpha(*p) && *p!='_') { error("Invalid labelname",temp); return 0; }
      while (isalnum(*p) || *p=='_' || *p=='.' || *p=='?' || *p=='!' || *p=='#' || *p=='@') {
        *np=*p; ++np; ++p;
      }
      *np=0;
      if (strlen(temp)>LABMAX+len) {
        error("2Label too long",temp+len);
        temp[LABMAX+len]=0;
      }
      np=temp; g=1;
      do {
        if (labtab.zoek(np,val)) return 1;
        labelnotfound=olabelnotfound;
        while ('o') {
          if (*np=='>') { g=0; break; }
          if (*np=='.') { ++np; break; }
          ++np;
        }
      } while (g);
    }
  }

  p=op;
  switch (*p) {
  case '@': g=1; ++p; break;
  case '.': l=1; ++p; break;
  default: break;
  }
  temp[0]=0;
  if (!g && modlabp) { strcat(temp,modlabp); strcat(temp,"."); }
  if (l) { strcat(temp,vorlabp); strcat(temp,"."); }
  len=strlen(temp); np=temp+len;
  if (!isalpha(*p) && *p!='_') { error("Invalid labelname",temp); return 0; }
  while (isalnum(*p) || *p=='_' || *p=='.' || *p=='?' || *p=='!' || *p=='#' || *p=='@') {
    *np=*p; ++np; ++p;
  }
  *np=0;
  if (strlen(temp)>LABMAX+len) {
    error("3Label too long",temp+len);
    temp[LABMAX+len]=0;
  }
  if (labtab.zoek(temp,val)) return 1;
  labelnotfound=olabelnotfound;
  if (!l && !g && labtab.zoek(temp+len,val)) return 1;
  if (pass==2) { error("Label not found",temp); return 1; }
  val=0;
  return 1;
}

int getLocaleLabelValue(char *&op,aint &val) {
  aint nval;
  int nummer;
  char *p=op,naam[LINEMAX],*np,ch;
  skipblanks(p);
  np=naam;
  if (!isdigit(*p)) return 0;
  while (*p) {
    if (!isdigit(*p)) break;
    *np=*p; ++p; ++np;
  }
  *np=0; nummer=atoi(naam);
  ch=*p++;
  if (isalnum(*p)) return 0;
  switch (ch) {
  case 'b': case 'B': nval=loklabtab.zoekb(nummer); break;
  case 'f': case 'F': nval=loklabtab.zoekf(nummer); break;
  default: return 0;
  }
  if (nval==(aint)-1)
    if (pass==2) { error("Label not found",naam,SUPPRES); return 1; }
    else nval=0;
  op=p; val=nval;
  return 1;
}

labtabentrycls::labtabentrycls() {
  name=NULL; value=used=0;
}

labtabcls::labtabcls() {
  nextlocation=1;
}

int labtabcls::insert(char *nname,aint nvalue) {
  if (nextlocation>=LABTABSIZE*2/3) error("Label table full",0,FATAL);
  int tr,htr;
  tr=hash(nname);
  while(htr=hashtable[tr]) {
    if (!strcmp((labtab[htr].name),nname)) return 0;
    else if (++tr>=LABTABSIZE) tr=0;
  }
  hashtable[tr]=nextlocation;
  labtab[nextlocation].name=strdup(nname); 
  labtab[nextlocation].value=nvalue; labtab[nextlocation].used=-1;
  ++nextlocation;
  return 1;
}

int labtabcls::zoek(char *nname,aint &nvalue) {
  int tr,htr,otr;
  otr=tr=hash(nname);
  while(htr=hashtable[tr]) {
    if (!strcmp((labtab[htr].name),nname)) { 
      nvalue=labtab[htr].value; if (pass==2) ++labtab[htr].used; return 1; 
    }
    if (++tr>=LABTABSIZE) tr=0;
    if (tr==otr) break;
  }
  labelnotfound=1;
  nvalue=0;
  return 0;
}

int labtabcls::hash(char* s) {
  char *ss=s;
  unsigned int h=0,g;
  for (;*ss!='\0';ss++) {
    h=(h<<4)+ *ss;
    if (g=h & 0xf0000000) { h^=g>>24; h^=g; }
  }
  return h % LABTABSIZE;
}

void labtabcls::dump() {
  char line[LINEMAX],*ep;
  if (!listfile) { listfile=1; OpenList(); }
  fputs("\nvalue      label\n",listfp);
  fputs("-------- - -----------------------------------------------------------\n",listfp);
  for (int i=1; i<nextlocation; ++i) {
    ep=line; *ep=0;
    printhex32(ep,labtab[i].value); *(ep++)=' ';
    *(ep++)=labtab[i].used?' ':'X'; *(ep++)=' ';
    strcpy(ep,labtab[i].name); strcat(line,"\n");
    fputs(line,listfp);
  }
}

void labtabcls::dumpsym() {
  FILE *symfp;
  char lnrs[16],*l;
  if (!(symfp=fopen(symfilename,"w"))) { ErrorOpeningFile(symfilename); }
  for (int i=1; i<nextlocation; ++i) {
    if (!isdigit(labtab[i].name[0])) {
      strcpy(eline,labtab[i].name); strcat(eline,": equ "); l=lnrs;
      printhex32(l,labtab[i].value); *l=0; strcat(eline,lnrs); strcat(eline,"h\n");
      fputs(eline,symfp);
    }
  }
    fclose(symfp);
}

funtabcls::funtabcls() {
  nextlocation=1;
}

int funtabcls::insert(char *nname, void(*nfunp)(void)) {
  char *p;
  if (nextlocation>=FUNTABSIZE*2/3) { ErrorAndExit("funtab full", ERR_FATAL); }
  int tr,htr;
  tr=hash(nname);
  while(htr=hashtable[tr]) {
    if (!strcmp((funtab[htr].name),nname)) return 0;
    else if (++tr>=FUNTABSIZE) tr=0;
  }
  hashtable[tr]=nextlocation;
  funtab[nextlocation].name=strdup(nname);
  funtab[nextlocation].funp=nfunp;
  ++nextlocation;

  strcpy(p=temp,nname); while(*p=(char)toupper(*p)) ++p;
  
  if (nextlocation>=FUNTABSIZE*2/3) { ErrorAndExit("funtab full", ERR_FATAL); }
  tr=hash(temp);
  while(htr=hashtable[tr]) {
    if (!strcmp((funtab[htr].name),temp)) return 0;
    else if (++tr>=FUNTABSIZE) tr=0;
  }
  hashtable[tr]=nextlocation;
  funtab[nextlocation].name=strdup(temp);
  funtab[nextlocation].funp=nfunp;
  ++nextlocation;

  return 1;
}

int funtabcls::insertd(char *nname, void(*nfunp)(void)) {
  char *buf=new char[strlen(nname)+2];
  strcpy(buf,nname); if (!insert(buf,nfunp)) return 0;
  strcpy(buf+1,nname); buf[0]='.'; return insert(buf,nfunp);
}

int funtabcls::zoek(char *nname) {
  int tr,htr,otr;
  otr=tr=hash(nname);
  while(htr=hashtable[tr]) {
    if (!strcmp((funtab[htr].name),nname)) { 
      (*funtab[htr].funp)(); return 1; 
    }
    if (++tr>=FUNTABSIZE) tr=0;
    if (tr==otr) break;
  }
  return 0;
}

int funtabcls::hash(char* s) {
  char* ss=s;
  unsigned int h=0;
  for (;*ss!='\0';ss++) {
    h=(h<<3)+ *ss;
  }
  return h % FUNTABSIZE;
}

loklabtabentrycls::loklabtabentrycls(aint nnummer, aint nvalue, loklabtabentrycls *n) {
  regel=gcurlin; nummer=nnummer; value=nvalue;
  prev=n; next=NULL; if (n) n->next=this;
}

loklabtabcls::loklabtabcls() {
  first=last=NULL;
}

void loklabtabcls::insert(aint nnummer, aint nvalue) {
  last=new loklabtabentrycls(nnummer,nvalue,last);
  if (!first) first=last;
}

aint loklabtabcls::zoekf(aint nnum) {
  loklabtabentrycls *l=first;
  while (l) if (l->regel<=gcurlin) l=l->next; else break;
  while (l) if (l->nummer==nnum) return l->value; else l=l->next;
  return (aint)-1;
}

aint loklabtabcls::zoekb(aint nnum) {
  loklabtabentrycls *l=last;
  while (l) if (l->regel>gcurlin) l=l->prev; else break;
  while (l) if (l->nummer==nnum) return l->value; else l=l->prev;
  return (aint)-1;
}

definetabentrycls::definetabentrycls(char *nnaam,char *nvervanger, definetabentrycls *nnext) {
  char *s1, *s2;
  naam=strdup(nnaam);
  vervanger=new char[strlen(nvervanger)+1];
  s1=vervanger; s2=nvervanger; skipblanks(s2);
  while (*s2 && *s2!='\n' && *s2!='\r') { *s1=*s2; ++s1; ++s2; } *s1=0;
  next=nnext;
}

void definetabcls::init() {
  for (int i=0; i<128; defs[i++]=0);
}

void definetabcls::add(char *naam, char *vervanger) {
  if (bestaat(naam)) error("Duplicate define",naam);
  defs[*naam]=new definetabentrycls(naam,vervanger,defs[*naam]);
}

char *definetabcls::getverv(char *naam) {
  definetabentrycls *p=defs[*naam];
  while (p) {
    if (!strcmp(naam,p->naam)) return p->vervanger;
    p=p->next;
  }
  return NULL;
}

int definetabcls::bestaat(char *naam) {
  definetabentrycls *p=defs[*naam];
  while (p) {
    if (!strcmp(naam,p->naam)) return 1;
    p=p->next;
  }
  return 0;
}

void macdefinetabcls::init() {
  defs=NULL;
  for (int i=0; i<128; used[i++]=0);
}

void macdefinetabcls::macroadd(char *naam, char *vervanger) {
  defs=new definetabentrycls(naam,vervanger,defs);
  used[*naam]=1;
}

definetabentrycls *macdefinetabcls::getdefs() {
  return defs;
}

void macdefinetabcls::setdefs(definetabentrycls *ndefs) {
  defs=ndefs;
}

char *macdefinetabcls::getverv(char *naam) {
  definetabentrycls *p=defs;
  if (!used[*naam]) return NULL;
  while (p) {
    if (!strcmp(naam,p->naam)) return p->vervanger;
    p=p->next;
  }
  return NULL;
}

int macdefinetabcls::bestaat(char *naam) {
  definetabentrycls *p=defs;
  if (!used[*naam]) return 0;
  while (p) {
    if (!strcmp(naam,p->naam)) return 1;
    p=p->next;
  }
  return 0;
}

stringlst::stringlst(char*nstring,stringlst*nnext) {
  string=strdup(nstring);
  next=nnext;
}

macrotabentrycls::macrotabentrycls(char *nnaam,macrotabentrycls *nnext) {
  naam=nnaam; next=nnext; args=body=NULL;
}

void macrotabcls::init() {
  macs=NULL;
  for (int i=0; i<128; used[i++]=0);
}

int macrotabcls::bestaat(char *naam) {
  macrotabentrycls *p=macs;
  if (!used[*naam]) return 0;
  while (p) {
    if (!strcmp(naam,p->naam)) return 1;
    p=p->next;
  }
  return 0;
}

void macrotabcls::add(char *nnaam,char *&p) {
  char *n;
  stringlst *s,*l=NULL,*f=NULL;
  if (bestaat(nnaam)) error("Duplicate macroname",0,PASS1);
  macs=new macrotabentrycls(nnaam,macs);
  used[*nnaam]=1;
  skipblanks(p);
  while (*p) {
    if (!(n=getid(p))) { error("Illegal macro argument",p,PASS1); break; }
    s=new stringlst(n,NULL); if (!f) f=s; if (l) l->next=s; l=s;
    skipblanks(p); if (*p==',') ++p; else break;
  }
  macs->args=f;
  if (*p) error("Unexpected",p,PASS1);
  ListFile();
  if (!ReadFileToStringLst(macs->body,"endm")) error("Unexpected end of macro",0,PASS1);
  
  insideCompassStyleMacroDefinition = 0;
}

int macrotabcls::emit(char *naam, char *&p) {
  stringlst *a,*olijstp;
  char *n,labnr[LINEMAX],ml[LINEMAX],*omacrolabp;
  macrotabentrycls *m=macs;
  definetabentrycls *odefs;
  int olistmacro,olijst;
  if (!used[*naam]) return 0;
  while (m) {
    if (!strcmp(naam,m->naam)) break;
    m=m->next;
  }
  if (!m) return 0;
  omacrolabp=macrolabp;
  sprintf(labnr,"%d",macronummer++);
  macrolabp=labnr;
  if (omacrolabp) { strcat(macrolabp,"."); strcat(macrolabp,omacrolabp); } else macdeftab.init();
  odefs=macdeftab.getdefs();
  a=m->args;
  while (a) {
    n=ml;
    skipblanks(p);
    if (!*p) { error("Not enough arguments",0); return 1; }
    if (*p=='<') {
      ++p;
      while (*p!='>') {
        if (!*p) { error("Not enough arguments",0); return 1; }
        if (*p=='!') {
          ++p; if (!*p) { error("Not enough arguments",0); return 1; }
        }
        *n=*p; ++n; ++p;
      }
      ++p;
    } else while (*p!=',' && *p) { *n=*p; ++n; ++p; }
    *n=0; macdeftab.macroadd(a->string,ml);
    skipblanks(p); a=a->next; if (a && *p!=',') { error("Not enough arguments",0); return 1; }
    if (*p==',') ++p;
  }
  skipblanks(p); if (*p) error("Too many arguments",0);
  ListFile();
  olistmacro=listmacro; listmacro=1;
  olijstp=lijstp; olijst=lijst;
  lijstp=m->body; lijst=1;
  strcpy(ml,line);
  while (lijstp) {
    strcpy(line,lijstp->string); lijstp=lijstp->next;
    ParseLine();
  }
  strcpy(line,ml); lijst=olijst; lijstp=olijstp;
  macdeftab.setdefs(odefs); macrolabp=omacrolabp;
  listmacro=olistmacro; donotlist=1; return 0;
}

structmembncls::structmembncls(char *nnaam, aint noffset) {
  next=0; naam=strdup(nnaam); offset=noffset;
}

structmembicls::structmembicls(aint noffset,aint nlen,aint ndef,structmembs nsoort) {
  next=0; offset=noffset; len=nlen; def=ndef; soort=nsoort;
}

structcls::structcls(char *nnaam,char *nid,int idx,int no,int ngl,structcls *p) {
  mnf=mnl=0; mbf=mbl=0;
  naam=strdup(nnaam); id=strdup(nid); binding=idx; next=p; noffset=no; global=ngl;
}

void structcls::addlabel(char *nnaam) {
  structmembncls *n=new structmembncls(nnaam,noffset);
  if (!mnf) mnf=n; if (mnl) mnl->next=n; mnl=n;
}

void structcls::addmemb(structmembicls *n) {
  if (!mbf) mbf=n; if (mbl) mbl->next=n; mbl=n;
  noffset+=n->len;
}

void structcls::copylabel(char *nnaam, aint offset) {
  structmembncls *n=new structmembncls(nnaam,noffset+offset);
  if (!mnf) mnf=n; if (mnl) mnl->next=n; mnl=n;
}

void structcls::cpylabels(structcls *st) {
  char str[LINEMAX],str2[LINEMAX];
  structmembncls *np=st->mnf;
  if (!np || !prevlab) return;
  str[0]=0; strcat(str,prevlab); strcat(str,".");
  while (np) { strcpy(str2,str); strcat(str2,np->naam); copylabel(str2,np->offset); np=np->next; }
}

void structcls::copymemb(structmembicls *ni, aint ndef) {
  structmembicls *n=new structmembicls(noffset,ni->len,ndef,ni->soort);
  if (!mbf) mbf=n; if (mbl) mbl->next=n; mbl=n;
  noffset+=n->len;
}

void structcls::cpymembs(structcls *st,char *&lp) {
  structmembicls *ip;
  aint val;
  int haakjes=0;
  ip=new structmembicls(noffset,0,0,SMEMBPARENOPEN); addmemb(ip);
  skipblanks(lp); if (*lp=='{') { ++haakjes; ++lp; }
  ip=st->mbf;
  while (ip) {
    switch(ip->soort) {
    case SMEMBBLOCK: copymemb(ip,ip->def); break;
    case SMEMBBYTE: 
    case SMEMBWORD:
    case SMEMBD24:
    case SMEMBDWORD:
      synerr=0; if (!ParseExpression(lp,val)) val=ip->def; synerr=1; copymemb(ip,val); comma(lp); break;
    case SMEMBPARENOPEN: skipblanks(lp); if (*lp=='{') { ++haakjes; ++lp; } break;
    case SMEMBPARENCLOSE: skipblanks(lp); if (haakjes && *lp=='}') { --haakjes; ++lp; comma(lp); } break;
    default: error("internalerror structcls::cpymembs",0,FATAL);
    }
    ip=ip->next;
  }
  while (haakjes--) if (!need(lp,'}')) error("closing } missing",0);
  ip=new structmembicls(noffset,0,0,SMEMBPARENCLOSE); addmemb(ip);
}

void structcls::deflab() {
  char ln[LINEMAX],sn[LINEMAX],*p,*op;
  aint oval;
  structmembncls *np=mnf;
  strcpy(sn,"@"); strcat(sn,id);
  op=p=sn;
  p=MaakLabNaam(p);
  if (pass==2) {
    if (!getLabelValue(op,oval)) error("Internal error. ParseLabel()",0,FATAL);
    if (noffset!=oval) error("Label has different value in pass 2",temp);
  } else {
    if (!labtab.insert(p,noffset)) error("Duplicate label",tp,PASS1);
  }
  strcat(sn,".");  
  while (np) {
    strcpy(ln,sn); strcat(ln,np->naam);
    op=ln;
    if (!(p=MaakLabNaam(ln))) error("Illegal labelname",ln,PASS1);
    if (pass==2) {
      if (!getLabelValue(op,oval)) error("Internal error. ParseLabel()",0,FATAL);
      if (np->offset!=oval) error("Label has different value in pass 2",temp);
    } else {
      if (!labtab.insert(p,np->offset)) error("Duplicate label",tp,PASS1);
    }
    np=np->next;
  }
}

void structcls::emitlab(char *iid) {
  char ln[LINEMAX],sn[LINEMAX],*p,*op;
  aint oval;
  structmembncls *np=mnf;
  strcpy(sn,iid);
  op=p=sn;
  p=MaakLabNaam(p);
  if (pass==2) {
    if (!getLabelValue(op,oval)) error("Internal error. ParseLabel()",0,FATAL);
    if (adres!=oval) error("Label has different value in pass 2",temp);
  } else {
    if (!labtab.insert(p,adres)) error("Duplicate label",tp,PASS1);
  }
  strcat(sn,".");  
  while (np) {
    strcpy(ln,sn); strcat(ln,np->naam);
    op=ln;
    if (!(p=MaakLabNaam(ln))) error("Illegal labelname",ln,PASS1);
    if (pass==2) {
      if (!getLabelValue(op,oval)) error("Internal error. ParseLabel()",0,FATAL);
      if (np->offset+adres!=oval) error("Label has different value in pass 2",temp);
    } else {
      if (!labtab.insert(p,np->offset+adres)) error("Duplicate label",tp,PASS1);
    }
    np=np->next;
  }
}

void structcls::emitmembs(char*&p) {
  int *e,et=0,t;
  e=new int[noffset];
  structmembicls *ip=mbf;
  aint val;
  int haakjes=0;
  skipblanks(p); if (*p=='{') { ++haakjes; ++p; }
  while (ip) {
    switch(ip->soort) {
    case SMEMBBLOCK: t=ip->len; while (t--) { e[et++]=ip->def; } break;
    case SMEMBBYTE: 
      synerr=0; if (!ParseExpression(p,val)) val=ip->def; synerr=1;
      e[et++]=val%256;
      check8(val); comma(p);
      break;
    case SMEMBWORD:
      synerr=0; if (!ParseExpression(p,val)) val=ip->def; synerr=1;
      e[et++]=val%256; e[et++]=(val>>8)%256;
      check16(val); comma(p);
      break;
    case SMEMBD24:
      synerr=0; if (!ParseExpression(p,val)) val=ip->def; synerr=1;
      e[et++]=val%256; e[et++]=(val>>8)%256; e[et++]=(val>>16)%256;
      check24(val); comma(p);
      break;
    case SMEMBDWORD:
      synerr=0; if (!ParseExpression(p,val)) val=ip->def; synerr=1;
      e[et++]=val%256; e[et++]=(val>>8)%256; e[et++]=(val>>16)%256; e[et++]=(val>>24)%256;
      comma(p);
      break;
    case SMEMBPARENOPEN: skipblanks(p); if (*p=='{') { ++haakjes; ++p; } break;
    case SMEMBPARENCLOSE: skipblanks(p); if (haakjes && *p=='}') { --haakjes; ++p; comma(p); } break;
    default: error("internalerror structcls::emitmembs",0,FATAL);
    }
    ip=ip->next;
  }
  while (haakjes--) if (!need(p,'}')) error("closing } missing",0);
  skipblanks(p); if (*p) error("Syntax error - too many arguments?",0);
  e[et]=-1; EmitBytes(e);
}

void structtabcls::init() {
  for (int i=0; i<128; strs[i++]=0);
}

structcls* structtabcls::add(char *naam,int no,int idx,int gl) {
  char sn[LINEMAX],*sp;
  sn[0]=0; if (!gl && modlabp) { strcpy(sn,modlabp); strcat(sn,"."); }
  sp=strcat(sn,naam);
  if (bestaat(sp)) error("Duplicate structurename",naam,PASS1);
  strs[*sp]=new structcls(naam,sp,idx,0,gl,strs[*sp]);
  if (no) strs[*sp]->addmemb(new structmembicls(0,no,0,SMEMBBLOCK));
  return strs[*sp];
}

structcls *structtabcls::zoek(char *naam,int gl) {
  char sn[LINEMAX],*sp;
  sn[0]=0; if (!gl && modlabp) { strcpy(sn,modlabp); strcat(sn,"."); }
  sp=strcat(sn,naam);
  structcls *p=strs[*sp];
  while (p) { if (!strcmp(sp,p->id)) return p; p=p->next; }
  if (!gl && modlabp) {
    sp+=1+strlen(modlabp); p=strs[*sp];
    while (p) { if (!strcmp(sp,p->id)) return p; p=p->next; }
  }
  return 0;
}

int structtabcls::bestaat(char *naam) {
  structcls *p=strs[*naam];
  while (p) { if (!strcmp(naam,p->naam)) return 1; p=p->next; }
  return 0;
}

int structtabcls::emit(char *naam, char *l, char *&p, int gl) {
  structcls *st=zoek(naam,gl);
  if (!st) return 0;
  if (l) st->emitlab(l);
  st->emitmembs(p);
  return 1;
}

#ifdef SECTIONS
pooldatacls::pooldatacls() {
  zp=pp=first=last=NULL;
}

void PoolData() {
  aint val, regel;
  ++lp;
  if (!ParseExpression(lp,val)) error("internal error pooldata",0,FATAL);
  if (!ParseExpression(lp,regel)) error("internal error pooldata",0,FATAL);
  pooldata.pool(regel,adres); emitarm(val);
}

void pooldatacls::add(aint nregel, aint ndata, int nwok) {
  pooldataentrycls *tmp;
  tmp=new pooldataentrycls;
  tmp->regel=nregel;
  tmp->data=ndata;
  tmp->adres=~0;
  tmp->wok=nwok;
  if (!first) first=tmp;
  if (last) last->next=tmp;
  last=tmp; tmp->next=NULL;
}

int pooldatacls::zoekregel(aint nregel, aint &data, aint &adres, int &wok) {
  if (!zp) { zp=first; if (!zp) return 0; }
  while (zp && zp->regel!=nregel) zp=zp->next;
  if (!zp) {
    zp=first; while (zp && zp->regel!=nregel) zp=zp->next;
    if (!zp) return 0;
  }
  adres=zp->adres; wok=zp->wok; data=zp->data;
  return 1;
}

int pooldatacls::zoek(aint data) {
  p=first; zoekdit=data;
  return p!=NULL;
}

int pooldatacls::zoeknext(aint &adres, int &reg) {
  while (p) {
    if (p->data==zoekdit && !p->wok) {
      adres=p->adres;
      reg=(p->regel==curlin);
      p=p->next;
      return 1;
    }
    p=p->next;
  }
  return 0;
}

void pooldatacls::pool(aint regel, aint nadres) {
  if (!pp) { pp=first; if (!pp) error("internal error pooldatacls",0,FATAL); }
  while (pp && pp->regel!=regel) pp=pp->next;
  if (!pp) {
    pp=first; while (pp && pp->regel!=regel) pp=pp->next;
    if (!pp) error("internal error pooldatacls",0,FATAL);
  }
  pp->adres=nadres;
}

pooltabcls::pooltabcls() {
  first=last=NULL;
}

void pooltabcls::add(char *ndata) {
  char *data;
  data=new char[strlen(ndata)+2];
  strcpy(data," "); strcat(data,ndata);
  addlabel(data);
}

void pooltabcls::addlabel(char *ndata) {
  pooltabentrycls *tmp;
  tmp=new pooltabentrycls;
  tmp->data=strdup(ndata);
  tmp->regel=lcurlin;
  if (!first) first=tmp;
  if (last) last->next=tmp;
  last=tmp; tmp->next=NULL;
}

void pooltabcls::emit() {
  int olistdata=listdata;
  char *olp=lp,oline[LINEMAX];
  strcpy(oline,line); strcpy(line,"| Pool\n");
  if (first
#ifdef METARM
    && cpu!=Z80
#endif
    ) { EmitBlock(0,(~adres+1)&3); ListFile(); }
  while (first) { 
    if (*(lp=first->data)==' ') {
      if (*(lp+1)=='!') {
        lp++; PoolData(); listdata=0; ListFile(); listdata=olistdata;
      } else {
        ParseDirective(); ListFile();
      }
    } else {
      if (pass==1) if(!labtab.insert(lp,adres)) error("Duplicate label",tp,PASS1);
      break;
    }
    first=first->next; 
  }
  last=first; lp=olp;
  strcpy(line,oline);
}
#endif
//eof tables.cpp
