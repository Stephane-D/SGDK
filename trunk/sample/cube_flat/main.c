#include "genesis.h"
#include "meshs.h"


#define MAX_POINTS  8


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

static void VBlank_Function();
static void HBlank_Function();

void showDebugInfos();
void showVector(u16 posX, u16 posY, u8 col, Vect3D_f16 *vect);


int main()
{
    VDP_setScreenWidth256();
    VDP_setHInterrupt(0);
    VDP_setHilightShadow(0);

    JOY_setEventHandler(handleJoyEvent);

    setVBlankCallback(VBlank_Function);
    setHBlankCallback(HBlank_Function);

    BMP_init(0 |
//             BMP_ENABLE_WAITVSYNC |
//             BMP_ENABLE_ASYNCFLIP |
//             BMP_ENABLE_BLITONBLANK |
             BMP_ENABLE_EXTENDEDBLANK |
             BMP_ENABLE_FPSDISPLAY |
             BMP_ENABLE_BFCULLING |
             0);

    reset3D();

    trans.x = intToFix16(0);
    trans.y = intToFix16(0);
    trans.z = intToFix16(30);

    rot.x = intToFix16(0);
    rot.y = intToFix16(0);
    rot.z = intToFix16(0);
    rotstep.x = intToFix16(0);
    rotstep.y = intToFix16(0);
    rotstep.z = intToFix16(0);

    flatDrawing = 1;

    while (1)
    {
        BMP_clear();

        doActionJoy(JOY_1, JOY_readJoypad(JOY_1));
        doActionJoy(JOY_2, JOY_readJoypad(JOY_2));

        // do work here
        rot.x += rotstep.x;
        rot.y += rotstep.y;
        rot.z += rotstep.z;

        setTransMat3D(&trans);
        setRotMat3D(&rot);

        updatePointsPos(MAX_POINTS);

        drawPoints(MAX_POINTS, 0xFF);

        BMP_flip();
    }
}

void updatePointsPos(u16 num)
{
    // transform 3D point
    transform3D(cube_coord, pts_3D, 8);
    // project 3D point (f16) to 2D point (s16)
    project3D_s16(pts_3D, pts_2D, 8, BMP_WIDTH, BMP_HEIGHT);
}

#define draw_flat

void drawPoints(u16 num, u8 col)
{
    Vect2D_s16 v[4];
    Line l;
    u16 i;

    if (flatDrawing)
    {
        i = 6;
        while (i--)
        {

            v[0].x = pts_2D[cube_poly_ind[i][0]].x;
            v[0].y = pts_2D[cube_poly_ind[i][0]].y;
            v[1].x = pts_2D[cube_poly_ind[i][1]].x;
            v[1].y = pts_2D[cube_poly_ind[i][1]].y;
            v[2].x = pts_2D[cube_poly_ind[i][2]].x;
            v[2].y = pts_2D[cube_poly_ind[i][2]].y;
            v[3].x = pts_2D[cube_poly_ind[i][3]].x;
            v[3].y = pts_2D[cube_poly_ind[i][3]].y;

            BMP_drawPolygone(v, 4, i + 1);
        }
    }
    else
    {
        i = 12;
        while (i--)
        {
            l.pt1.x = pts_2D[cube_line_ind[i][0]].x;
            l.pt1.y = pts_2D[cube_line_ind[i][0]].y;
            l.pt2.x = pts_2D[cube_line_ind[i][1]].x;
            l.pt2.y = pts_2D[cube_line_ind[i][1]].y;
            l.col = col;

            BMP_drawLine(&l);
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

static void VBlank_Function()
{

}

static void HBlank_Function()
{

}
