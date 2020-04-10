#include <genesis.h>

#include "main.h"


#define RAND_32     0x9579BD26
#define RAND_16     0xB8E7


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
    u8 s8, d8;
    u16 s16, d16;
    u32 s32, d32;
    u8 *ps8, *pd8;
    u16 *ps16, *pd16;
    u32 *ps32, *pd32;
    u16 *score;
    u16 globalScore;

    score = scores;
    globalScore = 0;

    y = 0;
    VDP_drawText("Executing basic maths tests...", 1, y++);
    y++;

    VDP_drawText("2000000 8bit add (reg)", 2, y++);
    s8 = getZeroU8();
    d8 = getZeroU8();
    i = 2000000 / 32;
    start = getTimeAsFix32(FALSE);
#ifdef ENABLE_ASM
    bench_add8reg(i);
#else
    while(i--)
    {
        d8 += s8;
        d8 += s8;
        d8 += s8;
        d8 += s8;
        d8 += s8;
        d8 += s8;
        d8 += s8;
        d8 += s8;

        d8 += s8;
        d8 += s8;
        d8 += s8;
        d8 += s8;
        d8 += s8;
        d8 += s8;
        d8 += s8;
        d8 += s8;

        d8 += s8;
        d8 += s8;
        d8 += s8;
        d8 += s8;
        d8 += s8;
        d8 += s8;
        d8 += s8;
        d8 += s8;

        d8 += s8;
        d8 += s8;
        d8 += s8;
        d8 += s8;
        d8 += s8;
        d8 += s8;
        d8 += s8;
        d8 += s8;
    }
#endif // ENABLE_ASM
    end = getTimeAsFix32(FALSE);
    *score = displayResult(2000000, end - start, y++, d8) / 1;
    globalScore += *score++;
    y++;

    VDP_drawText("2000000 16bit add (reg)", 2, y++);
    s16 = getZeroU16();
    d16 = getZeroU16();
    i = 2000000 / 32;
    start = getTimeAsFix32(FALSE);
#ifdef ENABLE_ASM
    bench_add16reg(i);
#else
    while(i--)
    {
        d16 += s16;
        d16 += s16;
        d16 += s16;
        d16 += s16;
        d16 += s16;
        d16 += s16;
        d16 += s16;
        d16 += s16;

        d16 += s16;
        d16 += s16;
        d16 += s16;
        d16 += s16;
        d16 += s16;
        d16 += s16;
        d16 += s16;
        d16 += s16;

        d16 += s16;
        d16 += s16;
        d16 += s16;
        d16 += s16;
        d16 += s16;
        d16 += s16;
        d16 += s16;
        d16 += s16;

        d16 += s16;
        d16 += s16;
        d16 += s16;
        d16 += s16;
        d16 += s16;
        d16 += s16;
        d16 += s16;
        d16 += s16;
    }
#endif // ENABLE_ASM
    end = getTimeAsFix32(FALSE);
    *score = displayResult(2000000, end - start, y++, d16) / 1;
    globalScore += *score++;
    y++;

    VDP_drawText("2000000 32bit add (reg)", 2, y++);
    s32 = getZeroU32();
    d32 = getZeroU32();
    i = 2000000 / 32;
    start = getTimeAsFix32(FALSE);
#ifdef ENABLE_ASM
    bench_add32reg(i);
#else
    while(i--)
    {
        d32 += s32;
        d32 += s32;
        d32 += s32;
        d32 += s32;
        d32 += s32;
        d32 += s32;
        d32 += s32;
        d32 += s32;

        d32 += s32;
        d32 += s32;
        d32 += s32;
        d32 += s32;
        d32 += s32;
        d32 += s32;
        d32 += s32;
        d32 += s32;

        d32 += s32;
        d32 += s32;
        d32 += s32;
        d32 += s32;
        d32 += s32;
        d32 += s32;
        d32 += s32;
        d32 += s32;

        d32 += s32;
        d32 += s32;
        d32 += s32;
        d32 += s32;
        d32 += s32;
        d32 += s32;
        d32 += s32;
        d32 += s32;
    }
#endif // ENABLE_ASM
    end = getTimeAsFix32(FALSE);
    *score = displayResult(2000000, end - start, y++, d32) / 1;
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
    while(i--)
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
    while(i--)
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
    while(i--)
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
    u16 m16, n16;
    s16 sn16;
    u32 m32;
    s32 sm32;
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
    m32 = RAND_32;
    n16 = RAND_16;
    i = 500000 / 16;
    start = getTimeAsFix32(FALSE);
#ifdef ENABLE_ASM
    bench_mulu(m32, n16, i);
