#include "dma.h"

#include "config.h"
#include "vdp.h"
#include "sys.h"
#include "memory.h"
#include "z80_ctrl.h"

#include "kdebug.h"
#include "tools.h"
#include "timer.h"


//#define DMA_DEBUG

#define DMA_AUTOFLUSH               1
#define DMA_OVERCAPACITY_IGNORE     2


// we don't want to share it
extern vu16 VBlankProcess;

// DMA queue (initialized on reset)
DMAOpInfo *dmaQueues;

// DMA data buffer (initialized on reset)
u16* dmaDataBuffer;
static u16* nextDataBuffer;

// DMA queue settings
static u16 queueSize;
static u16 maxTransferPerFrame;
static u16 flag;

// DMA data buffer settings
static u16 dataBufferSize;

// current queue index (0 = empty / queueSize = full)
static u16 queueIndex;
static u16 queueIndexLimit;
static u16 queueTransferSize;

// do not share (assembly methods)
extern void flushQueue(u16 num);


void DMA_init()
{
    DMA_initEx(0, -1, 0);
}

NO_INLINE void DMA_initEx(u16 size, u16 capacity, u16 bufferSize)
{
    // -1/65535 means no limit
    maxTransferPerFrame = capacity;
    // auto flush is enabled by default
    flag = DMA_AUTOFLUSH;
    VBlankProcess |= PROCESS_DMA_TASK;

    // release buffers first
    if (dmaQueues)
    {
        MEM_free(dmaQueues);
        dmaQueues = NULL;
    }
    if (dmaDataBuffer)
    {
        MEM_free(dmaDataBuffer);
        dmaDataBuffer = NULL;
    }

    // try to pack memory free blocks (help to avoid memory fragmentation)
    MEM_pack();

    // define queue size
    if (size) queueSize = max(DMA_QUEUE_SIZE_MIN, size);
    else queueSize = DMA_QUEUE_SIZE_DEFAULT;

    // allocate DMA queue
    dmaQueues = MEM_alloc(queueSize * sizeof(DMAOpInfo));

    // define DMA data buffer size (in words)
    // this actually clear the DMA queue
    if (bufferSize) DMA_setBufferSize(bufferSize);
    else DMA_setBufferSizeToDefault();
}

bool DMA_getAutoFlush()
{
    return (flag & DMA_AUTOFLUSH) ? TRUE : FALSE;
}

void DMA_setAutoFlush(bool value)
{
    if (value)
    {
        flag |= DMA_AUTOFLUSH;
        VBlankProcess |= PROCESS_DMA_TASK;
    }
    else
    {
        flag &= ~DMA_AUTOFLUSH;
        VBlankProcess &= ~PROCESS_DMA_TASK;
    }
}

u16 DMA_getMaxQueueSize()
{
    return queueSize;
}

void DMA_setMaxQueueSize(u16 value)
{
    queueSize = max(DMA_QUEUE_SIZE_MIN, value);

    // already allocated ?
    if (dmaQueues) MEM_free(dmaQueues);
    // allocate DMA queue
    dmaQueues = MEM_alloc(queueSize * sizeof(DMAOpInfo));

    // reset queue
    DMA_clearQueue();
}

void DMA_setMaxQueueSizeToDefault()
{
    DMA_setMaxQueueSize(DMA_QUEUE_SIZE_DEFAULT);
}

u16 DMA_getMaxTransferSize()
{
    return ((s16) maxTransferPerFrame == -1)?0:maxTransferPerFrame;
}

void DMA_setMaxTransferSize(u16 value)
{
    if (value) maxTransferPerFrame = value;
    else maxTransferPerFrame = -1;
}

void DMA_setMaxTransferSizeToDefault()
{
    DMA_setMaxTransferSize(IS_PAL_SYSTEM ? DMA_TRANSFER_CAPACITY_PAL_LOW : DMA_TRANSFER_CAPACITY_NTSC);
}

u16 DMA_getBufferSize()
{
    return dataBufferSize * 2;
}

