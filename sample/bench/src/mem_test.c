#include <genesis.h>
#include <kdebug.h>

#include "main.h"


// forward
static u16 doAlloc(u16 num, u16 size, void **allocs, u16 verif);
static u16 doRelease(u16 num, u16 size, void **allocs, u16 verif);
static u16 doVRamAlloc(VRAMRegion *region, u16 num, u16 size, s16 *allocs, u16 verif);
static u16 doVRamRelease(VRAMRegion *region, u16 num, u16 size, s16 *allocs, u16 verif);
static u32 displayResult(u32 bytes, fix32 time, u16 y);
static u32 displayResultAlloc(u32 nb, fix32 time, u16 y);


u16 executeMemsetTest(u16 *scores)
{
    fix32 start;
    fix32 end;
    u32 i;
    u16 len, y;
    u8 *buffer;
    u8 value;
    u16 *score;
    u16 globalScore;

    buffer = MEM_alloc(16384);

    score = scores;
    globalScore = 0;

    value = 0;
    y = 0;
    VDP_drawText("Executing memset tests...", 1, y++);
    y++;

    VDP_drawText("50000 x 16 bytes", 2, y++);
    i = 50000;
    len = 16;
    start = getTimeAsFix32(FALSE);
    while(i--) memset(buffer, value, len);
    end = getTimeAsFix32(FALSE);
    *score = displayResult(50000 * 16, end - start, y++) / 2;
    globalScore += *score++;
    y++;

    VDP_drawText("20000 x 128 bytes", 2, y++);
    i = 20000;
    len = 128;
    start = getTimeAsFix32(FALSE);
    while(i--) memset(buffer, value, len);
    end = getTimeAsFix32(FALSE);
    *score = displayResult(20000 * 128, end - start, y++) / 2;
    globalScore += *score++;
    y++;

    VDP_drawText("5000 x 1024 bytes", 2, y++);
    i = 5000;
    len = 1024;
    start = getTimeAsFix32(FALSE);
    while(i--) memset(buffer, value, len);
    end = getTimeAsFix32(FALSE);
    *score = displayResult(5000 * 1024, end - start, y++) / 2;
    globalScore += *score++;
    y++;

    VDP_drawText("500 x 16384 bytes", 2, y++);
    i = 500;
    len = 16384;
    start = getTimeAsFix32(FALSE);
    while(i--) memset(buffer, value, len);
    end = getTimeAsFix32(FALSE);
    *score = displayResult(500 * 16384, end - start, y++) / 2;
    globalScore += *score++;
    y++;
    y++;

    VDP_drawText("32768 small (1 to 64 bytes)", 2, y++);
    i = 32768;
    start = getTimeAsFix32(FALSE);
    while(i--) memset(buffer, value, (i & 0x3F) + 1);
    end = getTimeAsFix32(FALSE);
    *score = displayResult(32768 * (1 + 32), end - start, y++) / 2;
    globalScore += *score++;
    y++;

    VDP_drawText("16384 medium (64 to 319 bytes)", 2, y++);
    i = 16384;
    start = getTimeAsFix32(FALSE);
    while(i--) memset(buffer, value, (i & 0xFF) + 64);
    end = getTimeAsFix32(FALSE);
    *score = displayResult(16384 * (64 + 128), end - start, y++) / 2;
    globalScore += *score++;
    y++;

    VDP_drawText("4096 large (512 to 1535 bytes)", 2, y++);
    i = 4096;
    start = getTimeAsFix32(FALSE);
    while(i--) memset(buffer, value, 512 + (i & 0x3FF));
    end = getTimeAsFix32(FALSE);
    *score = displayResult(4096 * (512 + 512), end - start, y++) / 2;
    globalScore += *score++;
    y++;

    waitMs(5000);
    VDP_clearPlane(BG_A, TRUE);

    MEM_free(buffer);

    return globalScore;
}

