#include "config.h"
#include "types.h"

#include "timer.h"
#include "memory.h"

#include "joy.h"
#include "sys.h"
#include "vdp.h"


#define JOY_TYPE_SHIFT          12


static u8 joyType[8];
static u16 joyState[8];
static s16 joyAxisX[8];
static s16 joyAxisY[8];

static u8 portSupport[2];
static u8 portType[2];

static u8 retry;
static u8 phase;

static u8 gun;
static u8 extSet;
static u16 gport;


static _joyEventCallback *joyEventCB;


void JOY_init()
{
    vu8 *pb;
    u8  a, id;
    u16 i;

    joyEventCB = NULL;
    gport = 0xFFFF;

    // disable ints
    SYS_disableInts();

    /* check for EA 4-Way Play */
    pb = (vu8 *)0xa10009;
    *pb = 0x40;
    pb = (vu8 *)0xa1000b;
    *pb = 0x43;
    pb = (vu8 *)0xa10005;
    *pb = 0x7C;
    pb = (vu8 *)0xa1000b;
    *pb = 0x7F;
    pb = (vu8 *)0xa10005;
    *pb = 0x7C;
    pb = (vu8 *)0xa10003;
    a = *pb & 3;

    if (a == 0)
    {
        /* EA 4-Way Play detected */
        portType[PORT_1] = PORT_TYPE_EA4WAYPLAY;
        portType[PORT_2] = PORT_TYPE_EA4WAYPLAY;
        portSupport[PORT_1] = JOY_SUPPORT_EA4WAYPLAY;
        portSupport[PORT_2] = JOY_SUPPORT_OFF;

        for (i=JOY_1; i<JOY_NUM; i++)
        {
            joyType[i] = JOY_TYPE_UNKNOWN; /* default to unknown */
            joyState[i] = 0;
            joyAxisX[i] = 0;
            joyAxisY[i] = 0;
        }

        // restore ints
        SYS_enableInts();

        /* wait a few vblanks for JOY_update() to get valid data */
        VDP_waitVSync();
        VDP_waitVSync();
        VDP_waitVSync();

        return; /* EA 4-Way Play is the only thing that can be plugged in as it takes both ports */
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

    VDP_waitVSync();
    VDP_waitVSync();

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

    portSupport[PORT_1] = JOY_SUPPORT_OFF; /* default to off */
    portSupport[PORT_2] = JOY_SUPPORT_OFF; /* default to off */

    for (i=JOY_1; i<JOY_NUM; i++)
    {
        joyType[i] = JOY_TYPE_UNKNOWN; /* default to unknown */
        joyState[i] = 0;
        joyAxisX[i] = 0;
        joyAxisY[i] = 0;
    }

    switch (portType[PORT_1])
    {
        case PORT_TYPE_MENACER:
        case PORT_TYPE_JUSTIFIER:
            /* init port for light gun control */
            pb = (vu8 *)0xa10009;
            *pb = 0x30;
            pb = (vu8 *)0xa10003;
            *pb = 0x30;
            break;
        case PORT_TYPE_MOUSE:
        case PORT_TYPE_TEAMPLAYER:
            /* init port for Three Line Handshake Method */
            pb = (vu8 *)0xa10009;
            *pb = 0x60;
            pb = (vu8 *)0xa10003;
            *pb = 0x60;
            break;
        case PORT_TYPE_PAD:
            portSupport[PORT_1] = JOY_SUPPORT_6BTN; /* default to on for pads */
            break;
    }

    switch (portType[PORT_2])
    {
        case PORT_TYPE_MENACER:
        case PORT_TYPE_JUSTIFIER:
            /* init port for light gun control */
            pb = (vu8 *)0xa1000b;
            *pb = 0x30;
            pb = (vu8 *)0xa10005;
            *pb = 0x30;
            break;
        case PORT_TYPE_MOUSE:
        case PORT_TYPE_TEAMPLAYER:
            /* init port for Three Line Handshake Method */
            pb = (vu8 *)0xa1000b;
            *pb = 0x60;
            pb = (vu8 *)0xa10005;
            *pb = 0x60;
            break;
        case PORT_TYPE_PAD:
            portSupport[PORT_2] = JOY_SUPPORT_6BTN; /* default to on for pads */
            break;
    }

    /* check if need to turn on an input device */
    if ((portType[PORT_1] != PORT_TYPE_PAD) && (portType[PORT_2] != PORT_TYPE_PAD))
    {
        /* no pads - look for teamplayer or mouse */
        if (portType[PORT_1] == PORT_TYPE_TEAMPLAYER)
            JOY_setSupport(PORT_1, JOY_SUPPORT_TEAMPLAYER);
        else if (portType[PORT_2] == PORT_TYPE_TEAMPLAYER)
            JOY_setSupport(PORT_2, JOY_SUPPORT_TEAMPLAYER);
        else if (portType[PORT_1] == PORT_TYPE_MOUSE)
            JOY_setSupport(PORT_1, JOY_SUPPORT_MOUSE);
        else if (portType[PORT_2] == PORT_TYPE_MOUSE)
            JOY_setSupport(PORT_2, JOY_SUPPORT_MOUSE);
    }

    // restore ints
    SYS_enableInts();

    /* wait a few vblanks for JOY_update() to get valid data */
    VDP_waitVSync();
    VDP_waitVSync();
    VDP_waitVSync();

    /* now update pads to reflect true type (3 or 6 button) */
    if (portType[PORT_1] == PORT_TYPE_PAD)
        if (joyType[JOY_1] == JOY_TYPE_PAD3)
            portSupport[PORT_1] = JOY_SUPPORT_3BTN;
    if (portType[PORT_2] == PORT_TYPE_PAD)
        if (joyType[JOY_2] == JOY_TYPE_PAD3)
            portSupport[PORT_2] = JOY_SUPPORT_3BTN;
}


void JOY_setEventHandler(_joyEventCallback *CB)
{
    joyEventCB = CB;
}


static void externalIntCB()
{
    vu8 *pb;
    u16 hv;

    hv = GET_HVCOUNTER;                  /* read HV counter */

    if (extSet || (gport == 0xFFFF)) return;

    pb = (vu8 *)0xa10003 + gport*2;
    if (portType[gport] == PORT_TYPE_JUSTIFIER)
        *pb = gun | 0x10;                   /* deselect gun */

    if (!gun)
    {
        /* blue gun or menacer */
        joyAxisX[gport] = hv & 0x00FF;
        joyAxisY[gport] = hv >> 8;
    }
    else
    {
        /* red gun */
        joyAxisX[gport ? JOY_6 : JOY_3] = hv & 0x00FF;
        joyAxisY[gport ? JOY_6 : JOY_3] = hv >> 8;
    }
    extSet = 1;                             /* we got light sense */
}


void JOY_setSupport(u16 port, u16 support)
{
    if (port > PORT_2) return;

    if ((portType[port] == PORT_TYPE_MENACER) || (portType[port] == PORT_TYPE_JUSTIFIER))
    {
        vu8 *pb;
        u8 val;

        if ((support == JOY_SUPPORT_JUSTIFIER_BLUE) || (support == JOY_SUPPORT_JUSTIFIER_BOTH) || (support == JOY_SUPPORT_MENACER))
        {
            /* enable lightgun support if support currently off */
            if (portSupport[port] == JOY_SUPPORT_OFF)
            {
                SYS_setInterruptMaskLevel(7);   /* disable ints */
                /* OBVIOUSLY blue gun or menacer is present due to portType ID */
                joyType[port] = (support == JOY_SUPPORT_MENACER) ? JOY_TYPE_MENACER : JOY_TYPE_JUSTIFIER;
                joyState[port] = 0;
                joyAxisX[port] = -1;
                joyAxisY[port] = -1;
                gun = 0;
                extSet = 0;
                gport = port;
                if (support == JOY_SUPPORT_JUSTIFIER_BOTH)
                {
                    joyType[port ? JOY_6 : JOY_3] = JOY_TYPE_JUSTIFIER; /* allow red gun */
                    joyState[port ? JOY_6 : JOY_3] = 0;
                    joyAxisX[port ? JOY_6 : JOY_3] = -1;
                    joyAxisY[port ? JOY_6 : JOY_3] = -1;
                }

                SYS_setExtIntCallback(externalIntCB);
                pb = (vu8 *)0xa10009 + port*2;
                *pb = 0xB0;                     /* enable TH->HL */
                val = VDP_getReg(0);
                VDP_setReg(0, val | 0x02);      /* set M3, enable HV counter latch */
                val = VDP_getReg(11);
                VDP_setReg(11, val | 0x08);     /* set IE2, enable external int */
                SYS_setInterruptMaskLevel(1);   /* External int allowed */

                /* set last since this starts the vblank handling of the gun(s) */
                portSupport[port] = support;
            }
        }
        else if (support == JOY_SUPPORT_OFF)
        {
            /* disable lightgun support if was on */
            if ((portSupport[port] == JOY_SUPPORT_JUSTIFIER_BLUE) || (portSupport[port] == JOY_SUPPORT_JUSTIFIER_BOTH) || (portSupport[port] == JOY_SUPPORT_MENACER))
            {
                SYS_setInterruptMaskLevel(7);       /* disable ints */
                pb = (vu8 *)0xa10009 + port*2;
                *pb = 0x30;                         /* disable TH->HL */
                val = VDP_getReg(0);
                VDP_setReg(0, val & ~0x02);         /* clear M3, disable HV counter latch */
                val = VDP_getReg(11);
                VDP_setReg(11, val & ~0x08);        /* clear IE2, disable external int */
                SYS_setExtIntCallback(NULL);

                if (port == PORT_1)
                {
                    joyType[JOY_1] = JOY_TYPE_UNKNOWN;
                    joyType[JOY_3] = JOY_TYPE_UNKNOWN;
                    joyState[JOY_1] = 0;
                    joyState[JOY_3] = 0;
                }
                else
                {
                    joyType[JOY_2] = JOY_TYPE_UNKNOWN;
                    joyType[JOY_6] = JOY_TYPE_UNKNOWN;
                    joyState[JOY_2] = 0;
                    joyState[JOY_6] = 0;
                }
                gport = 0xFFFF;
                SYS_setInterruptMaskLevel(3);       /* External int disallowed */
            }
            portSupport[port] = JOY_SUPPORT_OFF;
        }
        else if (support == JOY_SUPPORT_TRACKBALL)
        {
            /*
             * The port ID for the trackball is 0x0F the first time, then
             * 0x00 (same as menacer) until you power off.
             *
             * Reset port direction and data for TH Control Method.
             */
            pb = (vu8 *)0xa10009 + port*2;
            *pb = 0x40;
            pb = (vu8 *)0xa10003 + port*2;
            *pb = 0x40;
            portSupport[port] = support;
        }
    }
    else
        portSupport[port] = support;
}


u8 JOY_getPortType(u16 port)
{
    if (port > PORT_2) return PORT_TYPE_UKNOWN;

    return portType[port];
}

u8 JOY_getJoypadType(u16 joy)
{
    if (joy < JOY_NUM)
        return joyType[joy];

    return JOY_TYPE_UNKNOWN;
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

    return 0;
}

u16 JOY_readJoypadX(u16 joy)
{
    if (joy < JOY_NUM)
        return joyAxisX[joy];

    return 0;
}

u16 JOY_readJoypadY(u16 joy)
{
    if (joy < JOY_NUM)
        return joyAxisY[joy];

    return 0;
}


void JOY_waitPressBtn()
{
    JOY_waitPressTime(JOY_ALL, BUTTON_BTN, 0);
}

u16 JOY_waitPressBtnTime(u16 time)
{
    return JOY_waitPressTime(JOY_ALL, BUTTON_BTN, time);
}

u16 JOY_waitPress(u16 joy, u16 btn)
{
    return JOY_waitPressTime(joy, btn, 0);
}

u16 JOY_waitPressTime(u16 joy, u16 btn, u16 time)
{
    u32 maxtime;
    u32 current;

    // (getTime() << 2) give a good estimation of time in ms
    current = (getTime(TRUE) << 2);

    // time == 0 --> wait indefinitely
    if (time) maxtime = current + time;
    else maxtime = 0xFFFFFFFF;

    while(current < maxtime)
    {
        u16 state;

        VDP_waitVSync();

        /* vblank int won't occur - do JOY_update() manually */
        if (SYS_getInterruptMaskLevel() >= 6)
            JOY_update();

        if (joy == JOY_ALL)
        {
            u16 i;

            i = JOY_NUM;
            while(i--)
            {
                state = joyState[i] & btn;

                if (state)
                    return state;
            }
        }
        else
        {
            state = joyState[joy] & btn;

            if (state)
                return state;
        }

        // (getTime() << 2) give a good estimation of time in ms
        current = (getTime(TRUE) << 2);
    }

    return FALSE;
}


static u16 TH_CONTROL_PHASE(vu8 *pb)
{
    u16 val;

    *pb = 0x00; /* TH (select) low */
    asm volatile ("nop");
    asm volatile ("nop");
    val = *pb;

    *pb = 0x40; /* TH (select) high */
    val <<= 8;
    val |= *pb;

    return val;
}


static u16 read3Btn(u16 port)
{
    vu8 *pb;
    u16 val;

    pb = (vu8 *)0xa10003 + port*2;

    val = TH_CONTROL_PHASE(pb);                   /* - 0 s a 0 0 d u - 1 c b r l d u */
    val = ((val & 0x3000) >> 6) | (val & 0x003F); /* 0 0 0 0 0 0 0 0 s a c b r l d u */
    val ^= 0x00FF;                                /* 0 0 0 0 0 0 0 0 S A C B R L D U */

    return val | (JOY_TYPE_PAD3 << JOY_TYPE_SHIFT);
}


static u16 read6Btn(u16 port)
{
    vu8 *pb;
    u16 val, v1, v2;

    pb = (vu8 *)0xa10003 + port*2;

    v1 = TH_CONTROL_PHASE(pb);                    /* - 0 s a 0 0 d u - 1 c b r l d u */
    val = TH_CONTROL_PHASE(pb);                   /* - 0 s a 0 0 d u - 1 c b r l d u */
    v2 = TH_CONTROL_PHASE(pb);                    /* - 0 s a 0 0 0 0 - 1 c b m x y z */
    val = TH_CONTROL_PHASE(pb);                   /* - 0 s a 1 1 1 1 - 1 c b r l d u */

    if ((val & 0x0F00) != 0x0F00) v2 = (JOY_TYPE_PAD3 << JOY_TYPE_SHIFT) | 0x0F00; /* three button pad */
    else v2 = (JOY_TYPE_PAD6 << JOY_TYPE_SHIFT) | ((v2 & 0x000F) << 8); /* six button pad */

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


static s16 start3lhs(u16 port, u8 *hdr, u16 len)
{
    vu8 *pb;
    u16 i;

    retry = 255;
    phase = 0x20; /* selected */

    /* make sure port direction is correct */
    pb = (vu8 *)0xa10009 + port*2;
    *pb = 0x60;

    /* make sure port is deselected to start at first phase */
    pb = (vu8 *)0xa10003 + port*2;
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


static u16 readMouse(u16 port)
{
    vu8 *pb;
    u16 val, i, mx, my;
    s16 sts;
    u8 hdr[4], md[6];

    pb = (vu8 *)0xa10003 + port*2;
    val = (JOY_TYPE_MOUSE << JOY_TYPE_SHIFT);

    sts = start3lhs(port, hdr, 4);
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
            joyAxisX[port] += (s16)mx;
            joyAxisY[port] += (s16)my;

            if (md[1] & 8) val |= BUTTON_START;
            if (md[1] & 4) val |= BUTTON_MMB;
            if (md[1] & 2) val |= BUTTON_RMB;
            if (md[1] & 1) val |= BUTTON_LMB;

            if ((s16)mx < -2) val |= BUTTON_LEFT;
            else if ((s16)mx > 2) val |= BUTTON_RIGHT;
            if ((s16)my < -2) val |= BUTTON_DOWN;
            else if ((s16)my > 2) val |= BUTTON_UP;
        }
    }
    *pb = 0x60; /* end request */

    return val;
}


static void readTeamPlayer(u16 port)
{
    static const u16 xlt_all[2][4] = {
        { JOY_1, JOY_3, JOY_4, JOY_5 },
        { JOY_2, JOY_6, JOY_7, JOY_8 }
    };

    vu8 *pb;
    u16 change;
    u16 val = 0, v1, v2, v3, mx, my, typ = JOY_TYPE_UNKNOWN, i, j;
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
                    val = 0;
                    typ = JOY_TYPE_UNKNOWN;
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
                    typ = JOY_TYPE_PAD3;
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
                    val = (v3 << 8) | (v2 << 4) | v1;
                    val ^= 0x0FFF; /* 0 0 0 1 M X Y Z S A C B R L D U */
                    typ = JOY_TYPE_PAD6;
                    break;
                case JOY_TYPE_MOUSE:
                    for (i=0; i<6; i++)
                    {
                        md[i] = THREELINE_HANDSHAKE(pb, phase);
                        phase ^= 0x20;
                        if (!retry)
                            break; /* timeout */
                    }
                    if (!retry)
                        break;

                    val = 0;
                    if (md[0] & 0x04)
                        mx = 256; /* x overflow */
                    else
                        mx = (md[2] << 4) | md[3];
                    if (md[0] & 0x01)
                        mx |= 0xFF00; /* x sign extend */
                    if (md[0] & 0x08)
                        my = 256; /* y overflow */
                    else
                        my = (md[4] << 4) | md[5];
                    if (md[0] & 0x02)
                        my |= 0xFF00; /* y sign extend */
                    joyAxisX[xlt] += (s16)mx;
                    joyAxisY[xlt] += (s16)my;

                    if (md[1] & 8) val |= BUTTON_START;
                    if (md[1] & 4) val |= BUTTON_MMB;
                    if (md[1] & 2) val |= BUTTON_RMB;
                    if (md[1] & 1) val |= BUTTON_LMB;

                    if ((s16)mx < -2) val |= BUTTON_LEFT;
                    else if ((s16)mx > 2) val |= BUTTON_RIGHT;
                    if ((s16)my < -2) val |= BUTTON_DOWN;
                    else if ((s16)my > 2) val |= BUTTON_UP;

                    typ = JOY_TYPE_MOUSE;
                    break;
            }
            if (!retry)
                break; /* timeout */

            change = joyState[xlt] ^ val;
            joyType[xlt] = typ;
            joyState[xlt] = val;
            if ((joyEventCB) && (change)) joyEventCB(xlt, change, val);
        }
    }
    *pb = 0x60; /* end request */
}


