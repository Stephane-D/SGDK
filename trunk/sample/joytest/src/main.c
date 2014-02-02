#include "genesis.h"


static void showPortState();

static u16 posY, posX;

static char *hex = "0123456789ABCDEF";


int main()
{
    VDP_setScreenWidth320();
    VDP_setHInterrupt(0);
    VDP_setHilightShadow(0);
    VDP_setPalette(PAL0, font_lib.palette);
    VDP_setPaletteColor((PAL1 * 16) + 1, 0x0888);
    VDP_setTextPalette(PAL0);

    while(1)
    {
        showPortState();
        VDP_waitVSync();
    }
}


static void printChar(char c, u16 state)
{
    char temp[2];
    temp[0] = c;
    temp[1] = 0;
    VDP_setTextPalette(state ? PAL1 : PAL0);
    VDP_drawText(temp, posX, posY);
    posX += 2;
}

static void printWord(u16 val, u16 state)
{
    char temp[8];
    temp[0] = '0';
    temp[1] = 'x';
    temp[2] = hex[val >> 12];
    temp[3] = hex[(val >> 8) & 15];
    temp[4] = hex[(val >> 4) & 15];
    temp[5] = hex[val & 15];
    temp[6] = 0;
    VDP_setTextPalette(state ? PAL1 : PAL0);
    VDP_drawText(temp, posX, posY);
    posX += 8;
}

static void showPortState()
{
    u16 i, typ, state;

    posY = 5;
    for(i=JOY_1; i<JOY_NUM; i++)
    {
        posX = 2;
        printChar(hex[i+1], 0);
        posX -= 1;
        printChar(':', 0);
        typ = JOY_getJoypadType(i);
        state = JOY_readJoypad(i);
        switch (typ)
        {
            case JOY_TYPE_PAD3:
                VDP_drawText("3 button ", posX, posY);
                posX += 10;
                break;
            case JOY_TYPE_PAD6:
                VDP_drawText("6 button ", posX, posY);
                posX += 10;
                break;
            case JOY_TYPE_MOUSE:
                VDP_drawText("megamouse", posX, posY);
                posX += 10;
                break;
            default:
                VDP_drawText("unknown  ", posX, posY);
                posX += 10;
                break;
        }
        printChar('U', state & BUTTON_UP);
        printChar('D', state & BUTTON_DOWN);
        printChar('L', state & BUTTON_LEFT);
        printChar('R', state & BUTTON_RIGHT);
        printChar('A', state & BUTTON_A);
        printChar('B', state & BUTTON_B);
        printChar('C', state & BUTTON_C);
        printChar('S', state & BUTTON_START);
        if (typ == JOY_TYPE_PAD6)
        {
            printChar('X', state & BUTTON_X);
            printChar('Y', state & BUTTON_Y);
            printChar('Z', state & BUTTON_Z);
            printChar('M', state & BUTTON_MODE);
        }
        else if (typ == JOY_TYPE_MOUSE)
        {
            posY++;
            posX = 15;
            printWord(JOY_readJoypadX(i), 1);
            printWord(JOY_readJoypadY(i), 1);
        }
        posY++;
    }
}
