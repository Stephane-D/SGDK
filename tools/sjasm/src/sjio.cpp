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

// sjio.cpp

#include "sjasm.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define DESTBUFLEN 8192

int EB[1024*64],nEB=0;
char destbuf[DESTBUFLEN];
FILE *input, *output;
FILE *listfp,*expfp=NULL;
aint eadres,epadres,desttel=0,skiperrors=0;;
char hd[]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
char* errorSorts[] = { "ALL", "PASS1", "PASS2", "FATAL", "CATCHALL", "SUPPRES" };

void error(char *fout,char *bd,int soort) {
  char *ep=eline;
  if (skiperrors && preverror==lcurlin && soort!=FATAL) return;
  if (soort==CATCHALL && preverror==lcurlin) return;
  if (soort==PASS1 && pass!=1) return;
  if ((soort==CATCHALL || soort==SUPPRES || soort==PASS2) && pass!=2) return;
  skiperrors=(soort==SUPPRES);
  preverror=lcurlin;
  ++nerror;

  if(useVsErrorFormat)
	sprintf(ep, "%s(%lu) : error %s : %s", filename, lcurlin, errorSorts[soort], fout);
  else
	sprintf(ep,"%s line %lu: %s", filename, lcurlin, fout);
  
  if (bd) { strcat(ep,": "); strcat(ep,bd); }
  if (!strchr(ep,'\n')) strcat(ep,"\n");
  if (listfile) fputs(eline,listfp);
  errout << eline;
  if (soort==FATAL) exit(ERR_FATAL);
}

void WriteDest() {
  if (!desttel) return;
  destlen+=desttel;
  if(fwrite(destbuf,1,desttel,output)<desttel) error("Write error (disk full?)",0,FATAL);
  desttel=0;
}

void printhex8(char *&p, aint h) {
  unsigned hh=h&0xff;
  *(p++)=hd[hh>>4];
  *(p++)=hd[hh&15];
}

void listbytes(char *&p) {
  int i=0;
  while (nEB--) { printhex8(p,EB[i++]); *(p++)=' '; }
  i=4-i;
  while (i--) { *(p++)=' '; *(p++)=' '; *(p++)=' '; }
}

void listbytes2(char *&p) {
  for (int i=0;i!=5;++i) printhex8(p,EB[i]);
  *(p++)=' '; *(p++)=' ';
}

void printlcurlin(char *&p) {
  aint v=lcurlin;
  switch (reglenwidth) {
  default: *(p++)=(unsigned char)('0'+v/1000000); v%=1000000;
  case 6: *(p++)=(unsigned char)('0'+v/100000); v%=100000;
  case 5: *(p++)=(unsigned char)('0'+v/10000); v%=10000;
  case 4: *(p++)=(unsigned char)('0'+v/1000); v%=1000;
  case 3: *(p++)=(unsigned char)('0'+v/100); v%=100;
  case 2: *(p++)=(unsigned char)('0'+v/10); v%=10;
  case 1: *(p++)=(unsigned char)('0'+v);
  }
  *(p++)=include>0?'+':' ';
  *(p++)=include>1?'+':' ';
  *(p++)=include>2?'+':' ';
}

void printhex32(char *&p, aint h) {
  unsigned hh=h&0xffffffff;
  *(p++)=hd[hh>>28]; hh&=0xfffffff;
  *(p++)=hd[hh>>24]; hh&=0xffffff;
  *(p++)=hd[hh>>20]; hh&=0xfffff;
  *(p++)=hd[hh>>16]; hh&=0xffff;
  *(p++)=hd[hh>>12]; hh&=0xfff;
  *(p++)=hd[hh>>8];  hh&=0xff;
  *(p++)=hd[hh>>4];  hh&=0xf;
  *(p++)=hd[hh];
}

void printhex16(char *&p, aint h) {
  unsigned hh=h&0xffff;
  *(p++)=hd[hh>>12]; hh&=0xfff;
  *(p++)=hd[hh>>8]; hh&=0xff;
  *(p++)=hd[hh>>4]; hh&=0xf;
  *(p++)=hd[hh];
}

