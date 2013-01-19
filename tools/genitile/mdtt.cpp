#define _CRT_SECURE_NO_DEPRECATE

///////////////////////////////////////////////////////////////////////////////
// standard includes
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "lz77.h"
#include "nemesis.h"

///////////////////////////////////////////////////////////////////////////////
// Define
#define PLUGINS_DIR					"plugins"
#define	optTILESET					"-tileset"
#define	optECHO						"-noecho"
#define optOUT						"-out"
#define optTAG						'-'

#define	TW	8
#define TH	8

#define	TILE	1
#define PAL		2
#define TILESET	4
#define	SPRITE	8
#define MAP		16

///////////////////////////////////////////////////////////////////////////////
// app includes
#include "tgApp.h"
#include "tgTools.h"
#include "mdttSDK.h"

///////////////////////////////////////////////////////////////////////////////
// Command lines arguments
/*
typedef struct
{	char Name[20];
	char ShortCut[8];
	bool GotArg;
	bool IsString;
	char StringArg[256];
	int IntArg;
	bool Set;
	char HelpText[256];
}tgArg;
*/
tgArg gLineArgs[]={
	{	"-tileset","-t",true,true,"",-1,false,	""},
	{	"-out","-o",true,true,"bin",-1,false,	""},
	{	"-pal","-p",true,false,"",0,false,		""},
	{	"-map","-m",false,false,"",-1,false,	""},
	{	"-noecho","-ne",false,false,"",-1,false,""},
	{	"-outdir","-od",true,true,"",-1,false,	""},
	{	"-sprw","-sw",true,false,"",1,false,	""},
	{	"-sprh","-sh",true,false,"",1,false,	""},
	{	"-notile","-nt",false,false,"",-1,false,""},
	{	"-nbpal","-np",true,false,"",1,false,	""},
	{	"-shadow","-s",true,false,"",0,false,	""},
	{	"-mapoffset","-mo",true,false,"",0,false,	""},
	{	"-mappal","-mp",true,false,"",0,false,""},
	{	"-mapprio","-mpr",true,true,"",-1,false,""},

	{	"-mapp","-mpp",true,false,"",0,false,	""},

	{	"-crpx","-cx",true,false,"",0,false,	""},
	{	"-crpy","-cy",true,false,"",0,false,	""},
	{	"-crpw","-cw",true,false,"",1,false,	""},
	{	"-crph","-ch",true,false,"",1,false,	""},
	{	"-oname","-on",true,true,"",-1,false,	""},
	{	"-lz77","-lz",false,false,"",-1,false,	""},
	{	"-pakt","-pt",false,false,"",-1,false,	""},
	{	"-pakm","-pm",false,false,"",-1,false,	""},
	{	"-nem","-nm",false,false,"",-1,false,	""},
	{	"-noopt","-noo",false,false,"",-1,false,""},
	{	NULL,NULL,false,false,NULL,NULL,NULL,NULL},
};

#define ID_TILESET		0
#define ID_OUT			1
#define ID_PAL			2
#define ID_MAP			3
#define ID_NOECHO		4
#define ID_OUTDIR		5
#define ID_SPRW			6
#define ID_SPRH			7
#define ID_NOTILE		8
#define ID_NBPAL		9
#define ID_SHADOW		10
#define ID_MAPOFFSET	11
#define ID_MAPPAL		12
#define ID_MAPPRIORITY	13

#define ID_MAPW			14

#define ID_CROPX		15
#define ID_CROPY		16
#define ID_CROPW		17
#define ID_CROPH		18
#define ID_OUTNAME		19
#define ID_LZ77			20
#define ID_PAKT			21
#define ID_PAKM			22
#define ID_NEMESIS		23
#define ID_NOOPTIMISATION	24

#define	dH	1
#define dV	2

///////////////////////////////////////////////////////////////////////////////
// Locals


int gSpw=1,gSph=1;
int gCropx=0,gCropy=0,gCropw=0,gCroph=0,gMapw=0,gMaph=0;
tgApp			*gApp=NULL;
int				gNoEcho=0;
char			gImgFileName[256]={""};
char			gFilePriority[256]={""};
char			gWildCard[256]={""};
int				gMode=0;

///////////////////////////////////////////////////////////////////////////////
// display message
void echo(char *fmt, ...)
{
	if(gNoEcho==0)
	{
		va_list ap;
		char token[0x100];
		char msg[0x100];

		strcpy(msg, "\0");
		va_start(ap, fmt);
		vsprintf(token, fmt, ap);
		strcat(msg, token);
		va_end(ap);

		printf(msg);
	}
}

void abort(char *fmt, ...)
{
	va_list ap;
	char token[0x100];
	char msg[0x100];
	char msg2[0x100];

	strcpy(msg, "\0");
	va_start(ap, fmt);
	vsprintf(token, fmt, ap);
	strcat(msg, token);
	va_end(ap);

	strcpy(msg2,"*ABORDED*: ");
	strcat(msg2,msg);

	printf(msg2);
}

///////////////////////////////////////////////////////////////////////////////
// error
void print_error(int id)
{
	switch(id)
	{
		case tgERR_OUTMEMORY:
		{	abort("can not allocate memory\n");	}break;

		case tgERR_OPENINGFILE:
		{	abort("can not open file(s)\n");	}break;

		case tgERR_SAVINGFILE:
		{	abort("can not save file(s)\n");	}break;

		case tgERR_BADBPP:
		{	abort("bad bpp, should be 8 or 24\n");	}break;

		case tgERR_FILEUNSUPPORTED:
		{	abort("format is unsupported\n");	}break;

		case tgERR_COPYDATA:
		{	abort("unable to copy memory chunk\n");	}break;

		case tgERR_NOOUTPUT:
		{	abort("not any output plugin found\n");}break;

		case tgERR_NOINPUT:
		{	abort("not any input file found\n");}break;

		default:
		{	abort("i don't know why\n");	}break;
	}
}

///////////////////////////////////////////////////////////////////////////////
// get the position of the argument
int checkarg_pos(int argc,char** argv,char* id)
{
	int i;
	int ret=-1;
	for(i=1;i<argc && ret==-1;i++)
	{	// found
		if(stricmp(id,argv[i])==0)
		{	ret=i;	}
	}
	return ret;
}

///////////////////////////////////////////////////////////////////////////////
// initialize arguments value
void read_arg(int argc,char** argv,int pos,tgArg *a)
{
	if(pos!=-1)
	{	if(pos==argc-1)
		{	// no value

		}
		else
		{	// check if got the argument
			if(a->GotArg==true)
			{	if(argv[pos+1][0]!=optTAG)
				{	if(a->IsString==true)
					{	strcpy(a->StringArg,argv[pos+1]);
					}
					else
					{	a->IntArg = atoi(argv[pos+1]);
					}
				}
			}
		}
	}
}