#else
    while(i--)
    {
        m32 = (u16)m32 * n16;   // force 32 bits result to use unsigned multiplications
        m32 = (u16)m32 * n16;
        m32 = (u16)m32 * n16;
        m32 = (u16)m32 * n16;
        m32 = (u16)m32 * n16;
        m32 = (u16)m32 * n16;
        m32 = (u16)m32 * n16;
        m32 = (u16)m32 * n16;

        m32 = (u16)m32 * n16;
        m32 = (u16)m32 * n16;
        m32 = (u16)m32 * n16;
        m32 = (u16)m32 * n16;
        m32 = (u16)m32 * n16;
        m32 = (u16)m32 * n16;
        m32 = (u16)m32 * n16;
        m32 = (u16)m32 * n16;
    }
#endif // ENABLE_ASM
    end = getTimeAsFix32(FALSE);
    *score = displayResult(500000, end - start, y++, m32);
    globalScore += *score++;
    y++;

    VDP_drawText("500000 16x16=32 signed multiply", 2, y++);
    sm32 = RAND_32;
    sn16 = RAND_16;
    i = 500000 / 16;
    start = getTimeAsFix32(FALSE);
#ifdef ENABLE_ASM
    bench_muls(sm32, sn16, i);
#else
    while(i--)
    {
        sm32 = (s16)sm32 * sn16;   // force 32 bits result
        sm32 = (s16)sm32 * sn16;
        sm32 = (s16)sm32 * sn16;
        sm32 = (s16)sm32 * sn16;
        sm32 = (s16)sm32 * sn16;
        sm32 = (s16)sm32 * sn16;
        sm32 = (s16)sm32 * sn16;
        sm32 = (s16)sm32 * sn16;

        sm32 = (s16)sm32 * sn16;
        sm32 = (s16)sm32 * sn16;
        sm32 = (s16)sm32 * sn16;
        sm32 = (s16)sm32 * sn16;
        sm32 = (s16)sm32 * sn16;
        sm32 = (s16)sm32 * sn16;
        sm32 = (s16)sm32 * sn16;
        sm32 = (s16)sm32 * sn16;
    }
#endif // ENABLE_ASM
    end = getTimeAsFix32(FALSE);
    *score = displayResult(500000, end - start, y++, sm32);
    globalScore += *score++;
    y++;

    VDP_drawText("200000 32/16=16 unsign division", 2, y++);
    m32 = RAND_32;
    n16 = RAND_16;
    i = 200000 / 16;
    start = getTimeAsFix32(FALSE);
#ifdef ENABLE_ASM
    bench_divu(m32, n16, i);
#else
    while(i--)
    {
        m32 = m16 = m32 / n16;
        m32 = m16 = m32 / n16;
        m32 = m16 = m32 / n16;
        m32 = m16 = m32 / n16;
        m32 = m16 = m32 / n16;
        m32 = m16 = m32 / n16;
        m32 = m16 = m32 / n16;
        m32 = m16 = m32 / n16;

        m32 = m16 = m32 / n16;
        m32 = m16 = m32 / n16;
        m32 = m16 = m32 / n16;
        m32 = m16 = m32 / n16;
        m32 = m16 = m32 / n16;
        m32 = m16 = m32 / n16;
        m32 = m16 = m32 / n16;
        m32 = m16 = m32 / n16;
    }
#endif // ENABLE_ASM
    end = getTimeAsFix32(FALSE);
    *score = displayResult(200000, end - start, y++, m16);
    globalScore += *score++;
    y++;

    VDP_drawText("200000 32/16=16 signed division", 2, y++);
    sm32 = RAND_32;
    sn16 = RAND_16;
    i = 200000 / 16;
    start = getTimeAsFix32(FALSE);
#ifdef ENABLE_ASM
    bench_divs(sm32, sn16, i);
#else
    while(i--)
    {
        sm32 = sm16 = sm32 / sn16;
        sm32 = sm16 = sm32 / sn16;
        sm32 = sm16 = sm32 / sn16;
        sm32 = sm16 = sm32 / sn16;
        sm32 = sm16 = sm32 / sn16;
        sm32 = sm16 = sm32 / sn16;
        sm32 = sm16 = sm32 / sn16;
        sm32 = sm16 = sm32 / sn16;

        sm32 = sm16 = sm32 / sn16;
        sm32 = sm16 = sm32 / sn16;
        sm32 = sm16 = sm32 / sn16;
        sm32 = sm16 = sm32 / sn16;
        sm32 = sm16 = sm32 / sn16;
        sm32 = sm16 = sm32 / sn16;
        sm32 = sm16 = sm32 / sn16;
        sm32 = sm16 = sm32 / sn16;
    }
#endif // ENABLE_ASM
    end = getTimeAsFix32(FALSE);
    *score = displayResult(200000, end - start, y++, m16);
    globalScore += *score++;
    y++;

    // 3D maths tests

    // init points coordinates
    for(i = 0; i < 1024; i++)
    {
        src_3D[i].x = random();
        src_3D[i].y = random();
        src_3D[i].z = random();
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
    while(i--)
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
    while(i--)
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
    while(i--)
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