void listbytes3(int pad) {
  int i=0,t;
  char *pp,*sp=pline+3+reglenwidth;
  while (nEB) {
    pp=sp;
#ifdef METARM
    if (cpu==Z80) printhex16(pp,pad); else printhex32(pp,pad);
#else
    printhex16(pp,pad);
#endif
    *(pp++)=' '; t=0;
    while (nEB && t<32) { printhex8(pp,EB[i++]); --nEB; ++t; }
    *(pp++)='\n'; *pp=0;
    fputs(pline,listfp);
    pad+=32;
  }
}

#ifdef METARM
void listi32(char *&p) {
  printhex8(p,EB[3]);
  printhex8(p,EB[2]);
  printhex8(p,EB[1]);
  printhex8(p,EB[0]);
  *p=0; strcat(pline,"    "); p+=4;
}

void listi16(char *&p) {
  printhex8(p,EB[1]);
  printhex8(p,EB[0]);
  *p=0; strcat(pline,"        "); p+=8;
}
#endif

void ListFile() {
  char *pp=pline;
  aint pad;
  if (pass==1 || !listfile || donotlist) { donotlist=nEB=0; return; }
  if (listmacro) if (!nEB) return;
  if ((pad=eadres)==(aint)-1) pad=epadres;
  if (strlen(line) && line[strlen(line)-1]!=10) strcat(line,"\n");
  *pp=0;
  printlcurlin(pp);
#ifdef METARM
  if (cpu==Z80) printhex16(pp,pad); else printhex32(pp,pad);
#else
  printhex16(pp,pad);
#endif
  *(pp++)=' ';
#ifdef METARM
  switch (cpu) {
  case ARM:
    if (nEB==4 && !listdata) { listi32(pp); *pp=0; if (listmacro) strcat(pp,">"); strcat(pp,line); fputs(pline,listfp); break; }
    if (nEB<5) { listbytes(pp); *pp=0; if (listmacro) strcat(pp,">"); strcat(pp,line); fputs(pline,listfp); }
    else if (nEB<6) { listbytes2(pp); *pp=0; if (listmacro) strcat(pp,">"); strcat(pp,line); fputs(pline,listfp); }
    else { listbytes2(pp); *pp=0; if (listmacro) strcat(pp,">"); strcat(pp,line); fputs(pline,listfp); listbytes3(pad); fputs(pline,listfp); }
    break;
  case THUMB:
    if (nEB==2 && !listdata) { listi16(pp); *pp=0; if (listmacro) strcat(pp,">"); strcat(pp,line); fputs(pline,listfp); break; }
    if (nEB==4 && !listdata) { listi32(pp); *pp=0; if (listmacro) strcat(pp,">"); strcat(pp,line); fputs(pline,listfp); break; }
    if (nEB<5) { listbytes(pp); *pp=0; if (listmacro) strcat(pp,">"); strcat(pp,line); fputs(pline,listfp); }
    else if (nEB<6) { listbytes2(pp); *pp=0; if (listmacro) strcat(pp,">"); strcat(pp,line); fputs(pline,listfp); }
    else { listbytes2(pp); *pp=0; if (listmacro) strcat(pp,">"); strcat(pp,line); fputs(pline,listfp); listbytes3(pad); fputs(pline,listfp); }
    break;
  case Z80:
#endif
    if (nEB<5) { listbytes(pp); *pp=0; if (listmacro) strcat(pp,">"); strcat(pp,line); fputs(pline,listfp); }
    else if (nEB<6) { listbytes2(pp); *pp=0; if (listmacro) strcat(pp,">"); strcat(pp,line); fputs(pline,listfp); }
    else { for (int i=0;i!=12;++i) *(pp++)=' '; *pp=0; if (listmacro) strcat(pp,">"); strcat(pp,line); fputs(pline,listfp); listbytes3(pad); }
#ifdef METARM
    break;
  default:
    error("internal error listfile",0,FATAL);
  }
#endif
  epadres=adres; eadres=(aint)-1; nEB=0; listdata=0;
}

