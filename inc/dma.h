/**
 *  \file dma.h
 *  \brief DMA support
 *  \author Stephane Dallongeville
 *  \date 04/2015
 *
 * This unit provides methods to use the hardware DMA capabilities.
 */

#include "config.h"
#include "types.h"

#ifndef _DMA_H_
#define _DMA_H_


/**
 *  \brief
 *      VRAM destination for DMA operation.
 */
#define DMA_VRAM    0
/**
 *  \brief
 *      CRAM destination for DMA operation.
 */
#define DMA_CRAM    1
/**
 *  \brief
 *      VSRAM destination for DMA operation.
 */
#define DMA_VSRAM   2


/**
 *  \brief
 *      DMA transfer definition (used for DMA queue)
 */
typedef struct
{
    u32 regStepLenL;    // (0x8F00 | step) | ((0x9300 | (len & 0xFF)) << 16)
    u32 regLenHAddrL;   // (0x9400 | ((len >> 8) & 0xFF)) | ((0x9500 | ((addr >> 1) & 0xFF)) << 16)
    u32 regAddrMAddrH;  // (0x9600 | ((addr >> 9) & 0xFF)) | ((0x9700 | ((addr >> 17) & 0x7F)) << 16)
    u32 regCtrlWrite;   // GFX_DMA_VRAMCOPY_ADDR(to)
} DMAOpInfo;

/**
 *  \brief
 *      DMA queue structure
 */
extern DMAOpInfo *dmaQueues;


/**
 *  \brief
 *      Initialize the DMA queue sub system.
 *
 *      SGDK automatically call this method on hard reset so you don't need to call it again unless
 *      you want to change the default parameters.
 *
 *  \param size
 *      The queue size (0 = default size = 64).
 *  \param capacity
 *      The maximum allowed size (in bytes) to transfer per #DMA_flushQueue() call (0 = default = no limit).
 */
void DMA_init(u16 size, u16 capacity);

/**
 *  \brief
 *      Returns TRUE if the DMA_flushQueue() method is automatically called at VBlank
 *      to process DMA operations pending in the queue.
 *
 *  \see DMA_setAutoFlush()
 *  \see DMA_flushQueue()
 */
u16 DMA_getAutoFlush();
/**
 *  \brief
 *      If set to TRUE (default) then the DMA_flushQueue() method is automatically called at VBlank
 *      to process DMA operations pending in the queue otherwise you have to call the DMA_flushQueue()
 *      method by yourself.
 *
 *  \see DMA_flushQueue()
 */
void DMA_setAutoFlush(u16 value);
/**
 *  \brief
 *      Return the "over capacity ignore" state (see #DMA_setIgnoreOverCapacity(..) method).
 *
 *  \see DMA_setIgnoreOverCapacity()
 */
u16 DMA_getIgnoreOverCapacity();
/**
 *  \brief
 *      Set the "over capacity" DMA queue strategy.
 *
 *      When set to <i>TRUE</i> any DMA operation queued after we reached the maximum defined transfer capacity
 *      with #DMA_setMaxTransferSize(..) are ignored.<br>
 *      When set to <i>FALSE</i> DMA operations are postponed to the next frame.
 *
 *  \see DMA_setMaxTransferSize()
 */
void DMA_setIgnoreOverCapacity(u16 value);
/**
 *  \brief
 *      Returns the maximum allowed size (in bytes) to transfer per #DMA_flushQueue() call.
 *
 *  \see DMA_setMaxTransferSize()
 */
s16 DMA_getMaxTransferSize();
/**
 *  \brief
 *      Sets the maximum amount of data (in bytes) to transfer per #DMA_flushQueue() call.<br>
 *      VBlank period allows to transfer up to 7.2 KB on NTSC system and 15 KB on PAL system.<br>
 *      By default there is no size limit (0).
 *
 *  \param value
 *      The maximum amount of data (in KiloBytes) to transfer during DMA_flushQueue() operation.<br>
 *      Use <b>0</b> for no limit.
 *
 *  \see DMA_setIgnoreOverCapacity()
 *  \see DMA_flushQueue()
 */
void DMA_setMaxTransferSize(s16 value);

/**
 *  \brief
 *      Clears the DMA queue, any queued operation is lost.<br>
 *  \see DMA_flushQueue()
 */
