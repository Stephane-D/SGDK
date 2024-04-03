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

// tables.h

char *MaakLabNaam(char*);
extern char *prevlab;
int getLabelValue(char *&p, aint &val);
int getLocaleLabelValue(char *&op,aint &val);
#ifdef SECTIONS
void PoolData();
#endif

class labtabentrycls {
public:
  char *name;
  aint value,used;
  labtabentrycls();
};

class labtabcls {
public:
  labtabcls();
  int insert(char*,aint);
  int zoek(char*,aint&);
  void dump();
  void dumpsym();
private:
  int hashtable[LABTABSIZE],nextlocation;
  labtabentrycls labtab[LABTABSIZE];
  int hash(char*);
};

class funtabentrycls {
public:
  char *name;
  void (*funp)(void);
};

class funtabcls {
public:
  funtabcls();
  int insert(char*,void(*)(void));
  int insertd(char*,void(*)(void));
  int zoek(char*);
private:
  int hashtable[FUNTABSIZE],nextlocation;
  funtabentrycls funtab[FUNTABSIZE];
  int hash(char*);
};

class loklabtabentrycls {
public:
  aint regel,nummer,value;
  loklabtabentrycls *next,*prev;
  loklabtabentrycls(aint,aint,loklabtabentrycls*);
};

class loklabtabcls {
public:
  loklabtabcls();
  aint zoekf(aint);
  aint zoekb(aint);
  void insert(aint,aint);
private:
  loklabtabentrycls *first,*last;
};

class definetabentrycls {
public:
  char *naam, *vervanger;
  definetabentrycls *next;
  definetabentrycls(char*,char*,definetabentrycls*);
};

class definetabcls {
public:
  void init();
  void add(char*,char*);
  char *getverv(char*);
  int bestaat(char*);
  definetabcls() { init(); }
private:
  definetabentrycls *defs[128];
};

class macdefinetabcls {
public:
  void init();
  void macroadd(char*,char*);
  definetabentrycls *getdefs();
  void setdefs(definetabentrycls *);
  char *getverv(char*);
  int bestaat(char*);
  macdefinetabcls() { init(); }
private:
  int used[128];
  definetabentrycls *defs;
};

class adrlst {
public:
  aint val;
  adrlst *next;
  adrlst() { next=0; }
  adrlst(aint nval,adrlst*nnext) { val=nval; next=nnext; }
};

class stringlst {
public:
  char *string;
  stringlst *next;
  stringlst() { next=0; }
  stringlst(char*,stringlst*);
};

class macrotabentrycls {
public:
  char *naam;
  stringlst *args, *body;
  macrotabentrycls *next;
  macrotabentrycls(char*,macrotabentrycls*);
};

class macrotabcls {
public:
  void add(char*,char*&);
  int emit(char*,char*&);
  int bestaat(char*);
  void init();
  macrotabcls() { init(); }
private:
  int used[128];
  macrotabentrycls *macs;
};

class structmembncls {
public:
  char *naam;
  aint offset;
  structmembncls *next;
  structmembncls(char*,aint);
};

class structmembicls {
public:
  aint offset,len,def;
  structmembs soort;
  structmembicls *next;
  structmembicls(aint,aint,aint,structmembs);
};

class structcls {
public:
  char *naam,*id;
  int binding;
  int global;
  aint noffset;
  void addlabel(char*);
  void addmemb(structmembicls*);
  void copylabel(char*,aint);
  void cpylabels(structcls*);
  void copymemb(structmembicls*,aint);
  void cpymembs(structcls*,char*&);
  void deflab();
  void emitlab(char*);
  void emitmembs(char*&);
  structcls *next;
  structcls(char*,char*,int,int,int,structcls*);
private:
  structmembncls *mnf,*mnl;
  structmembicls *mbf,*mbl;
};

class structtabcls {
public:
  structcls* add(char*,int,int,int);
  void init();
  structtabcls() { init(); }
  structcls *zoek(char*,int);
  int bestaat(char*);
  int emit(char*,char*,char*&,int);
private:
  structcls *strs[128];
};

#ifdef SECTIONS
class pooldataentrycls {
public:
  int wok;
  aint regel, data, adres;
  pooldataentrycls *next;
};

class pooldatacls {
public:
  pooldatacls();
  void add(aint,aint,int);
  int zoek(aint);
  int zoeknext(aint&,int&);
  void pool(aint,aint);
  int zoekregel(aint,aint&,aint&,int&);
private:
  aint zoekdit;
  pooldataentrycls *first, *last, *p, *pp, *zp;
};

class pooltabentrycls {
public:
  char *data;
  aint regel;
  pooltabentrycls *next;
};

class pooltabcls {
public:
  pooltabcls();
  void add(char*);
  void addlabel(char*);
  void emit();
private:
  pooltabentrycls *first,*last;
};
#endif
//eof tables.h