void read_args(int argc,char** argv,tgArg *args)
{
	int i=0;
	int idx=-1;
	tgArg *a=&args[i++];
	while(strlen(a->Name)>0)
	{
		// look for the arg
		idx=checkarg_pos(argc,argv,a->Name);
		if(idx!=-1)
		{	// found
			read_arg(argc,argv,idx,a);
			a->Set=true;
		}
		else
		{	// not found try to look for the abbreviated one
			if(strlen(a->ShortCut)>0)
			{
				idx=checkarg_pos(argc,argv,a->ShortCut);
				if(idx!=-1)
				{	// found
					read_arg(argc,argv,idx,a);
					a->Set=true;
				}
			}
		}
		a=&args[i++];
	}
}

///////////////////////////////////////////////////////////////////////////////
// initialize configuration and check
int check_config(int argc,char** argv)
{
	int i=0;
	int err=1;

	// echo
	gNoEcho= gLineArgs[ID_NOECHO].Set==true?1:0;

	// output plugins
	if(gApp->IsOutputSupported(gLineArgs[ID_OUT].StringArg)==0)
	{	if(gApp->GetOutputWriter()!=NULL)
		{	strcpy(gLineArgs[ID_OUT].StringArg,gApp->GetOutputWriter()->GetID());
			abort("output specified not found\n");
			err=0;
		}
	}

	// check if output dir exists
	if(gLineArgs[ID_OUTDIR].Set && strlen(gLineArgs[ID_OUTDIR].StringArg)>0)
	{
		WIN32_FIND_DATA FindData;
		HANDLE hFirst;
		char path[256]={""};

		strcpy(path,gLineArgs[ID_OUTDIR].StringArg);
		if(path[strlen(path)]!=optTAG)
		{	strcat(path,"\\");}
		remove_car(path,'"');
		strcpy(gLineArgs[ID_OUTDIR].StringArg,path);
		strcat(path,"*.*");

		if((hFirst = FindFirstFile (path, &FindData)) == INVALID_HANDLE_VALUE)
		{	abort("output directory doesn't exist\n");
			err=0;
		}
	}

	// sprite size
	if(gLineArgs[ID_SPRW].IntArg==0)
	{	abort("sprite width invalid\n");
		err=0;
	}

	if(gLineArgs[ID_SPRH].IntArg==0 )
	{	abort("sprite height invalid\n");
		err=0;
	}

	if(gLineArgs[ID_SPRH].IntArg>4)
	{	abort("sprite height can not be bigger than 4\n");
		err=0;
	}

	if(gLineArgs[ID_SPRW].IntArg>4)
	{	abort("sprite width can not be bigger than 4\n");
		err=0;
	}

	if(gLineArgs[ID_PAL].IntArg>15)
	{	abort("pal index should be lower than 15\n");
		err=0;
	}

	if(gLineArgs[ID_SHADOW].IntArg!=0 && gLineArgs[ID_SHADOW].IntArg!=1)
	{	abort("-shadow value can only be 0 or 1\n");
		err=0;
	}

	if(gLineArgs[ID_MAPPAL].IntArg<0 && gLineArgs[ID_MAPPAL].IntArg>3)
	{	abort("-mappal value can only be within 0 and 3\n");
		err=0;
	}



	if(((gLineArgs[ID_PAL].IntArg*16)+(gLineArgs[ID_NBPAL].IntArg*16))>256)
	{	abort("-pal index combine with -nbpal is out of bounds\n");
		err=0;
	}

	// tiled
	if(gLineArgs[ID_SPRW].IntArg>1 || gLineArgs[ID_SPRH].IntArg>1)
	{	if(gLineArgs[ID_MAP].Set==true || gLineArgs[ID_SHADOW].Set==true || gLineArgs[ID_TILESET].Set==true || gLineArgs[ID_MAPOFFSET].Set==true || gLineArgs[ID_MAPPAL].Set==true || gLineArgs[ID_MAPPRIORITY].Set==true || gLineArgs[ID_NOOPTIMISATION].Set==true)
		{	abort("-map,-shadow,-mappal,-mapoffset,-mapprio,-noopt and -tileset invalid in sprite mode\n");
			err=0;
		}
	}

	// export en image uniquement pour les tilesets
	if(gLineArgs[ID_TILESET].Set==true)
	{
		if( gLineArgs[ID_LZ77].Set==true || gLineArgs[ID_PAKT].Set==true  || gLineArgs[ID_PAKM].Set==true || gLineArgs[ID_NEMESIS].Set==true || gLineArgs[ID_PAL].Set==true || gLineArgs[ID_MAP].Set==true ||
			gLineArgs[ID_SPRW].Set==true || gLineArgs[ID_SPRH].Set==true || gLineArgs[ID_MAPOFFSET].Set==true || gLineArgs[ID_MAPPAL].Set==true || gLineArgs[ID_MAPPRIORITY].Set==true  )
		{
			if(gApp->GetOutputWriter()!=NULL && gApp->GetOutputWriter()->GetOutputType()!=tgOutputType_Raw )
			{
				if(gApp->GetOutputWriter()->GetOutputType()==tgOutputType_Picture)
				{	abort("you can not use -pal,-map,-mp,-sw,-sh,-mo,-mpr,-sx,-sy,-lz77,-nem switches with a picture output\n");
					err=0;
				}
			}
		}
	}

	return err;
}

///////////////////////////////////////////////////////////////////////////////
// Load picture