void DMA_setBufferSize(u16 value)
{
    dataBufferSize = max(DMA_BUFFER_SIZE_MIN, value) / 2;

    // already allocated ?
    if (dmaDataBuffer) MEM_free(dmaDataBuffer);
    // allocate DMA data buffer
    if (dataBufferSize)
        dmaDataBuffer = MEM_alloc(dataBufferSize * sizeof(u16));
    else
        dmaDataBuffer = NULL;

    // reset queue
    DMA_clearQueue();
}

void DMA_setBufferSizeToDefault()
{
    DMA_setBufferSize(IS_PAL_SYSTEM ? DMA_BUFFER_SIZE_PAL_LOW : DMA_BUFFER_SIZE_NTSC);
}

bool DMA_getIgnoreOverCapacity()
{
    return (flag & DMA_OVERCAPACITY_IGNORE) ? TRUE : FALSE;
}

void DMA_setIgnoreOverCapacity(bool value)
{
    if (value) flag |= DMA_OVERCAPACITY_IGNORE;
    else flag &= ~DMA_OVERCAPACITY_IGNORE;
}

void DMA_clearQueue()
{
    queueIndex = 0;
    queueIndexLimit = 0;
    queueTransferSize = 0;

    // reset DMA data buffer pointer
    nextDataBuffer = dmaDataBuffer;

}

void DMA_flushQueue()
{
    u16 i;
    u8 autoInc;

    // default
    i = queueIndex;

    // limit reached ?
    if (queueIndexLimit)
    {
        // we choose to ignore over capacity transfers ?
        if (flag & DMA_OVERCAPACITY_IGNORE)
        {
            i = queueIndexLimit;

#if (LIB_LOG_LEVEL >= LOG_LEVEL_WARNING)
            KLog_U3_("DMA_flushQueue(..) warning: frame #", vtimer, " - transfer size is above ", maxTransferPerFrame, " bytes (", queueTransferSize, "), some transfers are ignored !");
#endif
        }
    }

#ifdef DMA_DEBUG
    KLog_U3("DMA_flushQueue: queueIndexLimit=", queueIndexLimit, " queueIndex=", queueIndex, " i=", i);
#endif

    // save autoInc
    autoInc = VDP_getAutoInc();

#if (DMA_DISABLED != 0)
    // DMA disabled --> replace with software copy

    DMAOpInfo *info = dmaQueues;

    while(i--)
    {
        u16 len = (info->regLenL & 0xFF) | ((info->regLenH & 0xFF) << 8);
        s16 step = info->regAddrMStep & 0xFF;
        u32 from = ((info->regAddrMStep & 0xFF0000) >> 7) | ((info->regAddrHAddrL & 0x7F00FF) << 1);
        // replace DMA command by WRITE command
        u32 cmd = info->regCtrlWrite & ~0x80;

        // software copy
        DMA_doCPUCopyDirect(cmd, (void*) from, len, step);

        // next
        info++;
    }
#else

#if (HALT_Z80_ON_DMA != 0)
    bool busTaken = Z80_isBusTaken();
    // disable Z80 before processing DMA
    if (!busTaken) Z80_requestBus(FALSE);
#endif  // HALT_Z80_ON_DMA

    flushQueue(i);

#if (HALT_Z80_ON_DMA != 0)
    // re-enable Z80 after all DMA
    if (!busTaken) Z80_releaseBus();
#endif  // HALT_Z80_ON_DMA

#endif  // DMA_DISABLED

    // can clear the queue now
    DMA_clearQueue();
    // restore autoInc
    VDP_setAutoInc(autoInc);
}

