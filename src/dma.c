#include "dma.h"

#include "vdp.h"
#include "sys.h"
#include "memory.h"

#include "kdebug.h"
#include "tools.h"


//#define DMA_DEBUG

#define DMA_DEFAULT_QUEUE_SIZE      64

#define DMA_AUTOFLUSH               0x1
#define DMA_OVERCAPACITY_IGNORE     0x2


// we don't want to share it
extern vu32 VIntProcess;

// DMA queue
DMAOpInfo *dmaQueues = NULL;

// DMA queue settings
static u16 queueSize;
static s16 maxTransferPerFrame;
static u16 flags;

// current queue index (0 = empty / queueSize = full)
static u16 queueIndex;
static u16 queueIndexLimit;
static u32 queueTransferSize;
static u32 queueTransferSizeLimit;


void DMA_init(u16 size, u16 capacity)
{
    if (size) queueSize = size;
    else queueSize = DMA_DEFAULT_QUEUE_SIZE;

    maxTransferPerFrame = capacity;

    // auto flush is enabled by default
    flags = DMA_AUTOFLUSH;

    // already allocated ?
    if (dmaQueues) MEM_free(dmaQueues);
    // allocate DMA queue
    dmaQueues = MEM_alloc(queueSize * sizeof(DMAOpInfo));

    // clear queue
    DMA_clearQueue();
}

u16 DMA_getAutoFlush()
{
    return (flags & DMA_AUTOFLUSH)?TRUE:FALSE;
}

void DMA_setAutoFlush(u16 value)
{
    if (value)
    {
        flags |= DMA_AUTOFLUSH;
        // auto flush enabled and transfer size > 0 --> set process on VBlank
        if (queueTransferSize > 0)
             VIntProcess |= PROCESS_DMA_TASK;
    }
    else flags &= ~DMA_AUTOFLUSH;
}

u16 DMA_getIgnoreOverCapacity()
{
    return (flags & DMA_OVERCAPACITY_IGNORE)?TRUE:FALSE;
}

void DMA_setIgnoreOverCapacity(u16 value)
{
    if (value) flags |= DMA_OVERCAPACITY_IGNORE;
    else flags &= ~DMA_OVERCAPACITY_IGNORE;
}

s16 DMA_getMaxTransferSize()
{
    return maxTransferPerFrame;
}

void DMA_setMaxTransferSize(s16 value)
{
    maxTransferPerFrame = value;
}

void DMA_clearQueue()
{
    queueIndex = 0;
    queueIndexLimit = 0;
    queueTransferSize = 0;
    queueTransferSizeLimit = 0;
}

void DMA_flushQueue()
{
    u16 i;
    vu32 *pl;
    u32 *info;

    // transfer size limit ?
    if (queueIndexLimit) i = queueIndexLimit;
    else i = queueIndex;
    info = (u32*) dmaQueues;
    pl = (u32*) GFX_CTRL_PORT;

#ifdef DMA_DEBUG
    KLog_U3("DMA_flushQueue: queueIndexLimit=", queueIndexLimit, " queueIndex=", queueIndex, " i=", i);
#endif

    while(i--)
    {
        // set DMA parameters and trigger it
        *pl = *info++;  // regStepLenL = (0x8F00 | step) | ((0x9300 | (len & 0xFF)) << 16)
        *pl = *info++;  // regLenHAddrL = (0x9400 | ((len >> 8) & 0xFF)) | ((0x9500 | ((addr >> 1) & 0xFF)) << 16)
        *pl = *info++;  // regAddrMAddrH = (0x9600 | ((addr >> 9) & 0xFF)) | ((0x9700 | ((addr >> 17) & 0x7F)) << 16)
        *pl = *info++;  // regCtrlWrite =  GFX_DMA_VRAMCOPY_ADDR(to)
    }

    // transfer size limit ?
    if (queueIndexLimit)
    {
        // just ignore
        if (flags & DMA_OVERCAPACITY_IGNORE)
        {
#ifdef DMA_DEBUG
            KLog_U1("  Ignore remaining transfer starting at index: ", queueIndexLimit);
#endif

            queueIndex = 0;
            queueIndexLimit = 0;
            queueTransferSize = 0;
            queueTransferSizeLimit = 0;
        }
        else
        {
#ifdef DMA_DEBUG
            KLog_U2_("  Delay remaining transfer on next frame, queue[", queueIndexLimit, "] moved to queue[0] (", queueIndex - queueIndexLimit, " elements copied)");
            KLog_U2("    Before: queueIndex=", queueIndex, " queueTransferSize=", queueTransferSize);
#endif

            queueIndex -= queueIndexLimit;

            // copy remaining transfer at beggining of the queue (not optimal but simpler)
            memcpy(&dmaQueues[0], &dmaQueues[queueIndexLimit], sizeof(DMAOpInfo) * queueIndex);
            queueTransferSize -= queueTransferSizeLimit;
            queueIndexLimit = 0;
            queueTransferSizeLimit = 0;

#ifdef DMA_DEBUG
            KLog_U2("    After: queueIndex=", queueIndex, " queueTransferSize=", queueTransferSize);
#endif
        }
    }
    else
    {
        queueIndex = 0;
        queueIndexLimit = 0;
        queueTransferSize = 0;
        queueTransferSizeLimit = 0;
    }

    // we do that to fix cached auto inc value (instead of losing time in updating it during queue flush)
    VDP_setAutoInc(2);
}

