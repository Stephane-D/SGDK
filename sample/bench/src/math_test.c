#include <genesis.h>

#include "main.h"


#define RAND_32         (getrandom() | (getrandom() << 16))
#define RAND_16         (getrandom())
#define RAND_8          (getrandom() & 0xFF)
#define FIX_RAND_32     0x957FBE6
#define FIX_RAND_16     0x1DE7
#define FIX_RAND_8      0xCE


// extern assembly methods
void bench_add8reg(u16 len);
void bench_add16reg(u16 len);
void bench_add32reg(u16 len);
void bench_add8mem(u8 *src, u8 *dst, u16 len);
void bench_add16mem(u16 *src, u16 *dst, u16 len);
void bench_add32mem(u32 *src, u32 *dst, u16 len);
void bench_mulu(u16 src, u16 dst, u16 len);
void bench_muls(u16 src, u16 dst, u16 len);
void bench_divu(u32 src, u16 dst, u16 len);
void bench_divs(u32 src, u16 dst, u16 len);

// forward
static u32 displayResult(u32 op, fix32 time, u16 y, u32 dirty);
static u32 displayResult3D(u32 op, fix32 time, u16 y, u32 dirty);

u16 executeMathsBasicTest(u16 *scores)
{
    u8 *buffer;
    fix32 start;
    fix32 end;
    u16 i;
    u16 y;
#ifdef ENABLE_ASM
    u8 *ps8, *pd8;
    u16 *ps16, *pd16;
    u32 *ps32, *pd32;
#else
    u8 _s8;
    u16 _s16;
    u32 _s32;
    vu8 d8;             // need volatile to not be optimized but that screw up the results (much slower)
    vu16 d16;
    vu32 d32;
    vu8 *ps8, *pd8;
    vu16 *ps16, *pd16;
    vu32 *ps32, *pd32;
#endif
    u16 *score;
    u16 globalScore;

    score = scores;
    globalScore = 0;

    y = 0;
    VDP_drawText("Executing basic maths tests...", 1, y++);
    y++;

    VDP_drawText("2000000 8bit add (reg)", 2, y++);
#ifndef ENABLE_ASM
    _s8 = RAND_8;
    d8 = RAND_8;
#endif
    i = 2000000 / 32;
    start = getTimeAsFix32(FALSE);
#ifdef ENABLE_ASM
    bench_add8reg(i);
#else
    while (i--)
    {
        d8 += _s8;
        d8 += _s8;
        d8 += _s8;
        d8 += _s8;
        d8 += _s8;
        d8 += _s8;
        d8 += _s8;
        d8 += _s8;

        d8 += _s8;
        d8 += _s8;
        d8 += _s8;
        d8 += _s8;
        d8 += _s8;
        d8 += _s8;
        d8 += _s8;
        d8 += _s8;

        d8 += _s8;
        d8 += _s8;
        d8 += _s8;
        d8 += _s8;
        d8 += _s8;
        d8 += _s8;
        d8 += _s8;
        d8 += _s8;

        d8 += _s8;
        d8 += _s8;
        d8 += _s8;
        d8 += _s8;
        d8 += _s8;
        d8 += _s8;
        d8 += _s8;
        d8 += _s8;
    }
#endif // ENABLE_ASM
    end = getTimeAsFix32(FALSE);
    *score = displayResult(2000000, end - start, y++, 0) / 1;
    globalScore += *score++;
    y++;

    VDP_drawText("2000000 16bit add (reg)", 2, y++);
#ifndef ENABLE_ASM
    _s16 = RAND_16;
    d16 = RAND_16;
#endif
    i = 2000000 / 32;
    start = getTimeAsFix32(FALSE);
#ifdef ENABLE_ASM
    bench_add16reg(i);
#else
    while (i--)
    {
        d16 += _s16;
        d16 += _s16;
        d16 += _s16;
        d16 += _s16;
        d16 += _s16;
        d16 += _s16;
        d16 += _s16;
        d16 += _s16;

        d16 += _s16;
        d16 += _s16;
        d16 += _s16;
        d16 += _s16;
        d16 += _s16;
        d16 += _s16;
        d16 += _s16;
        d16 += _s16;

        d16 += _s16;
        d16 += _s16;
        d16 += _s16;
        d16 += _s16;
        d16 += _s16;
        d16 += _s16;
        d16 += _s16;
        d16 += _s16;

        d16 += _s16;
        d16 += _s16;
        d16 += _s16;
        d16 += _s16;
        d16 += _s16;
        d16 += _s16;
        d16 += _s16;
        d16 += _s16;
    }
#endif // ENABLE_ASM
    end = getTimeAsFix32(FALSE);
    *score = displayResult(2000000, end - start, y++, 0) / 1;
    globalScore += *score++;
    y++;

    VDP_drawText("2000000 32bit add (reg)", 2, y++);
#ifndef ENABLE_ASM
    _s32 = RAND_32;
    d32 = RAND_32;
#endif // ENABLE_ASM
    i = 2000000 / 32;
    start = getTimeAsFix32(FALSE);
#ifdef ENABLE_ASM
    bench_add32reg(i);
#else
    while (i--)
    {
        d32 += _s32;
        d32 += _s32;
        d32 += _s32;
        d32 += _s32;
        d32 += _s32;
        d32 += _s32;
        d32 += _s32;
        d32 += _s32;

        d32 += _s32;
        d32 += _s32;
        d32 += _s32;
        d32 += _s32;
        d32 += _s32;
        d32 += _s32;
        d32 += _s32;
        d32 += _s32;

        d32 += _s32;
        d32 += _s32;
        d32 += _s32;
        d32 += _s32;
        d32 += _s32;
        d32 += _s32;
        d32 += _s32;
        d32 += _s32;

        d32 += _s32;
        d32 += _s32;
        d32 += _s32;
        d32 += _s32;
        d32 += _s32;
        d32 += _s32;
        d32 += _s32;
        d32 += _s32;
    }
#endif // ENABLE_ASM
    end = getTimeAsFix32(FALSE);
    *score = displayResult(2000000, end - start, y++, 0) / 1;
    globalScore += *score++;
    y++;

    buffer = MEM_alloc(1024);

    VDP_drawText("1000000 8bit add (mem)", 2, y++);
    ps8 = buffer + 128;
    pd8 = buffer + 0;
    i = 1000000 / 32;
    start = getTimeAsFix32(FALSE);
#ifdef ENABLE_ASM
    bench_add8mem(ps8, pd8, i);
#else
    while (i--)
    {
        *pd8++ += *ps8++;
        *pd8++ += *ps8++;
        *pd8++ += *ps8++;
        *pd8++ += *ps8++;
        *pd8++ += *ps8++;
        *pd8++ += *ps8++;
        *pd8++ += *ps8++;
        *pd8++ += *ps8++;

        *pd8++ += *ps8++;
        *pd8++ += *ps8++;
        *pd8++ += *ps8++;
        *pd8++ += *ps8++;
        *pd8++ += *ps8++;
        *pd8++ += *ps8++;
        *pd8++ += *ps8++;
        *pd8++ += *ps8++;

        *pd8++ += *ps8++;
        *pd8++ += *ps8++;
        *pd8++ += *ps8++;
        *pd8++ += *ps8++;
        *pd8++ += *ps8++;
        *pd8++ += *ps8++;
        *pd8++ += *ps8++;
        *pd8++ += *ps8++;

        *pd8++ += *ps8++;
        *pd8++ += *ps8++;
        *pd8++ += *ps8++;
        *pd8++ += *ps8++;
        *pd8++ += *ps8++;
        *pd8++ += *ps8++;
        *pd8++ += *ps8++;
        *pd8++ += *ps8++;

        pd8 -= 32;
        ps8 -= 32;
    }
#endif // ENABLE_ASM
    end = getTimeAsFix32(FALSE);
    *score = displayResult(1000000, end - start, y++, 0) / 1;
    globalScore += *score++;
    y++;

    VDP_drawText("1000000 16bit add (mem)", 2, y++);
    ps16 = (u16*) buffer + 128;
    pd16 = (u16*) buffer + 0;
    i = 1000000 / 32;
    start = getTimeAsFix32(FALSE);
#ifdef ENABLE_ASM
    bench_add16mem(ps16, pd16, i);
#else
    while (i--)
    {
        *pd16++ += *ps16++;
        *pd16++ += *ps16++;
        *pd16++ += *ps16++;
        *pd16++ += *ps16++;
        *pd16++ += *ps16++;
        *pd16++ += *ps16++;
        *pd16++ += *ps16++;
        *pd16++ += *ps16++;

        *pd16++ += *ps16++;
        *pd16++ += *ps16++;
        *pd16++ += *ps16++;
        *pd16++ += *ps16++;
        *pd16++ += *ps16++;
        *pd16++ += *ps16++;
        *pd16++ += *ps16++;
        *pd16++ += *ps16++;

        *pd16++ += *ps16++;
        *pd16++ += *ps16++;
        *pd16++ += *ps16++;
        *pd16++ += *ps16++;
        *pd16++ += *ps16++;
        *pd16++ += *ps16++;
        *pd16++ += *ps16++;
        *pd16++ += *ps16++;

        *pd16++ += *ps16++;
        *pd16++ += *ps16++;
        *pd16++ += *ps16++;
        *pd16++ += *ps16++;
        *pd16++ += *ps16++;
        *pd16++ += *ps16++;
        *pd16++ += *ps16++;
        *pd16++ += *ps16++;

        pd16 -= 32;
        ps16 -= 32;
    }
#endif // ENABLE_ASM
    end = getTimeAsFix32(FALSE);
    *score = displayResult(1000000, end - start, y++, 0) / 1;
    globalScore += *score++;
    y++;

    VDP_drawText("1000000 32bit add (mem)", 2, y++);
    ps32 = (u32*) buffer + 128;
    pd32 = (u32*) buffer + 0;
    i = 1000000 / 32;
    start = getTimeAsFix32(FALSE);
#ifdef ENABLE_ASM
    bench_add32mem(ps32, pd32, i);
#else
    while (i--)
    {
        *pd32++ += *ps32++;
        *pd32++ += *ps32++;
        *pd32++ += *ps32++;
        *pd32++ += *ps32++;
        *pd32++ += *ps32++;
        *pd32++ += *ps32++;
        *pd32++ += *ps32++;
        *pd32++ += *ps32++;

        *pd32++ += *ps32++;
        *pd32++ += *ps32++;
        *pd32++ += *ps32++;
        *pd32++ += *ps32++;
        *pd32++ += *ps32++;
        *pd32++ += *ps32++;
        *pd32++ += *ps32++;
        *pd32++ += *ps32++;

        *pd32++ += *ps32++;
        *pd32++ += *ps32++;
        *pd32++ += *ps32++;
        *pd32++ += *ps32++;
        *pd32++ += *ps32++;
        *pd32++ += *ps32++;
        *pd32++ += *ps32++;
        *pd32++ += *ps32++;

        *pd32++ += *ps32++;
        *pd32++ += *ps32++;
        *pd32++ += *ps32++;
        *pd32++ += *ps32++;
        *pd32++ += *ps32++;
        *pd32++ += *ps32++;
        *pd32++ += *ps32++;
        *pd32++ += *ps32++;

        pd32 -= 32;
        ps32 -= 32;
    }
#endif // ENABLE_ASM
    end = getTimeAsFix32(FALSE);
    *score = displayResult(1000000, end - start, y++, 0) / 1;
    globalScore += *score++;
    y++;
    y++;


    waitMs(5000);
    VDP_clearPlane(BG_A, TRUE);

    MEM_free(buffer);

    return globalScore;
}


