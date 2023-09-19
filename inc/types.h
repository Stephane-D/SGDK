/**
 *  \file types.h
 *  \brief Types definition
 *  \author Stephane Dallongeville
 *  \date 08/2011
 *
 * SGDK Types definition.
 */

#ifndef _TYPES_H_
#define _TYPES_H_

/**
 *  \brief
 *      FALSE define (equivalent to 0).
 */
#ifndef FALSE
#define FALSE   0
#endif
/**
 *  \brief
 *      TRUE define (equivalent to 1).
 */
#ifndef TRUE
#define TRUE    1
#endif
/**
 *  \brief
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
 *  \typedef s8
 *      8 bits signed integer (equivalent to char).
 */
typedef char s8;
/**
 *  \typedef s16
 *      16 bits signed integer (equivalent to short).
 */
typedef short s16;
/**
 *  \typedef s32
 *      32 bits signed integer (equivalent to long).
 */
typedef long s32;

/**
 *  \typedef u8
 *      8 bits unsigned integer (equivalent to unsigned char).
 */
typedef unsigned char u8;
/**
 *  \typedef u16
 *      16 bits unsigned integer (equivalent to unsigned short).
 */
typedef unsigned short u16;
/**
 *  \typedef u32
 *      32 bits unsigned integer (equivalent to unsigned long).
 */
typedef unsigned long u32;

/**
 *  \typedef bool
 *      boolean type, to be used with TRUE and FALSE constant.
 *      (internally set as unsigned char)
 */
#ifndef __cplusplus
typedef u8 bool;
#endif

/**
 *  \typedef vs8
 *      volatile 8 bits signed integer.
 */
typedef volatile s8 vs8;
/**
 *  \typedef vs16
 *      volatile 16 bits signed integer.
 */
typedef volatile s16 vs16;
/**
 *  \typedef vs32
 *      volatile 32 bits signed integer.
 */
typedef volatile s32 vs32;

/**
 *  \typedef vu8
 *      volatile 8 bits unsigned integer.
 */
typedef volatile u8 vu8;
/**
 *  \typedef vu16
 *      volatile 16 bits unsigned integer.
 */
typedef volatile u16 vu16;
/**
 *  \typedef vu32
 *      volatile 32 bits unsigned integer.
 */
typedef volatile u32 vu32;

/**
 *  \typedef vbool
 *      volatile boolean type.
 *      (internally set as volatile unsigned char)
 */
typedef vu8 vbool;

/**
 *  \typedef p16
 *      short pointer for fast 16 bit addressing (GCC does correctly cast that to pointer).
 *      Limited to 0xFFFF8000-0x00007FFF memory region (first 32KB bank of ROM, and last 32KB of RAM)
 */
typedef s16 p16;


#if !defined(uint8_t) && !defined(__int8_t_defined)
#define uint8_t     u8
#define int8_t      s8
#endif
#if !defined(uint16_t) && !defined(__int16_t_defined)
#define uint16_t    u16
#define int16_t     s16
#endif
#if !defined(uint32_t) && !defined(__int32_t_defined)
#define uint32_t    u32
#define int32_t     s32
#endif


/**
 *  \typedef fix16
 *      16 bits fixed point (10.6) type
 */
typedef s16 fix16;
/**
 *  \typedef fix32
 *      32 bits fixed point (22.10) type
 */
typedef s32 fix32;
/**
 *  \typedef f16
 *      16 bits fixed point (10.6) type - short version
 */
typedef s16 f16;
/**
 *  \typedef f32
 *      32 bits fixed point (22.10) type - short version
 */
typedef s32 f32;

/**
 *  \typedef fastfix16
 *      "fast" 16 bits fixed point (8.8) type
 */
typedef s16 fastfix16;
/**
 *  \typedef fastfix32
 *      "fast" 32 bits fixed point (16.16) type
 */
typedef s32 fastfix32;
/**
 *  \typedef ff16
 *      "fast" 16 bits fixed point (8.8) type - short version
 */
typedef s16 ff16;
/**
 *  \typedef ff32
 *      "fast" 32 bits fixed point (16.16) type - short version
 */
typedef s32 ff32;

/**
 *  \typedef vfix16
 *      volatile 16 bits fixed point (10.6) type.
 */
typedef vs16 vfix16;
/**
 *  \typedef vfix32
 *      volatile 32 bits fixed point (22.10) type.
 */
typedef vs32 vfix32;
/**
 *  \typedef vf16
 *      volatile 16 bits fixed point (10.6) type - short version
 */
typedef vs16 vf16;
/**
 *  \typedef vf32
 *      volatile 32 bits fixed point (22.10) type - short version
 */
typedef vs32 vf32;


#define FASTCALL

/**
 *  \brief
 *      Simple Box structure
 *
 *  \param x
 *      X position (left)
 *  \param y
 *      Y position (top)
 *  \param w
 *      width
 *  \param h
 *      heigth
 */
typedef struct
{
    s16 x;
    s16 y;
    u16 w;
    u16 h;
} Box;

/**
 *  \brief
 *      Simple Circle structure
 *
 *  \param x
 *      X center position
 *  \param y
 *      Y center position
 *  \param ray
 *      circle ray
 */
typedef struct
{
    s16 x;
    s16 y;
    u16 ray;
} Circle;


typedef void VoidCallback(void);


u8  getZeroU8(void);
u16 getZeroU16(void);
u32 getZeroU32(void);

/**
 *  \brief
 *      ROL instruction for byte (8 bit) value
 *
 *  \param value
 *      value to apply bit rotation
 *  \param number
 *      number of bit rotation
 */
u8  rol8(u8 value, u16 number);
/**
 *  \brief
 *      ROL instruction for short (16 bit) value
 *
 *  \param value
 *      value to apply bit rotation
 *  \param number
 *      number of bit rotation
 */
u16 rol16(u16 value, u16 number);
/**
 *  \brief
 *      ROL instruction for long (32 bit) value
 *
 *  \param value
 *      value to apply bit rotation
 *  \param number
 *      number of bit rotation
 */
u32 rol32(u32 value, u16 number);
/**
 *  \brief
 *      ROR instruction for byte (8 bit) value
 *
 *  \param value
 *      value to apply bit rotation
 *  \param number
 *      number of bit rotation
 */
u8  ror8(u8 value, u16 number);
/**
 *  \brief
 *      ROR instruction for short (16 bit) value
 *
 *  \param value
 *      value to apply bit rotation
 *  \param number
 *      number of bit rotation
 */
u16 ror16(u16 value, u16 number);
/**
 *  \brief
 *      ROR instruction for long (32 bit) value
 *
 *  \param value
 *      value to apply bit rotation
 *  \param number
 *      number of bit rotation
 */
u32 ror32(u32 value, u16 number);


#endif // _TYPES_H_
