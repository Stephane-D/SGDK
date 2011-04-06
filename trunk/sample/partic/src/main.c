#include "genesis.h"


#define MAX_PARTIC  999
#define TAIL_LEN    2


typedef struct
{
    Vect2D_f16 pos;
    Vect2D_f16 mov;
} _particule;

_particule partics[MAX_PARTIC];
//Vect2D_u16 partics[MAX_PARTIC];

s16 numpartic;

fix16 baseposx;
fix16 baseposy;
fix16 gravity;
u8 paused;

static void handleInput();
static void joyEvent(u16 joy, u16 changed, u16 state);

static void initPartic(s16 num);
static void updatePartic(_particule *part_pos, s16 num);
static void drawPartic(_particule *part_pos, s16 num, u8 col);


int main()
{
    char col;

    JOY_setEventHandler(joyEvent);

    VDP_setScreenWidth256();
    VDP_setHInterrupt(0);
    VDP_setHilightShadow(0);

    BMP_init(0 |
//             BMP_ENABLE_WAITVSYNC |
//             BMP_ENABLE_ASYNCFLIP |
//             BMP_ENABLE_BLITONBLANK |
             BMP_ENABLE_EXTENDEDBLANK |
             BMP_ENABLE_FPSDISPLAY |
             0);

    paused = 0;
    col = 0xFF;

    /* Initialise particules */
    baseposx = intToFix16(BMP_WIDTH / 2);
    baseposy = intToFix16(BMP_HEIGHT / 2);
    gravity = FIX16(0.4);

    initPartic(100);

    /* Do main job here */
    while(1)
    {
        char str[8];

        handleInput();

        if (!paused)
        {
            // clear bitmap
            BMP_clear();

            // calculates particules physic
            updatePartic(partics, numpartic);

            // draw particules
            drawPartic(partics, numpartic, col);

            // swap buffer
            BMP_flip();
        }

        // display particul number
        intToStr(numpartic, str, 1);
        BMP_clearText(1, 3, 4);
        BMP_drawText(str, 1, 3);

        // display gravity
        fix16ToStr(gravity, str);
        BMP_clearText(1, 4, 5);
        if (paused) BMP_drawText("PAUSE", 1, 4);
        else BMP_drawText(str, 1, 4);
    }
}

static void handleInput()
{
    u16 value;

    value = JOY_readJoypad(JOY_1);

    if (value & BUTTON_A)
    {
        if (value & BUTTON_UP)
        {
            if (numpartic < MAX_PARTIC)
            {
                if (numpartic == 1) numpartic += 9;
                else numpartic += 10;
                if (numpartic > MAX_PARTIC) numpartic = MAX_PARTIC;
            }
        }
        if (value & BUTTON_DOWN)
        {
            if (numpartic > 1)
            {
                if (numpartic == MAX_PARTIC) numpartic -= 9;
                else numpartic -= 10;
                if (numpartic < 1) numpartic = 1;
            }
        }
    }
    else if (value & BUTTON_B)
    {
        if (value & BUTTON_UP)
        {
            if (gravity > FIX16(0.1)) gravity -= FIX16(0.02);
        }
        if (value & BUTTON_DOWN)
        {
            if (gravity < FIX16(1.0)) gravity += FIX16(0.02);
        }
    }
    else
    {
        if (value & BUTTON_UP)
        {
            if (baseposy < intToFix16(BMP_HEIGHT - 10)) baseposy += FIX16(2.0);
        }
        if (value & BUTTON_DOWN)
        {
            if (baseposy > intToFix16(10)) baseposy -= FIX16(2.0);
        }
        if (value & BUTTON_LEFT)
        {
            if (baseposx > intToFix16(10)) baseposx -= FIX16(1.5);
        }
        if (value & BUTTON_RIGHT)
        {
            if (baseposx < intToFix16(BMP_WIDTH - 10)) baseposx += FIX16(1.5);
        }
    }
}


static void joyEvent(u16 joy, u16 changed, u16 state)
{
    // START button state changed
    if (changed & BUTTON_START)
    {
        // START button pressed ?
        if (state & BUTTON_START)
        {
            // toggle pause
            if (paused) paused = 0;
            else paused = 1;
        }
    }

    if (changed & state & BUTTON_A)
    {

    }
    if (changed & state & BUTTON_B)
    {

    }

    // C button state changed
    if (changed & BUTTON_C)
    {
        // C button pressed ?
        if (state & BUTTON_C)
        {
            // toggle vsync
            if (BMP_getFlags() & BMP_ENABLE_WAITVSYNC) BMP_disableWaitVSync();
            else BMP_enableWaitVSync();
        }
    }
}


static void initPartic(s16 num)
{
    _particule *p;
    s16 i;

    numpartic = num;

    i = num;
    p = partics;
    while(i--)
    {
        p->pos.x = intToFix16(0);
        p->pos.y = intToFix16(0);
        p->mov.x = intToFix16(0);
        p->mov.y = intToFix16(0);
        p++;
    }
}

static void updatePartic(_particule *part, s16 num)
{
    _particule *p;
    s16 i;

    i = num;
    p = part;
    while(i--)
    {
        if ((p->pos.x >= intToFix16(BMP_WIDTH)) || (p->pos.x <= intToFix16(0)))
        {
            // re-init paticule
            p->pos.x = baseposx;
            p->pos.y = baseposy;
            p->mov.x = intToFix16(1) - (random() & (FIX16_FRAC_MASK << 1));
            p->mov.y = intToFix16(2) + (random() & (FIX16_FRAC_MASK << 3));
        }
        else if (p->pos.y <= intToFix16(0))
        {
            if (p->mov.y > -(gravity << 3))
            {
                // re-init paticule
                p->pos.x = baseposx;
                p->pos.y = baseposy;
                p->mov.x = intToFix16(1) - (random() & (FIX16_FRAC_MASK << 1));
                p->mov.y = intToFix16(2) + (random() & (FIX16_FRAC_MASK << 3));
            }
            else
            {
                // handle re-bound
                p->pos.x += p->mov.x;
                p->pos.y -= p->mov.y;
                p->mov.y = -p->mov.y;
                p->mov.y >>= 1;
            }
        }
        else
        {
            p->pos.x += p->mov.x;
            p->pos.y += p->mov.y;
            p->mov.y -= gravity;
        }

        p++;
    }
}

static void drawPartic(_particule *part, s16 num, u8 col)
{
    Vect2D_u16 part_pos[num];
    _particule *p;
    Vect2D_u16 *pos;
    s16 i, maxy;

    i = num;
    maxy = BMP_HEIGHT;
    p = part;
    pos = part_pos;
    while(i--)
    {
        pos->x = fix16ToInt(p->pos.x);
        pos->y = maxy - fix16ToInt(p->pos.y);

        p++;
        pos++;
    }

    BMP_setPixels_V2D(part_pos, col, num);
}