u16 executeMemcpyTest(u16 *scores)
{
    fix32 start;
    fix32 end;
    u16 i;
    u16 len, y;
    u8 *buffer;
    u16 *score;
    u16 globalScore;

    buffer = MEM_alloc(16384);
    score = scores;
    globalScore = 0;

    y = 0;
    VDP_drawText("Executing memcpy tests...", 1, y++);
    y++;

    VDP_drawText("50000 x 16 bytes", 2, y++);
    i = 50000;
    len = 16;
    start = getTimeAsFix32(FALSE);
    while(i--) memcpy(buffer + 8182, buffer, len);
    end = getTimeAsFix32(FALSE);
    *score = displayResult(50000 * 16, end - start, y++) / 1;
    globalScore += *score++;

    y++;

    VDP_drawText("20000 x 128 bytes", 2, y++);
    i = 20000;
    len = 128;
    start = getTimeAsFix32(FALSE);
    while(i--) memcpy(buffer + 8182, buffer, len);
    end = getTimeAsFix32(FALSE);
    *score = displayResult(20000 * 128, end - start, y++) / 1;
    globalScore += *score++;
    y++;

    VDP_drawText("5000 x 1024 bytes", 2, y++);
    i = 5000;
    len = 1024;
    start = getTimeAsFix32(FALSE);
    while(i--) memcpy(buffer + 8182, buffer, len);
    end = getTimeAsFix32(FALSE);
    *score = displayResult(5000 * 1024, end - start, y++) / 1;
    globalScore += *score++;
    y++;

    VDP_drawText("500 x 8192 bytes", 2, y++);
    i = 500;
    len = 8192;
    start = getTimeAsFix32(FALSE);
    while(i--) memcpy(buffer + 8182, buffer, len);
    end = getTimeAsFix32(FALSE);
    *score = displayResult(500 * 8192, end - start, y++) / 1;
    globalScore += *score++;
    y++;
    y++;

    VDP_drawText("32768 x [1-64] bytes", 2, y++);
    i = 32768;
    start = getTimeAsFix32(FALSE);
    while(i--) memcpy(buffer + 8182, buffer, (i & 0x3F) + 1);
    end = getTimeAsFix32(FALSE);
    *score = displayResult(32768 * (1 + 32), end - start, y++) / 1;
    globalScore += *score++;
    y++;

    VDP_drawText("16384 x [64-319] bytes", 2, y++);
    i = 16384;
    start = getTimeAsFix32(FALSE);
    while(i--) memcpy(buffer + 8182, buffer, (i & 0xFF) + 64);
    end = getTimeAsFix32(FALSE);
    *score = displayResult(16384 * (64 + 128), end - start, y++) / 1;
    globalScore += *score++;
    y++;

    VDP_drawText("50000 x 16 bytes (unaligned)", 2, y++);
    i = 50000;
    len = 16;
    start = getTimeAsFix32(FALSE);
    while(i--) memcpy(buffer + 8183, buffer, len);
    end = getTimeAsFix32(FALSE);
    *score = displayResult(50000 * 16, end - start, y++) / 1;
    globalScore += *score++;
    y++;

    VDP_drawText("5000 x 1024 bytes (unaligned)", 2, y++);
    i = 2000;
    len = 1024;
    start = getTimeAsFix32(FALSE);
    while(i--) memcpy(buffer + 8183, buffer, len);
    end = getTimeAsFix32(FALSE);
    *score = displayResult(2000 * 1024, end - start, y++) / 1;
    globalScore += *score++;
    y++;

    waitMs(5000);
    VDP_clearPlane(BG_A, TRUE);

    MEM_free(buffer);

    return globalScore;
}

extern u32 _bend;

