///////////////////////////////////////////////////////////////////////////////
// Output bmp plugins for mdtt
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

RGBQUAD	gRgbSquad[256];
BITMAPFILEHEADER gFileHeader;
BITMAPINFOHEADER gInfoHeader;

///////////////////////////////////////////////////////////////////////////////
// Get the description of the plugins

DLLEXPORT char* GetDescription(void)
{
	return "Bmp Output";
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
// Get id of the plugins (should be unique!)

DLLEXPORT char* GetID(void)
{
	return"bmp";
}

///////////////////////////////////////////////////////////////////////////////
// file extension

DLLEXPORT char* GetExt(void)
{
	return"bmp";
}

///////////////////////////////////////////////////////////////////////////////
// define the type of ouput

DLLEXPORT int GetOutputType(void)
{
	return tgOutputType_Picture;
}

///////////////////////////////////////////////////////////////////////////////
// Output the data

DLLEXPORT int OutputPicture(const char *filename,tgPictureInfo *info,uint8* pixels,tgColor *pal)
{

	FILE	*fp;
	int size=0;
	int off=0;

	gInfoHeader.biWidth=info->Width;
	gInfoHeader.biHeight=info->Height;
	gInfoHeader.biBitCount=8;

	fp=fopen(filename,"wb");
    if(!fp)
        return tgERR_SAVINGFILE;

	off=(sizeof(RGBQUAD)*256)+sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);
	size=(gInfoHeader.biWidth*abs(gInfoHeader.biHeight)*(gInfoHeader.biBitCount>>3));

	// init headers
	gFileHeader.bfType=19778;
	gFileHeader.bfSize=size+off;
	gFileHeader.bfReserved1=0;
	gFileHeader.bfReserved2=0;
	gFileHeader.bfOffBits=off;

	gInfoHeader.biSize=40;
	gInfoHeader.biPlanes=1;
	gInfoHeader.biCompression=0;
	gInfoHeader.biSizeImage=0;
	gInfoHeader.biXPelsPerMeter=0;
	gInfoHeader.biYPelsPerMeter=0;
	gInfoHeader.biClrUsed=0;
	gInfoHeader.biClrImportant=0;

	fwrite(&gFileHeader,sizeof(gFileHeader),1,fp);
	fwrite(&gInfoHeader,sizeof(gInfoHeader),1,fp);

	for(int i=0;i<256;i++)
	{	gRgbSquad[i].rgbBlue=pal[i].Blue;
		gRgbSquad[i].rgbRed=pal[i].Red;
		gRgbSquad[i].rgbGreen=pal[i].Green;
	}


	if(gInfoHeader.biBitCount==8)
	{	fwrite(&gRgbSquad,sizeof(RGBQUAD)*256,1,fp);
	}

	// flip
	if(gInfoHeader.biHeight>0)
	{
		int row_pitch=gInfoHeader.biWidth * (gInfoHeader.biBitCount>>3);
		for(int i=gInfoHeader.biHeight-1;i>=0;i--)
		{	fwrite(&pixels[row_pitch*i],row_pitch,1,fp);	}

	}
	else
	{	fwrite(pixels,size,1,fp);	}

	fclose(fp);

	return tgOK;
}

