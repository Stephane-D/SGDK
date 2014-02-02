///////////////////////////////////////////////////////////////////////////////
// Output asm plugins for mdtt
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
// Local variables


///////////////////////////////////////////////////////////////////////////////
// Get the description of the plugins

DLLEXPORT char* GetDescription(void)
{
	return "68k asm output";
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
	return"asgcc";
}

///////////////////////////////////////////////////////////////////////////////
// file extension

DLLEXPORT char* GetExt(void)
{
	return"s";
}

///////////////////////////////////////////////////////////////////////////////
// define the type of ouput

DLLEXPORT int GetOutputType(void)
{
	return tgOutputType_Raw;
}

///////////////////////////////////////////////////////////////////////////////
// Output the data

DLLEXPORT int OutputData(const char *filename,const char *name,int type,uint8* data,int size)
{
	FILE *out_file=NULL;
    char* filename_h = strdup(filename);
    char* nameup = strupr(strdup(name));

    filename_h[strlen(filename_h) - 1] = 'h';

	out_file=fopen(filename,"wt");
	if(!out_file)
	{	return tgERR_SAVINGFILE;}

	fprintf(out_file,"* ---------------------------\n");
	fprintf(out_file,"\t.global %s\n", name);
	fprintf(out_file,"%s:\n",name);
	fprintf(out_file,"* ---------------------------\n");
	fprintf(out_file,"* size:%d bytes\n\n",size);

	switch(type)
	{
		case OUTPUT_TILES:
		{
			uint32* d32=(uint32*) data;
			for(int i=0;i<size/4;i++)
			{	if((i&7)==0)
					fprintf(out_file,"\n* ---------------------------\n");

				fprintf(out_file,"\tdc.l 0x%.8x\n",d32[i]);
			}

		}break;

		case OUTPUT_PAL:
		{
			uint16 *d16=(uint16*) data;
			for(int i=0;i<size/2;i++)
			{
				if((i&15)==0)
					fprintf(out_file,"\n* ---------------------------\n");

				fprintf(out_file,"\tdc.w 0x%.4x\n",d16[i]);
			}

		}break;

		case OUTPUT_MAP:
		{
			uint16 *d16=(uint16*) data;
			for(int i=0;i<size/2;i++)
			{
				fprintf(out_file,"\tdc.w 0x%.4x\n",d16[i]);
			}

		}break;

		default:
		{
			for(int i=0;i<size;i++)
			{	fprintf(out_file,"\tdc.b 0x%.2x\n",data[i]);
			}

		}break;


	}

	fclose(out_file);

	out_file=fopen(filename_h,"wt");
	if(!out_file)
	{	return tgERR_SAVINGFILE;}

	fprintf(out_file,"#ifndef %s_H\n", nameup);
	fprintf(out_file,"#define %s_H\n", nameup);
	fprintf(out_file,"\n");

	switch(type)
	{
		case OUTPUT_TILES:
            fprintf(out_file,"extern const u32 %s[%d];\n",name, size/4);
            break;

		case OUTPUT_PAL:
		case OUTPUT_MAP:
            fprintf(out_file,"extern const u16 %s[%d];\n",name, size/2);
            break;

		default:
            fprintf(out_file,"extern const u8 %s[%d];\n",name, size/1);
            break;
    }

	fprintf(out_file,"\n");
	fprintf(out_file,"#endif //%s_H\n", nameup);

	return tgOK;
}

