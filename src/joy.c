#include "config.h"
#include "types.h"

#include "timer.h"
#include "memory.h"

#include "joy.h"
#include "sys.h"
#include "vdp.h"
#include "tools.h"
#include "z80_ctrl.h"


#define JOY_TYPE_SHIFT          12

#if (HALT_Z80_ON_IO != 0)
    #define Z80_STATE_DECL      u16 z80state;
    #define Z80_STATE_SAVE      z80state = Z80_getAndRequestBus(TRUE);
    #define Z80_STATE_RESTORE   if (!z80state) Z80_releaseBus();
#else
    #define Z80_STATE_DECL
    #define Z80_STATE_SAVE
    #define Z80_STATE_RESTORE
#endif



// don't want to share it
extern bool randomSeedSet;

static vu8 joyType[8];
static vu16 joyState[8];
static vs16 joyAxisX[8];
static vs16 joyAxisY[8];

static u8 portSupport[2];
static u8 portType[2];

static u8 retry;
static u8 phase;

static u8 gun;
static u8 extSet;
static u16 gport;


static JoyEventCallback *joyEventCB;


void JOY_init()
{
    /* Reset controller state change event callback */
    joyEventCB = NULL;

    /* Then perform controller port reset and peripheral detection */
    JOY_reset();
}

NO_INLINE void JOY_reset()
{
    vu8 *pb;
    u8 a, b;
    u8 id;
    u16 i;
    Z80_STATE_DECL

    gport = 0xFFFF;

    /* disable ints (we can do it here for safety) */
    SYS_disableInts();

    Z80_STATE_SAVE

    /* check for EA 4-Way Play */
    pb = (vu8 *)0xa10009;
    *pb = 0x40;
    pb = (vu8 *)0xa1000b;
    *pb = 0x7F;
    pb = (vu8 *)0xa10003;
    *pb = 0x40;

    pb = (vu8 *)0xa10005;
    *pb = 0x0C;
    asm volatile ("nop");
    asm volatile ("nop");
    pb = (vu8 *)0xa10003;
    a = *pb & 3;

    pb = (vu8 *)0xa10005;
    *pb = 0x7C;
    asm volatile ("nop");
    asm volatile ("nop");
    pb = (vu8 *)0xa10003;
    b = *pb & 3;

    Z80_STATE_RESTORE

    /* EA 4-Way Play detected */
    if (a != 0 && b == 0)
    {
        /* EA 4-Way Play is the only thing that can be plugged in as it takes both ports */
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
    }
    else
    {
        /*
         * Initialize ports for peripheral interface protocol - default to
         * TH Control Method for pads
         */

        Z80_STATE_SAVE

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

        Z80_STATE_RESTORE

        /* need to wait */
        VDP_waitVSync();
        VDP_waitVSync();

        Z80_STATE_SAVE

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

        Z80_STATE_RESTORE

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

        Z80_STATE_SAVE

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

        Z80_STATE_RESTORE

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
    }

    /* now update pads to reflect true type (3 or 6 button) */
    if ((portType[PORT_1] == PORT_TYPE_PAD) && (joyType[JOY_1] == JOY_TYPE_PAD3))
        portSupport[PORT_1] = JOY_SUPPORT_3BTN;
    if ((portType[PORT_2] == PORT_TYPE_PAD) && (joyType[JOY_2] == JOY_TYPE_PAD3))
        portSupport[PORT_2] = JOY_SUPPORT_3BTN;

    /* restore ints */
    SYS_enableInts();

    /* wait a few vblanks for JOY_update() to get valid data */
    SYS_doVBlankProcess();
    SYS_doVBlankProcess();
    SYS_doVBlankProcess();
}


JoyEventCallback* JOY_getEventHandler()
{
    return joyEventCB;
}

void JOY_setEventHandler(JoyEventCallback *CB)
{
    joyEventCB = CB;
}


