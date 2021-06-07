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

#define DMA_QUEUE_SIZE_DEFAULT          80
#define DMA_QUEUE_SIZE_MIN              32

#define DMA_TRANSFER_CAPACITY_NTSC      7200
#define DMA_TRANSFER_CAPACITY_PAL_LOW   8000
#define DMA_TRANSFER_CAPACITY_PAL_MAX   15000

#define DMA_BUFFER_SIZE_NTSC            DMA_TRANSFER_CAPACITY_NTSC
#define DMA_BUFFER_SIZE_PAL_LOW         DMA_TRANSFER_CAPACITY_PAL_LOW
#define DMA_BUFFER_SIZE_PAL_MAX         (14 * 1024)
#define DMA_BUFFER_SIZE_MIN             (2 * 1024)


/**
 *  \brief
 *      VRAM transfer method
 */
typedef enum
{
    CPU = 0,            /**< Transfer through the CPU immediately (slower.. useful for testing purpose mainly) */
    DMA = 1,            /**< Transfer through DMA immediately, using DMA is faster but can lock Z80 execution */
    DMA_QUEUE = 2,      /**< Put in the DMA queue so it will be transferred at next VBlank. Using DMA is faster but can lock Z80 execution */
    DMA_QUEUE_COPY = 3  /**< Copy the buffer and put in the DMA queue so it will be transferred at next VBlank. Using DMA is faster but can lock Z80 execution */
} TransferMethod;


/**
 *  \brief
 *      DMA transfer definition (used for DMA queue)
 */
typedef struct
{
    u16 regLenL;        // (newLen & 0xFF) | 0x9300;
    u16 regLenH;        // ((newLen >> 8) & 0xFF) | 0x9400;
    u32 regAddrMStep;   // (((addr << 7) & 0xFF0000) | 0x96008F00) + step;
    u32 regAddrHAddrL;  // ((addr >> 1) & 0x7F00FF) | 0x97009500;
    u32 regCtrlWrite;   // GFX_DMA_VRAMCOPY_ADDR(to)
} DMAOpInfo;


/**
 *  \brief
 *      DMA queue structure
 */
extern DMAOpInfo *dmaQueues;
/**
 *  \brief
 *      DMA queue structure
 */
extern u16* dmaDataBuffer;

/**
 *  \brief
 *      Initialize the DMA queue sub system with default settings.
 *
 *      SGDK automatically call this method on hard reset so you don't need to call it again unless
 *      you want to change the default parameters in which casse you should use #DMA_initEx(..)
 *
 * \see #DMA_initEx(..)
 */
void DMA_init();
/**
 *  \brief
 *      Initialize the DMA queue sub system.
 *
 *  \param size
 *      The queue size (0 = default size = 64, min size = 20).
 *  \param capacity
 *      The maximum allowed size (in bytes) to transfer per #DMA_flushQueue() call (0 = default = no limit).<br>
 *      Depending the current selected strategy, furthers transfers can be ignored (by default all transfers are done whatever is the limit).
 *      See the #DMA_setIgnoreOverCapacity(..) method to change the strategy to adopt when capacity limit is reached.
 *  \param bufferSize
 *      Size of the buffer (in bytes) to store temporary data that will be transferred through the DMA queue (0 = default size = 8192 on NTSC and 14336 on PAL).<br>
 *      The buffer should be big enough to contains all temporary that you need to store before next #DMA_flushQueue() call.
 *
 *      SGDK automatically call this method on hard reset so you don't need to call it again unless
 *      you want to change the default parameters.
 *
 * \see #DMA_setIgnoreOverCapacity(..)
 */
void DMA_initEx(u16 size, u16 capacity, u16 bufferSize);

/**
 *  \brief
 *      Returns TRUE if the DMA_flushQueue() method is automatically called at VBlank to process DMA operations
 *      pending in the queue.
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
void DMA_setAutoFlush(bool value);
/**
 *  \brief
 *      Returns the maximum allowed number of pending transfer in the queue (allocated queue size).
 *
 *  \see DMA_setMaxQueueSize()
 */
u16 DMA_getMaxQueueSize();
/**
 *  \brief
 *      Sets the maximum allowed number of pending transfer in the queue (allocated queue size).<br>
 *      <b>WARNING:</b> changing the queue size will clear the DMA queue.
 *
 *  \param value
 *      The queue size (minimum allowed size = 20)
 *
 *  \see DMA_getMaxQueueSize()
 *  \see DMA_setMaxQueueSizeToDefault()
 */
void DMA_setMaxQueueSize(u16 value);
/**
 *  \brief
 *      Sets the maximum allowed number of pending transfer in the queue (allocated queue size) to default value (64).<br>
 *      <b>WARNING:</b> changing the queue size will clear the DMA queue.
 *
 *  \see DMA_setMaxQueueSize()
 */
