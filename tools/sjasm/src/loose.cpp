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

// loose.cpp

#ifndef WIN32

#include "sjasm.h"

void GetCurrentDirectory(int whatever, char *pad) {
  pad[0]=0;
}

int SearchPath(char*oudzp,char*filename,char*whatever,int maxlen,char*nieuwzp,char**ach) {
  FILE *fp;
  char *p,*f;
  if (filename[0]=='/') strcpy(nieuwzp,filename);
  else { 
    strcpy(nieuwzp,oudzp); 
    if (*nieuwzp && nieuwzp[strlen(nieuwzp)]!='/') strcat(nieuwzp,"/");
    strcat(nieuwzp,filename); 
  }
  if (ach) {
    p=f=nieuwzp;
    while (*p) { if (*p=='/') f=p+1; ++p; }
    *ach=f;
  }
  fp=fopen(nieuwzp,"r"); if (fp) fclose(fp);
  return (long)fp;
}

#endif

//eof loose.cpp
