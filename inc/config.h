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
 *      Log disable
 */
#define LOG_LEVEL_DISABLE   0
/**
 *  \brief
 *      Log for error only
 */
#define LOG_LEVEL_ERROR     1
/**
 *  \brief
 *      Log for error and warning
 */
#define LOG_LEVEL_WARNING   2
/**
 *  \brief
 *      Log for error, warning and info (as memory allocation)
 */
#define LOG_LEVEL_INFO      3

/**
 *  \brief
 *      Define library log level (for debug build)
 */
#define LIB_LOG_LEVEL       LOG_LEVEL_WARNING

/**
 *  \brief
 *      Set it to 1 to enable KDebug logging (Gens KMod) to log some errors (as memory allocation).
 */
#if (DEBUG != 0)
    #define LIB_DEBUG       1
#else
    #define LIB_DEBUG       0
    #undef LIB_LOG_LEVEL
    #define LIB_LOG_LEVEL   LOG_LEVEL_DISABLE
#endif

/**
 *  \brief
 *      Set it to 1 if you want to force Z80 halt during DMA operation (default).<br>
 *      Some Megadrive models need it to prevent some possible DMA transfer corruption or even 68000 memory or Z80 invalid data fetch in very rare case.<br>
 *      This actually happen when Z80 access the main BUS exactly at same time you trigger a DMA operation.<br>
 *      If you are 100% sure that you are actually avoiding that case you may try to disable the flag (at your own risk though).
 */
#define HALT_Z80_ON_DMA     1

/**
 *  \brief
 *      Set it to 1 if you want to force Z80 halt during IO port (controller) accesses.<br>
 *      Some Megadrive models (as some MD2) need it to prevent some possible (but very rare) Z80 corruption bugs
 *      (may happen when Z80 access the main BUS during IO port access from 68K).
 */
#define HALT_Z80_ON_IO      1

/**
 *  \brief
 *      Set it to 1 if you want to completely disable DMA transfers (for testing purpose) and replace them with (slower) software CPU copy.
 */
#define DMA_DISABLED        0

/**
 *  \brief
 *      Set it to 1 to enable automatic bank switch using official SEGA mapper for ROM > 4MB.
 *
 *      When automatic bank switch is enabled all internal BIN data structures declared in .far_rodata section
 *      will be accessed using BANK_getFarData(..) method (mapper.c). That may impact performance quite a bit
 *      it's why it's disabled by default if you don't require bank switch.
 */
#define ENABLE_BANK_SWITCH  0

/**
 *  \brief
 *      Set it to 1 if you want to use newlib with SGDK.<br>
 *      That will disable any standard methods from SGDK to replace them by newlib library implementation.
 */
#define ENABLE_NEWLIB       0

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

/**
 *  \brief
 *      Set it to 1 if you want to use EVERDRIVE programming methods (provided by Krikzz).<br>
 */
#define MODULE_EVERDRIVE    0

/**
 *  \brief
 *      Set it to 1 if you want to use FAT16 methods for Everdrive cart (provided by Krikzz).<br>
 *      This cost a bit more than 1 KB of RAM.
 */
#define MODULE_FAT16        0

// FAT16 need EVERDRIVE
#if ((MODULE_EVERDRIVE == 0) && (MODULE_FAT16 != 0))
#error "Cannot enable FAT16 module without EVERDRIVE module"
#endif

/**
 *  \brief
 *      Set it to 1 if you want to enable MegaWiFi functions and support code
 */
#define MODULE_MEGAWIFI     0

#endif // _CONFIG_
