#include "genesis.h"


static void showPortState();

static u16 posY, posX;

static char *hex = "0123456789ABCDEF";


int main()
{
    u8 value;

    VDP_setScreenWidth320();
    VDP_setHInterrupt(0);
    VDP_setHilightShadow(0);
    VDP_setPaletteColor(15+16, 0x0222);
    VDP_setTextPalette(0);
    VDP_setPaletteColor(0, 0x0882);

    value = JOY_getPortType(PORT_1);
    switch (value)
    {
        case PORT_TYPE_MENACER:
            JOY_setSupport(PORT_1, JOY_SUPPORT_MENACER);
            break;
        case PORT_TYPE_JUSTIFIER:
            JOY_setSupport(PORT_1, JOY_SUPPORT_JUSTIFIER_BOTH);
            break;
        case PORT_TYPE_MOUSE:
            JOY_setSupport(PORT_1, JOY_SUPPORT_MOUSE);
            break;
        case PORT_TYPE_TEAMPLAYER:
            JOY_setSupport(PORT_1, JOY_SUPPORT_TEAMPLAYER);
            break;
    }

#if 0
    JOY_setSupport(PORT_2, JOY_SUPPORT_TRACKBALL);
#endif

#if 1
    JOY_setSupport(PORT_2, JOY_SUPPORT_PHASER);
#endif

#if 0
    value = JOY_getPortType(PORT_2);
    switch (value)
    {
        case PORT_TYPE_MENACER:
            JOY_setSupport(PORT_2, JOY_SUPPORT_MENACER);
            break;
        case PORT_TYPE_JUSTIFIER:
            JOY_setSupport(PORT_2, JOY_SUPPORT_JUSTIFIER_BOTH);
            break;
        case PORT_TYPE_MOUSE:
            JOY_setSupport(PORT_2, JOY_SUPPORT_MOUSE);
            break;
        case PORT_TYPE_TEAMPLAYER:
            JOY_setSupport(PORT_2, JOY_SUPPORT_TEAMPLAYER);
            break;
    }
#endif

    while(1)
    {
        SYS_doVBlankProcess();
        showPortState();
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
    u16 i, value, type;

    posY = 2;
    for(i=0; i<2; i++)
    {
        posX = 2;
        printChar(hex[i+1], 0);
        posX -= 1;
        printChar(':', 0);
        value = JOY_getPortType(i);
        printChar(hex[value & 15], 0);
        switch (value)
        {
            case PORT_TYPE_MENACER:
                VDP_drawText("Menacer   ", posX, posY);
                break;
            case PORT_TYPE_JUSTIFIER:
                VDP_drawText("Justifier ", posX, posY);
                break;
            case PORT_TYPE_MOUSE:
                VDP_drawText("Mouse     ", posX, posY);
                break;
            case PORT_TYPE_TEAMPLAYER:
                VDP_drawText("TeamPlayer", posX, posY);
                break;
            case PORT_TYPE_PAD:
                VDP_drawText("Pad       ", posX, posY);
                break;
            case PORT_TYPE_UKNOWN:
                VDP_drawText("Unknown   ", posX, posY);
                break;
            case PORT_TYPE_EA4WAYPLAY:
                VDP_drawText("EA 4-Way  ", posX, posY);
                break;
            default:
                VDP_drawText("Unknown   ", posX, posY);
                break;
        }
        posY++;
    }

    posY = 5;
    for(i=JOY_1; i<JOY_NUM; i++)
    {
        posX = 2;
        printChar(hex[i+1], 0);
        posX -= 1;
        printChar(':', 0);
        value = JOY_readJoypad(i);
        type = JOY_getJoypadType(i);
        switch (type)
        {
            case JOY_TYPE_PAD3:
                VDP_drawText("3 button ", posX, posY);
                posX += 10;
                break;
            case JOY_TYPE_PAD6:
                VDP_drawText("6 button ", posX, posY);
                posX += 10;
                break;
            case JOY_TYPE_TRACKBALL:
                VDP_drawText("trackball", posX, posY);
                posX += 10;
                break;
            case JOY_TYPE_MOUSE:
                VDP_drawText("megamouse", posX, posY);
                posX += 10;
                break;
            case JOY_TYPE_MENACER:
                VDP_drawText("menacer  ", posX, posY);
                posX += 10;
                break;
            case JOY_TYPE_JUSTIFIER:
                VDP_drawText("justifier", posX, posY);
                posX += 10;
                break;
            case JOY_TYPE_PHASER:
                VDP_drawText("phaser", posX, posY);
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
        if (type == JOY_TYPE_PAD6)
        {
            printChar('X', value & BUTTON_X);
            printChar('Y', value & BUTTON_Y);
            printChar('Z', value & BUTTON_Z);
            printChar('M', value & BUTTON_MODE);
        }
        else if ((type == JOY_TYPE_TRACKBALL) ||
                 (type == JOY_TYPE_MOUSE) ||
                 (type == JOY_TYPE_MENACER) ||
                 (type == JOY_TYPE_JUSTIFIER) ||
                 (type == JOY_TYPE_PHASER))
        {
            posY++;
            posX = 15;
            printWord(JOY_readJoypadX(i), 1);
            printWord(JOY_readJoypadY(i), 1);
        }
        posY++;
    }
}
