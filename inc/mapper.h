/**
 *  \file mapper.h
 *  \brief Mapper / bank switch methods.
 *  \author Stephane Dallongeville
 *  \date 01/2020
 *
 * This unit provides tools to deal with ROM larger than 4MB.<br>
 * It allows to do classic bank switching using the official SEGA mapper but also provide methods to get easy access to "far" data.<br>
 * Note that you can use the ENABLE_BANK_SWITCH flag in config.h file to enable automatic bank switch on binary data access.<br>
 *<br>
 * SEGA official mapper description (taken from Segaretro.org):<br>
 * The bankswitching mechanism is very simple. It views the addressable 4 mega-bytes of ROM as 8 512KB regions.<br>
 * The first area, 0x000000-0x07FFFF is fixed and cannot be remapped because that is where the vector table resides.<br>
 * The banking registers on the cartridge work by allocating the 512KB chunk to a given part of the addressable 4MB ROM space.<br>
 * Below are the registers and what range they correspond to. The value written to a register will cause the specified 512KB page to be mapped to that region.<br>
 * A page is specified with 6 bits (bits 7 and 6 are always 0) thus allowing a possible 64 pages = 32 MB (SSFII only has 10, though.)<br>
 *<br>
 *       0xA130F3:       0x080000 - 0x0FFFFF<br>
 *       0xA130F5:       0x100000 - 0x17FFFF<br>
 *       0xA130F7:       0x180000 - 0x1FFFFF<br>
 *       0xA130F9:       0x200000 - 0x27FFFF<br>
 *       0xA130FB:       0x280000 - 0x2FFFFF<br>
 *       0xA130FD:       0x300000 - 0x37FFFF<br>
 *       0xA130FF:       0x380000 - 0x3FFFFF<br>
 *<br>
 * The registers are accessed through byte writes.<br>
 * Examples:<br>
 *       If 0x01 is written to register 0xA130FF, 0x080000-0x0FFFFF is visible at 0x380000-0x3FFFFF.<br>
 *       If 0x08 is written to register 0xA130F9, the first 512KB of the normally invisible upper 1MB of ROM is now visible at 0x200000-0x27FFFF.<br>
 *<br>
 * The registers simply represent address ranges in the 4MB ROM area and you can page in data to these ranges by specifying the bank #
 */

#ifndef _MAPPER_H_
#define _MAPPER_H_


#include "config.h"
#include "types.h"


#define MAPPER_BASE     0xA130F1

#define BANK_SIZE       0x80000
#define BANK_IN_MASK    (BANK_SIZE - 1)
#define BANK_OUT_MASK   (0xFFFFFFFF ^ BANK_IN_MASK)

/**
 *  \brief
 *      Give access to specified 'far' data through SEGA official bank switch mechanism if needed.
 *
 *  \see #ENABLE_BANK_SWITCH flag in config.h file
 *  \see #SYS_getFarData(..)
 *  \see #SYS_getFarDataSafe(..)
 */
#if (ENABLE_BANK_SWITCH != 0)
    #define FAR(data) SYS_getFarData((void*) (data))
    #define FAR_SAFE(data, size) SYS_getFarDataSafe((void*) (data), size)
#else
    #define FAR(data) data
    #define FAR_SAFE(data, size) data
#endif


/**
 *  \brief
 *      Returns the current bank of specified region index.
 *
 *  \param regionIndex the 512KB region index we want to get. Accepted values: 1-7 as region 0 (0x000000-0x07FFFF) is fixed.
 *  \return the effective 512KB data bank index mapped on this region (0 to 63)
 */
u16 SYS_getBank(u16 regionIndex);
/**
 *  \brief
 *      Set the current bank of specified region index.
 *
 *  \param regionIndex the 512KB region index we want to set. Accepted values: 1-7 as region 0 (0x000000-0x07FFFF) is fixed.
 *  \param bankIndex the effective 512KB data bank index mapped on this region. Accepted values: 0-63
 */
void SYS_setBank(u16 regionIndex, u16 bankIndex);

/**
 *  \brief
 *      Make the given binary data ressource accessible and return a pointer to it.
 *
 *  \param data data we want to access.
 *
 * This method will use bank switching to make the specified data accessible and return a valid pointer to it.<br>
 * <b>WARNING:</b> this method use the 0x00300000-0x003FFFFF range (2 regions) to make the requested data accessible using bank switching mechanism.<br>
 * If data bank is already accessible it re-uses the region otherwise it will change bank of one of the region so be careful of that if you want to access data
 * from different data bank at same time
 *
 *  \see SYS_getFarDataEx
 *  \see SYS_getFarDataSafe
 */
void* SYS_getFarData(void* data);
/**
 *  \brief
 *      Make the given binary data ressource accessible and return a pointer to it.
 *
 *  \param data far data we want to access.
 *  \param high if set to TRUE then we use the high remappable bank for the FAR acces otherwise we use the low one
 *
 * This method will use bank switching to make the specified data accessible and return a valid pointer to it.<br>
 * It will use the 0x00300000-0x0037FFFF or 0x00380000-0x003FFFFF region depending the value of <i>high</i> parameter.<br>
 *
 *  \see SYS_getFarData
 *  \see SYS_getFarDataSafe
 */
void* SYS_getFarDataEx(void* data, bool high);
/**
 *  \brief
 *      Returns TRUE if given binary data is crossing 2 512 KB banks
 *
 *  \param data far data pointer
 *  \param size size (in byte) of the far data block
 *
 * This method return TRUE is the given binary data block is crossing 2 512 KB banks
 *
 *  \see SYS_getFarDataSafe
 */
bool SYS_isCrossingBank(void* data, u32 size);
/**
 *  \brief
 *      Make the given binary data ressource accessible and return a pointer to it (safe version with possible bank crossing)
 *
 *  \param data far data we want to access.
 *  \param size size (in byte) of the far data block we want to access.<br>
 *     Note that size should be > 0, if you don't the size then use SYS_getFarData(..) method instead.
 *
 * This method will use bank switching to make the specified data accessible and return a valid pointer to it.<br>
 * <b>WARNING:</b> this method use the 0x00300000-0x003FFFFF range (2 regions) to make the requested data accessible using bank switching mechanism.<br>
 * If data bank is already accessible it re-uses the region otherwise it will change bank of one of the region so be careful of that if you want to access data
 * from different data bank at same time :p<br>
 * The method checks if the data is crossing banks in which case it will set the 2 switchable/remappable regions to make the data fully accessible.
 *
 *  \see SYS_getFarDataSafeEx
 *  \see SYS_getFarData
 */
void* SYS_getFarDataSafe(void* data, u32 size);
/**
 *  \brief
 *      Make the given binary data ressource accessible and return a pointer to it (safe version with possible bank crossing)
 *
 *  \param data address of far data we want to access.
 *  \param size size (in byte) of the far data block we want to access.<br>
 *     Note that size should be > 0, if you don't the size then use SYS_getFarData(..) method instead.
 *  \param high if set to TRUE then we use the high remappable bank for the FAR acces otherwise we use the low one
 *
 * This method will use bank switching to make the specified data accessible and return a valid pointer to it.<br>
 * It will use the 0x00300000-0x0037FFFF or 0x00380000-0x003FFFFF region depending the value of <i>high</i> parameter.<br>
 * The method checks if the data is crossing banks in which case it will set the 2 switchable/remappable regions to make the data fully accessible.
 *
 *  \see SYS_getFarDataSafe
 */
void* SYS_getFarDataSafeEx(void* data, u32 size, bool high);


#endif // _MAPPER_H_
