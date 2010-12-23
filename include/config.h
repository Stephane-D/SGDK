#ifndef _CONFIG_
#define _CONFIG_


// uncomment if you don't mind about rom size and want fast fix16 math operations
// (only LOG & POW2 supported right now).
//#define MATHS_BIG_TABLE

// uncomment if you don't mind about rom size and want fast write vram address
// this is usefull only for intense vram write address modification
//#define FAST_WRITE_VRAM_ADDR

// uncomment if you want to use FAT16 methods provided by Krik
// this cost a bit than more 1 KB
//#define FAT16_SUPPORT

// uncomment if you want to enable the software bitmap engine
// be careful, software bitmap engine consume a lot of memory (a bit more than 41 KB !)
#define ENABLE_BMP

#ifdef ENABLE_BMP

// uncomment if you want to have the kit intro logo (require software bitmap engine)
//#define ENABLE_LOGO

#ifdef ENABLE_LOGO

// uncomment if you want zoom intro logo effect instead of classic fading
//#define ZOOMING_LOGO

#endif // ENABLE_LOGO

#endif // ENABLE_BMP


#endif // _CONFIG_