void DMA_setMaxQueueSizeToDefault();
/**
 *  \brief
 *      Returns the maximum allowed size (in bytes) to transfer per #DMA_flushQueue() call.<br>
 *      A value of 0 means there is no DMA limit.
 *
 *  \see DMA_setMaxTransferSize()
 */
u16 DMA_getMaxTransferSize();
/**
 *  \brief
 *      Sets the maximum amount of data (in bytes) to transfer per #DMA_flushQueue() call.<br>
 *      VBlank period allows to transfer up to 7.2 KB on NTSC system and 15 KB on PAL system.<br>
 *      By default there is no size limit (0)
 *
 *  \param value
 *      The maximum amount of data (in bytes) to transfer during DMA_flushQueue() operation.<br>
 *      Use <b>0</b> for no limit.
 *
 *  \see DMA_flushQueue()
 */
void DMA_setMaxTransferSize(u16 value);
/**
 *  \brief
 *      Sets the maximum amount of data (in bytes) to default value (7.2 KB on NTSC system and 15 KB on PAL system).
 *
 *  \see DMA_setMaxTransferSize()
 */
void DMA_setMaxTransferSizeToDefault();
/**
 *  \brief
 *      Returns the size (in bytes) of the temporary data buffer which can be used to store data
 *      that will be transferred through the DMA queue.
 *
 *  \see DMA_setBufferSize()
 */
u16 DMA_getBufferSize();
/**
 *  \brief
 *      Sets the size (in bytes) of the temporary data buffer which can be used to store data
 *      that will be transferred through the DMA queue.<br>
 *      <b>WARNING:</b> changing the buffer size will clear the DMA queue.
 *
 *  \param value
 *      The size of the temporary data buffer (in bytes).<br>
 *      Minimum allowed buffer size if 2048 (internals methods require a minimal buffer size)
 *
 *  \see DMA_getBufferSize()
 *  \see DMA_setBufferSizeToDefault()
 */
void DMA_setBufferSize(u16 value);
/**
 *  \brief
 *      Sets the size (in bytes) of the temporary data buffer to default value (8 KB on NTSC system and 14 KB on PAL system).
 *
 *  \see DMA_setBufferSize()
 */
void DMA_setBufferSizeToDefault();
/**
 *  \brief
 *      Return TRUE means that we ignore future DMA operation when we reach the maximum capacity (see #DMA_setIgnoreOverCapacity(..) method).
 *
 *  \see DMA_setIgnoreOverCapacity()
 */
u16 DMA_getIgnoreOverCapacity();
/**
 *  \brief
 *      Set the "over capacity" DMA queue strategy (default is FALSE).
 *
 *      When set to <i>TRUE</i> any DMA operation queued after we reached the maximum defined transfer capacity
 *      with #DMA_setMaxTransferSize(..) are ignored.<br>
 *      When set to <i>FALSE</i> all DMA operations are done even when we are over the maximum capacity (which can lead to important slowdown).
 *
 *  \see DMA_setMaxTransferSize()
 */
void DMA_setIgnoreOverCapacity(bool value);

/**
 *  \brief
 *      Clears the DMA queue (all queued operations are lost).<br>
 *  \see DMA_flushQueue()
 */
void DMA_clearQueue();
/**
 *  \brief
 *      Transfer the content of the DMA queue to the VDP:<br>
 *      Each pending DMA operation is sent to the VDP and processed as quickly as possible.<br>
 *      This method returns when all DMA operations present in the queue has been transferred (or when maximum capacity has been reached).<br>
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
 *      Returns the size (in byte) of data to be transferred currently present in the DMA queue.<br>
 *      NTSC frame allows about 7.6 KB of data to be transferred during VBlank (in H40) while
 *      PAL frame allows about 17 KB (in H40).
 */
u16 DMA_getQueueTransferSize();

/**
 *  \brief
 *      Tool method allowing to allocate memory from the DMA temporary buffer if you need a very temporary buffer.<br>
 *      Don't forget to release memory using #DMA_releaseTemp(..).<br>
 *      <b>WARNING:</b> it's very important to disable interrupts while using the temporary DMA buffer as DMA buffer can be flushed on interrupt.
 *
 *  \param len
 *      Number of word to allocate.
 *  \return
 *      The source buffer pointer if allocation succeeded,.<br>
 *      Returns NULL if the buffer is full (or not big enough).
 *  \see DMA_releaseTemp(..)
 */
void* DMA_allocateTemp(u16 len);
/**
 *  \brief
 *      Tool method allowing to release memory previously allocated using #DMA_allocateTemp(..).<br>
 *      <b>WARNING:</b> it's very important to disable interrupts while using the temporary DMA buffer as DMA buffer can be flushed on interrupt.
 *
 *  \param len
 *      Number of word to release.
 *  \see DMA_allocateTemp(..)
 */