void ListFileSkip(char *line) {
  char *pp=pline;
  aint pad;
  if (pass==1 || !listfile || donotlist) { donotlist=nEB=0; return; }
  if (listmacro) return;
  if ((pad=eadres)==(aint)-1) pad=epadres;
  if (strlen(line) && line[strlen(line)-1]!=10) strcat(line,"\n");
  *pp=0;
  printlcurlin(pp);
#ifdef METARM
  if (cpu==Z80) printhex16(pp,pad); else printhex32(pp,pad);
#else
  printhex16(pp,pad);
#endif
  *pp=0; strcat(pp,"~            ");
  if (nEB) error("Internal error lfs",0,FATAL);
  if (listmacro) strcat(pp,">"); strcat(pp,line); fputs(pline,listfp);
  epadres=adres; eadres=(aint)-1; nEB=0; listdata=0;
}

void emit(int byte) {
  EB[nEB++]=byte;
  if (pass==2) { 
    destbuf[desttel++]=(char)byte;
    if (desttel==DESTBUFLEN) WriteDest();
  }
  ++adres;
}

void EmitByte(int byte) {
  eadres=adres;
  emit(byte);
}

void EmitBytes(int *bytes) {
  eadres=adres;
  if (*bytes==-1) { error("Illegal instruction",line,CATCHALL); *lp=0; }
  while (*bytes!=-1) emit(*bytes++);
}

void EmitWords(int *words) {
  eadres=adres;
  while (*words!=-1) {
    emit((*words)%256);
    emit((*words++)/256);
  }
}

void EmitBlock(aint byte, aint len) {
  eadres=adres;
  if (len) { EB[nEB++]=byte; }
  while (len--) {
    if (pass==2) { 
      destbuf[desttel++]=(char)byte; 
      if (desttel==DESTBUFLEN) WriteDest(); 
    }
    ++adres;
  }
}

char *getpath(char *fname, TCHAR **filenamebegin) {
  int g=0;
  char *kip,nieuwzoekpad[MAX_PATH];
  g=SearchPath(huidigzoekpad,fname,NULL,MAX_PATH,nieuwzoekpad,filenamebegin);
  if (!g) {
    if (fname[0]=='<') fname++;
    stringlst *dir=dirlstp;
    while (dir) {
      if (SearchPath(dir->string,fname,NULL,MAX_PATH,nieuwzoekpad,filenamebegin)) { g=1; break; }
      dir=dir->next;
    }
  }
  if (!g) SearchPath(huidigzoekpad,fname,NULL,MAX_PATH,nieuwzoekpad,filenamebegin);
  kip=strdup(nieuwzoekpad);
  if (filenamebegin) *filenamebegin+=kip-nieuwzoekpad;
  return kip;
}

void BinIncFile(char *fname,int offset,int len) {
  char *bp;
  FILE *bif;
  int res;
  char *nieuwzoekpad;
  nieuwzoekpad=getpath(fname,NULL);
  if (*fname=='<') fname++;
  if (!(bif=fopen(nieuwzoekpad,"rb"))) {
	  ErrorOpeningFile(fname);
  }
  if (offset>0) {
    bp=new char[offset+1];
    res=fread(bp,1,offset,bif);
    if (res==-1) error("Read error",fname,FATAL);
    if (res!=offset) error("Offset beyond filelength",fname,FATAL);
  }
  if (len>0) {
    bp=new char[len+1];
    res=fread(bp,1,len,bif);
    if (res==-1) error("Read error",fname,FATAL);
    if (res!=len) error("Unexpected end of file",fname,FATAL);
    while (len--) {
      if (pass==2) { destbuf[desttel++]=*bp++; if (desttel==DESTBUFLEN) WriteDest(); }
      ++adres;
    }
  } else {
    if (pass==2) WriteDest();
    do {
      res=fread(destbuf,1,DESTBUFLEN,bif);
      if (res==-1) error("Read error",fname,FATAL);
      if (pass==2) { desttel=res; WriteDest(); }
      adres+=res;
    } while (res==DESTBUFLEN);
  }
  fclose(bif);
}