u16 executeMemAllocTest(u16 *scores)
{
    fix32 start;
    fix32 end;
    fix32 time;
    u16 i, y;
    void **allocs;
    u16 *score;
    u16 globalScore;

    KLog_U1("Mem heap: ", (u32)&_bend);
    KLog_U2("Memory - Mem free before: ", MEM_getFree(), "   Mem allocated: ", MEM_getAllocated());

    allocs = MEM_alloc(1000 * sizeof(void*));

    score = scores;
    globalScore = 0;

    y = 0;
    VDP_drawText("Executing mem alloc tests...", 1, y++);
    y++;

    if (!doAlloc(1000, 16, allocs, TRUE))
        VDP_drawText("Error while allocating...", 2, y++);
    if (!doRelease(1000, 16, allocs, TRUE))
        VDP_drawText("Error while releasing...", 2, y++);

    VDP_drawText("50000 allocations of 16 bytes", 2, y++);
    i = 50;
    time = FIX32(0);
    while(i--)
    {
        // count time only for allocation
        start = getTimeAsFix32(FALSE);
        doAlloc(1000, 16, allocs, FALSE);
        end = getTimeAsFix32(FALSE);
        time += end - start;
        doRelease(1000, 16, allocs, FALSE);
    }
    *score = displayResultAlloc(50000, time, y++);
    globalScore += *score++;
    y++;

    if (!doAlloc(100, 256, allocs, TRUE))
        VDP_drawText("Error while allocating...", 2, y++);
    if (!doRelease(100, 256, allocs, TRUE))
        VDP_drawText("Error while releasing...", 2, y++);

    VDP_drawText("50000 allocations of 256 bytes", 2, y++);
    i = 500;
    time = FIX32(0);
    while(i--)
    {
        // count time only for allocation
        start = getTimeAsFix32(FALSE);
        doAlloc(100, 256, allocs, FALSE);
        end = getTimeAsFix32(FALSE);
        time += end - start;
        doRelease(100, 256, allocs, FALSE);
    }
    *score = displayResultAlloc(50000, time, y++);
    globalScore += *score++;
    y++;

    VDP_drawText("100000 releases of 16 bytes", 2, y++);
    i = 100;
    time = FIX32(0);
    while(i--)
    {
        doAlloc(1000, 16, allocs, FALSE);
        // count time only for release
        start = getTimeAsFix32(FALSE);
        doRelease(1000, 16, allocs, FALSE);
        end = getTimeAsFix32(FALSE);
        time += end - start;
    }
    *score = displayResultAlloc(100000, time, y++);
    globalScore += *score++;
    y++;

    VDP_drawText("100000 releases of 256 bytes", 2, y++);
    i = 1000;
    time = FIX32(0);
    while(i--)
    {
        doAlloc(100, 256, allocs, FALSE);
        // count time only for release
        start = getTimeAsFix32(FALSE);
        doRelease(100, 256, allocs, FALSE);
        end = getTimeAsFix32(FALSE);
        time += end - start;
    }
    *score = displayResultAlloc(100000, time, y++);
    globalScore += *score++;
    y++;

    if (!doAlloc(100, 32, allocs, TRUE))
        VDP_drawText("Error while allocating...", 2, y++);
    if (!doRelease(100, 32, allocs, TRUE))
        VDP_drawText("Error while releasing...", 2, y++);

    VDP_drawText("50000 alloc/release of 32 bytes", 2, y++);
    i = 500;
    start = getTimeAsFix32(FALSE);
    while(i--)
    {
        doAlloc(100, 32, allocs, FALSE);
        doRelease(100, 32, allocs, FALSE);
    }
    end = getTimeAsFix32(FALSE);
    *score = displayResultAlloc(50000, end - start, y++);
    globalScore += *score++;
    y++;

    // here we have about 55000 bytes free

    if (!doAlloc(100, 16, &allocs[0], TRUE))                    // 1800 bytes
        VDP_drawText("Error while allocating 16...", 2, y++);
    if (!doAlloc(300, 64, &allocs[100], TRUE))                  // +19800 = 21600
        VDP_drawText("Error while allocating 64...", 2, y++);
    if (!doAlloc(15, 1024, &allocs[400], TRUE))                 // +15390 = 36990
        VDP_drawText("Error while allocating 1024...", 2, y++);
    if (!doAlloc(20, 128, &allocs[415], TRUE))                  // +12900 = 49890
        VDP_drawText("Error while allocating 256...", 2, y++);
    if (!doRelease(300, 64, &allocs[100], TRUE))                // -19800 = 30090
        VDP_drawText("Error while releasing 64...", 2, y++);
    if (!doAlloc(50, 256, &allocs[500], TRUE))                 // +20520 = 50610
        VDP_drawText("Error while allocating 1024...", 2, y++);
    if (!doRelease(100, 16, &allocs[0], TRUE))                  // -1800 = 48810
        VDP_drawText("Error while releasing 16...", 2, y++);
    if (!doAlloc(15, 512, &allocs[550], TRUE))                  // +2600 = 51410
        VDP_drawText("Error while allocating 128...", 2, y++);
    if (!doRelease(15, 1024, &allocs[400], TRUE))               // -15390 = 36020
        VDP_drawText("Error while releasing 1024...", 2, y++);
    if (!doRelease(20, 128, &allocs[415], TRUE))                // -12900 = 23120
        VDP_drawText("Error while releasing 256...", 2, y++);
    if (!doRelease(50, 256, &allocs[500], TRUE))               // -20520 = 2600
        VDP_drawText("Error while releasing 1024...", 2, y++);
    if (!doRelease(15, 512, &allocs[550], TRUE))                // -2600 = 0
        VDP_drawText("Error while releasing 128...", 2, y++);

    VDP_drawText("50000 random size alloc/release", 2, y++);
    i = 100;
    start = getTimeAsFix32(FALSE);
    while(i--)
    {
        doAlloc(100, 16, &allocs[0], FALSE);                    // 1800 bytes
        doAlloc(300, 64, &allocs[100], FALSE);                  // +19800 = 21600
        doAlloc(15, 1024, &allocs[400], FALSE);                 // +15390 = 36990
        doAlloc(20, 128, &allocs[415], FALSE);                  // +12900 = 49890
        doRelease(300, 64, &allocs[100], FALSE);                // -19800 = 30090
        doAlloc(50, 256, &allocs[500], FALSE);                 // +20520 = 50610
        doRelease(100, 16, &allocs[0], FALSE);                  // -1800 = 48810
        doAlloc(15, 512, &allocs[550], FALSE);                  // +2600 = 51410
        doRelease(15, 1024, &allocs[400], FALSE);               // -15390 = 36020
        doRelease(20, 128, &allocs[415], FALSE);                // -12900 = 23120
        doRelease(50, 256, &allocs[500], FALSE);               // -20520 = 2600
        doRelease(15, 512, &allocs[550], FALSE);                // -2600 = 0
    }
    end = getTimeAsFix32(FALSE);
    *score = displayResultAlloc(50000, end - start, y++);
    globalScore += *score++;
    y++;

    waitMs(5000);
    VDP_clearPlane(BG_A, TRUE);

    MEM_free(allocs);
    MEM_pack();

    KLog_U2("Memory - Mem free after: ", MEM_getFree(), "   Mem allocated: ", MEM_getAllocated());

    return globalScore;
}

