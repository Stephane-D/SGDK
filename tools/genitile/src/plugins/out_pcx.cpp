///////////////////////////////////////////////////////////////////////////////
// Output pcx plugins for mdtt
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
	unsigned short	Hscreensize;	   /* hscreen siz 						*/
	unsigned short  Vscreensize;	   /*									*/
	char			Filler[54];        /* Zsoft wanted a 128 byte header    */

}pcx_hdr;

///////////////////////////////////////////////////////////////////////////////
// Local variables


///////////////////////////////////////////////////////////////////////////////
// Get the description of the plugins

DLLEXPORT char* GetDescription(void)
{
	return "ZSoft Pcx Output";
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
	return"pcx";
}

///////////////////////////////////////////////////////////////////////////////
// file extension

DLLEXPORT char* GetExt(void)
{
	return"pcx";
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

	FILE		*out_file;
	pcx_hdr		pcx_header;
    int         planes=1;
    int runcount;
    unsigned char runchar;
    unsigned char ch;

    //-- check existence --
    if(info->Width<=0 || info->Height<=0 || info->BytesPerPixel<=0)
	    return 0;

     //-- open file --
    out_file=fopen(filename,"wb");
    if(!out_file)
        return tgERR_SAVINGFILE;

    //-- Création du header --
    memset(&pcx_header,0,sizeof(pcx_hdr));
    pcx_header.Mfg  =   10;
    pcx_header.Ver  =   5;
    pcx_header.Enc  =   1;
    pcx_header.Bpp  =   8;
    pcx_header.Xmin =   0;
    pcx_header.Ymin =   0;
    pcx_header.Xmax =   info->Width-1;
    pcx_header.Ymax =   info->Height-1;
    pcx_header.Hres =   320;
    pcx_header.Vres =   200;
    pcx_header.Reserved     =   0;
    pcx_header.ClrPlanes    =   1;
    pcx_header.Bpl  =   info->Width;
    pcx_header.plType   =   1;
    pcx_header.Hscreensize  = info->Width;
    pcx_header.Vscreensize  = info->Height;

    fwrite(&pcx_header,sizeof(pcx_header),1,out_file);

    //--
    for(int y=0; y<info->Height; y++)
    {
        runcount = 0;
        runchar = 0;
        for(int x=0; x<info->Width*planes; x++)
        {
            ch = pixels[(y*info->Width)+x];

    	    if (runcount==0)
            {   runcount = 1;
	            runchar = ch;
	        }
	        else
            {
                if((ch != runchar) || (runcount >= 0x3f))
                {
	                if ((runcount > 1) || ((runchar & 0xC0) == 0xC0))
                        fputc((0xC0 | runcount),out_file);

    	            fputc(runchar,out_file);
	                runcount = 1;
	                runchar = ch;
	            }
	            else
	                runcount++;
    	    }
        }

        if((runcount > 1) || ((runchar & 0xC0) == 0xC0))
            fputc((0xC0 | runcount),out_file);

        fputc(runchar,out_file);
    }


    /* 256 color palette */
    fputc(12,out_file);

    for(int i=0; i<256; i++)
    {
        fputc(pal[i].Red,out_file);
        fputc(pal[i].Green,out_file);
        fputc(pal[i].Blue,out_file);
    }

    //-- fermeture du fichier --
    fclose(out_file);

   return tgOK;

}

