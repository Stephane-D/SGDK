#include "config.h"
#include "types.h"

#include "maths.h"
#include "maths3D.h"

#include "bmp.h"


#define MATOBJ      1
#define MATVIEW     2
#define MATPROJ     4
#define MATTRANS    8


static Vect3D_f16 light;

Vect3D_f16 light_trans;
//static Vect3D_f16 camview_trans;

// tranformation parameters
static fix16 Tx;
static fix16 Ty;
static fix16 Tz;
static fix16 Rx;
static fix16 Ry;
static fix16 Rz;

static Mat3D_f16 mat;
static Mat3D_f16 matInv;

Vect2D_u16 viewport;
Vect2D_f16 viewport_f16;

fix16 camDist;

static u16 rebuildMat;
static u16 light_enabled;


void M3D_reset()
{
    light_enabled = 0;

    // we assume default viewport for BMP drawing
    M3D_setViewport(BMP_WIDTH, BMP_HEIGHT);

    camDist = FIX16(256);

    light.x = FIX16(1);
    light.y = FIX16(0);
    light.z = FIX16(0);

    M3D_resetMat3D();
}


void M3D_setLightEnabled(u16 enabled)
{
    light_enabled = enabled;
}

u16 M3D_getLightEnabled()
{
    return light_enabled;
}


void M3D_setViewport(u16 w, u16 h)
{
    viewport.x = w;
    viewport.y = h;
    viewport_f16.x = intToFix16(w);
    viewport_f16.y = intToFix16(h);
}

void M3D_setCamDist3D(fix16 value)
{
    camDist = FIX16(value);
}

void M3D_setLightXYZ3D(fix16 x, fix16 y, fix16 z)
{
    light.x = x;
    light.y = y;
    light.z = z;
}

void M3D_setLight3D(Vect3D_f16 *value)
{
    light.x = value->x;
    light.y = value->y;
    light.z = value->z;
}


void M3D_resetMat3D()
{
    Tx = 0;
    Ty = 0;
    Tz = 0;
    Rx = 0;
    Ry = 0;
    Rz = 0;

    rebuildMat = 1;
}


void M3D_setTXMat3D(fix16 tx)
{
    Tx = tx;

    rebuildMat = 1;
}

void M3D_setTYMat3D(fix16 ty)
{
    Ty = ty;

    rebuildMat = 1;
}

void M3D_setTZMat3D(fix16 tz)
{
    Tz = tz;

    rebuildMat = 1;
}

void M3D_setTXYZMat3D(fix16 tx, fix16 ty, fix16 tz)
{
    Tx = tx;
    Ty = ty;
    Tz = tz;

    rebuildMat = 1;
}

void M3D_setTransMat3D(Vect3D_f16 *t)
{
    Tx = t->x;
    Ty = t->y;
    Tz = t->z;

    rebuildMat = 1;
}


void M3D_setRXMat3D(fix16 rx)
{
    Rx = rx;

    rebuildMat = 1;
}

void M3D_setRYMat3D(fix16 ry)
{
    Ry = ry;

    rebuildMat = 1;
}

void M3D_setRZMat3D(fix16 rz)
{
    Rz = rz;

    rebuildMat = 1;
}

void M3D_setRXYZMat3D(fix16 rx, fix16 ry, fix16 rz)
{
    Rx = rx;
    Ry = ry;
    Rz = rz;

    rebuildMat = 1;
}

void M3D_setRotMat3D(Vect3D_f16 *rot)
{
    Rx = rot->x;
    Ry = rot->y;
    Rz = rot->z;

    rebuildMat = 1;
}