void OpenFile(char *nfilename) {
  char ofilename[LINEMAX];
  char *ohuidigzoekpad,*nieuwzoekpad;
  TCHAR *filenamebegin;
  aint olcurlin=lcurlin;
  lcurlin=0;
  FILE *oinput=input;
  input=0;
  strcpy(ofilename,filename);
  if (++include>20) error("Over 20 files nested",0,FATAL);
  nieuwzoekpad=getpath(nfilename,&filenamebegin);
  if (*nfilename=='<') nfilename++;
  strcpy(filename,nfilename);
  if ((input=fopen(nieuwzoekpad,"r"))==NULL) { ErrorOpeningFile(nfilename); }
  ohuidigzoekpad=huidigzoekpad; *filenamebegin=0; huidigzoekpad=nieuwzoekpad;
  while(running && fgets(line,LINEMAX,input)) {
    ++lcurlin; ++curlin;
    if (strlen(line)==LINEMAX-1) error("Line too long",0,FATAL);
    ParseLine(); 
  }
  fclose(input);
  --include;
  huidigzoekpad=ohuidigzoekpad;
  strcpy(filename,ofilename);
  if (lcurlin>maxlin) maxlin=lcurlin;
  input=oinput; lcurlin=olcurlin;
}

void OpenList() {
  if (listfile) 
    if (!(listfp=fopen(listfilename,"w"))) {
		ErrorOpeningFile(listfilename);
    }
}

void CloseDest() {
  long pad;
  if (desttel) WriteDest();
  if (size!=-1) {
    if (destlen>size) error("File exceeds 'size'",0);
    else {
      pad=size-destlen;
      if (pad>0) 
        while (pad--) {
          destbuf[desttel++]=0;
          if (desttel==256) WriteDest();
      }
    if (desttel) WriteDest();
    }
  }
  fclose(output);
}

void SeekDest(long offset,int method) {
	WriteDest();
	if(fseek(output,offset,method)) error("File seek error (FORG)",0,FATAL);
}

void NewDest(char *ndestfilename) {
  NewDest(ndestfilename,OUTPUT_TRUNCATE);
}

void NewDest(char *ndestfilename,int mode) {
  CloseDest();
  strcpy(destfilename,ndestfilename);
  OpenDest(mode);
}

void OpenDest() {
  OpenDest(OUTPUT_TRUNCATE);
}

void OpenDest(int mode) {
  destlen=0;
  if(mode!=OUTPUT_TRUNCATE && !FileExists(destfilename)) mode=OUTPUT_TRUNCATE;
  if ((output = fopen( destfilename, mode==OUTPUT_TRUNCATE ? "wb" : "r+b" )) == NULL )
  {
	  ErrorOpeningFile(destfilename);
  }
  if(mode!=OUTPUT_TRUNCATE)
  {
     if(fseek(output,0,mode==OUTPUT_REWIND ? SEEK_SET : SEEK_END)) error("File seek error (OUTPUT)",0,FATAL); 
  }
}

int FileExists(char* filename) {
  int exists=0;
  FILE* test=fopen(filename,"r");
  if(test!=NULL) {
    exists=-1;
    fclose(test);
  }
  return exists;
}

void Close() {
  CloseDest();
  if (expfp) {
	fclose(expfp);
	expfp = NULL;
  }
  if (listfile) fclose(listfp);
}

Ending ReadFile() {
  stringlst *ol;
  char *p;
  while ('o') {
    if (!running) return END;
    if (lijst) { 
      if (!lijstp) return END;
      p=strcpy(line,lijstp->string); ol=lijstp; lijstp=lijstp->next;
    } else {
      if (!fgets(p=line,LINEMAX,input)) error("Unexpected end of file",0,FATAL);
      ++lcurlin; ++curlin;
      if (strlen(line)==LINEMAX-1) error("Line too long",0,FATAL);
    }
    skipblanks(p);
    if (*p=='.') ++p;
    if (cmphstr(p,"endif")) { return ENDIF; }
    if (cmphstr(p,"else")) { ListFile(); return ELSE; }
    if (cmphstr(p,"endt")) { return ENDTEXTAREA; }
    if (cmphstr(p,"dephase")) { return ENDTEXTAREA; }
	if (compassCompatibilityEnabled && cmphstr(p, "endc")) { return ENDIF; }
    ParseLine();
  }
}

