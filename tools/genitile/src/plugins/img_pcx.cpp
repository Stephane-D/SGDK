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
// Header for pcx

typedef struct
{
	char			Mfg;               /* manufacturer, always 0xa0         */
	char			Ver;               /* encoder version number (5)        */
	char			Enc;               /* encoding code, always 1           */
	char			Bpp;               /* bits per pixel, 8 in mode 0x13    */
	unsigned short	Xmin,Ymin;         /* image origin, usually 0,0         */
	unsigned short	Xmax,Ymax;         /* image dimensions                  */
	unsigned short	Hres;              /* horizontal resolution value       */
	unsigned short	Vres;              /* vertical resolution value         */
	char			Pal[48];           /* palette (not in mode 0x13)        */
	char			Reserved;          /* who knows?                        */
	char			ClrPlanes;         /* number of planes, 1 in mode 0x13  */
	unsigned short	Bpl;               /* bytes per line, 80 in mode 0x13   */
	unsigned short	plType;            /* Grey or Color palette flag        */
	char			Filler[58];        /* Zsoft wanted a 128 byte header    */

}pcx_hdr;

///////////////////////////////////////////////////////////////////////////////
// Local variables

tgPictureInfo	Info;
tgColor			Colors[tgMAX_COLORS];
uint8			*Pixels;

///////////////////////////////////////////////////////////////////////////////
// Get the description of the plugins

DLLEXPORT char* GetDescription(void)
{
	return "ZSoft PCX";
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
	return"|*.pcx|";
}

///////////////////////////////////////////////////////////////////////////////
// Get the picture info

DLLEXPORT int GetPictureInfo(const char* pFileName,tgPictureInfo* pInfo)
{
	FILE		*in_file;
	struct		stat statbuf;
	pcx_hdr		*pcx_header;
	uint8		*pcx;
	uint8		byte;
	int			numbytes,runlen,cnt,size;
	int			x,xx,y,po;
	int			bytes_per_line=0;

	// check if a pcx
	if (stricmp(strrchr(pFileName, '.'), ".pcx") != 0)
	{	return tgERR_UNSUPPORTED;}

	// Get the length of the file
	if (stat(pFileName, &statbuf) != 0)
	{	return(tgERR_OPENINGFILE);}
	size= statbuf.st_size;

	// open file
	in_file=fopen(pFileName,"rb");
	if(in_file==NULL)
	{	return tgERR_OPENINGFILE;
	}

	// read header
	pcx=(unsigned char*)malloc(size);
	if(!pcx)
	{	return tgERR_OUTMEMORY;
	}
	fread(pcx,1,size,in_file);

	pcx_header=(pcx_hdr*)pcx;

	// check of attributes
	Info.Width			= pcx_header->Xmax+1;
	Info.Height			= pcx_header->Ymax+1;
	Info.BytesPerPixel	= pcx_header->ClrPlanes;
	Info.Pitch			= Info.Width * Info.BytesPerPixel;
	bytes_per_line		= pcx_header->Bpl;

	if(Info.BytesPerPixel!=1)
	{	return tgERR_BADBPP;}

	// Copy Attributes
	memcpy(pInfo,&Info,sizeof(tgPictureInfo));

	// allocate the memory
	numbytes= (Info.Width * Info.Height * Info.BytesPerPixel);

	Pixels=(unsigned char*)malloc(numbytes);
	if(!Pixels)
	{	return tgERR_OUTMEMORY;
	}

	// skip header
	pcx= pcx+ sizeof(pcx_hdr);

	// Do RLE Decoding
    for(y=0;y<Info.Height;y++)
    {   x=xx=0;
        po = 0;
        while (x < (bytes_per_line*Info.BytesPerPixel))
        {
            byte = *pcx++;
            if ((byte & 0xC0) == 0xC0)
            {   runlen = (byte & 0x3F);
                byte = *pcx++;
            }
            else
            {   runlen=1;
            }

            if(Info.BytesPerPixel==1)
            {   while (runlen--)
                {   if (x < Info.Width)
                        Pixels[(y*Info.Width)+x] = byte;
                    x++;
	            }
            }
            else
            {
				while (runlen--)
	 	        {
                    if (xx < Info.Width)
                    {
						Pixels[(y*Info.Width)+(xx)+po] = byte;
                    }

        	        x++;
        	        if (x == bytes_per_line)
	                {
                        xx = 0;
            		  	po = 1;
        	        }
	                else
        	        {
                        if (x == bytes_per_line*2)
            	      	{   xx = 0;
		  	    	        po = 2;
        	            }
	                    else
		  	                xx++;
                    }
                }
	        }
        }
    }

	// if paletized
	if(pcx_header->ClrPlanes ==1)
	{
		pcx++;

		for (cnt=0; cnt<256; cnt++)
		{
			Colors[cnt].Red	=	pcx[cnt*3+0];
			Colors[cnt].Green = pcx[cnt*3+1];
			Colors[cnt].Blue=	pcx[cnt*3+2];
		}
	}

	// close the pcx
	fclose(in_file);

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