void DMA_releaseTemp(u16 len);

/**
 *  \brief
 *      General method to transfer data to VDP memory.
 *
 *  \param tm
 *      Transfer method.<br>
 *      Accepted values are:<br>
 *      - CPU<br>
 *      - DMA<br>
 *      - DMA_QUEUE<br>
 *      - DMA_QUEUE_COPY<br>
 *  \param location
 *      Destination location.<br>
 *      Accepted values:<br>
 *      - DMA_VRAM (for VRAM transfer).<br>
 *      - DMA_CRAM (for CRAM transfer).<br>
 *      - DMA_VSRAM (for VSRAM transfer).<br>
 *  \param from
 *      Source buffer.
 *  \param to
 *      VRAM/CRAM/VSRAM destination address.
 *  \param len
 *      Number of word to allocate and transfer.
 *  \param step
 *      destination (VRAM/VSRAM/CRAM) address increment step after each write (0 to 255).<br>
 *      By default you should set it to 2 for normal copy operation but you can use different value
 *      for specific operation.<br>
 *  \return
 *      TRUE if the operation succeeded, FALSE otherwise (buffer or queue full).
 *  \see DMA_queueDMA(..)
 */
bool DMA_transfer(TransferMethod tm, u8 location, void* from, u16 to, u16 len, u16 step);

/**
 *  \brief
 *      Return TRUE if we have enough DMA capacity to transfer the given data block len (see #DMA_setMaxTransferSize(..))
 *
 *  \param location
 *      Destination location.<br>
 *      Accepted values:<br>
 *      - DMA_VRAM (for VRAM transfer).<br>
 *      - DMA_CRAM (for CRAM transfer).<br>
 *      - DMA_VSRAM (for VSRAM transfer).<br>
 *  \param len
 *      Number of word we want to transfer.
 *  \return
 *      TRUE if we have enough capacity, FALSE otherwise
 *  \see DMA_getMaxTransferSize(..)
 *  \see DMA_queueDma(..)
 */
bool DMA_canQueue(u8 location, u16 len);
/**
 *  \brief
 *      Allocate temporary memory and queues the DMA transfer operation in the DMA queue.<br>
 *      The idea of the DMA queue is to burst all DMA operations during VBLank to maximize bandwidth usage.<br>
 *      <b>IMPORTANT:</b> You need to fill the returned data buffer before DMA occurs so it's a good practise to disable interrupts before
 *      calling this method and re-enable them *after* you filled the buffer to avoid DMA queue flush operation happening in between.
 *
 *  \param location
 *      Destination location.<br>
 *      Accepted values:<br>
 *      - DMA_VRAM (for VRAM transfer).<br>
 *      - DMA_CRAM (for CRAM transfer).<br>
 *      - DMA_VSRAM (for VSRAM transfer).<br>
 *  \param to
 *      VRAM/CRAM/VSRAM destination address.
 *  \param len
 *      Number of word to allocate and transfer.
 *  \param step
 *      destination (VRAM/VSRAM/CRAM) address increment step after each write (0 to 255).<br>
 *      By default you should set it to 2 for normal copy operation but you can use different value
 *      for specific operation.<br>
 *  \return
 *      The source buffer pointer that will be used for the DMA transfer so you can fill its content.<br>
 *      Returns NULL if the buffer is full or if the DMA queue operation failed (queue is full).
 *  \see DMA_queueDMA(..)
 *  \see DMA_copyAndQueueDma(..)
 */
void* DMA_allocateAndQueueDma(u8 location, u16 to, u16 len, u16 step);
/**
 *  \brief
 *      Same as #DMA_queueDma(..) method except that it first copies the data to transfer through DMA queue into a temporary buffer.<br>
 *      This is useful when you know that source data may be modified before DMA acutally occurs and want to avoid that.
 *
 *  \param location
 *      Destination location.<br>
 *      Accepted values:<br>
 *      - DMA_VRAM (for VRAM transfer).<br>
 *      - DMA_CRAM (for CRAM transfer).<br>
 *      - DMA_VSRAM (for VSRAM transfer).<br>
 *  \param from
 *      Source buffer.
 *  \param to
 *      VRAM/CRAM/VSRAM destination address.
 *  \param len
 *      Number of word to transfer.
 *  \param step
 *      destination (VRAM/VSRAM/CRAM) address increment step after each write (0 to 255).<br>
 *      By default you should set it to 2 for normal copy operation but you can use different value
 *      for specific operation.<br>
 *  \return
 *      FALSE if the operation failed (queue is full)
 *  \see DMA_doDma(..)
 *  \see DMA_queueDma(..)
 */