static void readEa4WayPlay()
{
    static const u16 xlt[4] = { JOY_1, JOY_3, JOY_4, JOY_5 };

    vu8 *pb;
    u16 change;
    u16 val, v1, v2, typ, ind, i;

    for (i=0; i<4; i++)
    {
        pb = (vu8 *)0xa10005;
        *pb = (i<<4) | 0x0C;                          /* select port i */

        pb = (vu8 *)0xa10003;
        v1 = TH_CONTROL_PHASE(pb);                    /* - 0 s a 0 0 d u - 1 c b r l d u */
        if (v1 & 0x0C00)
            continue;                                 /* no pad */
        val = TH_CONTROL_PHASE(pb);                   /* - 0 s a 0 0 d u - 1 c b r l d u */
        v2 = TH_CONTROL_PHASE(pb);                    /* - 0 s a 0 0 0 0 - 1 c b m x y z */
        val = TH_CONTROL_PHASE(pb);                   /* - 0 s a 1 1 1 1 - 1 c b r l d u */

        pb = (vu8 *)0xa10005;
        *pb = 0x7C;                                   /* deselect port */

        if ((val & 0x0F00) != 0x0F00)
        {
            typ = JOY_TYPE_PAD3;                      /* three button pad */
            v2 = 0x0F00;
        }
        else
        {
            typ = JOY_TYPE_PAD6;                      /* six button pad */
            v2 = (v2 & 0x000F) << 8;
        }

        val = ((v1 & 0x3000) >> 6) | (v1 & 0x003F);   /* 0 0 0 0 0 0 0 0 s a c b r l d u  */
        val |= v2;                                    /* 0 0 0 1 m x y z s a c b r l d u  or  0 0 0 0 1 1 1 1 s a c b r l d u */
        val ^= 0x0FFF;                                /* 0 0 0 1 M X Y Z S A C B R L D U  or  0 0 0 0 0 0 0 0 S A C B R L D U */

        ind = xlt[i];

        change = joyState[ind] ^ val;
        joyType[ind] = typ;
        joyState[ind] = val;
        if ((joyEventCB) && (change)) joyEventCB(ind, change, val);
    }
}


