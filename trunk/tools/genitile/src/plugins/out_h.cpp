///////////////////////////////////////////////////////////////////////////////
// Output h plugins for mdtt
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
	return "Header file output";
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
	return"h";
}

///////////////////////////////////////////////////////////////////////////////
// file extension

DLLEXPORT char* GetExt(void)
{
	return"h";
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

	out_file=fopen(filename,"wt");
	if(!out_file)
	{	return tgERR_SAVINGFILE;}

	switch(type)
	{
		case OUTPUT_TILES:
		{
			fprintf(out_file,"\nconst unsigned char %s[%d]={\n",name,size);
			for(int i=0;i<size/4;i++)
			{
				for(int j=0;j<4;j++)
					fprintf(out_file,"0x%.2x,",data[(i*4)+j]);
				fprintf(out_file,"\n");
			}
			fprintf(out_file,"\n};\n");

		}break;

		case OUTPUT_PAL:
		{
			fprintf(out_file,"\nconst unsigned short %s[%d]={\n",name,size/2);
			uint16 *d16=(uint16*) data;

			for(int i=0;i<size/2/16;i++)
			{	for(int j=0;j<16;j++)
					fprintf(out_file,"0x%.4x,",d16[(i*16)+j]);
				fprintf(out_file,"\n");
			}
			fprintf(out_file,"\n};\n");

		}break;

		case OUTPUT_MAP:
		{
			fprintf(out_file,"\nconst unsigned short %s[%d]={\n",name,size/2);
			uint16 *d16=(uint16*) data;

			int i=0;
			int b=0;

			while(i<(size/2))
			{
				if(b>15)
				{	b=0;
					fprintf(out_file,"\n");
				}
				fprintf(out_file,"0x%.4x,",d16[i]);
				b++;
				i++;
			}


			/*uint16 *d16=(uint16*) data;
			for(int i=0;i<size/2;i++)
			{
				fprintf(out_file,"\tdc.w $%.4x\n",d16[i]);
			}*/

			fprintf(out_file,"\n};\n");

		}break;

		default:
		{/*
			for(int i=0;i<size;i++)
			{	fprintf(out_file,"\tdc.b $%.2x\n",data[i]);
			}
		*/
		}break;


	}

	fclose(out_file);

	return tgOK;
}

