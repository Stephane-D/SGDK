#include "dma.h"

#include "config.h"
#include "vdp.h"
#include "sys.h"
#include "memory.h"
#include "z80_ctrl.h"

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
static u16 flag;

// current queue index (0 = empty / queueSize = full)
static u16 queueIndex;
static u16 queueIndexLimit;
static u32 queueTransferSize;


void DMA_init(u16 size, u16 capacity)
{
    if (size) queueSize = size;
    else queueSize = DMA_DEFAULT_QUEUE_SIZE;

    // 0 means no limit
    maxTransferPerFrame = capacity;

    // auto flush is enabled by default
    flag = DMA_AUTOFLUSH;

    // already allocated ?
    if (dmaQueues) MEM_free(dmaQueues);
    // allocate DMA queue
    dmaQueues = MEM_alloc(queueSize * sizeof(DMAOpInfo));

    // clear queue
    DMA_clearQueue();
}

bool DMA_getAutoFlush()
{
    return (flag & DMA_AUTOFLUSH)?TRUE:FALSE;
}

void DMA_setAutoFlush(bool value)
{
    if (value)
    {
        flag |= DMA_AUTOFLUSH;
        // auto flush enabled and transfer size > 0 --> set process on VBlank
        if (queueTransferSize > 0)
             VIntProcess |= PROCESS_DMA_TASK;
    }
    else flag &= ~DMA_AUTOFLUSH;
}

s16 DMA_getMaxTransferSize()
{
    return maxTransferPerFrame;
}

void DMA_setMaxTransferSize(s16 value)
{
    maxTransferPerFrame = value;
}

void DMA_setMaxTransferSizeToDefault()
{
    DMA_setMaxTransferSize(IS_PALSYSTEM?15000:7200);
}

bool DMA_getIgnoreOverCapacity()
{
    return (flag & DMA_OVERCAPACITY_IGNORE)?TRUE:FALSE;
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
}

void DMA_flushQueue()
{
    vu32 *pl;
    u32 *info;
    u16 i;
#if (HALT_Z80_ON_DMA == 1)
    u16 z80state;
#endif

    // default
    i = queueIndex;

    // limit reached ?
    if (queueIndexLimit)
    {
        // we choose to ignore over capacity transfers ?
        if (flag & DMA_OVERCAPACITY_IGNORE)
        {
            i = queueIndexLimit;

#if (LIB_DEBUG != 0)
            KLog_U2_("DMA_flushQueue(..) warning: transfer size is above ", maxTransferPerFrame, " bytes (", queueTransferSize, "), some transfers are ignored.");
#endif
        }
#if (LIB_DEBUG != 0)
        else KLog_U2_("DMA_flushQueue(..) warning: transfer size is above ", maxTransferPerFrame, " bytes (", queueTransferSize, ").");
#endif
    }

    info = (u32*) dmaQueues;

#ifdef DMA_DEBUG
    KLog_U3("DMA_flushQueue: queueIndexLimit=", queueIndexLimit, " queueIndex=", queueIndex, " i=", i);
#endif

    // wait for DMA FILL / COPY operation to complete
    VDP_waitDMACompletion();

#if (HALT_Z80_ON_DMA == 1)
    z80state = Z80_isBusTaken();
    if (!z80state) Z80_requestBus(FALSE);
#endif

    pl = (vu32*) GFX_CTRL_PORT;

    while(i--)
    {
        // set DMA parameters and trigger it
        *pl = *info++;  // regStepLenL = (0x8F00 | step) | ((0x9300 | (len & 0xFF)) << 16)
        *pl = *info++;  // regLenHAddrL = (0x9400 | ((len >> 8) & 0xFF)) | ((0x9500 | ((addr >> 1) & 0xFF)) << 16)
        *pl = *info++;  // regAddrMAddrH = (0x9600 | ((addr >> 9) & 0xFF)) | ((0x9700 | ((addr >> 17) & 0x7F)) << 16)
        *pl = *info++;  // regCtrlWrite =  GFX_DMA_xxx_ADDR(to)
    }

#if (HALT_Z80_ON_DMA == 1)
    if (!z80state) Z80_releaseBus();
#endif

    // can clear the queue now
    DMA_clearQueue();
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

        // return FALSE as transfer will be ignored
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

    // get DMA info structure and pass to next one
    info = &dmaQueues[queueIndex];

    // $14:len H  $13:len L (DMA length in word)
    info->regLen = ((newlen | (newlen << 8)) & 0xFF00FF) | 0x94009300;
    // $16:M  $f:step (DMA address M and Step register)
    info->regAddrMStep = (((from << 7) & 0xFF0000) | 0x96008F00) + step;
    // $17:H  $15:L (DMA address H & L)
    info->regAddrHAddrL = ((from >> 1) & 0x7F00FF) | 0x97009500;

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

    // pass to next index
    queueIndex++;
    // keep trace of transfered size
    queueTransferSize += newlen << 1;

#ifdef DMA_DEBUG
    KLog_U2("  Queue index=", queueIndex, " new queueTransferSize=", queueTransferSize);
#endif

    // auto flush enabled --> set process on VBlank
    if (flag & DMA_AUTOFLUSH) VIntProcess |= PROCESS_DMA_TASK;

    // we are above the defined limit ?
    if (maxTransferPerFrame && (queueTransferSize > maxTransferPerFrame))
    {
        // first time we reach the limit ? store index where to stop transfer
        if (queueIndexLimit == 0)
        {
#if (LIB_DEBUG != 0)
            KLog_S3("DMA_queueDma(..) warning: transfer size limit raised on transfer #", queueIndex - 1, ", current size = ", queueTransferSize, "  max allowed = ", maxTransferPerFrame);
#endif

            // store limit index
            queueIndexLimit = queueIndex - 1;

#ifdef DMA_DEBUG
            KLog_U1("  Queue index limit set at ", queueIndexLimit);
#endif
        }

        // return FALSE if transfer will be ignored
        return (flag & DMA_OVERCAPACITY_IGNORE)?FALSE:TRUE;
    }

    return TRUE;
}

