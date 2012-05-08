/**
 * \file types.h
 * \brief Types definition
 * \author Stephane Dallongeville
 * \date 08/2011
 *
 * SGDK Types definition.
 */

#ifndef _TYPES_H_
#define _TYPES_H_

/**
 *  \def FALSE
 *      FALSE define (equivalent to 0).
 */
#ifndef FALSE
#define FALSE   0
#endif
/**
 *  \def TRUE
 *      TRUE define (equivalent to 1).
 */
#ifndef TRUE
#define TRUE    1
#endif
/**
 *  \def NULL
 *      NULL define (equivalent to 0).
 */
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


/**
 *  \def s8
 *      8 bits signed integer (equivalent to char).
 */
#define s8      char
/**
 *  \def s16
 *      16 bits signed integer (equivalent to short).
 */
#define s16     short
/**
 *  \def s32
 *      32 bits signed integer (equivalent to long).
 */
#define s32     long

/**
 *  \def u8
 *      8 bits unsigned integer (equivalent to unsigned char).
 */
#define u8      unsigned char
/**
 *  \def u16
 *      16 bits unsigned integer (equivalent to unsigned short).
 */
#define u16     unsigned short
/**
 *  \def u32
 *      32 bits unsigned integer (equivalent to unsigned long).
 */
#define u32     unsigned long

/**
 *  \def vs8
 *      volatile 8 bits signed integer.
 */
#define vs8     volatile s8
/**
 *  \def vs16
 *      volatile 16 bits signed integer.
 */
#define vs16    volatile s16
/**
 *  \def vs32
 *      volatile 32 bits signed integer.
 */
#define vs32    volatile s32

/**
 *  \def vu8
 *      volatile 8 bits unsigned integer.
 */
#define vu8     volatile u8
/**
 *  \def vu16
 *      volatile 16 bits unsigned integer.
 */
#define vu16    volatile u16
/**
 *  \def vu32
 *      volatile 32 bits unsigned integer.
 */
#define vu32    volatile u32


/**
 *  \typedef fix16
 *      16 bits fixed floting point type.
 */
typedef s16 fix16;
/**
 *  \typedef fix32
 *      32 bits fixed floating point type.
 */
typedef s32 fix32;

// used for memcpy and memset ISO C method
typedef unsigned int size_t;


#define FASTCALL


typedef void _voidCallback();


u8  getZeroU8();
u16 getZeroU16();
u32 getZeroU32();


#endif // _TYPES_H_

