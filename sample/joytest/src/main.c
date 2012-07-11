#include "genesis.h"


static void showPortState();

static u16 posY, posX;

static char *hex = "0123456789ABCDEF";


int main()
{
    VDP_setScreenWidth320();
    VDP_setHInterrupt(0);
    VDP_setHilightShadow(0);
    VDP_setPaletteColor(PAL1, 15, 0x0888);
    VDP_setTextPalette(0);

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
    VDP_setTextPalette(state ? 1 : 0);
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
    VDP_setTextPalette(state ? 1 : 0);
    VDP_drawText(temp, posX, posY);
    posX += 8;
}

static void showPortState()
{
    u16 i, value;

    posY = 5;
    for(i=JOY_1; i<JOY_NUM; i++)
    {
        posX = 2;
        printChar(hex[i+1], 0);
        posX -= 1;
        printChar(':', 0);
        value = JOY_readJoypad(i);
        switch (value >> JOY_TYPE_SHIFT)
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
        printChar('U', value & BUTTON_UP);
        printChar('D', value & BUTTON_DOWN);
        printChar('L', value & BUTTON_LEFT);
        printChar('R', value & BUTTON_RIGHT);
        printChar('A', value & BUTTON_A);
        printChar('B', value & BUTTON_B);
        printChar('C', value & BUTTON_C);
        printChar('S', value & BUTTON_START);
        if ((value >> JOY_TYPE_SHIFT) == JOY_TYPE_PAD6)
        {
            printChar('X', value & BUTTON_X);
            printChar('Y', value & BUTTON_Y);
            printChar('Z', value & BUTTON_Z);
            printChar('M', value & BUTTON_MODE);
        }
        else if ((value >> JOY_TYPE_SHIFT) == JOY_TYPE_MOUSE)
        {
            posY++;
            posX = 15;
            printWord(JOY_readJoypadX(i), 1);
            printWord(JOY_readJoypadY(i), 1);
        }
        posY++;
    }
}