u16 executeMathsAdvTest(u16 *scores)
{
    fix32 start;
    fix32 end;
    u16 i;
    u16 y;
    u16 _s16;
    s16 _ss16;
#ifdef ENABLE_ASM
    u16 d16;
    s16 sd16;
    u32 d32;
    s32 sd32;
#else
    vu16 d16;       // need volatile to not be optimized but that may screw up the results
    vs16 sd16;
    vu32 d32;
    vs32 sd32;
#endif
    u16 *score;
    u16 globalScore;
    Vect3D_f16 *src_3D;
    Vect3D_f16 *res_3D;
    Vect2D_s16 *res_2D;
    Rotation3D rotation;
    Translation3D translation;
    Transformation3D transformation;

    src_3D = MEM_alloc(1024 * sizeof(Vect3D_f16));
    res_3D = MEM_alloc(1024 * sizeof(Vect3D_f16));
    res_2D = MEM_alloc(1024 * sizeof(Vect2D_s16));

    score = scores;
    globalScore = 0;

    y = 0;
    VDP_drawText("Executing Adv. maths tests...", 1, y++);
    y++;

    VDP_drawText("500000 16x16=32 unsign multiply", 2, y++);
    _s16 = FIX_RAND_16;
    d16 = FIX_RAND_16;
    i = 500000 / 16;
    start = getTimeAsFix32(FALSE);
#ifdef ENABLE_ASM
    bench_mulu(_s16, d16, i);
#else
    while (i--)
    {
        d16 = mulu(_s16, d16);
        d16 = mulu(_s16, d16);
        d16 = mulu(_s16, d16);
        d16 = mulu(_s16, d16);
        d16 = mulu(_s16, d16);
        d16 = mulu(_s16, d16);
        d16 = mulu(_s16, d16);
        d16 = mulu(_s16, d16);

        d16 = mulu(_s16, d16);
        d16 = mulu(_s16, d16);
        d16 = mulu(_s16, d16);
        d16 = mulu(_s16, d16);
        d16 = mulu(_s16, d16);
        d16 = mulu(_s16, d16);
        d16 = mulu(_s16, d16);
        d16 = mulu(_s16, d16);
    }
#endif // ENABLE_ASM
    end = getTimeAsFix32(FALSE);
    *score = displayResult(500000, end - start, y++, 0);
    globalScore += *score++;
    y++;

    VDP_drawText("500000 16x16=32 signed multiply", 2, y++);
    _ss16 = FIX_RAND_16;
    sd16 = FIX_RAND_16;
    i = 500000 / 16;
    start = getTimeAsFix32(FALSE);
#ifdef ENABLE_ASM
    bench_muls(_ss16, sd16, i);
#else
    while (i--)
    {
        sd16 = muls(_ss16, sd16);
        sd16 = muls(_ss16, sd16);
        sd16 = muls(_ss16, sd16);
        sd16 = muls(_ss16, sd16);
        sd16 = muls(_ss16, sd16);
        sd16 = muls(_ss16, sd16);
        sd16 = muls(_ss16, sd16);
        sd16 = muls(_ss16, sd16);

        sd16 = muls(_ss16, sd16);
        sd16 = muls(_ss16, sd16);
        sd16 = muls(_ss16, sd16);
        sd16 = muls(_ss16, sd16);
        sd16 = muls(_ss16, sd16);
        sd16 = muls(_ss16, sd16);
        sd16 = muls(_ss16, sd16);
        sd16 = muls(_ss16, sd16);
    }
#endif // ENABLE_ASM
    end = getTimeAsFix32(FALSE);
    *score = displayResult(500000, end - start, y++, 0);
    globalScore += *score++;
    y++;

    VDP_drawText("200000 32/16=16 unsign division", 2, y++);
    d32 = FIX_RAND_32;
    _s16 = FIX_RAND_16;
    i = 200000 / 16;
    start = getTimeAsFix32(FALSE);
#ifdef ENABLE_ASM
    bench_divu(d32, _s16, i);
#else
    while (i--)
    {
        d32 = divu(d32, _s16);
        d32 = divu(d32, _s16);
        d32 = divu(d32, _s16);
        d32 = divu(d32, _s16);
        d32 = divu(d32, _s16);
        d32 = divu(d32, _s16);
        d32 = divu(d32, _s16);
        d32 = divu(d32, _s16);

        d32 = divu(d32, _s16);
        d32 = divu(d32, _s16);
        d32 = divu(d32, _s16);
        d32 = divu(d32, _s16);
        d32 = divu(d32, _s16);
        d32 = divu(d32, _s16);
        d32 = divu(d32, _s16);
        d32 = divu(d32, _s16);
    }
#endif // ENABLE_ASM
    end = getTimeAsFix32(FALSE);
    *score = displayResult(200000, end - start, y++, 0);
    globalScore += *score++;
    y++;

    VDP_drawText("200000 32/16=16 signed division", 2, y++);
    sd32 = FIX_RAND_32;
    _ss16 = FIX_RAND_16;
    i = 200000 / 16;
    start = getTimeAsFix32(FALSE);
#ifdef ENABLE_ASM
    bench_divs(sd32, _ss16, i);
#else
    while (i--)
    {
        sd32 = divs(sd32, _ss16);
        sd32 = divs(sd32, _ss16);
        sd32 = divs(sd32, _ss16);
        sd32 = divs(sd32, _ss16);
        sd32 = divs(sd32, _ss16);
        sd32 = divs(sd32, _ss16);
        sd32 = divs(sd32, _ss16);
        sd32 = divs(sd32, _ss16);

        sd32 = divs(sd32, _ss16);
        sd32 = divs(sd32, _ss16);
        sd32 = divs(sd32, _ss16);
        sd32 = divs(sd32, _ss16);
        sd32 = divs(sd32, _ss16);
        sd32 = divs(sd32, _ss16);
        sd32 = divs(sd32, _ss16);
        sd32 = divs(sd32, _ss16);
    }
#endif // ENABLE_ASM
    end = getTimeAsFix32(FALSE);
    *score = displayResult(200000, end - start, y++, 0);
    globalScore += *score++;
    y++;

    // 3D maths tests

    // init points coordinates
    for (i = 0; i < 1024; i++)
    {
        src_3D[i].x = getrandom();
        src_3D[i].y = getrandom();
        src_3D[i].z = getrandom();
    }

    M3D_reset();
    M3D_setCamDistance(FIX16(15));

    // allocate translation and rotation structure
    M3D_setTransform(&transformation, &translation, &rotation);
    M3D_resetTransform(&transformation);
    M3D_setTranslation(&transformation, FIX16(1.5), FIX16(-2.5), FIX16(20));
    M3D_setRotation(&transformation, FIX16(1.5), FIX16(2.4), FIX16(3.2));

    VDP_drawText("3D Transform for 1024 points (x50)", 2, y++);
    i = 50;
    start = getTimeAsFix32(FALSE);
    while (i--)
    {
        M3D_transform(&transformation, src_3D, res_3D, 1024);
    }
    end = getTimeAsFix32(FALSE);
    *score = displayResult3D(1024 * 50, end - start, y++, 0);
    globalScore += *score++;
    y++;

    VDP_drawText("2D projection for 1024 points (x100)", 2, y++);
    i = 100;
    start = getTimeAsFix32(FALSE);
    while (i--)
    {
        M3D_project_s16(res_3D, res_2D, 1024);
    }
    end = getTimeAsFix32(FALSE);
    *score = displayResult3D(1024 * 100, end - start, y++, 0);
    globalScore += *score++;
    y++;

    VDP_drawText("Transform + projection (x50)", 2, y++);
    i = 50;
    start = getTimeAsFix32(FALSE);
    while (i--)
    {
        M3D_transform(&transformation, src_3D, res_3D, 1024);
        M3D_project_s16(res_3D, res_2D, 1024);
    }
    end = getTimeAsFix32(FALSE);
    *score = displayResult3D(1024 * 50, end - start, y++, 0);
    globalScore += *score++;
    y++;


    waitMs(5000);
    VDP_clearPlane(BG_A, TRUE);

    MEM_free(src_3D);
    MEM_free(res_3D);
    MEM_free(res_2D);

    return globalScore;
}