//static void flushQueue(u16 num)
//{
//    u32 *info = (u32*) dmaQueues;
//    vu32 *pl = (vu32*) VDP_CTRL_PORT;
//    u16 i = num;
//
//    // flush DMA queue
//    while(i--)
//    {
//        *pl = *info++;  // regLen = 0x94000x9300 | (len | (len << 8) & 0xFF00FF)
//        *pl = *info++;  // regAddrMStep = 0x96008F00 | ((from << 7) & 0xFF0000) | step
//        *pl = *info++;  // regAddrHAddrL = 0x97009500 | ((from >> 1) & 0x7F00FF)
//        *pl = *info++;  // regCtrlWrite =  VDP_DMA_xxx_ADDR(to)
//    }
//}
//
//static void flushQueueSafe(u16 num, u16 z80restore)
//{
//    // z80 BUSREQ off state
//    const u16 z80off = 0x0100;
//    const u16 z80on = z80restore;
//
//    u32 *info = (u32*) dmaQueues;
//    vu32 *pl = (vu32*) VDP_CTRL_PORT;
//    vu16 *pw = (vu16*) Z80_HALT_PORT;
//    u16 i = num;
//
//    // flush DMA queue
//    while(i--)
//    {
//        *pl = *info++;  // regLen = 0x94000x9300 | (len | (len << 8) & 0xFF00FF)
//        *pl = *info++;  // regAddrMStep = 0x96008F00 | ((from << 7) & 0xFF0000) | step
//        *pl = *info++;  // regAddrHAddrL = 0x97009500 | ((from >> 1) & 0x7F00FF)
//
//        // DISABLE and RE-ENABLE Z80 immediately
//        // This allow to avoid DMA failure on some MD
//        // when Z80 try to access 68k BUS at same time the DMA starts.
//        // BUS arbitrer lantecy will disable Z80 for a very small amont of time
//        // when DMA start, avoiding that situation to happen.
//        *pw = z80off;
//        *pw = z80on;
//
//        // then trigger DMA
//        *pl = *info++;  // regCtrlWrite =  VDP_DMA_xxx_ADDR(to)
//    }
//}

u16 DMA_getQueueSize()
{
    return queueIndex;
}

u16 DMA_getQueueTransferSize()
{
    return queueTransferSize;
}

bool DMA_transfer(TransferMethod tm, u8 location, void* from, u16 to, u16 len, u16 step)
{
    // nothing to do (avoid transfering 65536 words when len = 0)
    if (!len) return TRUE;

    switch(tm)
    {
        // default = CPU transfer
        default:
            DMA_doCPUCopy(location, from, to, len, step);
            return TRUE;

        case DMA:
            DMA_doDma(location, from, to, len, step);
            return TRUE;

        case DMA_QUEUE:
            return DMA_queueDma(location, from, to, len, step);

        case DMA_QUEUE_COPY:
            return DMA_copyAndQueueDma(location, from, to, len, step);
    }
}

void* DMA_allocateTemp(u16 len)
{
    u16* result = nextDataBuffer;

    nextDataBuffer += len;

    if (nextDataBuffer > (dmaDataBuffer + dataBufferSize))
    {
#if (LIB_LOG_LEVEL >= LOG_LEVEL_ERROR)
        KLog_U2_("DMA_allocateTemp(..) failed: buffer over capacity (", (u32) nextDataBuffer - (u32) dmaDataBuffer, " raised, max capacity = ", DMA_getBufferSize(), ")");
#endif

        // failed --> revert allocation
        DMA_releaseTemp(len);

        // error
        return NULL;
    }

    return result;
}

void DMA_releaseTemp(u16 len)
{
    // release allocation
    nextDataBuffer -= len;
}

bool DMA_canQueue(u8 location, u16 len)
{
    u16 nextSize = queueTransferSize + (len << ((location == DMA_VRAM)?1:0));

    // return TRUE if we are below limit
    return (nextSize <= maxTransferPerFrame);
}

void* DMA_allocateAndQueueDma(u8 location, u16 to, u16 len, u16 step)
{
    u16* result = DMA_allocateTemp(len);

    // can't allocate --> exit
    if (result == NULL) return result;

#ifdef DMA_DEBUG
    KLog_U3_("DMA_allocateAndQueueDma: allocate ", 2 * len, " bytes - current allocated = ", (u32) nextDataBuffer - (u32) dmaDataBuffer, " on ", DMA_getBufferSize(), " availaible");
#endif

    // try to queue the DMA transfer (we can use FAST version as source is always located in RAM)
    if (!DMA_queueDmaFast(location, result, to, len, step))
    {
        // failed --> release allocation
        DMA_releaseTemp(len);
        // error
        return NULL;
    }

    // return buffer than will be fill by user before being transferred through the DMA queue
    return result;
}