static void readLightgun(u16 port)
{
    vu8 *pb;
    u16 newstate, change;
    u16 val = 0;
    u32 tmp;

    pb = (vu8 *)0xa10003 + port*2;
    if (portType[port] == PORT_TYPE_JUSTIFIER)
        *pb = gun | 0x10;                   /* deselect gun */

    if (!extSet)
    {
        /* no light sense => gun not pointed at screen */
        if (!gun)
        {
            /* blue gun */
            joyAxisX[port] = -1;
            joyAxisY[port] = -1;
        }
        else
        {
            /* red gun */
            joyAxisX[port ? JOY_6 : JOY_3] = -1;
            joyAxisY[port ? JOY_6 : JOY_3] = -1;
        }
    }

    if (portType[port] == PORT_TYPE_MENACER)
    {
        asm volatile ("move.w #160,%0\n"
            "1:\n\t"
            "dbra %0,1b\n\t"
            : "=d" (tmp) : : "cc"
        );
        *pb = 0xFF;                         /* deassert RST */
        asm volatile ("moveq #20,%0\n"
            "1:\n\t"
            "dbra %0,1b\n\t"
            : "=d" (tmp) : : "cc"
        );
        *pb = 0xDF;                         /* assert RST long */
        asm volatile ("moveq #60,%0\n"
            "1:\n\t"
            "dbra %0,1b\n\t"
            : "=d" (tmp) : : "cc"
        );
        *pb = 0xFF;                         /* deassert RST */
        asm volatile ("nop");
        *pb = 0xDF;                         /* assert RST long */
        asm volatile ("move.w #1080,%0\n"
            "1:\n\t"
            "dbra %0,1b\n\t"
            : "=d" (tmp) : : "cc"
        );
        val = *pb;
        *pb = 0xFF;                         /* deassert RST */
        asm volatile ("nop");
        *pb = 0xCF;                         /* assert RST short - clear counter */

        newstate = 0;
        if (val & 8) newstate |= BUTTON_START;
        if (val & 4) newstate |= BUTTON_C;
        if (val & 2) newstate |= BUTTON_A;
        if (val & 1) newstate |= BUTTON_B;

        change = joyState[port] ^ newstate;
        joyType[port] = JOY_TYPE_MENACER;
        joyState[port] = newstate;
        if ((joyEventCB) && (change)) joyEventCB(port, change, newstate);
    }
    else
    {
        *pb = 0x00;                         /* select blue gun */
        val = *pb;
        val = *pb;                          /* read twice... probably a setup delay thing */
        *pb = 0x10;                         /* deselect blue gun */

        newstate = 0;
        if (~val & 2) newstate |= BUTTON_START;
        if (~val & 1) newstate |= BUTTON_A;

        change = joyState[port] ^ newstate;
        joyType[port] = JOY_TYPE_JUSTIFIER;
        joyState[port] = newstate;
        if ((joyEventCB) && (change)) joyEventCB(port, change, newstate);

        if (joyType[port ? JOY_6 : JOY_3] == JOY_TYPE_JUSTIFIER)
        {
            *pb = 0x20;                     /* select red gun */
            val = *pb;
            val = *pb;                      /* read twice... probably a setup delay thing */
            *pb = 0x30;                     /* deselect red gun */

            newstate = 0;
            if (~val & 2) newstate |= BUTTON_START;
            if (~val & 1) newstate |= BUTTON_A;

            change = joyState[port ? JOY_6 : JOY_3] ^ newstate;
            joyType[port ? JOY_6 : JOY_3] = JOY_TYPE_JUSTIFIER;
            joyState[port ? JOY_6 : JOY_3] = newstate;
            if ((joyEventCB) && (change)) joyEventCB(port ? JOY_6 : JOY_3, change, newstate);

            /* only need to toggle gun selector if have two guns */
            gun ^= 0x20;
        }

        *pb = gun | 0x10;                   /* deselect gun */
        *pb = gun;                          /* leave gun selected to get light sense input */
    }

    extSet = 0;                             /* clear light sensed flag */
}