u16 executeVRamAllocTest(u16 *scores)
{
    fix32 start;
    fix32 end;
    fix32 time;
    u16 i, y;
    s16 *allocs;
    VRAMRegion region;
    u16 *score;
    u16 globalScore;

    KLog_U2("VRAM - Mem free before: ", MEM_getFree(), "   Mem allocated: ", MEM_getAllocated());

    VRAM_createRegion(&region, 16, 1000);
    allocs = MEM_alloc(1000 * sizeof(s16));
    score = scores;
    globalScore = 0;

    y = 0;
    VDP_drawText("Executing VRAM alloc tests...", 1, y++);
    y++;

    if (!doVRamAlloc(&region, 1000, 1, allocs, TRUE))
        VDP_drawText("Error while allocating...", 2, y++);
    if (!doVRamRelease(&region, 1000, 1, allocs, TRUE))
        VDP_drawText("Error while releasing...", 2, y++);

    VDP_drawText("50000 allocations of 1 tile", 2, y++);
    i = 50;
    time = FIX32(0);
    while(i--)
    {
        // count time only for allocation
        start = getTimeAsFix32(FALSE);
        doVRamAlloc(&region, 1000, 1, allocs, FALSE);
        end = getTimeAsFix32(FALSE);
        time += end - start;
        doVRamRelease(&region, 1000, 1, allocs, FALSE);
    }
    *score = displayResultAlloc(50000, time, y++);
    globalScore += *score++;
    y++;

    if (!doVRamAlloc(&region, 100, 8, allocs, TRUE))
        VDP_drawText("Error while allocating...", 2, y++);
    if (!doVRamRelease(&region, 100, 8, allocs, TRUE))
        VDP_drawText("Error while releasing...", 2, y++);

    VDP_drawText("50000 allocations of 8 tiles", 2, y++);
    i = 500;
    time = FIX32(0);
    while(i--)
    {
        // count time only for allocation
        start = getTimeAsFix32(FALSE);
        doVRamAlloc(&region, 100, 8, allocs, FALSE);
        end = getTimeAsFix32(FALSE);
        time += end - start;
        doVRamRelease(&region, 100, 8, allocs, FALSE);
    }
    *score = displayResultAlloc(50000, time, y++);
    globalScore += *score++;
    y++;

    if (!doVRamAlloc(&region, 10, 64, allocs, TRUE))
        VDP_drawText("Error while allocating...", 2, y++);
    if (!doVRamRelease(&region, 10, 64, allocs, TRUE))
        VDP_drawText("Error while releasing...", 2, y++);

    VDP_drawText("50000 allocations of 64 tiles", 2, y++);
    i = 5000;
    time = FIX32(0);
    while(i--)
    {
        // count time only for allocation
        start = getTimeAsFix32(FALSE);
        doVRamAlloc(&region, 10, 64, allocs, FALSE);
        end = getTimeAsFix32(FALSE);
        time += end - start;
        doVRamRelease(&region, 10, 64, allocs, FALSE);
    }
    *score = displayResultAlloc(50000, time, y++);
    globalScore += *score++;
    y++;

    VDP_drawText("100000 releases of 1 tile", 2, y++);
    i = 100;
    time = FIX32(0);
    while(i--)
    {
        doVRamAlloc(&region, 1000, 1, allocs, FALSE);
        // count time only for release
        start = getTimeAsFix32(FALSE);
        doVRamRelease(&region, 1000, 1, allocs, FALSE);
        end = getTimeAsFix32(FALSE);
        time += end - start;
    }
    *score = displayResultAlloc(100000, time, y++);
    globalScore += *score++;
    y++;

    VDP_drawText("100000 releases of 8 tiles", 2, y++);
    i = 1000;
    time = FIX32(0);
    while(i--)
    {
        doVRamAlloc(&region, 100, 8, allocs, FALSE);
        // count time only for release
        start = getTimeAsFix32(FALSE);
        doVRamRelease(&region, 100, 8, allocs, FALSE);
        end = getTimeAsFix32(FALSE);
        time += end - start;
    }
    *score = displayResultAlloc(100000, time, y++);
    globalScore += *score++;
    y++;

    VDP_drawText("100000 releases of 64 tiles", 2, y++);
    i = 10000;
    time = FIX32(0);
    while(i--)
    {
        doVRamAlloc(&region, 10, 64, allocs, FALSE);
        // count time only for release
        start = getTimeAsFix32(FALSE);
        doVRamRelease(&region, 10, 64, allocs, FALSE);
        end = getTimeAsFix32(FALSE);
        time += end - start;
    }
    *score = displayResultAlloc(100000, time, y++);
    globalScore += *score++;
    y++;

    VDP_drawText("50000 alloc/release of 8 tiles", 2, y++);
    i = 500;
    start = getTimeAsFix32(FALSE);
    while(i--)
    {
        doVRamAlloc(&region, 100, 8, allocs, FALSE);
        doVRamRelease(&region, 100, 8, allocs, FALSE);
    }
    end = getTimeAsFix32(FALSE);
    *score = displayResultAlloc(50000, end - start, y++);
    globalScore += *score++;
    y++;

    if (!doVRamAlloc(&region, 100, 1, &allocs[0], TRUE))
        VDP_drawText("Error while allocating...", 2, y++);
    if (!doVRamAlloc(&region, 200, 2, &allocs[100], TRUE))
        VDP_drawText("Error while allocating...", 2, y++);
    if (!doVRamAlloc(&region, 100, 1, &allocs[300], TRUE))
        VDP_drawText("Error while allocating...", 2, y++);
    if (!doVRamAlloc(&region, 20, 8, &allocs[400], TRUE))
        VDP_drawText("Error while allocating...", 2, y++);
    if (!doVRamRelease(&region, 200, 2, &allocs[100], TRUE))
        VDP_drawText("Error while releasing...", 2, y++);
    if (!doVRamAlloc(&region, 20, 16, &allocs[420], TRUE))
        VDP_drawText("Error while allocating...", 2, y++);
    if (!doVRamRelease(&region, 100, 1, &allocs[0], TRUE))
        VDP_drawText("Error while releasing...", 2, y++);
    if (!doVRamAlloc(&region, 50, 4, &allocs[440], TRUE))
        VDP_drawText("Error while allocating...", 2, y++);
    if (!doVRamRelease(&region, 20, 8, &allocs[400], TRUE))
        VDP_drawText("Error while releasing...", 2, y++);
    if (!doVRamRelease(&region, 100, 1, &allocs[300], TRUE))
        VDP_drawText("Error while releasing...", 2, y++);
    if (!doVRamAlloc(&region, 10, 32, &allocs[490], TRUE))
        VDP_drawText("Error while allocating...", 2, y++);
    if (!doVRamRelease(&region, 20, 16, &allocs[420], TRUE))
        VDP_drawText("Error while releasing...", 2, y++);
    if (!doVRamRelease(&region, 50, 4, &allocs[440], TRUE))
        VDP_drawText("Error while releasing...", 2, y++);
    if (!doVRamRelease(&region, 10, 32, &allocs[490], TRUE))
        VDP_drawText("Error while releasing...", 2, y++);

    VDP_drawText("50000 random size alloc/release", 2, y++);
    i = 100;
    start = getTimeAsFix32(FALSE);
    while(i--)
    {
        doVRamAlloc(&region, 100, 1, &allocs[0], FALSE);
        doVRamAlloc(&region, 200, 2, &allocs[100], FALSE);
        doVRamAlloc(&region, 100, 1, &allocs[300], FALSE);
        doVRamAlloc(&region, 20, 8, &allocs[400], FALSE);
        doVRamRelease(&region, 200, 2, &allocs[100], FALSE);
        doVRamAlloc(&region, 20, 16, &allocs[420], FALSE);
        doVRamRelease(&region, 100, 1, &allocs[0], FALSE);
        doVRamAlloc(&region, 50, 4, &allocs[440], FALSE);
        doVRamRelease(&region, 20, 8, &allocs[400], FALSE);
        doVRamRelease(&region, 100, 1, &allocs[300], FALSE);
        doVRamAlloc(&region, 10, 32, &allocs[490], FALSE);
        doVRamRelease(&region, 20, 16, &allocs[420], FALSE);
        doVRamRelease(&region, 50, 4, &allocs[440], FALSE);
        doVRamRelease(&region, 10, 32, &allocs[490], FALSE);
    }
    end = getTimeAsFix32(FALSE);
    *score = displayResultAlloc(50000, end - start, y++);
    globalScore += *score++;
    y++;

    waitMs(5000);
    VDP_clearPlane(BG_A, TRUE);

    MEM_free(allocs);
    VRAM_releaseRegion(&region);

    MEM_pack();

    KLog_U2("VRAM - Mem free after: ", MEM_getFree(), "   Mem allocated: ", MEM_getAllocated());

    return globalScore;
}


