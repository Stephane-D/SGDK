/**
 *  \file vdp_dma.h
 *  \brief VDP DMA support
 *  \author Stephane Dallongeville
 *  \date 08/2011
 *
 * This unit provides methods to use the VDP DMA capabilities.
 */

#ifndef _VDP_DMA_H_
#define _VDP_DMA_H_


/**
 *  \def VDP_DMA_VRAM
 *      VRAM location for DMA operation.
 */
#define VDP_DMA_VRAM    0
/**
 *  \def VDP_DMA_CRAM
 *      CRAM location for DMA operation.
 */
#define VDP_DMA_CRAM    1
/**
 *  \def VDP_DMA_VSRAM
 *      VSRAM location for DMA operation.
 */
#define VDP_DMA_VSRAM   2


/**
 *  \def VDP_doVRamDMA
 *      Start DMA transfert to VRAM.
 *
 *  \param from
 *      Source address.
 *  \param to
 *      Destination address in VRAM.
 *  \param len
 *      Number of word to transfert.
 */
#define VDP_doVRamDMA(from, to, len)            \
    VDP_doDMA(VDP_DMA_VRAM, from, to, len)
/**
 *  \def VDP_doCRamDMA
 *      Start DMA transfert to CRAM.
 *
 *  \param from
 *      Source address.
 *  \param to
 *      Destination address in CRAM.
 *  \param len
 *      Number of word to transfert.
 */
#define VDP_doCRamDMA(from, to, len)            \
    VDP_doDMA(VDP_DMA_CRAM, from, to, len)
/**
 *  \def VDP_doVSRamDMA
 *      Start DMA transfert to VSRAM.
 *
 *  \param from
 *      Source address.
 *  \param to
 *      Destination address in VSRAM.
 *  \param len
 *      Number of word to transfert.
 */
#define VDP_doVSRamDMA(from, to, len)           \
    VDP_doDMA(VDP_DMA_VSRAM, from, to, len)


/**
 *  \brief
 *      Do DMA transfert operation.
 *
 *  \param location
 *      Destination location.<br/>
 *      Accepted values:<br/>
 *      - VDP_DMA_VRAM (for VRAM transfert).<br/>
 *      - VDP_DMA_CRAM (for CRAM transfert).<br/>
 *      - VDP_DMA_VSRAM (for VSRAM transfert).<br/>
 *  \param from
 *      Source address.
 *  \param to
 *      Destination address.
 *  \param len
 *      Number of word to transfert.
 *  \param vramStep
 *      VRam address increment value (-1 to 255).<br/>
 *      By default you should set it to 2 for normal copy operation but you can use different value
 *      for specific operation.<br/>
 *      -1 means the VRam address increment register won't be modified (use current value).
 */
void VDP_doDMAEx(u8 location, u32 from, u16 to, u16 len, s16 vramStep);
/**
 *  \brief
 *      Do DMA transfert operation.
 *
 *  \param location
 *      Destination location.<br/>
 *      Accepted values:<br/>
 *      - VDP_DMA_VRAM (for VRAM transfert).<br/>
 *      - VDP_DMA_CRAM (for CRAM transfert).<br/>
 *      - VDP_DMA_VSRAM (for VSRAM transfert).<br/>
 *  \param from
 *      Source address.
 *  \param to
 *      Destination address.
 *  \param len
 *      Number of word to transfert.
 */
void VDP_doDMA(u8 location, u32 from, u16 to, u16 len);
/**
 *  \brief
 *      Do VRAM DMA fill operation.
 *
 *  \param to
 *      Destination address.
 *  \param len
 *      Number of byte to fill.
 *  \param value
 *      Fill value (byte).
 */
void VDP_doVRamDMAFill(u16 to, u16 len, u8 value);
/**
 *  \brief
 *      Do VRAM DMA copy operation.

 *  \param from
 *      Source address.
 *  \param to
 *      Destination address.
 *  \param len
 *      Number of byte to copy.
 */
void VDP_doVRamDMACopy(u16 from, u16 to, u16 len);


#endif // _VDP_DMA_H_