u16 DMA_getQueueSize()
{
    return queueIndex;
}

u32 DMA_getQueueTransferSize()
{
    return queueTransferSize;
}

u16 DMA_queueDma(u8 location, u32 from, u16 to, u16 len, u16 step)
{
    u32 newlen;
    u32 banklimitb;
    u32 banklimitw;
    DMAOpInfo *info;

    // queue is full --> error
    if (queueIndex >= queueSize)
    {
#if (LIB_DEBUG != 0)
        KDebug_Alert("DMA_queueDma(..) failed: queue is full !");
#endif

        return FALSE;
    }

    // DMA works on 64 KW bank
    banklimitb = 0x20000 - (from & 0x1FFFF);
    banklimitw = banklimitb >> 1;
    // bank limit exceeded
    if (len > banklimitw)
    {
        // we first do the second bank transfer
        DMA_queueDma(location, from + banklimitb, to + banklimitb, len - banklimitw, step);
        newlen = banklimitw;
    }
    // ok, use normal len
    else newlen = len;

    // keep trace of transfered size
    queueTransferSize += newlen << 1;

    // auto flush enabled --> set process on VBlank
    if (flags & DMA_AUTOFLUSH) VIntProcess |= PROCESS_DMA_TASK;

    // get DMA info structure and pass to next one
    info = &dmaQueues[queueIndex++];

    // Setup Step and DMA length (in word here)
    info->regStepLenL = (0x8F00 | step) | ((0x9300 | (newlen & 0xFF)) << 16);
    // Setup DMA address
    info->regLenHAddrL = (0x9400 | ((newlen >> 8) & 0xFF)) | ((0x9500 | ((from >> 1) & 0xFF)) << 16);
    info->regAddrMAddrH = (0x9600 | ((from >> 9) & 0xFF)) | ((0x9700 | ((from >> 17) & 0x7F)) << 16);

    // Trigger DMA
    switch(location)
    {
        case DMA_VRAM:
            info->regCtrlWrite = GFX_DMA_VRAM_ADDR(to);
#ifdef DMA_DEBUG
            KLog_U4("DMA_queueDma: VRAM from=", from, " to=", to, " len=", len, " step=", step);
#endif
            break;

        case DMA_CRAM:
            info->regCtrlWrite = GFX_DMA_CRAM_ADDR(to);
#ifdef DMA_DEBUG
            KLog_U4("DMA_queueDma: CRAM from=", from, " to=", to, " len=", len, " step=", step);
#endif
            break;

        case DMA_VSRAM:
            info->regCtrlWrite = GFX_DMA_VSRAM_ADDR(to);
#ifdef DMA_DEBUG
            KLog_U4("DMA_queueDma: VSRAM from=", from, " to=", to, " len=", len, " step=", step);
#endif
            break;
    }

#ifdef DMA_DEBUG
    KLog_U2("  Queue index=", queueIndex, " new queueTransferSize=", queueTransferSize);
#endif

    // we have a limit defined ?
    if (maxTransferPerFrame)
    {
        // above limit ?
        if ((queueTransferSize > maxTransferPerFrame) && (queueIndexLimit == 0))
        {
#if (LIB_DEBUG != 0)
            KLog_S2("DMA_queueDma(..) warning: transfer size limit raised: current = ", queueTransferSize, "  max = ", maxTransferPerFrame);
#endif

            // more than 1 transfer ?
            if (queueIndex > 1)
            {
                // stop on previous transfer
                queueIndexLimit = queueIndex - 1;
                queueTransferSizeLimit = queueTransferSize - (newlen << 1);
            }
            else
            {
                queueIndexLimit = queueIndex;
                queueTransferSizeLimit = queueTransferSize;
            }

#ifdef DMA_DEBUG
            KLog_U2("  Queue index limit set at ", queueIndexLimit, " and queueTransferSizeLimit = ", queueTransferSizeLimit);
#endif
        }
    }
#if (LIB_DEBUG != 0)
    else
    {
        if ((IS_PALSYSTEM) && (queueTransferSize > 17600))
            KDebug_Alert("DMA_queueDma(..) warning: transfer size is above 17600 bytes.");
        else if (queueTransferSize > 7500)
            KDebug_Alert("DMA_queueDma(..) warning: transfer size is above 7500 bytes.");
    }
#endif

    return TRUE;
}

