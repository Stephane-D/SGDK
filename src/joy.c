#include "config.h"
#include "types.h"

#include "sega.h"
#include "memory.h"

#include "joy.h"
#include "sys.h"
#include "vdp.h"


static u16 joyState[8];
static s16 joyAxisX[8];
static s16 joyAxisY[8];

static u8 portSupport[2];
static u8 portType[2];

static u8 retry;
static u8 phase;

static _joyEventCallback *joyEventCB;


void JOY_init()
{
    vu8 *pb;
    u8  a, id;
    u16 i;
    u16 saveint;

    joyEventCB = NULL;

    // disable ints
    saveint = SYS_getInterruptMaskLevel();
    SYS_setInterruptMaskLevel(7);

    for (i=JOY_1; i<JOY_NUM; i++)
    {
        joyState[i] = 0;
        joyAxisX[i] = 0;
        joyAxisY[i] = 0;
    }

    /*
     * Initialize ports for peripheral interface protocol - default to
     * TH Control Method for pads
     */

    /* set the port bits direction */
    pb = (vu8 *)0xa10009;
    *pb = 0x40;
    pb += 2;
    *pb = 0x40;
    pb += 2;
    *pb = 0x40;
    /* set the port bits value */
    pb = (vu8 *)0xa10003;
    *pb = 0x40;
    pb += 2;
    *pb = 0x40;
    pb += 2;
    *pb = 0x40;

    /* get ID port 1 */
    pb = (vu8 *)0xa10003;
    a = *pb;
    *pb = 0x00;
    id = (a & 8) | (a & 4) ? 8 : 0;
    id |= (a & 2) | (a & 1) ? 4 : 0;
    a = *pb;
    *pb = 0x40;
    id |= (a & 8) | (a & 4) ? 2 : 0;
    id |= (a & 2) | (a & 1) ? 1 : 0;
    portType[PORT_1] = id;

    /* get ID port 2 */
    pb = (vu8 *)0xa10005;
    a = *pb;
    *pb = 0x00;
    id = (a & 8) | (a & 4) ? 8 : 0;
    id |= (a & 2) | (a & 1) ? 4 : 0;
    a = *pb;
    *pb = 0x40;
    id |= (a & 8) | (a & 4) ? 2 : 0;
    id |= (a & 2) | (a & 1) ? 1 : 0;
    portType[PORT_2] = id;

    /* now set the port support */

    switch (portType[PORT_1])
    {
        case PORT_TYPE_PAD:
            portSupport[PORT_1] = JOY_SUPPORT_6BTN;
            for (i=JOY_3; i<JOY_6; i++)
                joyState[i] = JOY_TYPE_UNKNOWN << 12;
            break;
        case PORT_TYPE_MOUSE:
            /* init port for Three Line Handshake Method */
            pb = (vu8 *)0xa10009;
            *pb = 0x60;
            pb = (vu8 *)0xa10003;
            *pb = 0x60;
            portSupport[PORT_1] = JOY_SUPPORT_MOUSE;
            for (i=JOY_3; i<JOY_6; i++)
                joyState[i] = JOY_TYPE_UNKNOWN << 12;
            break;
        case PORT_TYPE_TEAMPLAY:
            /* init port for Three Line Handshake Method */
            pb = (vu8 *)0xa10009;
            *pb = 0x60;
            pb = (vu8 *)0xa10003;
            *pb = 0x60;
            portSupport[PORT_1] = JOY_SUPPORT_TEAMPLAY;
            break;
        case PORT_TYPE_UKNOWN:
            portSupport[PORT_1] = JOY_SUPPORT_OFF;
            joyState[JOY_1] = JOY_TYPE_UNKNOWN << 12;
            for (i=JOY_3; i<JOY_6; i++)
                joyState[i] = JOY_TYPE_UNKNOWN << 12;
            break;
    }

    switch (portType[PORT_2])
    {
        case PORT_TYPE_PAD:
            portSupport[PORT_2] = JOY_SUPPORT_6BTN;
            for (i=JOY_6; i<JOY_NUM; i++)
                joyState[i] = JOY_TYPE_UNKNOWN << 12;
            break;
        case PORT_TYPE_MOUSE:
            /* init port for Three Line Handshake Method */
            pb = (vu8 *)0xa1000b;
            *pb = 0x60;
            pb = (vu8 *)0xa10005;
            *pb = 0x60;
            portSupport[PORT_2] = JOY_SUPPORT_MOUSE;
            for (i=JOY_6; i<JOY_NUM; i++)
                joyState[i] = JOY_TYPE_UNKNOWN << 12;
            break;
        case PORT_TYPE_TEAMPLAY:
            /* init port for Three Line Handshake Method */
            pb = (vu8 *)0xa1000b;
            *pb = 0x60;
            pb = (vu8 *)0xa10005;
            *pb = 0x60;
            portSupport[PORT_2] = JOY_SUPPORT_TEAMPLAY;
            break;
        case PORT_TYPE_UKNOWN:
            portSupport[PORT_2] = JOY_SUPPORT_OFF;
            joyState[JOY_2] = JOY_TYPE_UNKNOWN << 12;
            for (i=JOY_6; i<JOY_NUM; i++)
                joyState[i] = JOY_TYPE_UNKNOWN << 12;
            break;
    }

    // restore ints
    SYS_setInterruptMaskLevel(saveint);

    /* wait a few vblanks for JOY_update() to get valid data */
    VDP_waitVSync();
    VDP_waitVSync();
    VDP_waitVSync();

    /* now update pads to reflect true type (3 or 6 button) */
    if (portType[PORT_1] == PORT_TYPE_PAD)
        if ((joyState[JOY_1] >> 12) == JOY_TYPE_PAD3)
            portSupport[PORT_1] = JOY_SUPPORT_3BTN;
    if (portType[PORT_2] == PORT_TYPE_PAD)
        if ((joyState[JOY_2] >> 12) == JOY_TYPE_PAD3)
            portSupport[PORT_2] = JOY_SUPPORT_3BTN;
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
        while(i--) res |= joyState[i] &  BUTTON_ALL;

        return res;
    }
    else if (joy < JOY_NUM)
        return joyState[joy];
    else
        return JOY_TYPE_UNKNOWN;
}


