/**
 *  \file sram.h
 *  \brief SRAM (Static RAM used for save backup) support.
 *  \author Chilly Willy & Stephane Dallongeville
 *  \date 08/2012
 *
 * This unit provides methods to read from or write to SRAM.<br>
 * By default we suppose SRAM is 8bit and connected to odd address.<br>
 * You can change to even address by changing SRAM_BASE from 0x200001 to 0x200000 and rebuild the library.<br>
 *<br>
 * Informations about SRAM (taken from Segaretro.org):<br>
 * The regions specified by 0xA130F9-0xA130FF (0x200000-0x3FFFFF) can be either ROM or RAM and can be write-protected.<br>
 * Here is the layout of the register as far as I know:<br>
 *<br>
 *        7  6  5  4  3  2  1  0<br>
 *      +-----------------------+<br>
 *      |??|??|??|??|??|??|WP|MD|<br>
 *      +-----------------------+<br>
 *<br>
 *      MD:     Mode -- 0 = ROM, 1 = RAM<br>
 *      WP:     Write protect -- 0 = writable, 1 = not writable
 */

#ifndef _SRAM_H_
#define _SRAM_H_


#include "mapper.h"


#define SRAM_CONTROL    MAPPER_BASE
#define SRAM_BASE       0x200001


/**
 *  \brief
 *      Enable SRAM in Read Write mode.
 */
void SRAM_enable();
/**
 *  \brief
 *      Enable SRAM in Read Only mode.
 */
void SRAM_enableRO();
/**
 *  \brief
 *      Disable SRAM.
 */
void SRAM_disable();

/**
 *  \brief
 *      Read a byte from the SRAM.
 *
 *  \param offset
 *      Offset where we want to read.
 *  \return value.
 */
u8 SRAM_readByte(u32 offset);
/**
 *  \brief
 *      Read a word from the SRAM.
 *
 *  \param offset
 *      Offset where we want to read.
 *  \return value.
 */
u16 SRAM_readWord(u32 offset);
/**
 *  \brief
 *      Read a long from the SRAM.
 *
 *  \param offset
 *      Offset where we want to read.
 *  \return value.
 */
u32 SRAM_readLong(u32 offset);
/**
 *  \brief
 *      Write a byte to the SRAM.
 *
 *  \param offset
 *      Offset where we want to write.
 *  \param val
 *      Value wto write.
 */
void SRAM_writeByte(u32 offset, u8 val);
/**
 *  \brief
 *      Write a word to the SRAM.
 *
 *  \param offset
 *      Offset where we want to write.
 *  \param val
 *      Value wto write.
 */
void SRAM_writeWord(u32 offset, u16 val);
/**
 *  \brief
 *      Write a long to the SRAM.
 *
 *  \param offset
 *      Offset where we want to write.
 *  \param val
 *      Value wto write.
 */
void SRAM_writeLong(u32 offset, u32 val);


#endif // _SRAM_H_