void DMA_clearQueue();
/**
 *  \brief
 *      Send the content of the DMA queue to the VDP:<br>
 *      Each pending DMA operation is sent to the VDP and processed as quickly as possible.<br>
 *      This method returns when all DMA operations present in the queue has been transfered.<br>
 *      Note that this method is automatically called at VBlank time and you shouldn't call yourself except if
 *      you want to process it before vblank (if you manually extend blank period with h-int for instance) in which case
 *      you can disable the auto flush feature (see DMA_setAutoFlush(...))
 *
 *  \see DMA_queue(...)
 *  \see DMA_setAutoFlush(...)
 */
void DMA_flushQueue();

/**
 *  \brief
 *      Returns the number of transfer currently pending in the DMA queue.
 */
u16 DMA_getQueueSize();
/**
 *  \brief
 *      Returns the size (in byte) of data to be transfered currently present in the DMA queue.<br>
 *      NTSC frame allows about 7.6 KB of data to be transfered during VBlank (in H40) while
 *      PAL frame allows about 17 KB (in H40).
 */
u32 DMA_getQueueTransferSize();

/**
 *  \brief
 *      Queues the specified DMA transfer operation in the DMA queue.<br>
 *      The idea of the DMA queue is to burst all DMA operations during VBLank to maximize bandwidth usage.<br>
 *
 *  \param location
 *      Destination location.<br>
 *      Accepted values:<br>
 *      - DMA_VRAM (for VRAM transfert).<br>
 *      - DMA_CRAM (for CRAM transfert).<br>
 *      - DMA_VSRAM (for VSRAM transfert).<br>
 *  \param from
 *      Source address.
 *  \param to
 *      Destination address.
 *  \param len
 *      Number of word to transfert.
 *  \param step
 *      destination (VRAM/VSRAM/CRAM) address increment step after each write (0 to 255).<br>
 *      By default you should set it to 2 for normal copy operation but you can use different value
 *      for specific operation.<br>
 *  \return
 *      FALSE if the operation failed (queue is full)
 *  \see DMA_do(..)
 */
u16 DMA_queueDma(u8 location, u32 from, u16 to, u16 len, u16 step);
/**
 *  \brief
 *      Do DMA transfer operation immediately
 *
 *  \param location
 *      Destination location.<br>
 *      Accepted values:<br>
 *      - DMA_VRAM (for VRAM transfert).<br>
 *      - DMA_CRAM (for CRAM transfert).<br>
 *      - DMA_VSRAM (for VSRAM transfert).<br>
 *  \param from
 *      Source address.
 *  \param to
 *      Destination address.
 *  \param len
 *      Number of word to transfert.
 *  \param step
 *      destination (VRAM/VSRAM/CRAM) address increment step after each write (-1 to keep current step).<br>
 *      By default you should set it to 2 for normal copy operation but you can use different value
 *      for specific operation.
 *  \see DMA_queue(..)
 */
void DMA_doDma(u8 location, u32 from, u16 to, u16 len, s16 step);

/**
 *  \brief
 *      Wait current DMA fill/copy operation to complete.
 */
void DMA_waitCompletion();

/**
 *  \brief
 *      Do VRAM DMA fill operation.
 *
 *  \param to
 *      Destination address.
 *  \param len
 *      Number of byte to fill (minimum is 2 for even addr destination and 3 for odd addr destination).<br>
 *      A value of 0 mean 0x10000.
 *  \param value
 *      Fill value (byte).
 *  \param step
 *      VRAM address increment step after each write (-1 to keep current step).<br>
 *      should be 1 for a classic fill operation but you can use different value for specific operation.
 */
void DMA_doVRamFill(u16 to, u16 len, u8 value, s16 step);
/**
 *  \brief
 *      Do VRAM DMA copy operation.

 *  \param from
 *      Source address.
 *  \param to
 *      Destination address.
 *  \param len
 *      Number of byte to copy.
 *  \param step
 *      VRAM address increment step after each write (-1 to keep current step).<br>
 *      should be 1 for a classic copy operation but you can use different value for specific operation.
 */
void DMA_doVRamCopy(u16 from, u16 to, u16 len, s16 step);


#endif // _DMA_H_