bool DMA_copyAndQueueDma(u8 location, void* from, u16 to, u16 len, u16 step)
{
    u16* buffer = DMA_allocateTemp(len);

    // can't allocate --> exit
    if (buffer == NULL) return FALSE;

#ifdef DMA_DEBUG
    KLog_U3_("DMA_copyAndQueueDma: allocate ", 2 * len, " bytes - current allocated = ", (u32) nextDataBuffer - (u32) dmaDataBuffer, " on ", DMA_getBufferSize(), " availaible");
#endif

    // do copy to temporal buffer (as from buffer may be modified in between)
    memcpy(buffer, from, len * 2);

    // try to queue the DMA transfer (we can use FAST version as source is always located in RAM)
    if (!DMA_queueDmaFast(location, buffer, to, len, step))
    {
        // failed --> release allocation
        DMA_releaseTemp(len);
        // error
        return FALSE;
    }

    return TRUE;
}

bool DMA_queueDma(u8 location, void* from, u16 to, u16 len, u16 step)
{
    // DMA works on 64 KWord bank
    u32 fromAddr = (u32) from;
    u32 bankLimitB = 0x20000 - (fromAddr & 0x1FFFF);
    u32 bankLimitW = bankLimitB >> 1;
    u16 newLen;

   // bank limit exceeded
    if (len > bankLimitW)
    {
        // we first do the second bank transfer (can use the fast version here)
        if (!DMA_queueDmaFast(location, (void*) (fromAddr + bankLimitB), to + bankLimitB, len - bankLimitW, step))
            return FALSE;
        newLen = bankLimitW;
    }
    // ok, use normal len
    else newLen = len;

    return DMA_queueDmaFast(location, from, to, newLen, step);
}

NO_INLINE bool DMA_queueDmaFast(u8 location, void* from, u16 to, u16 len, u16 step)
{
    u32 fromAddr;
    DMAOpInfo *info;

    // queue is full --> error
    if (queueIndex >= queueSize)
    {
#if (LIB_LOG_LEVEL >= LOG_LEVEL_ERROR)
        KDebug_Alert("DMA_queueDmaFast(..) failed: queue is full ! Try to increase queue size using DMA_setMaxQueueSize(..)");
#endif

        // return FALSE as transfer will be ignored
        return FALSE;
    }

    fromAddr = (u32) from;

    // get DMA info structure and pass to next one
    info = &dmaQueues[queueIndex];

    // $13:len L  $14:len H (DMA length in word)
    info->regLenL = 0x9300 + (len & 0xFF);
    info->regLenH = 0x9400 + ((len >> 8) & 0xFF);
    // $16:M  $f:step (DMA address M and Step register)
    info->regAddrMStep = 0x96008F00 + ((fromAddr << 7) & 0xFF0000) + step;
    // $17:H  $15:L (DMA address H & L)
    info->regAddrHAddrL = 0x97009500 + ((fromAddr >> 1) & 0x7F00FF);

    // Trigger DMA
    switch(location)
    {
        case DMA_VRAM:
            info->regCtrlWrite = VDP_DMA_VRAM_ADDR((u32)to);
    #ifdef DMA_DEBUG
            KLog_U4("DMA_queueDma: VRAM from=", fromAddr, " to=", to, " len=", len, " step=", step);
    #endif
            // keep trace of transferred size
            queueTransferSize += len << 1;
            break;

        case DMA_CRAM:
            info->regCtrlWrite = VDP_DMA_CRAM_ADDR((u32)to);
    #ifdef DMA_DEBUG
            KLog_U4("DMA_queueDma: CRAM from=", fromAddr, " to=", to, " len=", len, " step=", step);
    #endif
            // keep trace of transferred size (only half as CRAM trasnfer are 16 bit wide)
            queueTransferSize += len;
            break;

        case DMA_VSRAM:
            info->regCtrlWrite = VDP_DMA_VSRAM_ADDR((u32)to);
    #ifdef DMA_DEBUG
            KLog_U4("DMA_queueDma: VSRAM from=", fromAddr, " to=", to, " len=", len, " step=", step);
    #endif
            // keep trace of transferred size (only half as VSRAM trasnfer are 16 bit wide)
            queueTransferSize += len;
            break;
    }

    // pass to next index
    queueIndex++;

#ifdef DMA_DEBUG
    KLog_U2("  Queue index=", queueIndex, " new queueTransferSize=", queueTransferSize);
#endif

    // we are above the defined limit ?
    if (queueTransferSize > maxTransferPerFrame)
    {
        // first time we reach the limit ? store index where to stop transfer
        if (queueIndexLimit == 0)
        {
#if (LIB_LOG_LEVEL >= LOG_LEVEL_WARNING)
            KLog_S4("DMA_queueDmaFast(..) warning: transfer size limit raised on transfer #", queueIndex - 1, " on frame #", vtimer, ", current size = ", queueTransferSize, "  max allowed = ", maxTransferPerFrame);
#endif
            // store limit index
            queueIndexLimit = queueIndex - 1;

#ifdef DMA_DEBUG
            KLog_U1("  Queue index limit set at ", queueIndexLimit);
#endif
        }

        // return FALSE if transfer will be ignored
        return (flag & DMA_OVERCAPACITY_IGNORE) ? FALSE : TRUE;
    }

    return TRUE;
}