Ending SkipFile() {
  stringlst *ol;
  char *p;
  int iflevel=0;
  while ('o') {
    if (!running) return END;
    if (lijst) { 
      if (!lijstp) return END;
      p=strcpy(line,lijstp->string); ol=lijstp; lijstp=lijstp->next;
    } else {
      if (!fgets(p=line,LINEMAX,input)) error("Unexpected end of file",0,FATAL);
      ++lcurlin; ++curlin;
      if (strlen(line)==LINEMAX-1) error("Line too long",0,FATAL);
    }
    skipblanks(p);
    if (*p=='.') ++p;
    if (cmphstr(p,"if")) { ++iflevel; }
    if (cmphstr(p,"ifexist")) { ++iflevel; }
    if (cmphstr(p,"ifnexist")) { ++iflevel; }
    if (cmphstr(p,"ifdef")) { ++iflevel; }
    if (cmphstr(p,"ifndef")) { ++iflevel; }
    if (cmphstr(p,"endif")) { if (iflevel) --iflevel; else return ENDIF; }
    if (cmphstr(p,"else")) { if (!iflevel) { ListFile(); return ELSE; } }
    ListFileSkip(line);
  }
}

int ReadLine() {
  if (!running) return 0;
  if (!fgets(line,LINEMAX,input)) error("Unexpected end of file",0,FATAL);
  ++lcurlin; ++curlin;
  if (strlen(line)==LINEMAX-1) error("Line too long",0,FATAL);
  return 1;
}

void ConvertCompassStyleLocalLabels(char* line) {
	//Replaces "label@sym" into ".labelSym"

	char* lp = line;
	char* atSymPointer;
	char* labelStartPointer;
	int labelLength;
	int i;

	while (*lp) {
		if (!(*lp == '@' && tolower(lp[1]) == 's' && tolower(lp[2]) == 'y' && tolower(lp[3]) == 'm')) {
			lp++;
			continue;
		}

		atSymPointer = lp;
		lp--;
		if(lp < line) return;

		while(lp >= line && IsValidIdChar(*lp))
			lp--;

		labelStartPointer = lp + 1;
		labelLength = atSymPointer - labelStartPointer;
		if (labelLength == 0) {
			lp = atSymPointer + 4;
			continue;
		}

		for (i = labelLength - 1; i >= 0; i--)
			labelStartPointer[i + 1] = labelStartPointer[i];

		*labelStartPointer = '.';
		atSymPointer[1] = 'S';

		lp = atSymPointer + 4;
	}
}

int ReadFileToStringLst(stringlst *&f,char *end) {
  stringlst *s,*l=NULL;
  char *p; f=NULL;
  char* tempLp;
  while ('o') {
    if (!running) return 0;
    if (!fgets(p=line,LINEMAX,input)) error("Unexpected end of file",0,FATAL);
    ++lcurlin; ++curlin;
    if (strlen(line)==LINEMAX-1) error("Line too long",0,FATAL);

	if (insideCompassStyleMacroDefinition) {
		ConvertCompassStyleLocalLabels(line);
		ReplaceAtToUnderscore(line);
	}

    if (*p && *p<=' ') {
      skipblanks(p); if (*p=='.') ++p;
      if (cmphstr(p,end)) { return 1; }
    }
    s=new stringlst(line,NULL); if (!f) f=s; if (l) l->next=s; l=s;
    ListFileSkip(line);
  }
}

void WriteExp(char *n, aint v) {
  char lnrs[16],*l=lnrs;
  if (!expfp) {
    if (!(expfp=fopen(expfilename,"w"))) {
		ErrorOpeningFile(expfilename);
	}
  }
  strcpy(eline,n); strcat(eline,": EQU ");
  printhex32(l,v); *l=0; strcat(eline,lnrs); strcat(eline,"h\n");
  fputs(eline,expfp);
}

void emitarm(aint data) {
  eadres=adres;
  emit(data&255);
  emit((data>>8)&255);
  emit((data>>16)&255);
  emit((data>>24)&255);
}

#ifdef METARM
void emitthumb(aint data) {
  eadres=adres;
  emit(data&255);
  emit((data>>8)&255);
}

void emitarmdataproc(int cond, int I,int opcode,int S,int Rn,int Rd,int Op2) {
  aint i;
  i=(cond<<28)+(I<<25)+(opcode<<21)+(S<<20)+(Rn<<16)+(Rd<<12)+Op2;
  emitarm(i);
}
#endif
//eof sjio.cpp