void DMA_waitCompletion()
{
    while(GET_VDPSTATUS(VDP_DMABUSY_FLAG));
}

void DMA_doDma(u8 location, u32 from, u16 to, u16 len, s16 step)
{
    vu16 *pw;
    vu32 *pl;
    u32 newlen;
    u32 banklimitb;
    u32 banklimitw;

    if (step != -1)
        VDP_setAutoInc(step);

    // DMA works on 64 KW bank
    banklimitb = 0x20000 - (from & 0x1FFFF);
    banklimitw = banklimitb >> 1;
    // bank limit exceeded
    if (len > banklimitw)
    {
        // we first do the second bank transfer
        DMA_doDma(location, from + banklimitb, to + banklimitb, len - banklimitw, -1);
        newlen = banklimitw;
    }
    // ok, use normal len
    else newlen = len;

    pw = (u16 *) GFX_CTRL_PORT;

    // Setup DMA length (in word here)
    *pw = 0x9300 + (newlen & 0xff);
    *pw = 0x9400 + ((newlen >> 8) & 0xff);

    // Setup DMA address
    from >>= 1;
    *pw = 0x9500 + (from & 0xff);
    from >>= 8;
    *pw = 0x9600 + (from & 0xff);
    from >>= 8;
    *pw = 0x9700 + (from & 0x7f);

    // Enable DMA
    pl = (u32 *) GFX_CTRL_PORT;
    switch(location)
    {
        case DMA_VRAM:
            *pl = GFX_DMA_VRAM_ADDR(to);
            break;

        case DMA_CRAM:
            *pl = GFX_DMA_CRAM_ADDR(to);
            break;

        case DMA_VSRAM:
            *pl = GFX_DMA_VSRAM_ADDR(to);
            break;
    }
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

    pw = (u16 *) GFX_CTRL_PORT;

    // Setup DMA length
    *pw = 0x9300 + (l & 0xFF);
    *pw = 0x9400 + ((l >> 8) & 0xFF);

    // Setup DMA operation (VRAM FILL)
    *pw = 0x9780;

    // Write VRam DMA destination address
    pl = (u32 *) GFX_CTRL_PORT;
    *pl = GFX_DMA_VRAM_ADDR(to);

    // set up value to fill (need to be 16 bits extended)
    pw = (u16 *) GFX_DATA_PORT;
    *pw = value | (value << 8);
}

void DMA_doVRamCopy(u16 from, u16 to, u16 len, s16 step)
{
    vu16 *pw;
    vu32 *pl;

    if (step != -1)
        VDP_setAutoInc(step);

    pw = (u16 *) GFX_CTRL_PORT;

    // Setup DMA length
    *pw = 0x9300 + (len & 0xff);
    *pw = 0x9400 + ((len >> 8) & 0xff);

    // Setup DMA address
    *pw = 0x9500 + (from & 0xff);
    *pw = 0x9600 + ((from >> 8) & 0xff);

    // Setup DMA operation (VRAM COPY)
    *pw = 0x97C0;

    // Write VRam DMA destination address (start DMA copy operation)
    pl = (u32 *) GFX_CTRL_PORT;
    *pl = GFX_DMA_VRAMCOPY_ADDR(to);
}
