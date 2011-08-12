#include "genesis.h"
#include "meshs.h"

//#define FASTFILL
#define MAX_POINTS  8


Vect3D_f16 pts_3D[MAX_POINTS];
Vect2D_s16 pts_2D[MAX_POINTS];


extern Mat3D_f16 MatInv;
extern Mat3D_f16 Mat;


Vect3D_f16 vtab_3D[MAX_POINTS];
Vect2D_s16 vtab_2D[MAX_POINTS];

Vect3D_f16 trans;
Vect3D_f16 rot;
Vect3D_f16 rotstep;

u16 flatDrawing;


void initPointsPos(u16 num);
void updatePointsPos(u16 num);
void drawPoints(u16 num, u8 col);
void doActionJoy(u8 numjoy, u16 value);
void handleJoyEvent(u16 joy, u16 changed, u16 state);

void showDebugInfos();
void showVector(u16 posX, u16 posY, u8 col, Vect3D_f16 *vect);


int main()
{
    VDP_setScreenWidth256();
    VDP_setHInterrupt(0);
    VDP_setHilightShadow(0);

    JOY_setEventHandler(handleJoyEvent);

#ifdef FASTFILL
    BMP_FF_init(0 |
//             BMP_ENABLE_WAITVSYNC |
//             BMP_ENABLE_ASYNCFLIP |
//             BMP_ENABLE_BLITONBLANK |
                BMP_ENABLE_EXTENDEDBLANK |
                BMP_ENABLE_FPSDISPLAY |
//             BMP_ENABLE_BFCULLING |
                0);
#else
    BMP_init(0 |
//             BMP_ENABLE_WAITVSYNC |
//             BMP_ENABLE_ASYNCFLIP |
//             BMP_ENABLE_BLITONBLANK |
             BMP_ENABLE_EXTENDEDBLANK |
             BMP_ENABLE_FPSDISPLAY |
             BMP_ENABLE_BFCULLING |
             0);
#endif

    M3D_reset();
    M3D_setViewport(BMP_WIDTH, BMP_HEIGHT);
    M3D_setLightEnabled(1);
    M3D_setLightXYZ3D(FIX16(0.9), FIX16(0.9), FIX16(-0.9));

    trans.x = FIX16(0);
    trans.y = FIX16(0);
    trans.z = FIX16(8);
    rot.x = FIX16(0);
    rot.y = FIX16(0);
    rot.z = FIX16(0);
    rotstep.x = FIX16(0);
    rotstep.y = FIX16(0);
    rotstep.z = FIX16(0);

    flatDrawing = 0;

    while (1)
    {
#ifdef FASTFILL
        BMP_FF_clear();
#else
        BMP_clear();
#endif

        doActionJoy(JOY_1, JOY_readJoypad(JOY_1));
        doActionJoy(JOY_2, JOY_readJoypad(JOY_2));

        // do work here
        rot.x += rotstep.x;
        rot.y += rotstep.y;
        rot.z += rotstep.z;

        M3D_setTransMat3D(&trans);
        M3D_setRotMat3D(&rot);

        updatePointsPos(MAX_POINTS);

        drawPoints(MAX_POINTS, 0xFF);

#ifdef FASTFILL
        BMP_FF_flip();
#else
        BMP_flip();
#endif
    }
}

void updatePointsPos(u16 num)
{
    // transform 3D point
    M3D_transform3D(cube_coord, pts_3D, 8);
    // project 3D point (f16) to 2D point (s16)
    M3D_project3D_s16(pts_3D, pts_2D, 8);
}

#define draw_flat

void drawPoints(u16 num, u8 col)
{
    char str[16];

    if (flatDrawing)
    {
        Vect2D_s16 v[4];
        const Vect3D_f16 *norm;
        const u16 *poly_ind;
        u16 i;

        norm = cube_face_norm;
        poly_ind = cube_poly_ind;

        i = 6;

        while (i--)
        {
            Vect2D_s16 *pt_dst = v;
            fix16 dp;
            u8 col = 2;

            *pt_dst++ = pts_2D[*poly_ind++];
            *pt_dst++ = pts_2D[*poly_ind++];
            *pt_dst++ = pts_2D[*poly_ind++];
            *pt_dst = pts_2D[*poly_ind++];

            dp = fix16Mul(light_trans.x, norm->x) + fix16Mul(light_trans.y, norm->y) + fix16Mul(light_trans.z, norm->z);
            norm++;

            if (dp > 0) col += (dp >> (FIX16_FRAC_BITS - 2));

#ifdef FASTFILL
//            BMP_FF_drawPolygone(v, 4, i + 1);
#else
//            BMP_drawPolygone(v, 4, i + 1);
            BMP_drawPolygone(v, 4, col);
#endif
        }
    }
    else
    {
        Line l;
        const u16 *line_ind;
        u16 i;

        l.col = col;
        line_ind = cube_line_ind;

        i = 12;

        while (i--)
        {
            l.pt1 = pts_2D[*line_ind++];
            l.pt2 = pts_2D[*line_ind++];

#ifdef FASTFILL
            BMP_FF_drawLine(&l);
#else
            BMP_drawLine(&l);
#endif
        }
    }
}


void doActionJoy(u8 numjoy, u16 value)
{
    if (numjoy == JOY_1)
    {
        if (value & BUTTON_UP)
        {
            rotstep.x += intToFix16(1);
        }

        if (value & BUTTON_DOWN)
        {
            rotstep.x -= intToFix16(1);
        }

        if (value & BUTTON_LEFT)
        {
            rotstep.y += intToFix16(1);
        }

        if (value & BUTTON_RIGHT)
        {
            rotstep.y -= intToFix16(1);
        }

        if (value & BUTTON_A)
        {
            rotstep.x = intToFix16(0);
            rotstep.y = intToFix16(0);
        }

        if (value & BUTTON_B)
        {
            trans.z += intToFix16(1);
        }

        if (value & BUTTON_C)
        {
            trans.z -= intToFix16(1);
        }

        if (value & BUTTON_START)
        {

        }
    }
}

void handleJoyEvent(u16 joy, u16 changed, u16 state)
{
    if (joy == JOY_1)
    {
        if (changed & ~state & BUTTON_START)
        {
            flatDrawing = 1 - flatDrawing;
        }
    }
}