static u16 readTrackball(u16 port)
{
    vu8 *pb;
    u8 mx, my, v1, v2, v3, v4;
    u16 val;
    u32 tmp;

    pb = (vu8 *)0xa10003 + port*2;
    val = (JOY_TYPE_TRACKBALL << JOY_TYPE_SHIFT);

    *pb = 0x00; /* TH (select) low */
    /* ~ 84 us */
    asm volatile ("moveq #63,%0\n"
        "1:\n\t"
        "dbra %0,1b\n\t"
        : "=d" (tmp) : : "cc"
    );
    v1 = *pb;

    *pb = 0x40; /* TH (select) high */
    /* ~ 42 us */
    asm volatile ("moveq #31,%0\n"
        "1:\n\t"
        "dbra %0,1b\n\t"
        : "=d" (tmp) : : "cc"
    );
    v2 = *pb;

    *pb = 0x00; /* TH (select) low */
    /* ~ 42 us */
    asm volatile ("moveq #31,%0\n"
        "1:\n\t"
        "dbra %0,1b\n\t"
        : "=d" (tmp) : : "cc"
    );
    v3 = *pb;

    *pb = 0x40; /* TH (select) high */
    /* ~ 42 us */
    asm volatile ("moveq #31,%0\n"
        "1:\n\t"
        "dbra %0,1b\n\t"
        : "=d" (tmp) : : "cc"
    );
    v4 = *pb;

    mx = (v1 & 0x0F) << 4;
    mx |= v2 & 0x0F;
    my = (v3 & 0x0F) << 4;
    my |= v4 & 0x0F;

    joyAxisX[port] -= (s8)mx;
    joyAxisY[port] += (s8)my;

    if (~v4 & 32) val |= BUTTON_A;
    if (~v4 & 16) val |= BUTTON_START;
    if ((s8)mx < -2) val |= BUTTON_RIGHT;
    else if ((s8)mx > 2) val |= BUTTON_LEFT;
    if ((s8)my < -2) val |= BUTTON_DOWN;
    else if ((s8)my > 2) val |= BUTTON_UP;

    return val;
}