static u16 doAlloc(u16 num, u16 size, void **allocs, u16 verif)
{
    void **tab;
    u16 i;
    u16 free = 0;

    if (verif) free = MEM_getFree();

    tab = allocs;
    i = num;
    while(i > 10)
    {
        *tab++ = MEM_alloc(size);
        *tab++ = MEM_alloc(size);
        *tab++ = MEM_alloc(size);
        *tab++ = MEM_alloc(size);
        *tab++ = MEM_alloc(size);
        *tab++ = MEM_alloc(size);
        *tab++ = MEM_alloc(size);
        *tab++ = MEM_alloc(size);
        *tab++ = MEM_alloc(size);
        *tab++ = MEM_alloc(size);
        i -= 10;
    }
    while(i--) *tab++ = MEM_alloc(size);

    if (verif)
    {
        tab = allocs;
        i = num;
        while(i--)
        {
            if (*tab++ == NULL)
            {
                KDebug_Alert("Error alloc - first position:");
                KDebug_AlertNumber(tab - allocs);
                break;
            }
        }

        // verify allocation was correctly done
        if ((free - ((size + 2) * num)) != MEM_getFree())
        {
            KDebug_Alert("Error alloc");
            KDebug_AlertNumber(free);
            KDebug_AlertNumber((size + 2) * num);
            KDebug_AlertNumber(MEM_getFree());
            return FALSE;
        }
    }

    return TRUE;
}