int load_picture(char* filename,uint8 **pix,tgColor *pal,tgPictureInfo *info,int *size,int AllowCrop)
{
	int err=0;

	if(gApp->IsImgLoadingSupported(filename))
	{
		// load the picture
		err=gApp->GetPictureInfo(filename,info);
		if(err!=tgOK)
		{	print_error(err);
			return 0;
		}

		if(info->BytesPerPixel!=1)
		{	print_error(tgERR_BADBPP);
			return 0;
		}

		if( (gLineArgs[ID_CROPY].Set==false && gLineArgs[ID_CROPX].Set==false && gLineArgs[ID_CROPW].Set==false && gLineArgs[ID_CROPH].Set==false) || AllowCrop==0 )
		{

			if(gLineArgs[ID_NOTILE].Set==false)
			{
				if(info->Width==0)
				{	abort("picture %s width is wrong  for tiles generation\n",filename);
					return 0;
				}

				if(info->Height==0)
				{	abort("picture %s height is wrong  for tiles generation\n",filename);
					return 0;
				}

				if(info->Width % (gSpw * TW)!=0)
				{	abort("picture %s width is wrong  for tiles generation\n",filename);
					return 0;
				}

				if(info->Height % (gSph * TH)!=0)
				{	abort("picture %s height is wrong  for tiles generation\n",filename);
					return 0;
				}
			}

			// allocate memory
			if(info->Pitch!=0)
				*size=info->Pitch * info->Height;
			else
				*size=info->Width * info->Height * info->BytesPerPixel;

			*pix =(uint8*) malloc(*size);
			if(!pix)
			{	print_error(tgERR_OUTMEMORY);
				return 0;
			}

			// get the data
			err=gApp->GetPictureData(*pix,pal);
			if(err!=tgOK)
			{	print_error(err);
				return 0;
			}

			// image loaded sucessfully
			echo("Loaded image details:\n");
			echo("Filename        : %s\n",filename);
			echo("Width           : %d\n",info->Width);
			echo("Height          : %d\n",info->Height);
		}
		else
		{	// en mode sprite
			if(gLineArgs[ID_CROPX].Set==true || gLineArgs[ID_CROPY].Set==true || gLineArgs[ID_CROPW].Set==true || gLineArgs[ID_CROPH].Set==true )
			{	int l = gCropx;
				int t = gCropy;
				int r = gCropx + gCropw * TW;
				int b = gCropy + gCroph * TH;

				if( (gCropw * TW)> info->Width )
				{	abort("cropping width overlap picture bounds\n");
					return 0;
				}

				if( (gCroph * TH)> info->Height )
				{	abort("cropping width overlap picture bounds\n");
					return 0;
				}


				if( ((r-l)%TW)!=0)
				{	abort("incorrect cropping %s width for tile generation\n",filename);
					return 0;
				}

				if( ((b-t)%TH)!=0)
				{	abort("incorrect cropping %s height for tile generation\n",filename);
					return 0;
				}

				if(l>info->Width || b>info->Height)
				{	abort("cropping coords out of bounds\n",filename);
					return 0;
				}

				if((r-l) % (gSpw * TW)!=0)
				{	abort("picture %s width is wrong  for tiles generation\n",filename);
					return 0;
				}

				if((b-t) % (gSph * TH)!=0)
				{	abort("picture %s height is wrong  for tiles generation\n",filename);
					return 0;
				}

				// allocate memory
				if(info->Pitch!=0)
					*size=info->Pitch * info->Height;
				else
					*size=info->Width * info->Height * info->BytesPerPixel;

				*pix =(uint8*) malloc(*size);
				if(!pix)
				{	print_error(tgERR_OUTMEMORY);
					return 0;
				}
				// get the data
				err=gApp->GetPictureData(*pix,pal);
				if(err!=tgOK)
				{	print_error(err);
					return 0;
				}

				*size=	(r-l) * (b-t) * info->BytesPerPixel;
				uint8 *pix_crop =(uint8*) malloc(*size);
				if(!pix_crop)
				{	print_error(tgERR_OUTMEMORY);
					return 0;
				}

				for(int j=0;j<(b-t);j++)
				{	for(int i=0;i<(r-l);i++)
					{	pix_crop[(j*(r-l))+i]=(*pix)[((j+gCropy)*info->Width)+(i+gCropx)];
					}
				}

				// crop
				info->BytesPerPixel=1;
				info->Height=(b-t);
				info->Width=(r-l);
				info->Pitch=(info->Width);
				free(*pix);
				*pix=pix_crop;

				// image loaded sucessfully
				echo("Cropped image details:\n");
				echo("Filename        : %s\n",filename);
				echo("Width           : %d\n",info->Width);
				echo("Height          : %d\n",info->Height);

			}
		}
	}
	else
	{	print_error(tgERR_FILEUNSUPPORTED);
		return 0;
	}

	return tgOK;

}

///////////////////////////////////////////////////////////////////////////////
// generate the tileset

int look_for_tile(uint8 *src,uint8 *dst,int &cnt,tgPictureInfo *info,int &dir,bool add)
{
	int contloop=0;
	int contsearch=1;
	int tile_num=-1;

	unsigned src_col;
	unsigned dst_col;
	unsigned char *tile_data=NULL;
	dir=0;

	if(cnt==0 && add==true)
		tile_num=0;

	for(int tiles_cnt=0;tiles_cnt<cnt && contsearch==1;tiles_cnt++)
	{
		tile_data = &dst[tiles_cnt*TW*TH];
		contloop=1;

		for(int y=0;y<TH && contloop==1;y++)
		{	for(int x=0;x<TW && contloop==1;x++)
			{	src_col = src[(y*info->Width)+x];
				dst_col = tile_data[(y*TW)+x];
				if((src_col&0xf) !=(dst_col&0xf))
				{	contloop=0;

				}
			}
		}

		if(contloop==1)
			dir=0;

		// flip V
		if(contloop==0)
		{	contloop=1;
			for(int y=0;y<TH && contloop==1;y++)
			{
				for(int x=0;x<TW && contloop==1;x++)
				{	src_col = src[(y*info->Width)+x];
					dst_col = tile_data[((TH-1-y)*TW)+x];
					if((src_col&0xf) !=(dst_col&0xf))
					{	contloop=0;

					}
				}
			}

			if(contloop==1)
			{	dir|=dV;

			}
		}

		// flip H
		if(contloop==0)
		{	contloop=1;
			for(int y=0;y<TH && contloop==1;y++)
			{	for(int x=0;x<TW && contloop==1;x++)
				{	src_col = src[(y*info->Width)+x];
					dst_col = tile_data[(y*TW)+ (TW-x-1)];
					if((src_col&0xf) !=(dst_col&0xf))
					{	contloop=0;
					}
				}
			}

			if(contloop==1)
			{	dir|=dH;

			}
		}

		// flip V & H
		if(contloop==0)
		{	contloop=1;
			for(int y=0;y<TH && contloop==1;y++)
			{	for(int x=0;x<TW && contloop==1;x++)
				{	src_col = src[(y*info->Width)+x];
					dst_col = tile_data[((TH-1-y)*TW)+ (TW-x-1)];
					if((src_col&0xf) !=(dst_col&0xf))
					{	contloop=0;
					}
				}
			}

			if(contloop==1)
				dir|=dV | dH;
		}

		if(contloop==1)
		{	contsearch=0;
			tile_num=tiles_cnt;
		}
	}

	// le tile a pas été trouvé, le copie dans le tileset
	if(gLineArgs[ID_NOOPTIMISATION].Set==false)
	{
		if(contloop==0 && add==true)
		{	tile_data = &dst[cnt*TW*TH];
			for(int y=0;y<TH;y++)
			{	for(int x=0;x<TW;x++)
				{	tile_data[(y*TW)+x]=(src[(y*info->Width)+x]&0xf);
				}
			}
			cnt=cnt++;
		}
	}
	else
	{
		if( (contloop==0 && add==true) || (contloop==1 && add==true) )
		{	tile_data = &dst[cnt*TW*TH];
			for(int y=0;y<TH;y++)
			{	for(int x=0;x<TW;x++)
				{	tile_data[(y*TW)+x]=(src[(y*info->Width)+x]&0xf);
				}
			}
			cnt=cnt++;
		}
	}


	return tile_num;
}

///////////////////////////////////////////////////////////////////////////////
// lance la génération du tileset
int generate_tileset(uint8 *src,uint8 *dest,tgPictureInfo *info,int size)
{
	int tiles_cnt=0;
//	int pw,ph;
	int i=0;

	for(int map_cnt=0;map_cnt < ((info->Width/TW) * (info->Height/TH)); map_cnt++)
	{	// début du tile dans la map
		uint8 *map_data = &src[((map_cnt/(info->Width/TW))*(info->Width*TH))+((map_cnt%(info->Width/TW))*TW)];
		look_for_tile(map_data,dest,tiles_cnt,info,i,true);
	}
	return tiles_cnt;
}