bool DMA_copyAndQueueDma(u8 location, void* from, u16 to, u16 len, u16 step);
/**
 *  \brief
 *      Queues the specified DMA transfer operation in the DMA queue.<br>
 *      The idea of the DMA queue is to burst all DMA operations during VBLank to maximize bandwidth usage.<br>
 *
 *  \param location
 *      Destination location.<br>
 *      Accepted values:<br>
 *      - DMA_VRAM (for VRAM transfer).<br>
 *      - DMA_CRAM (for CRAM transfer).<br>
 *      - DMA_VSRAM (for VSRAM transfer).<br>
 *  \param from
 *      Source buffer address.
 *  \param to
 *      VRAM/CRAM/VSRAM destination address.
 *  \param len
 *      Number of word to transfer.
 *  \param step
 *      destination (VRAM/VSRAM/CRAM) address increment step after each write (0 to 255).<br>
 *      By default you should set it to 2 for normal copy operation but you can use different value
 *      for specific operation.<br>
 *  \return
 *      FALSE if the operation failed (queue is full)
 *  \see DMA_doDma(..)
 */
bool DMA_queueDma(u8 location, void* from, u16 to, u16 len, u16 step);
/**
 *  \brief
 *      Same as #DMA_queueDma(..) method except if doesn't check for 128 KB bank crossing on source
 *  \see #DMA_queueDma(..)
 */
bool DMA_queueDmaFast(u8 location, void* from, u16 to, u16 len, u16 step);

/**
 *  \brief
 *      Do DMA transfer operation immediately
 *
 *  \param location
 *      Destination location.<br>
 *      Accepted values:<br>
 *      - DMA_VRAM (for VRAM transfer).<br>
 *      - DMA_CRAM (for CRAM transfer).<br>
 *      - DMA_VSRAM (for VSRAM transfer).<br>
 *  \param from
 *      Source buffer address.
 *  \param to
 *      VRAM/CRAM/VSRAM destination address.
 *  \param len
 *      Number of word to transfer.
 *  \param step
 *      destination (VRAM/VSRAM/CRAM) address increment step after each write (-1 to keep current step).<br>
 *      By default you should set it to 2 for normal copy operation but you can use different value
 *      for specific operation.
 *  \see DMA_queue(..)
 */
void DMA_doDma(u8 location, void* from, u16 to, u16 len, s16 step);
/**
 *  \brief
 *      Same as #DMA_doDma(..) method except if doesn't check for 128 KB bank crossing on source
 *  \see #DMA_doDma(..)
 */
void DMA_doDmaFast(u8 location, void* from, u16 to, u16 len, s16 step);

/**
 *  \brief
 *      Do software (CPU) copy to VDP memory (mainly for testing purpose as it's slower than using DMA)
 *
 *  \param location
 *      Destination location.<br>
 *      Accepted values:<br>
 *      - DMA_VRAM (for VRAM transfer).<br>
 *      - DMA_CRAM (for CRAM transfer).<br>
 *      - DMA_VSRAM (for VSRAM transfer).<br>
 *  \param from
 *      Source buffer.
 *  \param to
 *      VRAM/CRAM/VSRAM destination address.
 *  \param len
 *      Number of word to transfer.
 *  \param step
 *      destination (VRAM/VSRAM/CRAM) address increment step after each write (-1 to keep current step).<br>
 *      By default you should set it to 2 for normal copy operation but you can use different value
 *      for specific operation.
 */
void DMA_doCPUCopy(u8 location, void* from, u16 to, u16 len, s16 step);
/**
 *  \brief
 *      Do software (CPU) copy to VDP memory (mainly for testing purpose as it's slower than using DMA)
 *
 *  \param cmd
 *      VDP packed control command (contains operation and destination address).
 *  \param from
 *      Source buffer.
 *  \param len
 *      Number of word to transfer.
 *  \param step
 *      destination (VRAM/VSRAM/CRAM) address increment step after each write (-1 to keep current step).<br>
 *      By default you should set it to 2 for normal copy operation but you can use different value
 *      for specific operation.
 */
void DMA_doCPUCopyDirect(u32 cmd, void* from, u16 len, s16 step);

/**
 *  \brief
 *      Do VRAM DMA fill operation.
 *
 *  \param to
 *      VRAM destination address.
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
 *      VRAM Source address.
 *  \param to
 *      VRAM destination address.
 *  \param len
 *      Number of byte to copy.
 *  \param step
 *      VRAM address increment step after each write (-1 to keep current step).<br>
 *      should be 1 for a classic copy operation but you can use different value for specific operation.
 */
void DMA_doVRamCopy(u16 from, u16 to, u16 len, s16 step);

/**
 *  \brief
 *      Wait current DMA fill/copy operation to complete (same as #VDP_waitDMACompletion())
 */
void DMA_waitCompletion();


#endif // _DMA_H_
