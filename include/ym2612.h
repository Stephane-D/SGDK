/**
 * \file ym2612.h
 * \brief YM2612 support
 * \author Stephane Dallongeville
 * \date 08/2011
 *
 * This unit provides access to the YM2612 through the 68000 CPU
 */

#ifndef _YM2612_H_
#define _YM2612_H_

#define YM2612_BASEPORT     0xA04000

#define PSG_ENVELOPE_MIN    15
#define PSG_ENVELOPE_MAX    0


void YM2612_reset();

u8   YM2612_read(const u16 port);
void YM2612_write(const u16 port, const u8 data);
void YM2612_writeSafe(const u16 port, const u8 data);
void YM2612_writeReg(const u16 part, const u8 reg, const u8 data);
void YM2612_writeRegSafe(const u16 part, const u8 reg, const u8 data);

void YM2612_enableDAC();
void YM2612_disableDAC();


#endif // _YM2612_H_
