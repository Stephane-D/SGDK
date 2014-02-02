///////////////////////////////////////////////////////////////////////////////
// Pcx picture plugins for tilegt
//
//
//
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Includes

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>

#include "../mdttSDK.h"

///////////////////////////////////////////////////////////////////////////////
// Header

///////////////////////////////////////////////////////////////////////////////
// Local variables

tgPictureInfo	Info;
tgColor			Colors[tgMAX_COLORS];
uint8			*Pixels;

RGBQUAD	gRgbSquad[256];
BITMAPFILEHEADER gFileHeader;
BITMAPINFOHEADER gInfoHeader;

///////////////////////////////////////////////////////////////////////////////
// Get the description of the plugins

DLLEXPORT char* GetDescription(void)
{
	return "Bimap";
}

///////////////////////////////////////////////////////////////////////////////
// Get the author name

DLLEXPORT char* GetAuthor(void)
{
	return "Spoutnick Team";
}

///////////////////////////////////////////////////////////////////////////////
// Get author contact info

DLLEXPORT char* GetContactInfo(void)
{
	return "www.spoutnickteam.com";
}

///////////////////////////////////////////////////////////////////////////////
// Get build version

DLLEXPORT char* GetBuildVersion(void)
{
	return "1.0";
}

///////////////////////////////////////////////////////////////////////////////
// Get supported files extension

DLLEXPORT char* GetSupportedExt(void)
{
	return"*.bmp";
}

///////////////////////////////////////////////////////////////////////////////
// Get the picture info

DLLEXPORT int GetPictureInfo(const char* pFileName,tgPictureInfo* pInfo)
{
	FILE *fp;
	unsigned char* data=NULL;
	int size=0;
	struct	stat statbuf;
	unsigned char* data_inc=NULL;

	// check if bmp
	if (stricmp(strrchr(pFileName, '.'), ".bmp") != 0)
	{	return tgERR_UNSUPPORTED;}

	// get the size
	if (stat(pFileName, &statbuf) != 0)
	{	return tgERR_OPENINGFILE;}
	size= statbuf.st_size;

	data=(unsigned char*) malloc(size);
	if(!data)
		return tgERR_OUTMEMORY;

	// read
	fp=fopen(pFileName,"rb");
	if(fp==NULL) return tgERR_OPENINGFILE;

	if(fread(data,size,1,fp)!=1)
		return tgERR_OPENINGFILE;

	// point to the structures
	data_inc=data;

	memcpy(&gFileHeader,data,sizeof(gFileHeader));
	data_inc+=sizeof(BITMAPFILEHEADER);

	memcpy(&gInfoHeader,data_inc,sizeof(gInfoHeader));
	data_inc+=sizeof(gInfoHeader);

	// check if compressed
	if(gInfoHeader.biCompression!=0)
		return tgERR_COMPRESSION;

	if(gInfoHeader.biBitCount==1)
		return tgERR_BADBPP;

	// load the palette
	if(gInfoHeader.biBitCount==8)
	{	memcpy(&gRgbSquad,data_inc,sizeof(RGBQUAD)*256);}

	Info.Width = gInfoHeader.biWidth;
	Info.Height= gInfoHeader.biHeight;
	Info.BytesPerPixel=1;
	Info.Pitch=Info.Width;

	pInfo->Width = gInfoHeader.biWidth;
	pInfo->Height= gInfoHeader.biHeight;
	pInfo->BytesPerPixel=1;
	pInfo->Pitch=Info.Width;

	// load the pixels
	data_inc=data + gFileHeader.bfOffBits;

	Pixels=(uint8*)malloc(gInfoHeader.biWidth*abs(gInfoHeader.biHeight)*gInfoHeader.biBitCount>>3);
	if(!Pixels)
		return tgERR_OUTMEMORY;

	// flip ?
	if(gInfoHeader.biHeight>0)
	{
		int row_pitch=gInfoHeader.biWidth * (gInfoHeader.biBitCount>>3);
		int cnt=0;

		for(int i=gInfoHeader.biHeight-1;i>=0;i--)
		{	memcpy(&Pixels[row_pitch*cnt],&data_inc[row_pitch*i],row_pitch);
			cnt++;
		}

	}
	else
	{	memcpy(Pixels,data_inc,size);}

	// copy the palette
	for(int i=0;i<256;i++)
	{	Colors[i].Red = gRgbSquad[i].rgbRed;
		Colors[i].Blue = gRgbSquad[i].rgbBlue;
		Colors[i].Green = gRgbSquad[i].rgbGreen;
	}

	// clear the readed data
	if(data)
		free(data);

	fclose(fp);

	return tgOK;
}

///////////////////////////////////////////////////////////////////////////////
// Get the picture data

DLLEXPORT int GetPictureData(uint8* Data,tgColor* Palette)
{
	// copy pixels
	memcpy(Data,Pixels, Info.Width * Info.Height * Info.BytesPerPixel);

	// copy palette
	if(Info.BytesPerPixel==1)
		memcpy(Palette,Colors,tgMAX_COLORS * sizeof(tgColor));
	else
		Palette=NULL;

	return tgOK;
}
