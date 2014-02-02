/*-----------------------------------------------------------------------------*\
|																				|
|	Nemesis.dll: Compression / Decompression of data in Nemesis format			|
|	Copyright © 2002-2004 The KENS Project Development Team						|
|																				|
|	This library is free software; you can redistribute it and/or				|
|	modify it under the terms of the GNU Lesser General Public					|
|	License as published by the Free Software Foundation; either				|
|	version 2.1 of the License, or (at your option) any later version.			|
|																				|
|	This library is distributed in the hope that it will be useful,				|
|	but WITHOUT ANY WARRANTY; without even the implied warranty of				|
|	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU			|
|	Lesser General Public License for more details.								|
|																				|
|	You should have received a copy of the GNU Lesser General Public			|
|	License along with this library; if not, write to the Free Software			|
|	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA	|
|																				|
\*-----------------------------------------------------------------------------*/

#ifndef _NEMESIS_H_
#define _NEMESIS_H_

#include <windows.h>

HMODULE hNemesis=NULL;

long (__cdecl *NComp)(char *SrcFile, char *DstFile);
long (__cdecl *NDecomp)(char *SrcFile, char *DstFile, long Pointer);
long (__cdecl *NCompToBuf)(char *SrcFile, char *&DstBuffer, long *BufSize);
long (__cdecl *NDecompToBuf)(char *SrcFile, char *&DstBuffer, long *BufSize, long Pointer);
long (__cdecl *NFreeBuffer)(char *Buffer);

bool NInit(const char *DLL)
{
	hNemesis= LoadLibrary(DLL)	;
	if (!hNemesis) return false;

	NComp = (long (*)(char *, char *))GetProcAddress(hNemesis, "Comp");
	NDecomp = (long (*)(char *, char *, long))GetProcAddress(hNemesis, "Decomp");
	NCompToBuf = (long (*)(char *, char *&, long *))GetProcAddress(hNemesis, "CompToBuf");
	NDecompToBuf = (long (*)(char *, char *&, long *, long))GetProcAddress(hNemesis, "DecompToBuf");
	NFreeBuffer = (long (*)(char *))GetProcAddress(hNemesis, "FreeBuffer");

	if (NComp==NULL) return false;
	if (NDecomp==NULL) return false;
	if (NCompToBuf==NULL) return false;
	if (NDecompToBuf==NULL) return false;
	if (NFreeBuffer==NULL) return false;

	return true;
}


#endif /* _NEMESIS_H_ */
