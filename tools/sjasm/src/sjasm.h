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

// sjasm.h

//#define METARM
//#define SECTIONS

#ifndef SjINCLUDE
#define SjINCLUDE

#ifdef WIN32
#include <windows.h>
#else
#include "loose.h"
#endif
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
using std::flush;
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define errout (useStdError ? cerr : cout)

#define ERR_BAD_CODE 1
#define ERR_OPEN_FILE 2
#define ERR_FATAL 3
#define ERR_NO_INPUT 4
#define ERR_INVALID_OPTION 5

#define LINEMAX 300
#define LABMAX 70
#define LABTABSIZE 32771
#define FUNTABSIZE 4497
#define aint long

extern char filename[],*lp,line[],temp[],*tp,pline[],eline[],*bp;
extern int pass,labelnotfound,nerror,include,running,labellisting,listfile,donotlist,listdata,listmacro;
extern int useStdError;
extern int useVsErrorFormat;
extern int compassCompatibilityEnabled;
extern int insideCompassStyleMacroDefinition;
extern int macronummer,lijst,reglenwidth,synerr,symfile;
extern aint adres,mapadr,gcurlin,lcurlin,curlin,destlen,size,preverror,maxlin,comlin;
extern FILE *input;
extern void (*piCPUp)(void);
extern char destfilename[],listfilename[],sourcefilename[],expfilename[],symfilename[];
extern char *modlabp,*vorlabp,*macrolabp;

void ReplaceAtToUnderscore(char* string);

void ErrorAndExit2(char* message, char* param, int exitCode);
#define ErrorAndExit(message, exitCode) ErrorAndExit2(message, "", exitCode)
#define ErrorOpeningFile(fname) ErrorAndExit2("Error opening file: ", fname, ERR_OPEN_FILE)

#define IsNotValidIdChar(p) (!isalnum(p) && p!='_' && p!='.' && p!='?' && p!='!' && p!='#' && p!='@')
#define IsValidIdChar(p) (!IsNotValidIdChar(p))

extern FILE *listfp;

#ifdef SECTIONS
enum sections { TEXT, DATA, POOL };
extern sections section;
#endif
#ifdef METARM
enum cpus { ARM, THUMB, Z80 };
extern cpus cpu;
#endif
enum structmembs { SMEMBUNKNOWN,SMEMBALIGN,SMEMBBYTE,SMEMBWORD,SMEMBBLOCK, SMEMBDWORD, SMEMBD24, SMEMBPARENOPEN, SMEMBPARENCLOSE };
extern char *huidigzoekpad;

#include "reader.h"
#include "tables.h"
extern stringlst *lijstp;
#include "sjio.h"

extern labtabcls labtab;
extern loklabtabcls loklabtab;
extern definetabcls definetab;
extern macdefinetabcls macdeftab;
extern macrotabcls macrotab;
extern structtabcls structtab;
extern adrlst *maplstp;
extern stringlst *modlstp,*dirlstp;
#ifdef SECTIONS
extern pooldatacls pooldata;
extern pooltabcls pooltab;
#endif

#include "parser.h"
#include "piz80.h"
#ifdef METARM
#include "piarm.h"
#include "pithumb.h"
#endif
#include "direct.h"

#endif
//eof sjasm.h
