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

// sjasm.cpp

#include "sjasm.h"

char destfilename[LINEMAX],listfilename[LINEMAX],expfilename[LINEMAX],sourcefilename[LINEMAX],symfilename[LINEMAX];

char filename[LINEMAX],*lp,line[LINEMAX],temp[LINEMAX],*tp,pline[LINEMAX*2],eline[LINEMAX*2],*bp;

int pass,labelnotfound,nerror,include=-1,running,labellisting=0,listfile=1,donotlist,listdata,listmacro;
int useStdError = 0;
int useVsErrorFormat = 0;
int compassCompatibilityEnabled = 0;
int macronummer,lijst,reglenwidth,synerr=1,symfile=0;
aint adres,mapadr,gcurlin,lcurlin,curlin,destlen,size=(aint)-1,preverror=(aint)-1,maxlin=0,comlin;
#ifdef METARM
cpus cpu;
#endif
char *huidigzoekpad;

void (*piCPUp)(void);

#ifdef SECTIONS
sections section;
#endif

char *modlabp,*vorlabp,*macrolabp;

stringlst *lijstp;
labtabcls labtab;
loklabtabcls loklabtab;
definetabcls definetab;
macdefinetabcls macdeftab;
macrotabcls macrotab;
structtabcls structtab;
adrlst *maplstp=0;
stringlst *modlstp=0,*dirlstp=0;
#ifdef SECTIONS
pooldatacls pooldata;
pooltabcls pooltab;bluemsx
#endif

void ReplaceAtToUnderscore(char* string) {
	char* tempLp = string;
	char c;
	int insideDoubleQuotedString = 0;
	int insideSingleQuotedString = 0;

	while (c = *tempLp) {
		if(c == '"' && !insideSingleQuotedString) {
			insideDoubleQuotedString = !insideDoubleQuotedString;
		}
		if (c == '\'' && !insideDoubleQuotedString) {
			insideSingleQuotedString = !insideSingleQuotedString;
		}
		if (c == '@' && !insideDoubleQuotedString && !insideSingleQuotedString)
			*tempLp = '_';

		tempLp++;
	}
}

void InitPass(int p) {
#ifdef SECTIONS
  section=TEXT;
#endif
  reglenwidth=1;
  if (maxlin>9) reglenwidth=2;
  if (maxlin>99) reglenwidth=3;
  if (maxlin>999) reglenwidth=4;
  if (maxlin>9999) reglenwidth=5;
  if (maxlin>99999) reglenwidth=6;
  if (maxlin>999999) reglenwidth=7;
  modlabp=0; vorlabp="_"; macrolabp=0; listmacro=0;
  pass=p; adres=mapadr=0; running=1; gcurlin=lcurlin=curlin=0;
  eadres=0; epadres=0; macronummer=0; lijst=0; comlin=0;
  modlstp=0;
#ifdef METARM
  cpu=Z80; piCPUp=piZ80;
#endif
  structtab.init();
  macrotab.init();
  definetab.init();
  macdeftab.init();
}

void getOptions(char **&argv,int &i) {
  char *p,c;
  char ps[2];
  while (argv[i] && *argv[i]=='-') {
    p=argv[i++]+1; 
    do {
      c=*p++;
      switch (tolower(c)) {
      case 'q': listfile=0; break;
      case 's': symfile=1; break;
      case 'l': labellisting=1; break;
      case 'i': dirlstp=new stringlst(p,dirlstp); p=""; break;
	  case 'e': useStdError = 1; break;
	  case 'c': compassCompatibilityEnabled = 1; break;
	  case 'v': useVsErrorFormat = 1; break;
      default:
		  ps[0] = c;
		  ps[1] = '\0';
		  ErrorAndExit2("Unrecognised option: ", ps, ERR_INVALID_OPTION);
      }
    } while (*p);
  }
}

void ErrorAndExit2(char* message, char* param, int exitCode) {
	if (useVsErrorFormat)
		errout << "sjasm : error FATAL : " << message << param << endl;
	else {
		errout << message << param << endl;
	}
	exit(exitCode);
}

int main(int argc, char *argv[]) {
  char zoekpad[MAX_PATH];
  char *p;
  int i=1;

  cout << "SjASM Z80 Assembler v0.39j" << endl;
  sourcefilename[0]=destfilename[0]=listfilename[0]=expfilename[0]=0;
  if (argc==1) {
    cout << "Copyright 2006 Sjoerd Mastijn - www.xl2s.tk" << endl;
	cout << "Copyright 2022 Konamiman - www.konamiman.com" << endl;
    cout << "\nUsage:\nsjasm [-options] sourcefile [targetfile [listfile [exportfile]]]\n";
    cout << "\nOption flags as follows:\n";
    cout << "  -l        Label table in listing\n";
    cout << "  -s        Generate .SYM symbol file\n";
    cout << "  -q        No listing\n";
    cout << "  -i<path>  Includepath\n";
	cout << "  -e        Send errors to standard error pipe\n";
	cout << "  -c        Enable Compass compatibility\n";
	cout << "  -v        Produce error messages with Visual Studio format\n";
	cout << "            (should be the first option)\n";
    exit(ERR_NO_INPUT);
  }

  GetCurrentDirectory(MAX_PATH,zoekpad);
  huidigzoekpad=zoekpad;

  getOptions(argv,i); if (argv[i]) strcpy(sourcefilename,argv[i++]);
  getOptions(argv,i); if (argv[i]) strcpy(destfilename,argv[i++]);
  getOptions(argv,i); if (argv[i]) strcpy(listfilename,argv[i++]);
  getOptions(argv,i); if (argv[i]) strcpy(expfilename,argv[i++]);
  getOptions(argv,i);

  if (!sourcefilename[0]) { ErrorAndExit("No inputfile", ERR_NO_INPUT); }
  if (!destfilename[0]) {
    strcpy(destfilename,sourcefilename);
    if (!(p=strchr(destfilename,'.'))) p=destfilename; else *p=0;
    strcat(p,".out");
  }
  if (!listfilename[0]) {
    strcpy(listfilename,sourcefilename);
    if (!(p=strchr(listfilename,'.'))) p=listfilename; else *p=0;
    strcat(p,".lst");
  }
  strcpy(symfilename,expfilename);
  if (!expfilename[0]) {
    strcpy(expfilename,sourcefilename);
    if (!(p=strchr(expfilename,'.'))) p=expfilename; else *p=0;
    strcat(p,".exp");
  }
  if (!symfilename[0]) {
    strcpy(symfilename,sourcefilename);
    if (!(p=strchr(symfilename,'.'))) p=symfilename; else *p=0;
    strcat(p,".sym");
  }

  Initpi();

  InitPass(1); OpenList(); OpenFile(sourcefilename);

  cout << "Pass 1 complete (" << nerror << " errors)" << endl;

  InitPass(2); OpenDest(); OpenFile(sourcefilename);

  if (labellisting) labtab.dump();

  cout << "Pass 2 complete" << endl;

  Close();

  if (symfile) labtab.dumpsym();

  cout << "Errors: " << nerror << endl << flush;

  return (nerror==0 ? 0 : ERR_BAD_CODE);
}
//eof sjasm.cpp
