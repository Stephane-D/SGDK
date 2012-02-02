#ifndef __TILEGTSDK_H__
#define __TILEGTSDK_H__

///////////////////////////////////////////////////////////////////////////////
// Data type
///////////////////////////////////////////////////////////////////////////////

typedef unsigned char uint8;
typedef unsigned short int uint16;
typedef unsigned long int uint32;

typedef signed char int8;
typedef signed short int int16;
typedef signed long int int32;

///////////////////////////////////////////////////////////////////////////////
// Define
///////////////////////////////////////////////////////////////////////////////

#define tgERR_UNKNOWN	0
#define	tgOK			1
#define tgKO			0

#define tgERR_OUTMEMORY	50
#define	tgERR_OPENINGFILE		51
#define	tgERR_SAVINGFILE		52
#define	tgERR_BADBPP			53
#define tgERR_UNSUPPORTED		54
#define tgERR_FILEUNSUPPORTED	55
#define tgERR_COPYDATA			56
#define tgERR_NOOUTPUT			57
#define tgERR_NOINPUT			58
#define tgERR_COMPRESSION		59

#define tgMAX_COLORS		256

#define DLLEXPORT	extern "C" __declspec(dllexport)

#define tgOutputType_Raw		0
#define tgOutputType_Picture	1

#define OUTPUT_TILES	0
#define OUTPUT_PAL		1
#define OUTPUT_MAP		2

///////////////////////////////////////////////////////////////////////////////
// Structures
///////////////////////////////////////////////////////////////////////////////

typedef struct
{	int Width;
	int Height;
	int	BytesPerPixel;
	int Pitch;
}tgPictureInfo;

typedef struct
{	uint8	Alpha;
	uint8	Red,Green,Blue;
}tgColor;

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

///////////////////////////////////////////////////////////////////////////////

#endif
