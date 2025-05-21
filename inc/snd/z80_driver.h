#ifndef _Z80_DRIVER_H_
#define _Z80_DRIVER_H_


#include "types.h"


/**
 *  \brief
 *      Function pointer type for loading a Z80 driver binary.
 *
 *  \param driver
 *      Pointer to the raw driver binary data.
 *  \param size
 *      Size in bytes of the driver binary data.
 */
typedef void (*Z80DriverLoader)(const u8* driver, const u16 size);

/**
 *  \brief
 *      Z80 driver boot configuration structure.<br>
 *      Used to configure driver loading behavior.
 *
 *  \param waitReady
 *      If TRUE, the driver load routine should wait for driver readiness before continuing.
 *  \param loader
 *      Function pointer to the function responsible for loading the Z80 driver binary.
 */
typedef struct
{
    bool waitReady;
    Z80DriverLoader loader;
} Z80DriverBoot;

/**
 *  \brief
 *      Z80 Driver interface definition.<br>
 *      Do not call the members directly, let #Z80_loadDriver(..) handle it.<br>
 *      Note: Drivers should also tweak Z80_DRV_STATUS, see #Z80_isDriverReady(..).
 *
 *  \param load
 *      Function pointer to the load routine.<br>
 *      Accepts a Z80DriverBoot structure to configure the loading behavior.
 *  \param unload
 *      Function pointer to the unload routine.<br>
 *      May be NULL or empty.
 */
typedef struct
{
    void (*load)(const Z80DriverBoot boot);
    void (*unload)(void);
} Z80Driver;


#endif // _Z80_DRIVER_H_
