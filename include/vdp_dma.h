/**
 * \file vdp_dma.h
 * \brief VDP DMA support
 * \author Stephane Dallongeville
 * \date 08/2011
 *
 * This unit provides methods to use the VDP DMA capabilities.
 */

#ifndef _VDP_DMA_H_
#define _VDP_DMA_H_


#define VDP_DMA_VRAM    0
#define VDP_DMA_CRAM    1
#define VDP_DMA_VSRAM   2


#define VDP_doVRamDMA(from, to, len)            \
    VDP_doDMA(VDP_DMA_VRAM, from, to, len)
#define VDP_doCRamDMA(from, to, len)            \
    VDP_doDMA(VDP_DMA_CRAM, from, to, len)
#define VDP_doVSRamDMA(from, to, len)           \
    VDP_doDMA(VDP_DMA_VSRAM, from, to, len)

#define VDP_doVRamDMAFill(to, len, value)       \
    VDP_doDMAFill(VDP_DMA_VRAM, to, len, value)
#define VDP_doCRamDMAFill(to, len, value)       \
    VDP_doDMAFill(VDP_DMA_CRAM, to, len, value)
#define VDP_doVSRamDMAFill(to, len, value)      \
    VDP_doDMAFill(VDP_DMA_VSRAM, to, len, value)

#define VDP_doVRamDMACopy(from, to, len)        \
    VDP_doDMACopy(VDP_DMA_VRAM, from, to, len)
#define VDP_doCRamDMACopy(from, to, len)        \
    VDP_doDMACopy(VDP_DMA_CRAM, from, to, len)
#define VDP_doVSRamDMACopy(from, to, len)       \
    VDP_doDMACopy(VDP_DMA_VSRAM, from, to, len)


void VDP_doDMA(u8 location, u32 from, u16 to, u16 len);
void VDP_doDMAFill(u8 location, u16 to, u16 len, u8 value);
void VDP_doDMACopy(u8 location, u16 from, u16 to, u16 len);


#endif // _VDP_DMA_H_
