#include "types.h"


u8 getZeroU8()
{
    return 0;
}

u16 getZeroU16()
{
    return 0;
}

u32 getZeroU32()
{
    return 0;
}

u8 rol8(u8 value, u16 number)
{
   return (value << number) | (value >> (8 - number));
}

u16 rol16(u16 value, u16 number)
{
   return (value << number) | (value >> (16 - number));
}

u32 rol32(u32 value, u16 number)
{
   return (value << number) | (value >> (32 - number));
}

u8 ror8(u8 value, u16 number)
{
   return (value >> number) | (value << (8 - number));
}

u16 ror16(u16 value, u16 number)
{
   return (value >> number) | (value << (16 - number));
}

u32 ror32(u32 value, u16 number)
{
   return (value >> number) | (value << (32 - number));
}