static u16 doRelease(u16 num, u16 size, void **allocs, u16 verif)
{
    void **tab;
    u16 i;
    u16 free = 0;

    if (verif) free = MEM_getFree();

    tab = allocs;
    i = num;
    while(i > 10)
    {
        MEM_free(*tab++);
        MEM_free(*tab++);
        MEM_free(*tab++);
        MEM_free(*tab++);
        MEM_free(*tab++);
        MEM_free(*tab++);
        MEM_free(*tab++);
        MEM_free(*tab++);
        MEM_free(*tab++);
        MEM_free(*tab++);
        i -= 10;
    }
    while(i--) MEM_free(*tab++);

    if (verif)
    {
        // verify release was correctly done
        if ((free + ((size + 2) * num)) != MEM_getFree())
        {
            KDebug_Alert("Error release");
            KDebug_AlertNumber(free);
            KDebug_AlertNumber((size + 2) * num);
            KDebug_AlertNumber(MEM_getFree());
            return FALSE;
        }
    }

    return TRUE;
}


static u16 doVRamAlloc(VRAMRegion *region, u16 num, u16 size, s16 *allocs, u16 verif)
{
    s16 *tab;
    u16 i;
    u16 free = 0;

    if (verif) free = VRAM_getFree(region);

    tab = allocs;
    i = num;
    while(i)
    {
        *tab++ = VRAM_alloc(region, size);
        *tab++ = VRAM_alloc(region, size);
        *tab++ = VRAM_alloc(region, size);
        *tab++ = VRAM_alloc(region, size);
        *tab++ = VRAM_alloc(region, size);
        *tab++ = VRAM_alloc(region, size);
        *tab++ = VRAM_alloc(region, size);
        *tab++ = VRAM_alloc(region, size);
        *tab++ = VRAM_alloc(region, size);
        *tab++ = VRAM_alloc(region, size);
        i -= 10;
    }

    if (verif)
    {
        tab = allocs;
        i = num;
        while(i--)
        {
            if (*tab++ == -1)
            {
                KDebug_Alert("Error alloc - first position:");
                KDebug_AlertNumber(tab - allocs);
                break;
            }
        }

        // verify allocation was correctly done
        if ((free - (size * num)) != VRAM_getFree(region))
        {
            KDebug_Alert("Error VRAM alloc");
            KDebug_AlertNumber(free);
            KDebug_AlertNumber(size * num);
            KDebug_AlertNumber(VRAM_getFree(region));
            return FALSE;
        }
    }

    return TRUE;
}

