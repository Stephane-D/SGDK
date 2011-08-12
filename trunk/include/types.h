#ifndef _TYPES_H_
#define _TYPES_H_

#ifndef FALSE
#define FALSE   0
#endif
#ifndef TRUE
#define TRUE    1
#endif
#ifndef NULL
#define NULL    0
#endif

#ifndef MIN_U8
#define MIN_U8  0x00
#endif
#ifndef MAX_U8
#define MAX_U8  0xFF
#endif
#ifndef MIN_S8
#define MIN_S8  -0x80
#endif
#ifndef MAX_S8
#define MAX_S8  0x7F
#endif

#ifndef MIN_U16
#define MIN_U16 0x0000
#endif
#ifndef MAX_U16
#define MAX_U16 0xFFFF
#endif
#ifndef MIN_S16
#define MIN_S16 -0x8000
#endif
#ifndef MAX_S16
#define MAX_S16 0x7FFF
#endif

#ifndef MIN_U32
#define MIN_U32 0x0000
#endif
#ifndef MAX_U32
#define MAX_U32 0xFFFFFFFF
#endif
#ifndef MIN_S32
#define MIN_S32 -0x80000000
#endif
#ifndef MAX_S32
#define MAX_S32 0x7FFFFFFF
#endif


#define s8      char
#define s16     short
#define s32     long

#define u8      unsigned char
#define u16     unsigned short
#define u32     unsigned long

#define vs8     volatile s8
#define vs16    volatile s16
#define vs32    volatile s32

#define vu8     volatile u8
#define vu16    volatile u16
#define vu32    volatile u32


//#define fix16   s16
//#define fix32   s32

typedef s16 fix16;
typedef s32 fix32;

// used for memcpy and memset ISO C method
typedef unsigned int size_t;


#define FASTCALL


typedef void _voidCallback();


u8  getZeroU8();
u16 getZeroU16();
u32 getZeroU32();


#endif // _TYPES_H_

