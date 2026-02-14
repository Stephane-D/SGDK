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
#define LOG_LEVEL_DISABLE       0
/**
 *  \brief
 *      Log for error only
 */
#define LOG_LEVEL_ERROR         1
/**
 *  \brief
 *      Log for error and warning
 */
#define LOG_LEVEL_WARNING       2
/**
 *  \brief
 *      Log for error, warning and info (as memory allocation)
 */
#define LOG_LEVEL_INFO          3

/**
 *  \brief
 *      Define library log level (for debug build)
 */
#define LIB_LOG_LEVEL           LOG_LEVEL_ERROR

/**
 *  \brief
 *      Set it to 1 to enable KDebug logging (Gens KMod) to log some errors (as memory allocation).
 */
#if (DEBUG != 0)
    #define LIB_DEBUG           1
#else
    #define LIB_DEBUG           0
    #undef LIB_LOG_LEVEL
    #define LIB_LOG_LEVEL       LOG_LEVEL_DISABLE
#endif

/**
 *  \brief
 *      Set it to 1 if you want to force Z80 halt during DMA operation (default).<br>
 *      Some Megadrive models need it to prevent some possible DMA transfer corruption or even 68000 memory or Z80 invalid data fetch in very rare case.<br>
 *      This actually happen when Z80 access the main BUS exactly at same time you trigger a DMA operation.<br>
 *      If you are 100% sure that you are actually avoiding that case you may try to disable the flag (at your own risk though).
 */
#define HALT_Z80_ON_DMA         1

/**
 *  \brief
 *      Set it to 1 if you want to force Z80 halt during IO port (controller) accesses.<br>
 *      Some Megadrive models (as some MD2) need it to prevent some possible (but very rare) Z80 corruption bugs
 *      (may happen when Z80 access the main BUS during IO port access from 68K).
 */
#define HALT_Z80_ON_IO          1

/**
 *  \brief
 *      Set it to 1 if you want to completely disable DMA transfers (for testing purpose) and replace them with (slower) software CPU copy.
 */
#define DMA_DISABLED            0

/**
 *  \brief
 *      Set it to 1 if you want to use the old sprite engine instead of the new one.<br>
 *      The old sprite engine allow to access or change the internal VDP sprite indexes, this can be useful in some situations (as sprite multiplexing).<br>
 *      The new sprite engine provide better VDP sprite usage in general allowing to display a bit more sprites in practice.
 */
#define LEGACY_SPRITE_ENGINE    0

/**
 * \brief
 *      Set it to 1 to use the original SGDK's error handling screen and vectors.<br>
 *      Otherwise error handler from the MD Debugger project is used, which supports source code symbols with "debug" build proifle and backtrace.<br>
 *
 *      The new error handler, however, completely takes over exception vectors and doesn't allow to set your own callbacks for errors (e.g. addressErrorCB).<br>
 *      If you rely on callbacks to override error handling behavior, consider using legacy system instead.
 */
#define LEGACY_ERROR_HANDLER    0

/**
 *  \brief
 *      Set it to 1 to enable automatic bank switch using official SEGA mapper for ROM > 4MB.
 *
 *      When automatic bank switch is enabled all internal BIN data structures declared in .far_rodata section
 *      will be accessed using BANK_getFarData(..) method (mapper.c). That may impact performance quite a bit
 *      it's why it's disabled by default if you don't require bank switch.
 */
#define ENABLE_BANK_SWITCH      0

/**
 *  \brief
 *      Set it to 1 if you want to use newlib with SGDK.<br>
 *      That will disable any standard methods from SGDK to replace them by newlib library implementation.
 */
#define ENABLE_NEWLIB           0

/**
 *  \brief
 *      Set it to 1 if you want to have the kit intro logo
 */
#define ENABLE_LOGO             0

#if (ENABLE_LOGO != 0)

/**
 *  \brief
 *      Set it to 1 if you want zoom intro logo effect instead of classic fading
 */
#define ZOOMING_LOGO            0

#endif // ENABLE_LOGO


/**
 *  \brief
 *      Set it to 1 if you want to use EVERDRIVE programming methods (written by Krikzz).
 */
#define MODULE_EVERDRIVE        0

/**
 *  \brief
 *      Set it to 1 if you want to use FAT16 methods for Everdrive cart (written by Krikzz).<br>
 *      This cost a bit more than 1 KB of RAM.
 */
#define MODULE_FAT16            0

// FAT16 need EVERDRIVE
#if ((MODULE_EVERDRIVE == 0) && (MODULE_FAT16 != 0))
#error "Cannot enable FAT16 module without EVERDRIVE module"
#endif

/**
 *  \brief
 *      Set it to 1 if you want to use PORT_2 or PORT_EXT as Serial port COMM.
 */
#define MODULE_SERIAL            0

/**
 *  \brief
 *      Set it to 1 if you want to enable MegaWiFi functions and support code (written by Jesus Alonso - doragasu)
 */
#define MODULE_MEGAWIFI         0
#if MODULE_MEGAWIFI

#define MEGAWIFI_IMPLEMENTATION_CROSS    0x01    // Cross (Serial)
#define MEGAWIFI_IMPLEMENTATION_MW_CART  0x02    // MegaWiFi Cart: Defined to use MegaWiFi Cart distributions
#define MEGAWIFI_IMPLEMENTATION_ED       0x04    // EverDrive: Defined to use EverDrive distributions (testing purposes)
#define MEGAWIFI_IMPLEMENTATION       (MEGAWIFI_IMPLEMENTATION_CROSS | MEGAWIFI_IMPLEMENTATION_ED) // Set the implementation to use
// Caution USING BOTH MW_CART AND EVERDRIVE IMPLEMENTATIONS MAY CAUSE ISSUES AS THEY BOTH USE SAME COMM VTABLE STRUCTURE
// MAKE SURE TO TEST PROPERLY IF YOU ENABLE BOTH IMPLEMENTATIONS

// Check that if using cross implementation, serial module is enabled
// Serial module is required for cross implementation
#if ((MODULE_SERIAL == 0) && (MEGAWIFI_IMPLEMENTATION & MEGAWIFI_IMPLEMENTATION_CROSS))
#error "Cannot enable MegaWiFi cross implementation without SERIAL module"
#endif
// Check that if using EverDrive implementation, EverDrive module is enabled
// Switching banks is required for EverDrive implementation
#if ((ENABLE_BANK_SWITCH == 0) && (MEGAWIFI_IMPLEMENTATION & MEGAWIFI_IMPLEMENTATION_ED))
#error "Cannot enable MegaWiFi module without BANK SWITCH"
#endif

#endif // MODULE_MEGAWIFI
/**
 *  \brief
 *      Set it to 1 if you want to enable Flash Save functions (written by Jesus Alonso - doragasu).<br>
 *      There is no reason to disable it as it doesn't consume extra memory
 */
#define MODULE_FLASHSAVE        1

/**
 *  \brief
 *      Set it to 1 if you want to enable the TTY text console module (written by Andreas Dietrich).<br>
 *      It consume about 28 bytes of memory when enabled.
 */
#define MODULE_CONSOLE          1

/**
 *  \brief
 *      Set it to 1 if you want to enable Link Cable Protocol functions (written by BlodTor)
 */
#define MODULE_LINK_CABLE       0

#endif // _CONFIG_