void DMA_doDma(u8 location, void* from, u16 to, u16 len, s16 step)
{
    // DMA works on 64 KWord bank
    u32 fromAddr = (u32) from;
    u32 bankLimitB = 0x20000 - (fromAddr & 0x1FFFF);
    u32 bankLimitW = bankLimitB >> 1;
    u32 newLen;

    // bank limit exceeded ?
    if (len > bankLimitW)
    {
        // we first do the second bank transfer (can use the fast version here)
        DMA_doDmaFast(location, (void*) (fromAddr + bankLimitB), to + bankLimitB, len - bankLimitW, step);
        newLen = bankLimitW;
    }
    // ok, use normal len
    else newLen = len;

    DMA_doDmaFast(location, from, to, newLen, step);
}

NO_INLINE void DMA_doDmaFast(u8 location, void* from, u16 to, u16 len, s16 step)
{
#if (DMA_DISABLED != 0)
    // DMA disabled --> replace with software copy
    DMA_doCPUCopy(location, from, to, len, step);
#else
    vu16 *pw;
    u32 cmd;
    u32 fromAddr;
    vu32 cmdbuf[1];
    u16* cmdbufp;
#if (HALT_Z80_ON_DMA != 0)
    bool busTaken;
#endif  // HALT_Z80_ON_DMA

    fromAddr = (u32) from;

    if (step != -1)
        VDP_setAutoInc(step);

    pw = (vu16*) VDP_CTRL_PORT;

    // Setup DMA length (in word here)
    *pw = 0x9300 + (len & 0xff);
    *pw = 0x9400 + ((len >> 8) & 0xff);

    // Setup DMA address
    fromAddr >>= 1;
    *pw = 0x9500 + (fromAddr & 0xff);
    fromAddr >>= 8;
    *pw = 0x9600 + (fromAddr & 0xff);
    fromAddr >>= 8;
    *pw = 0x9700 + (fromAddr & 0x7f);

    switch(location)
    {
        default:
        case DMA_VRAM:
            cmd = VDP_DMA_VRAM_ADDR((u32)to);
            break;

        case DMA_CRAM:
            cmd = VDP_DMA_CRAM_ADDR((u32)to);
            break;

        case DMA_VSRAM:
            cmd = VDP_DMA_VSRAM_ADDR((u32)to);
            break;
    }

    // force storing DMA command into memory
    cmdbuf[0] = cmd;

    // then force issuing DMA from memory word operand
    cmdbufp = (u16*) cmdbuf;
    // first command word
    *pw = *cmdbufp++;

#if (HALT_Z80_ON_DMA != 0)
    // disable Z80 before processing DMA
    busTaken = Z80_getAndRequestBus(FALSE);
#endif  // HALT_Z80_ON_DMA

    // trigger DMA (second word command wrote from memory to avoid possible failure on some MD)
    *pw = *cmdbufp;

#if (HALT_Z80_ON_DMA != 0)
    // re-enable Z80 after DMA
    if (!busTaken) Z80_releaseBus();
#endif  // HALT_Z80_ON_DMA

#endif  // DMA_DISABLED
}

