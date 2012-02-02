#ifndef _CONFIG_
#define _CONFIG_


/**
 *  \def FAT16_SUPPORT
 *      Set it to 1 if you want to use FAT16 methods provided by Krik.<br>
 *      This cost a bit than more 1 KB of RAM.
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