void DMA_waitCompletion()
{
    VDP_waitDMACompletion();
}

void DMA_doDma(u8 location, u32 from, u16 to, u16 len, s16 step)
{
    vu16 *pw;
    vu32 *pl;
    u32 newlen;
    u32 banklimitb;
    u32 banklimitw;
#if (HALT_Z80_ON_DMA == 1)
    u16 z80state;
#endif

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

    // wait for DMA FILL / COPY operation to complete
    VDP_waitDMACompletion();

    pw = (vu16*) GFX_CTRL_PORT;

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

#if (HALT_Z80_ON_DMA == 1)
    z80state = Z80_isBusTaken();
    if (!z80state) Z80_requestBus(FALSE);
#endif

    // Enable DMA
    pl = (vu32*) GFX_CTRL_PORT;
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

#if (HALT_Z80_ON_DMA == 1)
    if (!z80state) Z80_releaseBus();
#endif
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

    // wait for DMA FILL / COPY operation to complete
    VDP_waitDMACompletion();

    pw = (vu16*) GFX_CTRL_PORT;

    // Setup DMA length
    *pw = 0x9300 + (l & 0xFF);
    *pw = 0x9400 + ((l >> 8) & 0xFF);

    // Setup DMA operation (VRAM FILL)
    *pw = 0x9780;

    // Write VRam DMA destination address
    pl = (vu32*) GFX_CTRL_PORT;
    *pl = GFX_DMA_VRAM_ADDR(to);

    // set up value to fill (need to be 16 bits extended)
    pw = (vu16*) GFX_DATA_PORT;
    *pw = value | (value << 8);
}

void DMA_doVRamCopy(u16 from, u16 to, u16 len, s16 step)
{
    vu16 *pw;
    vu32 *pl;

    if (step != -1)
        VDP_setAutoInc(step);

    // wait for DMA FILL / COPY operation to complete
    VDP_waitDMACompletion();

    pw = (vu16*) GFX_CTRL_PORT;

    // Setup DMA length
    *pw = 0x9300 + (len & 0xff);
    *pw = 0x9400 + ((len >> 8) & 0xff);

    // Setup DMA address
    *pw = 0x9500 + (from & 0xff);
    *pw = 0x9600 + ((from >> 8) & 0xff);

    // Setup DMA operation (VRAM COPY)
    *pw = 0x97C0;

    // Write VRam DMA destination address (start DMA copy operation)
    pl = (vu32*) GFX_CTRL_PORT;
    *pl = GFX_DMA_VRAMCOPY_ADDR(to);
}