u16 JOY_readJoypadX(u16 joy)
{
    if (joy < JOY_NUM)
    {
        return joyAxisX[joy];
    }
    else
        return 0;
}


u16 JOY_readJoypadY(u16 joy)
{
    if (joy < JOY_NUM)
    {
        return joyAxisY[joy];
    }
    else
        return 0;
}


void JOY_waitPressBtn()
{
    JOY_waitPress(JOY_ALL, BUTTON_BTN);
}


void JOY_waitPress(u16 joy, u16 btn)
{
    while(1)
    {
        VDP_waitVSync(); // let JOY_update() occur in vblank

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


static u16 TH_CONTROL_PHASE(vu8 *pb)
{
    u16 val;

    *pb = 0x00; /* TH (select) low */
    asm("nop");
    asm("nop");
    val = *pb;

    *pb = 0x40; /* TH (select) high */
    val <<= 8;
    val |= *pb;

    return val;
}


static u16 read3Btn(u16 joy)
{
    vu8 *pb;
    u16 val;

    pb = (vu8 *)0xa10003 + joy*2;

    val = TH_CONTROL_PHASE(pb);                   /* - 0 s a 0 0 d u - 1 c b r l d u */
    val = ((val & 0x3000) >> 6) | (val & 0x003F); /* 0 0 0 0 0 0 0 0 s a c b r l d u */
    val ^= 0x00FF;                                /* 0 0 0 0 0 0 0 0 S A C B R L D U */

    return val;
}


static u16 read6Btn(u16 joy)
{
    vu8 *pb;
    u16 val, v1, v2;

    pb = (vu8 *)0xa10003 + joy*2;

    v1 = TH_CONTROL_PHASE(pb);                    /* - 0 s a 0 0 d u - 1 c b r l d u */
    val = TH_CONTROL_PHASE(pb);                   /* - 0 s a 0 0 d u - 1 c b r l d u */
    v2 = TH_CONTROL_PHASE(pb);                    /* - 0 s a 0 0 0 0 - 1 c b m x y z */
    val = TH_CONTROL_PHASE(pb);                   /* - 0 s a 1 1 1 1 - 1 c b r l d u */

    if ((val & 0x0F00) != 0x0F00) v2 = 0x0F00;    /* three button pad */
    else v2 = 0x1000 | ((v2 & 0x000F) << 8);      /* six button pad */

    val = ((v1 & 0x3000) >> 6) | (v1 & 0x003F);   /* 0 0 0 0 0 0 0 0 s a c b r l d u  */
    val |= v2;                                    /* 0 0 0 1 m x y z s a c b r l d u  or  0 0 0 0 1 1 1 1 s a c b r l d u */
    val ^= 0x0FFF;                                /* 0 0 0 1 M X Y Z S A C B R L D U  or  0 0 0 0 0 0 0 0 S A C B R L D U */

    return val;
}


static u8 THREELINE_HANDSHAKE(vu8 *pb, u8 ph)
{
    u8 val = 0, hs;

    *pb = ph; /* next phase in protocol */

    /* wait on handshake */
    hs = (ph >> 1) & 0x10;
    while (retry)
    {
        val = *pb;
        if ((val & 0x10) == hs)
            break; /* timeout */
        retry--;
    }

    return val & 0x0F;
}


static s16 start3lhs(u16 joy, u8 *hdr, u16 len)
{
    vu8 *pb;
    u16 i;

    retry = 255;
    phase = 0x20; /* selected */

    /* make sure port direction is correct */
    pb = (vu8 *)0xa10009 + joy*2;
    *pb = 0x60;

    /* make sure port is deselected to start at first phase */
    pb = (vu8 *)0xa10003 + joy*2;
    hdr[0] = THREELINE_HANDSHAKE(pb, 0x60);
    if (retry)
    {
        for (i=1; i<len; i++)
        {
            hdr[i] = THREELINE_HANDSHAKE(pb, phase);
            phase ^= 0x20;
            if (!retry)
                break; /* timeout */
        }
    }
    if (!retry)
        *pb = 0x60; /* timeout - end request */

    return retry ? 0 : -1;
}


static u16 readMouse(u16 joy)
{
    vu8 *pb;
    u16 val, i, mx, my;
    s16 sts;
    u8 hdr[4], md[6];

    pb = (vu8 *)0xa10003 + joy*2;
    val = 0x2000;

    sts = start3lhs(joy, hdr, 4);
    if ((sts == 0) &&
        (hdr[0] == 0x00) &&
        (hdr[1] == 0x0B) &&
        (hdr[2] == 0x0F) &&
        (hdr[3] == 0x0F))
    {
        /* handle mouse */
        for (i=0; i<6; i++)
        {
            md[i] = THREELINE_HANDSHAKE(pb, phase);
            phase ^= 0x20;
            if (!retry)
                break; /* timeout */
        }
        if (i == 6)
        {
            if (md[0] & 0x04)
                mx = 256; /* x overflow */
            else
                mx = md[2]<<4 | md[3];
            if (md[0] & 0x01)
                mx |= 0xFF00; /* x sign extend */
            if (md[0] & 0x08)
                my = 256; /* y overflow */
            else
                my = md[4]<<4 | md[5];
            if (md[0] & 0x02)
                my |= 0xFF00; /* y sign extend */
            joyAxisX[joy] += (s16)mx;
            joyAxisY[joy] += (s16)my;

            val |= md[1] << 4;
            if ((s16)mx < -2) val |= BUTTON_LEFT;
            else if ((s16)mx > 2) val |= BUTTON_RIGHT;
            if ((s16)my < -2) val |= BUTTON_DOWN;
            else if ((s16)my > 2) val |= BUTTON_UP;
        }
    }
    *pb = 0x60; /* end request */

    return val;
}


static void readTeamPlay(u16 port)
{
    static const u16 xlt_all[2][4] = {
        { JOY_1, JOY_3, JOY_4, JOY_5 },
        { JOY_2, JOY_6, JOY_7, JOY_8 }
    };

    vu8 *pb;
    u16 newstate, change;
    u16 val = 0, v1, v2, v3, mx, my, i, j;
    s16 sts;
    u8 hdr[8], md[6];
    const u16 *xlt_port = xlt_all[port];

    pb = (vu8 *)0xa10003 + port*2;

    sts = start3lhs(port, hdr, 8);
    if ((sts == 0) &&
        (hdr[0] == 0x03) &&
        (hdr[1] == 0x0F) &&
        (hdr[2] == 0x00) &&
        (hdr[3] == 0x00))
    {
        /* handle Sega Tap */
        for (j=0; j<4; j++)
        {
            const u16 xlt = xlt_port[j];

            switch(hdr[4+j])
            {
                case JOY_TYPE_UNKNOWN:
                    val = 0xF000;
                    break;
                case JOY_TYPE_PAD3:
                    v1 = THREELINE_HANDSHAKE(pb, phase); /* r l d u */
                    phase ^= 0x20;
                    if (!retry)
                        break; /* timeout */
                    v2 = THREELINE_HANDSHAKE(pb, phase); /* s a c b */
                    phase ^= 0x20;
                    if (!retry)
                        break; /* timeout */
                    val = (v2 << 4) | v1;
                    val ^= 0x00FF; /* 0 0 0 0 0 0 0 0 S A C B R L D U */
                    break;
                case JOY_TYPE_PAD6:
                    v1 = THREELINE_HANDSHAKE(pb, phase); /* r l d u */
                    phase ^= 0x20;
                    if (!retry)
                        break; /* timeout */
                    v2 = THREELINE_HANDSHAKE(pb, phase); /* s a c b */
                    phase ^= 0x20;
                    if (!retry)
                        break; /* timeout */
                    v3 = THREELINE_HANDSHAKE(pb, phase); /* m x y z */
                    phase ^= 0x20;
                    if (!retry)
                        break; /* timeout */
                    val = 0x1000 | (v3 << 8) | (v2 << 4) | v1;
                    val ^= 0x0FFF; /* 0 0 0 1 M X Y Z S A C B R L D U */
                    break;
                case JOY_TYPE_MOUSE:
                    val = 0x2000;
                    for (i=0; i<6; i++)
                    {
                        md[i] = THREELINE_HANDSHAKE(pb, phase);
                        phase ^= 0x20;
                        if (!retry)
                            break; /* timeout */
                    }
                    if (i == 6)
                    {
                        if (md[0] & 0x04)
                            mx = 256; /* x overflow */
                        else
                            mx = md[2]<<4 | md[3];
                        if (md[0] & 0x01)
                            mx |= 0xFF00; /* x sign extend */
                        if (md[0] & 0x08)
                            my = 256; /* y overflow */
                        else
                            my = md[4]<<4 | md[5];
                        if (md[0] & 0x02)
                            my |= 0xFF00; /* y sign extend */
                        joyAxisX[xlt] += (s16)mx;
                        joyAxisY[xlt] += (s16)my;

                        val |= md[1] << 4;
                        if ((s16)mx < -2) val |= BUTTON_LEFT;
                        else if ((s16)mx > 2) val |= BUTTON_RIGHT;
                        if ((s16)my < -2) val |= BUTTON_DOWN;
                        else if ((s16)my > 2) val |= BUTTON_UP;
                    }
                    break;
            }
            if (!retry)
                break; /* timeout */

            newstate = val;
            change = (joyState[xlt] & BUTTON_ALL) ^ (newstate & BUTTON_ALL);
            joyState[xlt] = newstate;
            if ((joyEventCB) && (change)) joyEventCB(xlt, change, newstate);
        }
    }
    *pb = 0x60; /* end request */
}


void JOY_update()
{
    u16 newstate;
    u16 change;

    switch (portSupport[PORT_1])
    {
        case JOY_SUPPORT_OFF:
            break;
        case JOY_SUPPORT_3BTN:
            newstate = read3Btn(0);
            change = joyState[0] ^ newstate;
            joyState[0] = newstate;
            if ((joyEventCB) && (change)) joyEventCB(0, change, newstate);
            break;
        case JOY_SUPPORT_6BTN:
            newstate = read6Btn(0);
            change = (joyState[0] & BUTTON_ALL) ^ (newstate & BUTTON_ALL);
            joyState[0] = newstate;
            if ((joyEventCB) && (change)) joyEventCB(0, change, newstate);
            break;
        case JOY_SUPPORT_MOUSE:
            newstate = readMouse(0);
            change = (joyState[0] & BUTTON_ALL) ^ (newstate & BUTTON_ALL);
            joyState[0] = newstate;
            if ((joyEventCB) && (change)) joyEventCB(0, change, newstate);
            break;
        case JOY_SUPPORT_TEAMPLAY:
            readTeamPlay(PORT_1);
            break;
        default:
            break;
    }
    switch (portSupport[PORT_2])
    {
        case JOY_SUPPORT_OFF:
            break;
        case JOY_SUPPORT_3BTN:
            newstate = read3Btn(1);
            change = joyState[1] ^ newstate;
            joyState[1] = newstate;
            if ((joyEventCB) && (change)) joyEventCB(1, change, newstate);
            break;
        case JOY_SUPPORT_6BTN:
            newstate = read6Btn(1);
            change = (joyState[1] & BUTTON_ALL) ^ (newstate & BUTTON_ALL);
            joyState[1] = newstate;
            if ((joyEventCB) && (change)) joyEventCB(1, change, newstate);
            break;
        case JOY_SUPPORT_MOUSE:
            newstate = readMouse(1);
            change = (joyState[1] & BUTTON_ALL) ^ (newstate & BUTTON_ALL);
            joyState[1] = newstate;
            if ((joyEventCB) && (change)) joyEventCB(1, change, newstate);
            break;
        case JOY_SUPPORT_TEAMPLAY:
            readTeamPlay(PORT_2);
            break;
        default:
            break;
    }
}
