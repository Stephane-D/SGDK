#include "config.h"
#include "types.h"

#include "sega.h"
#include "memory.h"

#include "joy.h"
#include "base.h"


// we don't want to share them
extern u32 VBlankProcess;
extern u32 HBlankProcess;


static u16 joyState[8];
static u8 portSupport[2];

static _joyEventCallback *joyEventCB;


void JOY_init()
{
    vu8 *pb;

    pb = (u8 *) 0xa10009;
    *pb = 0x40;
    pb += 2;
    *pb = 0x40;
    pb += 2;
    *pb = 0x40;

    memset(joyState, 0, sizeof(joyState));

    portSupport[PORT_1] = JOY_SUPPORT_3BTN;
    portSupport[PORT_2] = JOY_SUPPORT_3BTN;

    joyEventCB = NULL;
}


void JOY_setEventHandler(_joyEventCallback *CB)
{
    joyEventCB = CB;
}


void JOY_setSupport(u16 port, u16 support)
{
    if (port > 1) return;

    portSupport[port] = support;
}


u16 JOY_readJoypad(u16 joy)
{
    if (joy == JOY_ALL)
    {
        u16 i;
        u16 res;

        res = 0;
        i = JOY_NUM;
        while(i--) res |= joyState[i];

        return res;
    }
    else return joyState[joy];
}


void JOY_waitPressBtn()
{
    JOY_waitPress(JOY_ALL, BUTTON_BTN);
}

void JOY_waitPress(u16 joy, u16 btn)
{
    while(1)
    {
        JOY_update();

        if (joy == JOY_ALL)
        {
            u16 i;

            i = JOY_NUM;
            while(i--)
            {
                if (joyState[i] & btn) return;
            }
        }
        else
        {
            if (joyState[joy] & btn) return;
        }
    }
}


static u16 readJoypad(u16 joy)
{
    vu8 *pb;
    u8 i, j;

    if (joy < 2)
    {
        pb = (u8 *) (0xA10003 + (joy * 2));

        // check joy high pass
        *pb = 0x40;
        asm("nop");
        asm("nop");
        i = *pb & 0x3f;

        // check joy low pass
        *pb = 0;
        asm("nop");
        asm("nop");
        j = (*pb & 0x30) << 2;

        return (~(i | j)) & 0xFF;
    }
    else if (joy < 6)       // teamplay 1
    {
        // not yet supported
        return 0;
    }
    else if (joy < 9)       // teamplay 2
    {
        // not yet supported
        return 0;
    }
    else return 0;
}


void JOY_update()
{
    u16 newstate;
    u16 change;
    u16 i;

    if (portSupport[0])
    {
        newstate = readJoypad(0);
        change = joyState[0] ^ newstate;
        joyState[0] = newstate;
        if ((joyEventCB) && (change)) joyEventCB(0, change, newstate);

        if (portSupport[0] & JOY_SUPPORT_TEAMPLAY)
        {
            for (i = 2; i < 5; i++)
            {
                newstate = readJoypad(i);
                change = joyState[i] ^ newstate;
                joyState[i] = newstate;
                if ((joyEventCB) && (change)) joyEventCB(i, change, newstate);
            }
        }
    }
    if (portSupport[1])
    {
        newstate = readJoypad(1);
        change = joyState[1] ^ newstate;
        joyState[1] = newstate;
        if ((joyEventCB) && (change)) joyEventCB(1, change, newstate);

        if (portSupport[1] & JOY_SUPPORT_TEAMPLAY)
        {
            for (i = 5; i < 8; i++)
            {
                newstate = readJoypad(i);
                change = joyState[i] ^ newstate;
                joyState[i] = newstate;
                if ((joyEventCB) && (change)) joyEventCB(i, change, newstate);
            }
        }
    }
}
