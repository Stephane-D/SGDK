#ifndef _CONFIG_
#define _CONFIG_


/**
 *  \def VRAM_TABLE
 *      Set it to 1 to enable the big VRAM lookup table.<br>
 *      This table permits faster VRAM address set to the VDP (256 KB of ROM).
 *      BMP FastFill (FF) mode needs this table too.
 */
#define VRAM_TABLE          0

/**
 *  \def BMP_TABLES
 *      Set it to 1 to enable the BMP FastFill lookup tables.<br>
 *      BMP FastFill (FF) mode needs these tables (~42 KB of ROM).
 */
#define BMP_TABLES          0

/**
 *  \def MATH_BIG_TABLES
 *      Set it to 1 to enable the big Math lookup tables.<br>
 *      This table permits Log2, Log10 and Sqrt operation for fix16 type (128*3 KB of rom).
 */
#define MATH_BIG_TABLES     0

/**
 *  \def FAT16_SUPPORT
 *      Set it to 1 if you want to use FAT16 methods provided by Krik.<br>
 *      This cost a bit more than 1 KB of RAM.
 */
#define FAT16_SUPPORT       0

/**
 *  \def ENABLE_LOGO
 *      Set it to 1 if you want to have the kit intro logo
 */
#define ENABLE_LOGO         0

#if (ENABLE_LOGO != 0)

/**
 *  \def ENABLE_LOGO
 *      Set it to 1 if you want zoom intro logo effect instead of classic fading
 */
#define ZOOMING_LOGO        0

#endif // ENABLE_LOGO


#endif // _CONFIG_
