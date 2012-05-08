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


/**
 * \def VDP_DMA_VRAM
 *      VRAM location for DMA operation.
 */
#define VDP_DMA_VRAM    0
/**
 * \def VDP_DMA_CRAM
 *      CRAM location for DMA operation.
 */
#define VDP_DMA_CRAM    1
/**
 * \def VDP_DMA_VSRAM
 *      VSRAM location for DMA operation.
 */
#define VDP_DMA_VSRAM   2


/**
 * \def VDP_doVRamDMA
 *      Start DMA transfert to VRAM.
 *
 * \param from
 *      Source address.
 * \param to
 *      Destination address in VRAM.
 * \param len
 *      Number of byte to transfert.
 */
#define VDP_doVRamDMA(from, to, len)            \
    VDP_doDMA(VDP_DMA_VRAM, from, to, len)
/**
 * \def VDP_doCRamDMA
 *      Start DMA transfert to CRAM.
 *
 * \param from
 *      Source address.
 * \param to
 *      Destination address in CRAM.
 * \param len
 *      Number of word to transfert.
 */
#define VDP_doCRamDMA(from, to, len)            \
    VDP_doDMA(VDP_DMA_CRAM, from, to, len)
/**
 * \def VDP_doVSRamDMA
 *      Start DMA transfert to VSRAM.
 *
 * \param from
 *      Source address.
 * \param to
 *      Destination address in VSRAM.
 * \param len
 *      Number of word to transfert.
 */
#define VDP_doVSRamDMA(from, to, len)           \
    VDP_doDMA(VDP_DMA_VSRAM, from, to, len)

/**
 * \def VDP_doVRamDMAFill
 *      Start DMA VRAM fill operation.
 *
 * \param to
 *      Destination address in VRAM.
 * \param len
 *      Number of byte to fill.
 * \param value
 *      Fill value (byte).
 */
#define VDP_doVRamDMAFill(to, len, value)       \
    VDP_doDMAFill(VDP_DMA_VRAM, to, len, value)
/**
 * \def VDP_doCRamDMAFill
 *      Start DMA CRAM fill operation.
 *
 * \param to
 *      Destination address in CRAM.
 * \param len
 *      Number of word to fill.
 * \param value
 *      Fill value (byte).
 */
#define VDP_doCRamDMAFill(to, len, value)       \
    VDP_doDMAFill(VDP_DMA_CRAM, to, len, value)
/**
 * \def VDP_doVSRamDMAFill
 *      Start DMA VSRAM fill operation.
 *
 * \param to
 *      Destination address in VSRAM.
 * \param len
 *      Number of word to fill.
 * \param value
 *      Fill value (byte).
 */
#define VDP_doVSRamDMAFill(to, len, value)      \
    VDP_doDMAFill(VDP_DMA_VSRAM, to, len, value)

/**
 * \def VDP_doVRamDMACopy
 *      Start DMA VRAM copy operation.
 *
 * \param from
 *      Source address in VRAM.
 * \param to
 *      Destination address in VRAM.
 * \param len
 *      Number of byte to copy.
 */
#define VDP_doVRamDMACopy(from, to, len)        \
    VDP_doDMACopy(VDP_DMA_VRAM, from, to, len)
/**
 * \def VDP_doCRamDMACopy
 *      Start DMA CRAM copy operation.
 *
 * \param from
 *      Source address in CRAM.
 * \param to
 *      Destination address in CRAM.
 * \param len
 *      Number of byte to copy.
 */
#define VDP_doCRamDMACopy(from, to, len)        \
    VDP_doDMACopy(VDP_DMA_CRAM, from, to, len)
/**
 * \def VDP_doVSRamDMACopy
 *      Start DMA VSRAM copy operation.
 *
 * \param from
 *      Source address in VSRAM.
 * \param to
 *      Destination address in VSRAM.
 * \param len
 *      Number of byte to copy.
 */
#define VDP_doVSRamDMACopy(from, to, len)       \
    VDP_doDMACopy(VDP_DMA_VSRAM, from, to, len)


/**
 * \brief
 *      Do DMA transfert operation.
 *
 * \param location
 *      Destination location.<br/>
 *      Accepted values:<br/>
 *      - VDP_DMA_VRAM (for VRAM transfert).<br/>
 *      - VDP_DMA_CRAM (for CRAM transfert).<br/>
 *      - VDP_DMA_VSRAM (for VSRAM transfert).<br/>
 * \param from
 *      Source address.
 * \param to
 *      Destination address.
 * \param len
 *      Number of byte/word to transfert.
 */
void VDP_doDMA(u8 location, u32 from, u16 to, u16 len);
/**
 * \brief
 *      Do DMA fill operation.
 *
 * \param location
 *      Destination location.<br/>
 *      Accepted values:<br/>
 *      - VDP_DMA_VRAM (for VRAM fill).<br/>
 *      - VDP_DMA_CRAM (for CRAM fill).<br/>
 *      - VDP_DMA_VSRAM (for VSRAM fill).<br/>
 * \param to
 *      Destination address.
 * \param len
 *      Number of byte/word to fill.
 * \param value
 *      Fill value (byte).
 */
void VDP_doDMAFill(u8 location, u16 to, u16 len, u8 value);
/**
 * \brief
 *      Do DMA copy operation.
 *
 * \param location
 *      Copy location.<br/>
 *      Accepted values:<br/>
 *      - VDP_DMA_VRAM (for VRAM copy).<br/>
 *      - VDP_DMA_CRAM (for CRAM copy).<br/>
 *      - VDP_DMA_VSRAM (for VSRAM copy).<br/>
 * \param from
 *      Source address.
 * \param to
 *      Destination address.
 * \param len
 *      Number of byte/word to copy.
 */
void VDP_doDMACopy(u8 location, u16 from, u16 to, u16 len);


#endif // _VDP_DMA_H_