static u32 displayResult(u32 op, fix32 time, u16 y, u32 dirty)
{
    char timeStr[32];
    char speedStr[32];
    char str[64];
    fix32 speedKop;

    fix32ToStr(time, timeStr, 2);
    speedKop = intToFix32(op / 4096);
    // get speed in Kb/s
    speedKop = fix32Div(speedKop, time);
    // put it in speedStr
    fix32ToStr(speedKop * 4, speedStr, 2);

    strcpy(str, "Elapsed time = ");
    strcat(str, timeStr);
    strcat(str, "s (");
    strcat(str, speedStr);
    strcat(str, " Kop/s)");

    // display test string
    VDP_drawText(str, 3, y);

    return fix32ToInt(speedKop);
}

static u32 displayResult3D(u32 op, fix32 time, u16 y, u32 dirty)
{
    char timeStr[32];
    char speedStr[32];
    char str[64];
    fix32 speedKop;

    fix32ToStr(time, timeStr, 2);
    speedKop = intToFix32(op / 100);
    // get number of points computed per second
    speedKop = fix32Div(speedKop, time);
    // put it in speedStr
    intToStr(fix32ToRoundedInt(speedKop) * 100, speedStr, 1);

    strcpy(str, "Elapsed time = ");
    strcat(str, timeStr);
    strcat(str, "s (");
    strcat(str, speedStr);
    strcat(str, " pts/s)");

    // display test string
    VDP_drawText(str, 3, y);

    return fix32ToInt(speedKop);
}