static void buildMat3D()
{
    fix16 sx, sy, sz;
    fix16 cx, cy, cz;
    fix16 sxsy, cxsy;

    sx = sinFix16(fix16ToInt(Rx));
    sy = sinFix16(fix16ToInt(Ry));
    sz = sinFix16(fix16ToInt(Rz));
    cx = cosFix16(fix16ToInt(Rx));
    cy = cosFix16(fix16ToInt(Ry));
    cz = cosFix16(fix16ToInt(Rz));

    sxsy = fix16Mul(sx, sy);
    cxsy = fix16Mul(cx, sy);

    mat.a.x = fix16Mul(cy, cz);
    mat.b.x = -fix16Mul(cy, sz);
    mat.c.x = sy;

    mat.a.y = fix16Mul(sxsy, cz) + fix16Mul(cx, sz);
    mat.b.y = fix16Mul(cx, cz) - fix16Mul(sxsy, sz);
    mat.c.y = -fix16Mul(sx, cy);

    mat.a.z = fix16Mul(sx, sz) - fix16Mul(cxsy, cz);
    mat.b.z = fix16Mul(cxsy, sz) + fix16Mul(sx, cz);
    mat.c.z = fix16Mul(cx, cy);

    matInv.a.x = mat.a.x;
    matInv.b.x = mat.a.y;
    matInv.c.x = mat.a.z;

    matInv.a.y = mat.b.x;
    matInv.b.y = mat.b.y;
    matInv.c.y = mat.b.z;

    matInv.a.z = mat.c.x;
    matInv.b.z = mat.c.y;
    matInv.c.z = mat.c.z;

    rebuildMat = 0;
}


void M3D_transform3D(const Vect3D_f16 *src, Vect3D_f16 *dest, u16 numv)
{
    const Vect3D_f16 *s;
    Vect3D_f16 *d;
    u16 i;

    if (rebuildMat) buildMat3D();

    s = src;
    d = dest;
    i = numv;

    while (i--)
    {
        d->x = fix16Mul(s->x, mat.a.x) + fix16Mul(s->y, mat.a.y) + fix16Mul(s->z, mat.a.z) + Tx;
        d->y = fix16Mul(s->x, mat.b.x) + fix16Mul(s->y, mat.b.y) + fix16Mul(s->z, mat.b.z) + Ty;
        d->z = fix16Mul(s->x, mat.c.x) + fix16Mul(s->y, mat.c.y) + fix16Mul(s->z, mat.c.z) + Tz;

        s++;
        d++;
    }

    // transform light vector
    if (light_enabled)
    {
        light_trans.x = fix16Mul(light.x, matInv.a.x) + fix16Mul(light.y, matInv.a.y) + fix16Mul(light.z, matInv.a.z);
        light_trans.y = fix16Mul(light.x, matInv.b.x) + fix16Mul(light.y, matInv.b.y) + fix16Mul(light.z, matInv.b.z);
        light_trans.z = fix16Mul(light.x, matInv.c.x) + fix16Mul(light.y, matInv.c.y) + fix16Mul(light.z, matInv.c.z);
    }

    // transform camview vector (0, 0, 1)
//    camview_trans.x = fix16Mul(FIX16(1), matInv.a.z);
//    camview_trans.y = fix16Mul(FIX16(1), matInv.b.z);
//    camview_trans.z = fix16Mul(FIX16(1), matInv.c.z);
}

void M3D_project3D_f16_old(const Vect3D_f16 *src, Vect2D_f16 *dest, u16 numv)
{
    const Vect3D_f16 *s;
    Vect2D_f16 *d;
    fix16 zi;
    fix16 wi, hi;
    u16 i;

    wi = viewport_f16.x >> 1;
    hi = viewport_f16.y >> 1;
    s = src;
    d = dest;
    i = numv;

    while (i--)
    {
        if ((zi = s->z))
        {
            zi = fix16Div(camDist, zi);
            d->x = wi + fix16Mul(s->x >> 1, zi);
            d->y = hi - fix16Mul(s->y, zi);
        }
        else
        {
            d->x = FIX16(-1);
            d->y = FIX16(-1);
        }

        s++;
        d++;
    }
}

void M3D_project3D_s16_old(const Vect3D_f16 *src, Vect2D_s16 *dest, u16 numv)
{
    const Vect3D_f16 *s;
    Vect2D_s16 *d;
    fix16 zi;
    u16 wi, hi;
    u16 i;

    wi = viewport.x >> 1;
    hi = viewport.y >> 1;
    s = src;
    d = dest;
    i = numv;

    while (i--)
    {
        if ((zi = s->z))
        {
            zi = fix16Div(camDist, zi);
            d->x = wi + fix16ToInt(fix16Mul(s->x >> 1, zi));
            d->y = hi - fix16ToInt(fix16Mul(s->y, zi));
        }
        else
        {
            d->x = -1;
            d->y = -1;
        }

        s++;
        d++;
    }
}