///////////////////////////////////////////////////////////////////////////////
// lance la génération de la map
int generate_map(uint8 *img,uint8 *tiles,uint16* map,tgPictureInfo *src_info,tgPictureInfo *dst_info,int size,tgColor *pal)
{
	int tiles_cnt=0;
	int idx=0;
	int t=-1;
	int dir=0;
	int i=0;
	int shadow=0;
	int p=0;
	int nbTiles=(dst_info->Width/TW)*(dst_info->Height/TH);

	// si on a une map de prioritée
	uint8			*map_pixels=NULL;
	tgColor			map_palette[tgMAX_COLORS];
	tgPictureInfo	map_info;
	int				map_size=0;

	if(gLineArgs[ID_MAPPRIORITY].Set==true && strlen(gLineArgs[ID_MAPPRIORITY].StringArg)>0)
	{
		if(load_picture(gLineArgs[ID_MAPPRIORITY].StringArg,&map_pixels,&map_palette[0],&map_info,&map_size,0)!=tgOK)
		{	abort("Unable to load priority map picture : %s",gLineArgs[ID_MAPPRIORITY].StringArg);
			return 0;
		}

		if(map_info.Width!=src_info->Width)
		{	abort("Priority Map Image width is incorrect!");
			return 0;
		}

		if(map_info.Height!=src_info->Height)
		{	abort("Priority Map Image height is incorrect!");
			return 0;
		}

	}

	for(int map_cnt=0;map_cnt < ((src_info->Width/TW) * (src_info->Height/TH));map_cnt++)
	{	// début du tile dans la map
		dir=0;
		idx=((map_cnt/(src_info->Width/TW))*(src_info->Width*TH))+((map_cnt%(src_info->Width/TW))*TW);
		t=look_for_tile(&img[idx],tiles,nbTiles,src_info,dir,false);

		// définition de la map
		if(t!=-1)
		{	// Format the map to md format<TODO> !!!
			// t=num tile
			// dir= flip
			// pal_num1 = palette

			// calcul si en shadow highlight
			int pal_num1 = 0;
			int pal_num2 = 0;

			if(gLineArgs[ID_MAPPAL].Set==true)
			{	pal_num1 = gLineArgs[ID_MAPPAL].IntArg;	}
			else
				pal_num1 = img[idx]/16;

			pal_num2 = pal_num1+4;
			shadow=1;
			int v=gLineArgs[ID_SHADOW].IntArg;
			for(i=0;i<16;i++)
			{	if(	pal[pal_num2+i].Red!=pal[pal_num1+i].Red/2 ||
					pal[pal_num2+i].Green!=pal[pal_num1+i].Green/2 ||
					pal[pal_num2+i].Blue!=pal[pal_num1+i].Blue/2)
				{	shadow=0;	}
			}

			//
			if(gLineArgs[ID_SHADOW].Set==true)
			{
				if(shadow==1)
				{
					v=gLineArgs[ID_SHADOW].IntArg;
				}
				else
					v=1-gLineArgs[ID_SHADOW].IntArg;
			}

			p=0;
			if(gLineArgs[ID_MAPPRIORITY].Set==true && strlen(gLineArgs[ID_MAPPRIORITY].StringArg)==0)
			{	p=0x8000;
			}

			// si on a une map de pixel pour la prioritée
			if(gLineArgs[ID_MAPPRIORITY].Set==true && strlen(gLineArgs[ID_MAPPRIORITY].StringArg)>0)
			{
				if(map_pixels[idx]>0)
					p=0x8000;
			}

			int xxx=((gLineArgs[ID_MAPOFFSET].IntArg+t)&0x7ff);
			//map[map_cnt]= (v<<15) | ((pal_num1 &0x3)<<13) | ((dir & 0x3)<<11) | ((gLineArgs[ID_MAPOFFSET].IntArg+t)&0x7ff) | p ;

			int mx= ((map_cnt/(src_info->Width/TW)) * gMapw) + (map_cnt%(src_info->Width/TW));

			map[mx] = (v<<15) | ((pal_num1 &0x3)<<13) | ((dir & 0x3)<<11) | ((gLineArgs[ID_MAPOFFSET].IntArg+t)&0x7ff) | p ;

		}
		else
		{	int y=map_cnt/(src_info->Width/TW);
			int x=map_cnt%(src_info->Width/TW);
			abort("Unable to find tile(%d,%d) in the tileset\n",x,y);
			return 0;
		}
	}
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// output picture

int output_picture(const char *file,tgPictureInfo *info,uint8* pix,tgColor *pal,int file_idx)
{
	if(gApp->GetOutputWriter()!=NULL)
	{
		// crèe le nom de fichier
		char filename[256]={""};
		char path[256]={""};
		char name[256]={""};
		char t[256]={""};

		if(gLineArgs[ID_OUTDIR].Set==true && strlen(gLineArgs[ID_OUTDIR].StringArg)>0)
		{	strcat(path,gLineArgs[ID_OUTDIR].StringArg);
		}

		if(gLineArgs[ID_OUTNAME].Set==true && strlen(gLineArgs[ID_OUTNAME].StringArg)>0)
		{	// renome le fichier output
			strcpy(t,gLineArgs[ID_OUTNAME].StringArg);
			strtok(t,".");
			strcpy(name,t);
			strcpy(filename,name);
		}
		else
		{	// nom de fichier
			strcpy(t,file);
			strtok(t,".");
			strcpy(name,t);
			strcpy(filename,name);
		}

		// nom
		strcat(filename,"t");
		strcat(name,"_tiles");

		if(gLineArgs[ID_OUTNAME].Set==true && strlen(gLineArgs[ID_OUTNAME].StringArg)>0)
			sprintf(filename,"%s%d",filename,file_idx);

		strcat(filename,".");
		strcat(filename,gApp->GetOutputWriter()->GetExt());
		strcat(path,filename);

		gApp->GetOutputWriter()->OutputPicture(path,info,pix,pal);

		printf("OuPuT fIle: %s created\n",path);
	}
	else
	{	printf("\n");
		print_error(tgERR_NOOUTPUT);
	}

	return 0;

}

///////////////////////////////////////////////////////////////////////////////
// Load Dll

void get_dll_path(char* dest,char* dll)
{
	char Buffer[256]={""};
	char b[256]={""};
	GetModuleFileName(GetModuleHandle(NULL), Buffer, sizeof(Buffer));

	substring(b,Buffer,0,strlen(Buffer)-strlen("genitile.exe"));
	strcat(b,"plugins\\");
	strcat(b,dll);
	strcpy(dest,b);
}



///////////////////////////////////////////////////////////////////////////////
// output data

int output_data(const char *source,int type,uint8* data,int size,int file_idx)
{
	if(gApp->GetOutputWriter()!=NULL)
	{
		// crèe le nom de fichier
		char filename[256]={""};
		char lz_filename[256]={""};
		char path[256]={""};
		char name[256]={""};
		char t[256]={""};

		if(gLineArgs[ID_OUTDIR].Set==true && strlen(gLineArgs[ID_OUTDIR].StringArg)>0)
		{	strcat(path,gLineArgs[ID_OUTDIR].StringArg);
		}

		if(gLineArgs[ID_OUTNAME].Set==true && strlen(gLineArgs[ID_OUTNAME].StringArg)>0)
		{	// renome le fichier output
			strcpy(t,gLineArgs[ID_OUTNAME].StringArg);
			strtok(t,".");
			strcpy(name,t);
			strcpy(filename,name);
		}
		else
		{	// nom de fichier
			strcpy(t,source);
			strtok(t,".");
			strcpy(name,t);
			strcpy(filename,name);
		}



		// nom
		switch(type)
		{	case OUTPUT_TILES:
			{	strcat(name,"_tiles");
				strcat(filename,"t");
			}break;

			case OUTPUT_PAL:
			{	strcat(name,"_pal");
				strcat(filename,"p");
			}break;

			case OUTPUT_MAP:
			{	strcat(name,"_map");
				strcat(filename,"m");
			}break;
		}

		if(gLineArgs[ID_OUTNAME].Set==true && strlen(gLineArgs[ID_OUTNAME].StringArg)>0)
			sprintf(filename,"%s%d",filename,file_idx);

		strcat(lz_filename,path);
		strcat(lz_filename,filename);

		strcat(filename,".");
		strcat(filename,gApp->GetOutputWriter()->GetExt());
		strcat(path,filename);

		// vérifie si le fichier existait
		int existe=0;

		WIN32_FIND_DATA FindData;
		HANDLE hFirst;
		int cnt_files=0;

		if ( (hFirst = FindFirstFile (path, &FindData)) != INVALID_HANDLE_VALUE)
		{	existe=1;
		}

		gApp->GetOutputWriter()->OutputData(path,name,type,data,size);

		// compacte tout
		int crunch=0;

		if(	(gLineArgs[ID_LZ77].Set==true ||
			gLineArgs[ID_NEMESIS].Set==true)
			&& gLineArgs[ID_PAKM].Set==false && gLineArgs[ID_PAKT].Set==false
			)
		{

			if(type!=OUTPUT_PAL)
			{
				if(gLineArgs[ID_LZ77].Set==true)
				{	strcat(lz_filename,".lz7");
					LZ77_Encode(path,lz_filename);
					if(existe==0)
						DeleteFile(path);
					crunch=1;
				}

				if(gLineArgs[ID_NEMESIS].Set==true)
				{
					strcat(lz_filename,".nem");
					NComp(path,lz_filename);
					if(existe==0)
						DeleteFile(path);
					crunch=1;
				}
			}
		}
		else
		{
			if(gLineArgs[ID_LZ77].Set==true)
			{	strcat(lz_filename,".lz7");
				if(	gLineArgs[ID_PAKT].Set==true && type==OUTPUT_TILES)
				{	LZ77_Encode(path,lz_filename);

					if(existe==0)
						DeleteFile(path);
					crunch=1;
				}

				/*if(	gLineArgs[ID_PAKP].Set==true && type==OUTPUT_PAL)
				{	LZ77_Encode(path,lz_filename);
					if(existe==0)
						DeleteFile(path);
					crunch=1;
				}*/

				if(	gLineArgs[ID_PAKM].Set==true && type==OUTPUT_MAP)
				{	LZ77_Encode(path,lz_filename);
					if(existe==0)
						DeleteFile(path);
					crunch=1;
				}
			}

			if(gLineArgs[ID_NEMESIS].Set==true)
			{	strcat(lz_filename,".nem");
				if(	gLineArgs[ID_PAKT].Set==true && type==OUTPUT_TILES)
				{	NComp(path,lz_filename);

					if(existe==0)
						DeleteFile(path);
					crunch=1;
				}

				/*if(	gLineArgs[ID_PAKP].Set==true && type==OUTPUT_PAL)
				{	LZ77_Encode(path,lz_filename);
					if(existe==0)
						DeleteFile(path);
					crunch=1;
				}*/

				if(	gLineArgs[ID_PAKM].Set==true && type==OUTPUT_MAP)
				{	NComp(path,lz_filename);
					if(existe==0)
						DeleteFile(path);
					crunch=1;
				}
			}
		}

		if(crunch==0)
			printf("OuPuT fIle: %s created\n",path);
		else
			printf("OuPuT fIle: %s created\n",lz_filename);
	}
	else
	{	printf("\n");
		print_error(tgERR_NOOUTPUT);
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// convertit les tiles
int convert_tiles(uint8* src,uint8* dst,tgPictureInfo *info)
{
	int tiles_cnt=(info->Width/TW)*(info->Height/TH);
	int i=0;

	for(i=0;i<tiles_cnt;i++)
	{	uint8	*data=&src[((i/(info->Width/TW))*(info->Width*TH))+((i%(info->Width/TW))*TW)];
		uint32	*dst32=(uint32*)&dst[(i*TW*TH)/2];

		for(int y=0;y<8;y++)
		{	dst32[y]=	((data[y*info->Width+0]&0xf)<<28) |
						((data[y*info->Width+1]&0xf)<<24) |
						((data[y*info->Width+2]&0xf)<<20) |
						((data[y*info->Width+3]&0xf)<<16) |
						((data[y*info->Width+4]&0xf)<<12) |
						((data[y*info->Width+5]&0xf)<<8) |
						((data[y*info->Width+6]&0xf)<<4) |
						((data[y*info->Width+7]&0xf)<<0) ;
		}
	}
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// convertit en sprites

int convert_sprites(uint8* src,uint8* dst,tgPictureInfo *info)
{
	int cnt=0;
	int i=0;

	for(int my=0;my<info->Height/(TH*gSph);my++)
	{	for(int mx=0;mx<info->Width/(TW*gSpw);mx++)
		{
			for(int x=0;x<gSpw;x++)
			{
				for(int y=0;y<gSph*TH;y++)
				{
					if((y&7)==0 && i==1)
						cnt++;
					i=1;

					uint8 *data=&src[(my*gSph*TH*info->Width)+(mx*gSpw*TW)+(y*info->Width) + x*TW];
					int addr=((cnt*((TW*TH)/2))+((y&0x7)*(TW/2)));
					uint8 *d=&dst[addr];

					d[3]=(data[0]&0xf)<<4;
					d[3]|=(data[1]&0xf);

					d[2]=(data[2]&0xf)<<4;
					d[2]|=(data[3]&0xf);

					d[1]=(data[4]&0xf)<<4;
					d[1]|=(data[5]&0xf);

					d[0]=(data[6]&0xf)<<4;
					d[0]|=(data[7]&0xf);
				}
			}
		}
	}

	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// convertit la palette

int convert_palette(tgColor *src,uint8* dst,int index,int gPalSize)
{
	uint16* d=(uint16*) dst;
	int c=0;

	for(int i=index*16;i<(index*16)+gPalSize*16;i++)
	{	d[c++]= ((src[i].Blue>>4)<<8) | ((src[i].Green>>4)<<4) |((src[i].Red>>4));

	}

	return 1;
}


int check_args(int argc,char **argv,tgArg* args)
{
	int i,j;
	int err=1;
	int f=0;
	tgArg *a;

	for(j=0;j<argc;j++)
	{	f=0;
		i=0;
		a=&args[i++];

		if(argv[j][0]==optTAG)
		{	while(strlen(a->Name)>0)
			{	if(stricmp(a->Name,argv[j])==0)
				{	f=1; }
				else
				{	if(stricmp(a->ShortCut,argv[j])==0)
					{	f=1; }
				}
				a=&args[i++];
			}
			if(f==0)
			{
				abort("%s switch invalid \n",argv[j]);
				err=0;
			}
		}
	}

	return err;
}

///////////////////////////////////////////////////////////////////////////////
// main
int main(int argc,char** argv)
{
	int i=0;
	int err=0;
	int size=0;
	tgColor *ptrpal=NULL;
	int x,y;

	uint8			*gIMGPixels=NULL;
	uint8			*gTSPixels=NULL;
	uint8			*gTMPPixels=NULL;
	uint16			*gMap=NULL;

	int				gIMGSize=0;
	int				gTSSize=0;
	int				gTMPSize=0;
	int				tiles_cnt=0;
	bool			tileset_generated=false;

	tgColor			gIMGPalette[tgMAX_COLORS];
	tgColor			gTSPalette[tgMAX_COLORS];
//	tgColor			gTMPPalette[tgMAX_COLORS];

	tgPictureInfo	gIMGInfo;
	tgPictureInfo	gTSInfo;
	tgPictureInfo	gTMPInfo;

	uint8			gPalette[256];
	int				gPalSize=0;

		int cnt_files=0;

	// load the plugins
	gApp=new tgApp(PLUGINS_DIR);
	gApp->LoadPlugins(argv[0]);

	read_args(argc,argv,gLineArgs);

	if(!check_args(argc,argv,gLineArgs))
	{	goto end;
	}

	// check config
	if(!check_config(argc,argv))
	{	goto end;}

	if(argc==1)
	{	printf("GeNiTiLe V1.7 by PaScAl\n");
		printf("UsAge: GenItIle.ExE <imagefile> <opts> \n");
		printf("-tileset(t)   : <image file>\n");
		printf("-out(o)       : <output> define the output style (bmp,pcx,bin,h,asm,asmgcc)\n");
		printf("-pal(p)       : <0> define the index of the 16 colors pal,generate the pal\n");
		printf("-map(m)       : generate the map when in tileset mode\n");
		printf("-noecho(ne)   : no console echo\n");
		printf("-outdir(od)   : <output dir> define the output director\n");
		printf("-sprw(sw)     : <0> sprite width in tiles (1->8)\n");
		printf("-sprh(sh)     : <0> sprite height in tiles (1->8)\n");
		printf("-notile(nt)   : no tile generation \n");
		printf("-nbpal(np)    : <0> define the numbers of palettes to generate\n");
		printf("-shadow(s)    : <0> set the value of the shadow bit\n");
		printf("-mapoffset(mo): <0> set the number of the first map entry index\n");
		printf("-mappal(mp)   : <0> force the palette of the map entries\n");
		printf("-mapprio(mpr) : <image file> force the priority bit of the map entries\n");
		printf("-mapp(mpp)    : <0> destination map pitch (in tile)\n");
		printf("-crpx(cx)     : <0> crop x in pixels\n");
		printf("-crpy(cy)     : <0> crop y in pixels\n");
		printf("-crpw(cx)     : <0> crop w in tiles\n");
		printf("-crph(cy)     : <0> crop h in tiles\n");
		printf("-outname(on)  : <name>\n");
		printf("-lz77(lz)     : LZ77 pack all binaries output\n");
		printf("-nem(nm)      : Nemesis pack all binaries output\n");
		printf("-pakt(pt)     : pack only Tile binaries output\n");
		printf("-pakm(pm)     : pack only Map binaries output\n");
		printf("-noopt(noo)   : don't optimise flip for tileset generation\n");

		printf("ChEcK uPdAtEs At wWw.PaScAlOrAmA.cOm\n");
		goto end;
	}

	/*
	i=0;
	a=&gLineArgs[i++];
	while(strlen(a->Name)>0)
	{	printf("%s\t%s\t%d\t%d\n",a->Name,a->StringArg,a->IntArg,a->Set);
		a=&gLineArgs[i++];
	}*/

	// define les attributes
	if(gLineArgs[ID_SPRW].Set==true || gLineArgs[ID_SPRH].Set==true)
	{	gMode |= SPRITE;
	}

	if(gLineArgs[ID_SPRW].IntArg>1 || gLineArgs[ID_SPRH].IntArg>1)
	{	gMode |= SPRITE;	}

	if((gMode & SPRITE)!=0 && gLineArgs[ID_TILESET].Set==true)
	{	abort("no -tileset in sprite mode\n");
		goto end;
	}

	if((gMode & SPRITE)!=0 && gLineArgs[ID_MAP].Set==true)
	{	abort("no -map in sprite mode\n");
		goto end;
	}

	if((gMode & SPRITE)!=0 && gLineArgs[ID_SHADOW].Set==true)
	{	abort("no -shadow in sprite mode\n");
		goto end;
	}

	if(gLineArgs[ID_PAL].Set==true)
	{	gMode |= PAL;	}

	if(gLineArgs[ID_MAP].Set==true)
	{	gMode |= MAP;	}

	// look for gud img filename
	for(i=0;i<argc;i++)
	{	if(strrchr(argv[i], '.')!=0)
		{	if(i>0)
			{	int _ret=stricmp(argv[i-1],optTILESET);
				if(_ret>0)
				{	strcpy(gImgFileName,argv[i]);
					strcpy(gWildCard,gImgFileName);
				}
			}
		}
	}

	// si pas tileset alors erreur
	if(strlen(gImgFileName)==0 && gLineArgs[ID_TILESET].Set==false)
	{	print_error(tgERR_NOINPUT);
		goto end;
	}

	// pas trouvé de wilcard
	if(strlen(gWildCard)==0 && gLineArgs[ID_TILESET].Set==true && strlen(gLineArgs[ID_TILESET].StringArg)>0)
	{	strcpy(gWildCard,gLineArgs[ID_TILESET].StringArg);
	}


	if(gLineArgs[ID_TILESET].Set==true)
	{	gMode |= TILESET;

		if(strlen(gImgFileName)==0 && strlen(gLineArgs[ID_TILESET].StringArg)==0)
		{	print_error(tgERR_NOINPUT);
			goto end;
		}
	}

	if(gLineArgs[ID_MAP].Set==true || gLineArgs[ID_MAPPAL].Set==true || gLineArgs[ID_MAPOFFSET].Set==true || gLineArgs[ID_MAPPRIORITY].Set==true ||  gLineArgs[ID_MAPW].Set==true)
	{	if(gLineArgs[ID_TILESET].Set==false)
		{	abort("you need to specify a -tileset for the map generation\n");
			goto end;
		}
	}

	if(gLineArgs[ID_TILESET].Set==true)
	{	if((gLineArgs[ID_MAP].Set==false && gLineArgs[ID_PAL].Set==false) && gLineArgs[ID_NOTILE].Set==true)
		{	abort("no output specified use -map,-pal or remove -notile\n");
			goto end;
		}
	}

	if(gLineArgs[ID_NEMESIS].Set==false && gLineArgs[ID_LZ77].Set==false
		&& (gLineArgs[ID_PAKM].Set==true || gLineArgs[ID_PAKT].Set==true )
		)
	{	gLineArgs[ID_NEMESIS].Set=true;
	}

	if(gLineArgs[ID_TILESET].Set==false)
	{
		if(gLineArgs[ID_NOTILE].Set==true && gLineArgs[ID_PAL].Set==false)
		{	abort("no output specified use -pal or remove -notile\n");
			goto end;
		}
	}

	if(gLineArgs[ID_NEMESIS].Set==true)
	{
		char dll[256];
		get_dll_path(dll,"nemesis.dll");
		if(NInit(dll)==false)
		{	echo("UnAbLe To OpEn nemesis.dll\n");
			goto end;
		}
	}

	///////////////////////////////////////////////////////////////////////////
	// boucle
	gSpw = gLineArgs[ID_SPRW].IntArg;
	gSph = gLineArgs[ID_SPRH].IntArg;
	gCropx = gLineArgs[ID_CROPX].IntArg;
	gCropy = gLineArgs[ID_CROPY].IntArg;
	gCropw = gLineArgs[ID_CROPW].IntArg;
	gCroph = gLineArgs[ID_CROPH].IntArg;

	gMapw = gLineArgs[ID_MAPW].IntArg;
	gMaph = 0;

	WIN32_FIND_DATA FindData;
	HANDLE hFirst;


	if ( (hFirst = FindFirstFile (gWildCard, &FindData)) != INVALID_HANDLE_VALUE)
	{
		do
		{
			// propriété des wildcards
			if(strlen(gWildCard)>0 && strlen(gImgFileName)==0 && gLineArgs[ID_TILESET].Set==true && strlen(gLineArgs[ID_TILESET].StringArg)>0)
			{	strcpy(gLineArgs[ID_TILESET].StringArg,FindData.cFileName);	}

			if(strlen(gImgFileName)>0)
				strcpy(gImgFileName,FindData.cFileName);

			for(i=0;i<256;i++)
			{	gPalette[i]=0;
			}

			// cas 1: Génération de tileset sans map de base
			if( gLineArgs[ID_TILESET].Set==true && (
				(strlen(gLineArgs[ID_TILESET].StringArg)==0 &&  strlen(gImgFileName)>0) ||
				(strlen(gLineArgs[ID_TILESET].StringArg)>0 &&  strlen(gImgFileName)==0)) )
			{
				char tn[256]={""};
				int tt=0;
				gMode |=TILESET;

				// récupère le nom
				if(strlen(gLineArgs[ID_TILESET].StringArg)>0 &&  strlen(gImgFileName)>0)
				{	strcpy(tn,gLineArgs[ID_TILESET].StringArg);	}

				if(strlen(gLineArgs[ID_TILESET].StringArg)==0 &&  strlen(gImgFileName)>0)
				{	strcpy(tn,gImgFileName);	}

				if(strlen(gLineArgs[ID_TILESET].StringArg)>0 &&  strlen(gImgFileName)==0)
				{	strcpy(tn,gLineArgs[ID_TILESET].StringArg);	}

				if(load_picture(tn,&gTSPixels,gTSPalette,&gTSInfo,&gTSSize,1)!=tgOK)
				{	goto fin;	}

				// definit un buffer pour le nouveau tileset
				gTMPPixels=(uint8*) malloc(gTSSize);
				if(!gTMPPixels)
				{	print_error(tgERR_OUTMEMORY);
					goto fin;
				}
				memset(gTMPPixels,0,gTSSize);
				tiles_cnt=generate_tileset(gTSPixels,gTMPPixels,&gTSInfo,gTSSize);
				tileset_generated=true;
				tt=gTSSize;

				gTMPSize= tiles_cnt * TW*TH;
				gTMPInfo.BytesPerPixel=1;
				gTMPInfo.Height= tiles_cnt * TW;
				gTMPInfo.Width = TW;
				gTMPInfo.Pitch = gTSInfo.Height;

				// on doit généré une map
				if(gLineArgs[ID_MAP].Set==true )
				{	if(gTMPPixels!=NULL && gTSPixels!=NULL)
					{	// alloue la map
						if(gMapw==0)
							gMapw=(gTSInfo.Width/TW);

						if(gMaph==0)
							gMaph=(gTSInfo.Height/TH);

						if(gMapw * TW < gTSInfo.Width)
						{
							abort("Map pitch can not be smaller than picture width\n");
							goto fin;
						}
						gMap=(uint16*) malloc(gMapw*gMaph*2);
						memset(gMap,0,gMapw*gMaph*2);
						if(!gMap)
						{	print_error(tgERR_OUTMEMORY);}

						if(generate_map(gTSPixels,gTMPPixels,gMap,&gTSInfo,&gTMPInfo,gTSSize,gTSPalette)==0)
						{	goto fin;	}

						output_data(tn,OUTPUT_MAP,(uint8*)gMap,gMaph*gMapw*2,cnt_files);
					}
				}

				// recopie le buffer de tileset
				memset(gTSPixels,0,tt);
				gTSSize= tiles_cnt * TW*TH;
				gTSInfo.BytesPerPixel=1;
				gTSInfo.Height= tiles_cnt * TW;
				gTSInfo.Width = TW;
				gTSInfo.Pitch = gTSInfo.Height;
				memcpy(gTSPixels,gTMPPixels,gTSSize);
				free(gTMPPixels);
				gTMPPixels=NULL;

				// output
				if(gLineArgs[ID_PAL].Set==true)
				{	convert_palette(gTSPalette,gPalette,gLineArgs[ID_PAL].IntArg,gLineArgs[ID_NBPAL].IntArg);
					output_data(tn,OUTPUT_PAL,gPalette,gLineArgs[ID_NBPAL].IntArg*16*2,cnt_files);
				}

				if(gLineArgs[ID_NOTILE].Set==false)
				{
					gTMPSize=(gTSInfo.Width * gTSInfo.Height)/2;
					gTMPPixels=(uint8*) malloc(gTMPSize);
					if(!gTMPPixels)
					{	print_error(tgERR_OUTMEMORY);}
					memset(gTMPPixels,0,gTMPSize);

					convert_tiles(gTSPixels,gTMPPixels,&gTSInfo);

					if(gApp->GetOutputWriter()!=NULL)
					{	if(gApp->GetOutputWriter()->GetOutputType()==tgOutputType_Raw)
							output_data(tn,OUTPUT_TILES,gTMPPixels,gTMPSize,cnt_files);
						else
							output_picture(tn,&gTSInfo,gTSPixels,gTSPalette,cnt_files);
					}

					// efface
					free(gTMPPixels);
					gTMPPixels=NULL;
				}
			}

			// cas 2: on spécifie une pcx de tileset
			if(gLineArgs[ID_TILESET].Set==true &&
				strlen(gLineArgs[ID_TILESET].StringArg)>0 &&  strlen(gImgFileName)>0)
			{
				char tn[256]={""};
				gMode |=TILESET;

				// load les images, 1 le tileset
				if(load_picture(gLineArgs[ID_TILESET].StringArg,&gTSPixels,gTSPalette,&gTSInfo,&gTSSize,0)!=tgOK)
				{	goto fin;	}

				// réorganise le tileset en vertical
				gTMPPixels=(uint8*) malloc(gTSSize);
				if(!gTMPPixels)
				{	print_error(tgERR_OUTMEMORY);
					goto fin;
				}
				memset(gTMPPixels,0,gTSSize);
				tiles_cnt=(gTSInfo.Width/TW)*(gTSInfo.Height/TH);
				for(int tcnt=0;tcnt<tiles_cnt;tcnt++)
				{
					int addr=((tcnt/(gTSInfo.Width/TW))*gTSInfo.Width*TH)+((tcnt%(gTSInfo.Width/TW))*TW);
					unsigned char *data=&gTSPixels[addr];
					for(y=0;y<TH;y++)
					{	for(x=0;x<TW;x++)
						{	gTMPPixels[(tcnt*TW*TH)+(TW*y)+x]=data[(y*gTSInfo.Width)+x];
						}
					}
				}
				// recopie le buffer de tileset
				memset(gTSPixels,0,gTSSize);
				gTSSize= tiles_cnt * TW*TH;
				gTSInfo.BytesPerPixel=1;
				gTSInfo.Height= tiles_cnt * TW;
				gTSInfo.Width = TW;
				gTSInfo.Pitch = gTSInfo.Height;
				memcpy(gTSPixels,gTMPPixels,gTSSize);

				free(gTMPPixels);
				gTMPPixels=NULL;

				// load la map
				if(load_picture(gImgFileName,&gIMGPixels,gIMGPalette,&gIMGInfo,&gIMGSize,1)!=tgOK)
				{	goto fin;	}

				// output
				if(gLineArgs[ID_PAL].Set==true)
				{	convert_palette(gTSPalette,gPalette,gLineArgs[ID_PAL].IntArg,gLineArgs[ID_NBPAL].IntArg);
					output_data(gImgFileName,OUTPUT_PAL,gPalette,gLineArgs[ID_NBPAL].IntArg*16*2,cnt_files);
				}

				if(gLineArgs[ID_NOTILE].Set==false)
				{
					gTMPSize=(gTSInfo.Width * gTSInfo.Height)/2;
					gTMPPixels=(uint8*) malloc(gTMPSize);
					if(!gTMPPixels)
					{	print_error(tgERR_OUTMEMORY);}
					memset(gTMPPixels,0,gTMPSize);


					if(gApp->GetOutputWriter()!=NULL)
					{	if(gApp->GetOutputWriter()->GetOutputType()==tgOutputType_Raw)
						{	convert_tiles(gTSPixels,gTMPPixels,&gTSInfo);
							output_data(gImgFileName,OUTPUT_TILES,gTMPPixels,gTMPSize,cnt_files);
						}
						else
							output_picture(gImgFileName,&gTSInfo,gTSPixels,gTSPalette,cnt_files);
					}

					free(gTMPPixels);
					gTMPPixels=NULL;
				}

				// on doit généré une map
				if(	gLineArgs[ID_MAP].Set==true )
				{	if(gIMGPixels!=NULL && gTSPixels!=NULL)
					{	// alloue la map
						if(gMapw==0)
							gMapw=gIMGInfo.Width/TW;

						if(gMaph==0)
							gMaph=gIMGInfo.Height/TH;

						if(gMapw * TW < gIMGInfo.Width)
						{
							abort("Map pitch can not be smaller than picture width\n");
							goto fin;
						}

						gMap=(uint16*) malloc(gMapw*gMaph*2);
						memset(gMap,0,gMapw*gMaph*2);

						if(!gMap)
						{	print_error(tgERR_OUTMEMORY);}

						if(generate_map(gIMGPixels,gTSPixels,gMap,&gIMGInfo,&gTSInfo,gIMGSize,gIMGPalette)==0)
						{	goto fin;	}

						output_data(gImgFileName,OUTPUT_MAP,(uint8*)gMap,gMapw*gMaph*2,cnt_files);
					}
				}
			}

			// génèration de tiles normaux format 4 pixels
			if(gLineArgs[ID_TILESET].Set==false)
			{
				// charge l'image
				if(load_picture(gImgFileName,&gIMGPixels,gIMGPalette,&gIMGInfo,&gIMGSize,1)!=tgOK)
				{	goto fin;	}

				// alloue un buffer temp
				gTMPSize=(gIMGInfo.Width * gIMGInfo.Height)/2;
				gTMPPixels=(uint8*) malloc(gTMPSize);
				if(!gTMPPixels)
				{	print_error(tgERR_OUTMEMORY);}
				memset(gTMPPixels,255,gTMPSize);

				if((gMode & SPRITE)!=0)
				{
					if(gLineArgs[ID_NOTILE].Set==false)
					{	convert_sprites(gIMGPixels,gTMPPixels,&gIMGInfo);
						//output_data(gImgFileName,OUTPUT_TILES,gTMPPixels,gTMPSize);
						if(gApp->GetOutputWriter()->GetOutputType()==tgOutputType_Raw)
							output_data(gImgFileName,OUTPUT_TILES,gTMPPixels,gTMPSize,cnt_files);
						else
							output_picture(gImgFileName,&gIMGInfo,gIMGPixels,gIMGPalette,cnt_files);
					}
				}
				else
				{
					if(gLineArgs[ID_NOTILE].Set==false)
					{	convert_tiles(gIMGPixels,gTMPPixels,&gIMGInfo);
						//output_data(gImgFileName,OUTPUT_TILES,gTMPPixels,gTMPSize);
						if(gApp->GetOutputWriter()->GetOutputType()==tgOutputType_Raw)
							output_data(gImgFileName,OUTPUT_TILES,gTMPPixels,gTMPSize,cnt_files);
						else
							output_picture(gImgFileName,&gIMGInfo,gIMGPixels,gIMGPalette,cnt_files);
					}
				}

				// pal
				if(gLineArgs[ID_PAL].Set==true)
				{	convert_palette(gIMGPalette,gPalette,gLineArgs[ID_PAL].IntArg,gLineArgs[ID_NBPAL].IntArg);
					output_data(gImgFileName,OUTPUT_PAL,gPalette,gLineArgs[ID_NBPAL].IntArg*16*2,cnt_files);
				}

			}

			// génération de la palette
			cnt_files++;

			// pass a l'image suivante

		fin:
			if(gMap!=NULL)
				free(gMap);
			gMap=NULL;

			if(gTSPixels!=NULL)
				free(gTSPixels);
			gTSPixels=NULL;

			if(gTMPPixels!=NULL)
				free(gTMPPixels);
			gTMPPixels=NULL;

			if(gIMGPixels!=NULL)
				free(gIMGPixels);
			gIMGPixels=NULL;

		}
		while ( FindNextFile (hFirst, &FindData));
	}

	echo("\nFiles processed %d",cnt_files);

	echo("\nAlL dOnE ;)\n");

end:
	if(gApp!=NULL)
		delete(gApp);

    return 0;
}