static void externalIntCB()
{
    vu8 *pb;
    u16 hv;
    Z80_STATE_DECL

    hv = GET_HVCOUNTER;                     /* read HV counter */

    if (extSet || (gport == 0xFFFF)) return;

    Z80_STATE_SAVE

    pb = (vu8 *)0xa10003 + (gport * 2);
    if (portType[gport] == PORT_TYPE_JUSTIFIER)
        *pb = gun | 0x10;                   /* deselect gun */

    Z80_STATE_RESTORE

    if (!gun)
    {
        /* blue gun or menacer or phaser */
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


NO_INLINE void JOY_setSupport(u16 port, u16 support)
{
    Z80_STATE_DECL

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

                Z80_STATE_SAVE

                pb = (vu8 *)0xa10009 + port*2;
                *pb = 0xB0;                     /* enable TH->HL */

                Z80_STATE_RESTORE

                VDP_setHVLatching(TRUE);        /* enable HV counter latch */
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

                Z80_STATE_SAVE

                pb = (vu8 *)0xa10009 + port*2;
                *pb = 0x30;                         /* disable TH->HL */

                Z80_STATE_RESTORE

                VDP_setHVLatching(FALSE);           /* disable HV counter latch */
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
            Z80_STATE_SAVE

            pb = (vu8 *)0xa10009 + port*2;
            *pb = 0x40;
            pb = (vu8 *)0xa10003 + port*2;
            *pb = 0x40;

            Z80_STATE_RESTORE

            portSupport[port] = support;
        }
    }
    else if ((support == JOY_SUPPORT_PHASER) || (support == JOY_SUPPORT_OFF))
    {
        vu8 *pb;
        u8 val;

        if (support == JOY_SUPPORT_PHASER)
        {
            /* enable lightgun support if support currently off */
            if (portSupport[port] == JOY_SUPPORT_OFF)
            {
                SYS_setInterruptMaskLevel(7);   /* disable ints */

                joyType[port] = JOY_TYPE_PHASER;
                joyState[port] = 0;
                joyAxisX[port] = -1;
                joyAxisY[port] = -1;

                gun = 0;
                extSet = 0;
                gport = port;

                SYS_setExtIntCallback(externalIntCB);

                Z80_STATE_SAVE

                pb = (vu8 *)0xa10009 + port*2;
                *pb = 0x80;                     /* enable TH->HL */

                Z80_STATE_RESTORE

                VDP_setHVLatching(TRUE);        /* enable HV counter latch */
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
            if (portSupport[port] == JOY_SUPPORT_PHASER)
            {
                SYS_setInterruptMaskLevel(7);       /* disable ints */

                Z80_STATE_SAVE

                pb = (vu8 *)0xa10009 + port*2;
                *pb = 0x40;                         /* disable TH->HL */

                Z80_STATE_RESTORE

                VDP_setHVLatching(FALSE);           /* disable HV counter latch */
                val = VDP_getReg(11);
                VDP_setReg(11, val & ~0x08);        /* clear IE2, disable external int */

                SYS_setExtIntCallback(NULL);

                if (port == PORT_1)
                {
                    joyType[JOY_1] = JOY_TYPE_UNKNOWN;
                    joyState[JOY_1] = 0;
                }
                else
                {
                    joyType[JOY_2] = JOY_TYPE_UNKNOWN;
                    joyState[JOY_2] = 0;
                }

                gport = 0xFFFF;

                SYS_setInterruptMaskLevel(3);       /* External int disallowed */
            }

            portSupport[port] = JOY_SUPPORT_OFF;
        }
    }
    else portSupport[port] = support;
}


u8 JOY_getPortType(u16 port)
{
    if (port > PORT_2) return PORT_TYPE_UNKNOWN;

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
    else if (joy < JOY_NUM) return joyState[joy];

    return 0;
}


s16 JOY_readJoypadX(u16 joy)
{
    if (joy < JOY_NUM) return joyAxisX[joy];

    return 0;
}

s16 JOY_writeJoypadX(u16 joy, u16 pos)
{
    if (joy < JOY_NUM) joyAxisX[joy]=pos;

    return 0;
}

s16 JOY_readJoypadY(u16 joy)
{
    if (joy < JOY_NUM) return joyAxisY[joy];

    return 0;
}

s16 JOY_writeJoypadY(u16 joy, u16 pos)
{
    if (joy < JOY_NUM) joyAxisY[joy] = pos;

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

        // more than 1 frame remaining ? directly use SYS_doVBlankProcess()
        if ((maxtime - current) >= 20) SYS_doVBlankProcess();
        // just update JOY states
        else JOY_update();

        if (joy == JOY_ALL)
        {
            u16 i;

            i = JOY_NUM;
            while(i--)
            {
                state = joyState[i] & btn;
                if (state) return state;
            }
        }
        else
        {
            state = joyState[joy] & btn;
            if (state) return state;
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


static u16 read3Btn(u16 port)
{
    vu8 *pb;
    u16 val;
    Z80_STATE_DECL

    pb = (vu8 *)0xa10003 + port*2;

    Z80_STATE_SAVE
    val = TH_CONTROL_PHASE(pb);                   /* - 0 s a 0 0 d u - 1 c b r l d u */
    Z80_STATE_RESTORE

    val = ((val & 0x3000) >> 6) | (val & 0x003F); /* 0 0 0 0 0 0 0 0 s a c b r l d u */
    val ^= 0x00FF;                                /* 0 0 0 0 0 0 0 0 S A C B R L D U */

    return val | (JOY_TYPE_PAD3 << JOY_TYPE_SHIFT);
}

static u16 read6Btn(u16 port)
{
    vu8 *pb;
    u16 val, v1, v2;
    Z80_STATE_DECL

    pb = (vu8 *)0xa10003 + port*2;

    Z80_STATE_SAVE
    v1 = TH_CONTROL_PHASE(pb);                    /* - 0 s a 0 0 d u - 1 c b r l d u */
    val = TH_CONTROL_PHASE(pb);                   /* - 0 s a 0 0 d u - 1 c b r l d u */
    v2 = TH_CONTROL_PHASE(pb);                    /* - 0 s a 0 0 0 0 - 1 c b m x y z */
    val = TH_CONTROL_PHASE(pb);                   /* - 0 s a 1 1 x x - 1 c b r l d u */
                                                  /* x should be read as 1 on a 6 button controller but in some case we read 0 so take care of that */
    Z80_STATE_RESTORE

    // On a six-button controller, bits 0-3 of the high byte will always read 0.
    // This corresponds to up + down on a 3-button controller, on some controller or emulator that is a possible combination so we try
    // to compensante using last TH low read which should return 1111 on 6 buttons controller (not 100% though).
    if (((v2 & 0x0F00) != 0x0000) || ((val & 0x0C00) == 0x0000)) v2 = (JOY_TYPE_PAD3 << JOY_TYPE_SHIFT) | 0x0F00; /* three button pad */
    else v2 = (JOY_TYPE_PAD6 << JOY_TYPE_SHIFT) | ((v2 & 0x000F) << 8); /* six button pad */

    val = ((v1 & 0x3000) >> 6) | (v1 & 0x003F);   /* 0 0 0 0 0 0 0 0 s a c b r l d u  */
    val |= v2;                                    /* 0 0 0 1 m x y z s a c b r l d u  or  0 0 0 0 1 1 1 1 s a c b r l d u */
    val ^= 0x0FFF;                                /* 0 0 0 1 M X Y Z S A C B R L D U  or  0 0 0 0 0 0 0 0 S A C B R L D U */

    return val;
}

static s16 start3lhs(u16 port, u8 *hdr, u16 len)
{
    vu8 *pb;
    u16 i;
    Z80_STATE_DECL

    retry = 255;
    phase = 0x20; /* selected */

    Z80_STATE_SAVE

    /* make sure port direction is correct */
    pb = (vu8 *)0xa10009 + port*2;
    *pb = 0x60;

    /* make sure port is deselected to start at first phase */
    pb = (vu8 *)0xa10003 + port*2;
    *pb = 0x60;
    asm volatile ("nop");
    asm volatile ("nop");

    /* quick check for at least initial mouse/tap value to cut the amount
     * of time spent if a mouse/tap isn't really plugged in, but the game
     * still activates the support
     */
    i = *pb & 0x0F;
    if ((i != 0) && (i != 3))
    {
        Z80_STATE_RESTORE
        return -1;
    }

    hdr[0] = THREELINE_HANDSHAKE(pb, 0x60);
    if (retry)
    {
        for (i = 1; i < len; i++)
        {
            hdr[i] = THREELINE_HANDSHAKE(pb, phase);
            phase ^= 0x20;
            if (!retry) break; /* timeout */
        }
    }

    if (!retry) *pb = 0x60; /* timeout - end request */

    Z80_STATE_RESTORE

    return retry ? 0 : -1;
}


static u16 readMouse(u16 port)
{
    vu8 *pb;
    u16 val, i, mx, my;
    s16 sts;
    u8 hdr[4], md[6];
    Z80_STATE_DECL

    pb = (vu8 *)0xa10003 + port*2;
    val = (JOY_TYPE_MOUSE << JOY_TYPE_SHIFT);

    sts = start3lhs(port, hdr, 4);

    // only need to check 2 first nibbles
    if ((sts == 0) &&
        (hdr[0] == 0x00) &&
        (hdr[1] == 0x0B))
//    if ((sts == 0) &&
//        (hdr[0] == 0x00) &&
//        (hdr[1] == 0x0B) &&
//        (hdr[2] == 0x0F) &&
//        (hdr[3] == 0x0F))
    {

        Z80_STATE_SAVE
        /* handle mouse */
        for (i=0; i<6; i++)
        {
            md[i] = THREELINE_HANDSHAKE(pb, phase);
            phase ^= 0x20;
            if (!retry)
                break; /* timeout */
        }
        Z80_STATE_RESTORE

        if (i == 6)
        {
            if (md[0] & 0x04)
                mx = 256; /* x overflow */
            else
                mx = md[2]<<4 | md[3];
            if (md[0] & 0x01)
                mx |= mx ? 0xFF00 : 0xFFFF; /* x sign extend */
            if (md[0] & 0x08)
                my = 256; /* y overflow */
            else
                my = md[4]<<4 | md[5];
            if (md[0] & 0x02)
                my |= my ? 0xFF00 : 0xFFFF; /* y sign extend */
            joyAxisX[port] += (s16)mx;
            joyAxisY[port] -= (s16)my;

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

    Z80_STATE_SAVE
    *pb = 0x60; /* end request */
    Z80_STATE_RESTORE

    return val;
}

static void readTeamPlayer(u16 port)
{
    static const u16 xlt_all[2][4] =
    {
        { JOY_1, JOY_3, JOY_4, JOY_5 },
        { JOY_2, JOY_6, JOY_7, JOY_8 }
    };

    vu8 *pb;
    u16 val = 0, v1 = 0, v2 = 0, v3 = 0;
    u16 mx, my, typ = JOY_TYPE_UNKNOWN, i, j;
    s16 sts;
    u16 change;
    u8 hdr[8], md[6];
    const u16 *xlt_port = xlt_all[port];
    Z80_STATE_DECL

    pb = (vu8 *)0xa10003 + port*2;

    sts = start3lhs(port, hdr, 8);

    // TeamPlayer ?
    if ((sts == 0) && (hdr[0] == 0x03) && (hdr[1] == 0x0F) && (hdr[2] == 0x00) && (hdr[3] == 0x00))
    {
        /* handle Sega Tap */
        for (j = 0; j < 4; j++)
        {
            const u16 xlt = xlt_port[j];

            switch(hdr[4 + j])
            {
                case JOY_TYPE_UNKNOWN:
                    val = 0;
                    typ = JOY_TYPE_UNKNOWN;
                    break;

                case JOY_TYPE_PAD3:
                    Z80_STATE_SAVE
                    v1 = THREELINE_HANDSHAKE(pb, phase); /* r l d u */
                    phase ^= 0x20;
                    if (retry)
                    {
                        v2 = THREELINE_HANDSHAKE(pb, phase); /* s a c b */
                        phase ^= 0x20;
                    }
                    Z80_STATE_RESTORE
                    if (!retry) break; /* timeout */

                    val = (v2 << 4) | v1;
                    val ^= 0x00FF; /* 0 0 0 0 0 0 0 0 S A C B R L D U */
                    typ = JOY_TYPE_PAD3;
                    break;

                case JOY_TYPE_PAD6:
                    Z80_STATE_SAVE
                    v1 = THREELINE_HANDSHAKE(pb, phase); /* r l d u */
                    phase ^= 0x20;
                    if (retry)
                    {
                        v2 = THREELINE_HANDSHAKE(pb, phase); /* s a c b */
                        phase ^= 0x20;
                        if (retry)
                        {
                            v3 = THREELINE_HANDSHAKE(pb, phase); /* m x y z */
                            phase ^= 0x20;
                        }
                    }
                    Z80_STATE_RESTORE
                    if (!retry) break; /* timeout */

                    val = (v3 << 8) | (v2 << 4) | v1;
                    val ^= 0x0FFF; /* 0 0 0 1 M X Y Z S A C B R L D U */
                    typ = JOY_TYPE_PAD6;
                    break;

                case JOY_TYPE_MOUSE:
                    Z80_STATE_SAVE
                    for (i=0; i<6; i++)
                    {
                        md[i] = THREELINE_HANDSHAKE(pb, phase);
                        phase ^= 0x20;
                        if (!retry) break; /* timeout */
                    }
                    Z80_STATE_RESTORE
                    if (!retry) break;

                    val = 0;
                    if (md[0] & 0x04) mx = 256; /* x overflow */
                    else mx = (md[2] << 4) | md[3];
                    if (md[0] & 0x01) mx |= 0xFF00; /* x sign extend */
                    if (md[0] & 0x08) my = 256; /* y overflow */
                    else my = (md[4] << 4) | md[5];
                    if (md[0] & 0x02) my |= 0xFF00; /* y sign extend */
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

            if (!retry) break; /* timeout */

            joyType[xlt] = typ;
            change = joyState[xlt] ^ val;
            joyState[xlt] = val;
            if (change) joyEventCB(xlt, change, val);
        }
    }

    Z80_STATE_SAVE
    *pb = 0x60; /* end request */
    Z80_STATE_RESTORE
}

static void readEa4WayPlay()
{
    static const u16 xlt[4] = { JOY_1, JOY_3, JOY_4, JOY_5 };

    vu8 *pb;
    u16 change;
    u16 val, v1, v2, typ, ind, i;
    Z80_STATE_DECL

    for (i=0; i<4; i++)
    {
        pb = (vu8 *)0xa10005;

        Z80_STATE_SAVE

        *pb = (i<<4) | 0x0C;                          /* select port i */

        pb = (vu8 *)0xa10003;
        v1 = TH_CONTROL_PHASE(pb);                    /* - 0 s a 0 0 d u - 1 c b r l d u */

        if (v1 & 0x0C00)
        {
            Z80_STATE_RESTORE
            continue;                                 /* no pad */
        }

        val = TH_CONTROL_PHASE(pb);                   /* - 0 s a 0 0 d u - 1 c b r l d u */
        v2 = TH_CONTROL_PHASE(pb);                    /* - 0 s a 0 0 0 0 - 1 c b m x y z */
        val = TH_CONTROL_PHASE(pb);                   /* - 0 s a 1 1 1 1 - 1 c b r l d u */

        pb = (vu8 *)0xa10005;
        *pb = 0x7C;                                   /* deselect port */

        Z80_STATE_RESTORE

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

        joyType[ind] = typ;
        change = joyState[ind] ^ val;
        joyState[ind] = val;
        if (joyEventCB && change) joyEventCB(ind, change, val);
    }
}

static void readLightgun(u16 port)
{
    vu8 *pb;
    u16 newstate, change;
    u16 val = 0;
    u32 tmp;
    Z80_STATE_DECL

    pb = (vu8 *)0xa10003 + port*2;
    if (portType[port] == PORT_TYPE_JUSTIFIER)
    {
        Z80_STATE_SAVE
        *pb = gun | 0x10;                   /* deselect gun */
        Z80_STATE_RESTORE
    }

    if (!extSet)
    {
        /* no light sense => gun not pointed at screen */
        if (!gun)
        {
            /* blue gun or menacer or phaser*/
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
        Z80_STATE_SAVE
        *pb = 0xFF;                         /* deassert RST */
        Z80_STATE_RESTORE
        asm volatile ("moveq #20,%0\n"
            "1:\n\t"
            "dbra %0,1b\n\t"
            : "=d" (tmp) : : "cc"
        );
        Z80_STATE_SAVE
        *pb = 0xDF;                         /* assert RST long */
        Z80_STATE_RESTORE
        asm volatile ("moveq #60,%0\n"
            "1:\n\t"
            "dbra %0,1b\n\t"
            : "=d" (tmp) : : "cc"
        );
        Z80_STATE_SAVE
        *pb = 0xFF;                         /* deassert RST */
        asm volatile ("nop");
        *pb = 0xDF;                         /* assert RST long */
        Z80_STATE_RESTORE
        asm volatile ("move.w #1080,%0\n"
            "1:\n\t"
            "dbra %0,1b\n\t"
            : "=d" (tmp) : : "cc"
        );
        Z80_STATE_SAVE
        val = *pb;
        *pb = 0xFF;                         /* deassert RST */
        asm volatile ("nop");
        *pb = 0xCF;                         /* assert RST short - clear counter */
        Z80_STATE_RESTORE

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
    else if (portType[port] == PORT_TYPE_JUSTIFIER)
    {
        Z80_STATE_SAVE
        *pb = 0x00;                         /* select blue gun */
        val = *pb;
        val = *pb;                          /* read twice... probably a setup delay thing */
        *pb = 0x10;                         /* deselect blue gun */
        Z80_STATE_RESTORE

        newstate = 0;
        if (~val & 2) newstate |= BUTTON_START;
        if (~val & 1) newstate |= BUTTON_A;

        change = joyState[port] ^ newstate;
        joyType[port] = JOY_TYPE_JUSTIFIER;
        joyState[port] = newstate;
        if ((joyEventCB) && (change)) joyEventCB(port, change, newstate);

        if (joyType[port ? JOY_6 : JOY_3] == JOY_TYPE_JUSTIFIER)
        {
            Z80_STATE_SAVE
            *pb = 0x20;                     /* select red gun */
            val = *pb;
            val = *pb;                      /* read twice... probably a setup delay thing */
            *pb = 0x30;                     /* deselect red gun */
            Z80_STATE_RESTORE

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

        Z80_STATE_SAVE
        *pb = gun | 0x10;                   /* deselect gun */
        *pb = gun;                          /* leave gun selected to get light sense input */
        Z80_STATE_RESTORE
    }
    else
    {
        /* check Phaser trigger */
        Z80_STATE_SAVE
        val = *pb;                          /* - - - t - - - - */
        Z80_STATE_RESTORE

        newstate = 0;
        if (~val & 0x10) newstate = BUTTON_A;

        change = joyState[port] ^ newstate;
        joyType[port] = JOY_TYPE_PHASER;
        joyState[port] = newstate;
        if ((joyEventCB) && (change)) joyEventCB(port, change, newstate);
    }

    extSet = 0;                             /* clear light sensed flag */
}

static u16 readTrackball(u16 port)
{
    vu8 *pb;
    u8 mx, my, v1, v2, v3, v4;
    u16 val;
    u32 tmp;
    Z80_STATE_DECL

    pb = (vu8 *)0xa10003 + port*2;
    val = (JOY_TYPE_TRACKBALL << JOY_TYPE_SHIFT);

    Z80_STATE_SAVE
    *pb = 0x00; /* TH (select) low */
    Z80_STATE_RESTORE
    /* ~ 84 us */
    asm volatile ("moveq #63,%0\n"
        "1:\n\t"
        "dbra %0,1b\n\t"
        : "=d" (tmp) : : "cc"
    );
    Z80_STATE_SAVE
    v1 = *pb;
    *pb = 0x40; /* TH (select) high */
    Z80_STATE_RESTORE
    /* ~ 42 us */
    asm volatile ("moveq #31,%0\n"
        "1:\n\t"
        "dbra %0,1b\n\t"
        : "=d" (tmp) : : "cc"
    );
    Z80_STATE_SAVE
    v2 = *pb;
    *pb = 0x00; /* TH (select) low */
    Z80_STATE_RESTORE
    /* ~ 42 us */
    asm volatile ("moveq #31,%0\n"
        "1:\n\t"
        "dbra %0,1b\n\t"
        : "=d" (tmp) : : "cc"
    );
    Z80_STATE_SAVE
    v3 = *pb;
    *pb = 0x40; /* TH (select) high */
    Z80_STATE_RESTORE
    /* ~ 42 us */
    asm volatile ("moveq #31,%0\n"
        "1:\n\t"
        "dbra %0,1b\n\t"
        : "=d" (tmp) : : "cc"
    );
    Z80_STATE_SAVE
    v4 = *pb;
    Z80_STATE_RESTORE

    mx = (v1 & 0x0F) << 4;
    mx |= v2 & 0x0F;
    my = (v3 & 0x0F) << 4;
    my |= v4 & 0x0F;

    joyAxisX[port] -= (s8)mx;
    joyAxisY[port] += (s8)my;

    if (~v4 & 32) val |= BUTTON_A;
    if (~v4 & 16) val |= BUTTON_START;
    if ((s8) mx < -2) val |= BUTTON_RIGHT;
    else if ((s8) mx > 2) val |= BUTTON_LEFT;
    if ((s8) my < -2) val |= BUTTON_DOWN;
    else if ((s8) my > 2) val |= BUTTON_UP;

    return val;
}


NO_INLINE void JOY_update()
{
    u16 support;
    u16 val;
    u16 newstate;
    u16 change;

    SYS_disableInts();

    support = portSupport[PORT_1];
    switch (support)
    {
        default:
        case JOY_SUPPORT_OFF:
            break;

        case JOY_SUPPORT_3BTN:
            val = read3Btn(PORT_1);
            break;

        case JOY_SUPPORT_6BTN:
            val = read6Btn(PORT_1);
            break;

        case JOY_SUPPORT_MOUSE:
            val = readMouse(PORT_1);
            break;

        case JOY_SUPPORT_TRACKBALL:
            val = readTrackball(PORT_1);
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
        case JOY_SUPPORT_PHASER:
            readLightgun(PORT_1);
            break;
    }

    if ((support > JOY_SUPPORT_OFF) && (support < JOY_SUPPORT_TEAMPLAYER))
    {
        newstate = val & BUTTON_ALL;
        change = joyState[JOY_1] ^ newstate;
        joyType[JOY_1] = val >> JOY_TYPE_SHIFT;
        joyState[JOY_1] = newstate;
        if ((joyEventCB) && (change)) joyEventCB(JOY_1, change, newstate);
    }

    support = portSupport[PORT_2];
    switch (support)
    {
        default:
        case JOY_SUPPORT_OFF:
            break;

        case JOY_SUPPORT_3BTN:
            val = read3Btn(PORT_2);
            break;

        case JOY_SUPPORT_6BTN:
            val = read6Btn(PORT_2);
            break;

        case JOY_SUPPORT_MOUSE:
            val = readMouse(PORT_2);
            break;

        case JOY_SUPPORT_TRACKBALL:
            val = readTrackball(PORT_2);
            break;

        case JOY_SUPPORT_TEAMPLAYER:
            readTeamPlayer(PORT_2);
            break;

        case JOY_SUPPORT_MENACER:
        case JOY_SUPPORT_JUSTIFIER_BLUE:
        case JOY_SUPPORT_JUSTIFIER_BOTH:
        case JOY_SUPPORT_PHASER:
            readLightgun(PORT_2);
            break;
    }

    if ((support > JOY_SUPPORT_OFF) && (support < JOY_SUPPORT_TEAMPLAYER))
    {
        newstate = val & BUTTON_ALL;
        change = joyState[JOY_2] ^ newstate;
        joyType[JOY_2] = val >> JOY_TYPE_SHIFT;
        joyState[JOY_2] = newstate;
        if ((joyEventCB) && (change)) joyEventCB(JOY_2, change, newstate);
    }

    /* restore ints */
    SYS_enableInts();

    // random seed not yet set ?
    if (!randomSeedSet)
    {
        // button pressed ? --> set random seed from elapsed time (using tick is enough)
        if (joyState[JOY_1] || joyState[JOY_2])
        {
            u16 tick = getTick();
            setRandomSeed((tick << 0) ^ (tick << 8));
        }
    }
}