static u16 doVRamRelease(VRAMRegion *region, u16 num, u16 size, s16 *allocs, u16 verif)
{
    s16 *tab;
    u16 i;
    u16 free = 0;

    if (verif) free = VRAM_getFree(region);

    tab = allocs;
    i = num;
    while(i)
    {
        VRAM_free(region, *tab++);
        VRAM_free(region, *tab++);
        VRAM_free(region, *tab++);
        VRAM_free(region, *tab++);
        VRAM_free(region, *tab++);
        VRAM_free(region, *tab++);
        VRAM_free(region, *tab++);
        VRAM_free(region, *tab++);
        VRAM_free(region, *tab++);
        VRAM_free(region, *tab++);
        i -= 10;
    }

    if (verif)
    {
        // verify release was correctly done
        if ((free + (size * num)) != VRAM_getFree(region))
        {
            KDebug_Alert("Error VRAM release");
            KDebug_AlertNumber(free);
            KDebug_AlertNumber(size * num);
            KDebug_AlertNumber(MEM_getFree());
            return FALSE;
        }
    }

    return TRUE;
}


static u32 displayResult(u32 bytes, fix32 time, u16 y)
{
    char timeStr[32];
    char speedStr[32];
    char str[64];
    fix32 speedKb;

    fix32ToStr(time, timeStr, 2);
    speedKb = intToFix32(bytes / 4096);
    // get speed in Kb/s
    speedKb = fix32Div(speedKb, time);
    // put it in speedStr
    fix32ToStr(speedKb * 4, speedStr, 2);

    strcpy(str, "Elapsed time = ");
    strcat(str, timeStr);
    strcat(str, "s (");
    strcat(str, speedStr);
    strcat(str, " Kb/s)");

    // display test string
    VDP_drawText(str, 3, y);

    return fix32ToInt(speedKb);
}

static u32 displayResultAlloc(u32 nb, fix32 time, u16 y)
{
    char timeStr[32];
    char speedStr[32];
    char str[64];
    fix32 speed;

    fix32ToStr(time, timeStr, 2);
    speed = intToFix32(nb / 100);
    // get speed in op/s
    speed = fix32Div(speed, time);
    // put it in speedStr
    intToStr(fix32ToInt(speed) * 100, speedStr, 1);

    strcpy(str, "Elapsed time = ");
    strcat(str, timeStr);
    strcat(str, "s (");
    strcat(str, speedStr);
    strcat(str, " op/s)");

    // display test string
    VDP_drawText(str, 3, y);

    return fix32ToInt(speed);
}
