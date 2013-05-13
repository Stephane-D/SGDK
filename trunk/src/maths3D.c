#include "config.h"
#include "types.h"

#include "maths.h"
#include "maths3D.h"

#include "bmp.h"


Vect3D_f16 light;
Vect2D_u16 viewport;
Vect2D_f16 viewport_f16;

fix16 camDist;

u16 light_enabled;


void M3D_reset()
{
    light_enabled = 0;

    // we assume default viewport for BMP drawing
    M3D_setViewport(BMP_WIDTH, BMP_HEIGHT);

    // should be a high value as 200
    camDist = FIX16(200);

    light.x = FIX16(1);
    light.y = FIX16(0);
    light.z = FIX16(0);
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

void M3D_setCamDistance(fix16 value)
{
    camDist = FIX16(value);
}

void M3D_setLightXYZ(fix16 x, fix16 y, fix16 z)
{
    light.x = x;
    light.y = y;
    light.z = z;
}

void M3D_setLight(Vect3D_f16 *value)
{
    light.x = value->x;
    light.y = value->y;
    light.z = value->z;
}


void M3D_resetTransform(Transformation3D *t)
{
    Rotation3D *rot = t->rotation;
    Translation3D *trans = t->translation;

    rot->x = 0;
    rot->y = 0;
    rot->z = 0;
    trans->x = 0;
    trans->y = 0;
    trans->z = 0;

    t->mat.a.x = FIX16(1);
    t->mat.b.x = FIX16(0);
    t->mat.c.x = FIX16(0);

    t->mat.a.y = FIX16(0);
    t->mat.b.y = FIX16(1);
    t->mat.c.y = FIX16(0);

    t->mat.a.z = FIX16(0);
    t->mat.b.z = FIX16(0);
    t->mat.c.z = FIX16(1);

    t->matInv.a.x = FIX16(1);
    t->matInv.b.x = FIX16(0);
    t->matInv.c.x = FIX16(0);

    t->matInv.a.y = FIX16(0);
    t->matInv.b.y = FIX16(1);
    t->matInv.c.y = FIX16(0);

    t->matInv.a.z = FIX16(0);
    t->matInv.b.z = FIX16(0);
    t->matInv.c.z = FIX16(1);

    // transform camview vector (0, 0, 1)
    t->cameraInv.x = FIX16(0);
    t->cameraInv.y = FIX16(0);
    t->cameraInv.z = FIX16(1);

    t->rebuildMat = 0;

    // transform light vector (after rebuiltMat set to 0)
    if (light_enabled)
        M3D_rotateInv(t, &light, &(t->lightInv));
}

void M3D_setTransform(Transformation3D *tr, Translation3D *t, Rotation3D *r)
{
    tr->translation = t;

    if (tr->rotation != r)
    {
        tr->rotation = r;
        tr->rebuildMat = 1;
    }
}

void M3D_setTranslation(Transformation3D *t, fix16 x, fix16 y, fix16 z)
{
    Translation3D *trans = t->translation;

    trans->x = x;
    trans->y = y;
    trans->z = z;
}

void M3D_setRotation(Transformation3D *t, fix16 x, fix16 y, fix16 z)
{
    Rotation3D *rot = t->rotation;

    if ((rot->x != x) || (rot->y != y) || (rot->z != z))
    {
        rot->x = x;
        rot->y = y;
        rot->z = z;
        t->rebuildMat = 1;
    }
}


void M3D_combineTransform(Transformation3D *left, Transformation3D *right, Transformation3D *result)
{
    // rebuild matrice if needed
    if (left->rebuildMat) M3D_buildMat3DOnly(left);
    if (right->rebuildMat) M3D_buildMat3DOnly(right);

//              x y z t   x y z t
//
//          a   a b c x   k l m u   x     ak+bn+cq al+bo+cr am+bp+cs au+bv+cw+x
// tr.T =   b   d e f y . n o p v   y  =  dk+en+fq dl+eo+fr dm+ep+fs du+ev+fw+y  = 36 mul + 27 add
//          c   g h i z   q r s w   z     gk+hn+iq gl+ho+ir gm+hp+is gu+hv+iw+z
//              0 0 0 1   0 0 0 1

    const Mat3D_f16 *mat1 = &(left->mat);
    const Translation3D *t1 = left->translation;
    const Mat3D_f16 *mat2 = &(right->mat);
    const Translation3D *t2 = right->translation;

    Mat3D_f16 *matRes = &(result->mat);
    Translation3D *tRes = result->translation;

    // compute matrix product (36 multiplications + 27 additions... outch !!!)
    matRes->a.x = ((mat1->a.x * mat2->a.x) + (mat1->a.y * mat2->b.x) + (mat1->a.z * mat2->c.x)) >> FIX16_FRAC_BITS;
    matRes->a.y = ((mat1->a.x * mat2->a.y) + (mat1->a.y * mat2->b.y) + (mat1->a.z * mat2->c.y)) >> FIX16_FRAC_BITS;
    matRes->a.z = ((mat1->a.x * mat2->a.z) + (mat1->a.y * mat2->b.z) + (mat1->a.z * mat2->c.z)) >> FIX16_FRAC_BITS;
    tRes->x = (((mat1->a.x * t2->x) + (mat1->a.y * t2->y) + (mat1->a.z * t2->z)) >> FIX16_FRAC_BITS) + t1->x;

    matRes->b.x = ((mat1->b.x * mat2->a.x) + (mat1->b.y * mat2->b.x) + (mat1->b.z * mat2->c.x)) >> FIX16_FRAC_BITS;
    matRes->b.y = ((mat1->b.x * mat2->a.y) + (mat1->b.y * mat2->b.y) + (mat1->b.z * mat2->c.y)) >> FIX16_FRAC_BITS;
    matRes->b.z = ((mat1->b.x * mat2->a.z) + (mat1->b.y * mat2->b.z) + (mat1->b.z * mat2->c.z)) >> FIX16_FRAC_BITS;
    tRes->y = (((mat1->b.x * t2->x) + (mat1->b.y * t2->y) + (mat1->b.z * t2->z)) >> FIX16_FRAC_BITS) + t1->y;

    matRes->c.x = ((mat1->c.x * mat2->a.x) + (mat1->c.y * mat2->b.x) + (mat1->c.z * mat2->c.x)) >> FIX16_FRAC_BITS;
    matRes->c.y = ((mat1->c.x * mat2->a.y) + (mat1->c.y * mat2->b.y) + (mat1->c.z * mat2->c.y)) >> FIX16_FRAC_BITS;
    matRes->c.z = ((mat1->c.x * mat2->a.z) + (mat1->c.y * mat2->b.z) + (mat1->c.z * mat2->c.z)) >> FIX16_FRAC_BITS;
    tRes->z = (((mat1->c.x * t2->x) + (mat1->c.y * t2->y) + (mat1->c.z * t2->z)) >> FIX16_FRAC_BITS) + t1->z;

    // rebuild matrix cached infos
    M3D_buildMat3DExtras(result);
}

void M3D_combineTranslationLeft(Translation3D *left, Transformation3D *right, Transformation3D *result)
{
    // rebuild matrice if needed
    if (right->rebuildMat) M3D_buildMat3D(right);

    // copy unmodified value from right directly
    if (result != right)
    {
        result->mat = right->mat;
        result->matInv = right->matInv;
        result->cameraInv = right->cameraInv;
        result->lightInv = right->lightInv;
    }

//              x y z t   x y z t
//
//          a   1 0 0 x   k l m u   x     k l m u+x
// tr.T =   b   0 1 0 y . n o p v   y  =  n o p v+y
//          c   0 0 1 z   q r s w   z     q r s w+z
//              0 0 0 1   0 0 0 1

    const Translation3D *t1 = left;
    const Translation3D *t2 = right->translation;
    Translation3D *tRes = result->translation;

    // only need to modify translation object (3 additions... ok :) )
    tRes->x = t1->x + t2->x;
    tRes->y = t1->y + t2->y;
    tRes->z = t1->z + t2->z;
}

void M3D_combineTranslationRight(Transformation3D *left, Translation3D *right, Transformation3D *result)
{
    // rebuild matrice if needed
    if (left->rebuildMat) M3D_buildMat3D(left);

    // copy unmodified value from left directly
    if (result != left)
    {
        result->mat = left->mat;
        result->matInv = left->matInv;
        result->cameraInv = left->cameraInv;
        result->lightInv = left->lightInv;
    }

//              x y z t   x y z t
//
//          a   k l m u   1 0 0 x   x     k l m kx+ly+mz+u
// tr.T =   b   n o p v . 0 1 0 y   y  =  n o p nx+oy+pz+v
//          c   q r s w   0 0 1 z   z     q r s qx+ry+sz+w
//              0 0 0 1   0 0 0 1

    const Mat3D_f16 *mat1 = &(left->mat);
    const Translation3D *t1 = left->translation;
    const Translation3D *t2 = right;
    Translation3D *tRes = result->translation;

    // only need to modify translation object (9 multiplications + 6 additions... ok :) )
    tRes->x = (((mat1->a.x * t2->x) + (mat1->a.y * t2->y) + (mat1->a.z * t2->z)) >> FIX16_FRAC_BITS) + t1->x;
    tRes->y = (((mat1->b.x * t2->x) + (mat1->b.y * t2->y) + (mat1->b.z * t2->z)) >> FIX16_FRAC_BITS) + t1->y;
    tRes->z = (((mat1->c.x * t2->x) + (mat1->c.y * t2->y) + (mat1->c.z * t2->z)) >> FIX16_FRAC_BITS) + t1->z;
}


void M3D_buildMat3D(Transformation3D *t)
{
    M3D_buildMat3DOnly(t);
    M3D_buildMat3DExtras(t);
}

void M3D_buildMat3DOnly(Transformation3D *t)
{
    fix16 sx, sy, sz;
    fix16 cx, cy, cz;
    fix16 sxsy, cxsy;

    Rotation3D *rot = t->rotation;

    cx = rot->x;
    cy = rot->y;
    cz = rot->z;

    sx = sinFix16(cx);
    sy = sinFix16(cy);
    sz = sinFix16(cz);
    cx = cosFix16(cx);
    cy = cosFix16(cy);
    cz = cosFix16(cz);

    t->mat.a.x = fix16Mul(cy, cz);
    t->mat.b.x = -fix16Mul(cy, sz);
    t->mat.c.x = sy;

    sxsy = fix16Mul(sx, sy);
    t->mat.a.y = ((sxsy * cz) + (cx * sz)) >> FIX16_FRAC_BITS;
    t->mat.b.y = ((cx * cz) - (sxsy * sz)) >> FIX16_FRAC_BITS;
    t->mat.c.y = -fix16Mul(sx, cy);

    cxsy = fix16Mul(cx, sy);
    t->mat.a.z = ((sx * sz) - (cxsy * cz)) >> FIX16_FRAC_BITS;
    t->mat.b.z = ((cxsy * sz) + (sx * cz)) >> FIX16_FRAC_BITS;
    t->mat.c.z = fix16Mul(cx, cy);

    // matrix built
    t->rebuildMat = 0;
}

void M3D_buildMat3DExtras(Transformation3D *t)
{
    t->matInv.a.x = t->mat.a.x;
    t->matInv.b.x = t->mat.a.y;
    t->matInv.c.x = t->mat.a.z;

    t->matInv.a.y = t->mat.b.x;
    t->matInv.b.y = t->mat.b.y;
    t->matInv.c.y = t->mat.b.z;

    t->matInv.a.z = t->mat.c.x;
    t->matInv.b.z = t->mat.c.y;
    t->matInv.c.z = t->mat.c.z;

    // transform camview vector (0, 0, 1)
    t->cameraInv.x = t->matInv.a.z;
    t->cameraInv.y = t->matInv.b.z;
    t->cameraInv.z = t->matInv.c.z;

    // transform light vector (after rebuiltMat set to 0)
    if (light_enabled)
        M3D_rotateInv(t, &light, &(t->lightInv));
}


void M3D_translate(Transformation3D *t, Vect3D_f16 *vertices, u16 numv)
{
    fix16 *d;
    u16 i;
    Translation3D *trans = t->translation;

    const fix16 tx = trans->x;
    const fix16 ty = trans->y;
    const fix16 tz = trans->z;

    d = (fix16*) vertices;
    i = numv;

    while (i--)
    {
        *d++ += tx;
        *d++ += ty;
        *d++ += tz;
    }
}

void M3D_rotate(Transformation3D *t, const Vect3D_f16 *src, Vect3D_f16 *dest, u16 numv)
{
    const fix16 *s;
    fix16 *d;
    u16 i;

    if (t->rebuildMat) M3D_buildMat3D(t);

    s = (fix16*) src;
    d = (fix16*) dest;
    i = numv;

    while (i--)
    {
        const fix16 sx = *s++;
        const fix16 sy = *s++;
        const fix16 sz = *s++;

        *d++ = ((sx * t->mat.a.x) + (sy * t->mat.a.y) + (sz * t->mat.a.z)) >> FIX16_FRAC_BITS;
        *d++ = ((sx * t->mat.b.x) + (sy * t->mat.b.y) + (sz * t->mat.b.z)) >> FIX16_FRAC_BITS;
        *d++ = ((sx * t->mat.c.x) + (sy * t->mat.c.y) + (sz * t->mat.c.z)) >> FIX16_FRAC_BITS;
    }
}

void M3D_rotateInv(Transformation3D *t, const Vect3D_f16 *src, Vect3D_f16 *dest)
{
    if (t->rebuildMat) M3D_buildMat3D(t);

    const fix16 sx = src->x;
    const fix16 sy = src->y;
    const fix16 sz = src->z;

    dest->x = ((sx * t->matInv.a.x) + (sy * t->matInv.a.y) + (sz * t->matInv.a.z)) >> FIX16_FRAC_BITS;
    dest->y = ((sx * t->matInv.b.x) + (sy * t->matInv.b.y) + (sz * t->matInv.b.z)) >> FIX16_FRAC_BITS;
    dest->z = ((sx * t->matInv.c.x) + (sy * t->matInv.c.y) + (sz * t->matInv.c.z)) >> FIX16_FRAC_BITS;
}

void M3D_transform_old(Transformation3D *t, const Vect3D_f16 *src, Vect3D_f16 *dest, u16 numv)
{
    fix16 *s;
    fix16 *d;
    u16 i;

    if (t->rebuildMat) M3D_buildMat3D(t);

    Translation3D *trans = t->translation;

    const fix16 tx = trans->x;
    const fix16 ty = trans->y;
    const fix16 tz = trans->z;

    s = (fix16*) src;
    d = (fix16*) dest;
    i = numv;

    while (i--)
    {
        const fix16 sx = *s++;
        const fix16 sy = *s++;
        const fix16 sz = *s++;

        *d++ = fix16Mul(sx, t->mat.a.x) + fix16Mul(sy, t->mat.a.y) + fix16Mul(sz, t->mat.a.z) + tx;
        *d++ = fix16Mul(sx, t->mat.b.x) + fix16Mul(sy, t->mat.b.y) + fix16Mul(sz, t->mat.b.z) + ty;
        *d++ = fix16Mul(sx, t->mat.c.x) + fix16Mul(sy, t->mat.c.y) + fix16Mul(sz, t->mat.c.z) + tz;
    }
}

void M3D_project_f16_old(const Vect3D_f16 *src, Vect2D_f16 *dest, u16 numv)
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

void M3D_project_s16_old(const Vect3D_f16 *src, Vect2D_s16 *dest, u16 numv)
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
