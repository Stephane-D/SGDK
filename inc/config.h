/**
 *  \file config.h
 *  \brief Basic SGDK library configuration file
 *  \author Stephane Dallongeville
 *  \date 08/2011
 *
 * This unit is used to define some specific compilation option of the library.
 */

#ifndef _CONFIG_
#define _CONFIG_


/**
 *  \brief
 *      Set it to 1 to enable KDebug logging (Gens KMod) for some errors (as memory allocation).
 */
#if (DEBUG == 1)
  #define LIB_DEBUG         1
#else
  #define LIB_DEBUG         0
#endif

/**
 *  \brief
 *      Set it to 1 to enable the big Math lookup tables.<br>
 *      This table permits Log2, Log10 and Sqrt operation for fix16 type (128*3 KB of rom).
 */
#define MATH_BIG_TABLES     0

/**
 *  \brief
 *      Set it to 1 if you want to use FAT16 methods provided by Krik.<br>
 *      This cost a bit more than 1 KB of RAM.
 */
#define FAT16_SUPPORT       0

/**
 *  \brief
 *      Set it to 1 if you want to have the kit intro logo
 */
#define ENABLE_LOGO         0

#if (ENABLE_LOGO != 0)

/**
 *  \brief
 *      Set it to 1 if you want zoom intro logo effect instead of classic fading
 */
#define ZOOMING_LOGO        0

#endif // ENABLE_LOGO


#endif // _CONFIG_
