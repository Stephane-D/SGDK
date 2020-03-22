/**
 *  \file vdp_dma.h
 *  \brief VDP DMA support
 *  \author Stephane Dallongeville
 *  \date 08/2011
 *  \deprecated Use dma.h unit instead
 *
 * This unit provides methods to use the VDP DMA capabilities.
 */

#ifndef _VDP_DMA_H_
#define _VDP_DMA_H_


/**
 *  \brief
 *      VRAM location for DMA operation.
 */
#define VDP_DMA_VRAM    0
/**
 *  \brief
 *      CRAM location for DMA operation.
 */
#define VDP_DMA_CRAM    1
/**
 *  \brief
 *      VSRAM location for DMA operation.
 */
#define VDP_DMA_VSRAM   2


/**
 *  \deprecated Use DMA_xxx methods instead
 */
#define VDP_doVRamDMA(from, to, len)            \
    VDP_doDMA(VDP_DMA_VRAM, from, to, len)
/**
 *  \deprecated Use DMA_xxx methods instead
 */
#define VDP_doCRamDMA(from, to, len)            \
    VDP_doDMA(VDP_DMA_CRAM, from, to, len)
/**
 *  \deprecated Use DMA_xxx methods instead
 */
#define VDP_doVSRamDMA(from, to, len)           \
    VDP_doDMA(VDP_DMA_VSRAM, from, to, len)


/**
 *  \deprecated Use DMA_xxx methods instead
 */
void VDP_doDMAEx(u8 location, u32 from, u16 to, u16 len, s16 vramStep);
/**
 *  \deprecated Use DMA_xxx methods instead
 */
void VDP_doDMA(u8 location, u32 from, u16 to, u16 len);
/**
 *  \deprecated Use DMA_xxx methods instead
 */
void VDP_doVRamDMAFill(u16 to, u16 len, u8 value);
/**
 *  \deprecated Use DMA_xxx methods instead
 */
void VDP_doVRamDMACopy(u16 from, u16 to, u16 len);


#endif // _VDP_DMA_H_