void DMA_doCPUCopy(u8 location, void* from, u16 to, u16 len, s16 step)
{
    u32 cmd;

    switch(location)
    {
        default:
        case DMA_VRAM:
            cmd = VDP_WRITE_VRAM_ADDR((u32)to);
            break;

        case DMA_CRAM:
            cmd = VDP_WRITE_CRAM_ADDR((u32)to);
            break;

        case DMA_VSRAM:
            cmd = VDP_WRITE_VSRAM_ADDR((u32)to);
            break;
    }

    DMA_doCPUCopyDirect(cmd, from, len, step);
}

NO_INLINE void DMA_doCPUCopyDirect(u32 cmd, void* from, u16 len, s16 step)
{
    vu32 *plctrl;
    vu16 *pwdata;
    vu32 *pldata;
    u16 *srcw;
    u32 *srcl;
    u16 il;
    u16 iw;

    if (step != -1)
        VDP_setAutoInc(step);

    plctrl = (vu32*) VDP_CTRL_PORT;
    *plctrl = cmd;

    il = len / 16;
    iw = len & 0xF;

    srcl = (u32*) from;
    pldata = (vu32*) VDP_DATA_PORT;

    while(il--)
    {
        *pldata = *srcl++;
        *pldata = *srcl++;
        *pldata = *srcl++;
        *pldata = *srcl++;
        *pldata = *srcl++;
        *pldata = *srcl++;
        *pldata = *srcl++;
        *pldata = *srcl++;
    }

    srcw = (u16*) srcl;
    pwdata = (vu16*) VDP_DATA_PORT;

    while(iw--) *pwdata = *srcw++;
}


void DMA_doVRamFill(u16 to, u16 len, u8 value, s16 step)
{
    vu16 *pw;
    vu32 *pl;
    u16 l;

    if (step != -1)
        VDP_setAutoInc(step);

    // need to do some adjustement because of the way VRAM fill is done
    if (len)
    {
        if (to & 1)
        {
            if (len < 3) l = 1;
            else l = len - 2;
        }
        else
        {
            if (len < 2) l = 1;
            else l = len - 1;
        }
    }
    // special value of 0, we don't care
    else l = len;

//    DMA_doVRamFill(0, 1, 0xFF, 1);    // 01
//    DMA_doVRamFill(0, 1, 0xFF, 1);    // 01-3
//    DMA_doVRamFill(0, 2, 0xFF, 1);    // 01-3
//    DMA_doVRamFill(0, 2, 0xFF, 1);    // 0123

    pw = (vu16*) VDP_CTRL_PORT;

    // Setup DMA length
    *pw = 0x9300 + (l & 0xFF);
    *pw = 0x9400 + ((l >> 8) & 0xFF);

    // Setup DMA operation (VRAM FILL)
    *pw = 0x9780;

    // Write VRam DMA destination address
    pl = (vu32*) VDP_CTRL_PORT;
    *pl = VDP_DMA_VRAM_ADDR((u32)to);

    // set up value to fill (need to be 16 bits extended)
    pw = (vu16*) VDP_DATA_PORT;
    *pw = value | (value << 8);
}

void DMA_doVRamCopy(u16 from, u16 to, u16 len, s16 step)
{
    vu16 *pw;
    vu32 *pl;

    if (step != -1)
        VDP_setAutoInc(step);

    pw = (vu16*) VDP_CTRL_PORT;

    // Setup DMA length
    *pw = 0x9300 + (len & 0xff);
    *pw = 0x9400 + ((len >> 8) & 0xff);

    // Setup DMA address
    *pw = 0x9500 + (from & 0xff);
    *pw = 0x9600 + ((from >> 8) & 0xff);

    // Setup DMA operation (VRAM COPY)
    *pw = 0x97C0;

    // Write VRam DMA destination address (start DMA copy operation)
    pl = (vu32*) VDP_CTRL_PORT;
    *pl = VDP_DMA_VRAMCOPY_ADDR((u32)to);
}

void DMA_waitCompletion()
{
    VDP_waitDMACompletion();
}