void JOY_update()
{
    u16 val;
    u16 newstate;
    u16 change;

    switch (portSupport[PORT_1])
    {
        case JOY_SUPPORT_OFF:
            break;

        case JOY_SUPPORT_3BTN:
            val = read3Btn(PORT_1);
            newstate = val & BUTTON_ALL;
            change = joyState[JOY_1] ^ newstate;
            joyType[JOY_1] = val >> JOY_TYPE_SHIFT;
            joyState[JOY_1] = newstate;
            if ((joyEventCB) && (change)) joyEventCB(JOY_1, change, newstate);
            break;

        case JOY_SUPPORT_6BTN:
            val = read6Btn(PORT_1);
            newstate = val & BUTTON_ALL;
            change = joyState[JOY_1] ^ newstate;
            joyType[JOY_1] = val >> JOY_TYPE_SHIFT;
            joyState[JOY_1] = newstate;
            if ((joyEventCB) && (change)) joyEventCB(JOY_1, change, newstate);
            break;

        case JOY_SUPPORT_MOUSE:
            val = readMouse(PORT_1);
            newstate = val & BUTTON_ALL;
            change = joyState[JOY_1] ^ newstate;
            joyType[JOY_1] = val >> JOY_TYPE_SHIFT;
            joyState[JOY_1] = newstate;
            if ((joyEventCB) && (change)) joyEventCB(JOY_1, change, newstate);
            break;

        case JOY_SUPPORT_TRACKBALL:
            val = readTrackball(PORT_1);
            newstate = val & BUTTON_ALL;
            change = joyState[JOY_1] ^ newstate;
            joyType[JOY_1] = val >> JOY_TYPE_SHIFT;
            joyState[JOY_1] = newstate;
            if ((joyEventCB) && (change)) joyEventCB(JOY_1, change, newstate);
            break;

        case JOY_SUPPORT_TEAMPLAYER:
            readTeamPlayer(PORT_1);
            break;

        case JOY_SUPPORT_EA4WAYPLAY:
            readEa4WayPlay();
            break;

        case JOY_SUPPORT_MENACER:
        case JOY_SUPPORT_JUSTIFIER_BLUE:
        case JOY_SUPPORT_JUSTIFIER_BOTH:
            readLightgun(PORT_1);
            break;

        default:
            break;
    }

    switch (portSupport[PORT_2])
    {
        case JOY_SUPPORT_OFF:
            break;

        case JOY_SUPPORT_3BTN:
            val = read3Btn(PORT_2);
            newstate = val & BUTTON_ALL;
            change = joyState[JOY_2] ^ newstate;
            joyType[JOY_2] = val >> JOY_TYPE_SHIFT;
            joyState[JOY_2] = newstate;
            if ((joyEventCB) && (change)) joyEventCB(JOY_2, change, newstate);
            break;

        case JOY_SUPPORT_6BTN:
            val = read6Btn(PORT_2);
            newstate = val & BUTTON_ALL;
            change = joyState[JOY_2] ^ newstate;
            joyType[JOY_2] = val >> JOY_TYPE_SHIFT;
            joyState[JOY_2] = newstate;
            if ((joyEventCB) && (change)) joyEventCB(JOY_2, change, newstate);
            break;

        case JOY_SUPPORT_MOUSE:
            val = readMouse(PORT_2);
            newstate = val & BUTTON_ALL;
            change = joyState[JOY_2] ^ newstate;
            joyType[JOY_2] = val >> JOY_TYPE_SHIFT;
            joyState[JOY_2] = newstate;
            if ((joyEventCB) && (change)) joyEventCB(JOY_2, change, newstate);
            break;

        case JOY_SUPPORT_TRACKBALL:
            val = readTrackball(PORT_2);
            newstate = val & BUTTON_ALL;
            change = joyState[JOY_2] ^ newstate;
            joyType[JOY_2] = val >> JOY_TYPE_SHIFT;
            joyState[JOY_2] = newstate;
            if ((joyEventCB) && (change)) joyEventCB(JOY_2, change, newstate);
            break;

        case JOY_SUPPORT_TEAMPLAYER:
            readTeamPlayer(PORT_2);
            break;

        case JOY_SUPPORT_MENACER:
        case JOY_SUPPORT_JUSTIFIER_BLUE:
        case JOY_SUPPORT_JUSTIFIER_BOTH:
            readLightgun(PORT_2);
            break;

        default:
            break;
    }
}
