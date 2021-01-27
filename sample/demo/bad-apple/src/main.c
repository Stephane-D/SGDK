#include "genesis.h"

#include "tile.h"
#include "tilemap.h"
#include "main.h"
#include "pcm.h"

#include "sound_res.h"
#include "movie.h"

// we have 2 data packet to handle for each frame: tileset and tilemap
// ts_trans --> nb tile to transfer
// ts_decomp, tm_decomp
// flip_done

// reset:
// flip = TRUE
// ts_decomp, tm_decomp = FALSE

// loop:
//
// while(ts_decomp);    //
// ts_trans = tsUnpack();
// ts_decomp = TRUE;
// while(tm_decomp);    //
// tmUnpack();
// tm_decomp = TRUE;

// vblank:

// if (flip && (ts_decomp || tm_trans))  --> mean we have data to transfer
// {
//   if (ts_decomp)
//   {
//     ts_decomp = !tsTransfer(ts_trans);
//     ts_trans = 0;    // so we don't re-initiate tile transfer on next frame
//   }
//   else if (tm_decomp)
//     tm_decomp = !tmTransfer();
//
//   // all decompression done --> time to flip
//   if(!tm_decomp)
//     flip = FALSE;
// }

// if (time_for_flip && !flip)
//{
//   doFlip();          // flip tilemap view and flip buffers
//   setEvenPal();      // set palette for even frame
//   flip = TRUE;
//   palFrame = 2;
//}
//else if (--palFrame == 0)
//  setOddPal();      // set palette oddeven frame


static char str1[32];

static u16 pal_even[16];
static u16 pal_odd[16];

static u16 pal_step[16] =
{
    0x555,
    0x055,
    0x550,
    0x505,
    0x005,
    0x050,
    0x500,
    0x355,
    0x553,
    0x535,
    0x335,
    0x353,
    0x533,
    0x333,
    0x033,
    0x330
};

static u16 pal_step_index;

static vu8 paused;

static u16 gen_frame;
static u16 int_frame;
static u16 mov_frame;

static vu8 ts_decomp;
static vu8 tm_decomp;
static vu16 ts_trans;
static vu8 flip_done;

static bool back;
static u16 transfer_phase;

static u32 wait;
static u16 fps;
static u16 show_fps;


static void joyEvent(u16 joy, u16 changed, u16 state);
static void vblank();
static void set_palette();
static void reset();


int main()
{
    fps = 0;
    show_fps = 0;

    VDP_setPlaneSize(64, 64, TRUE);

    JOY_setEventHandler(joyEvent);
    JOY_setSupport(PORT_1, JOY_SUPPORT_3BTN);
    JOY_setSupport(PORT_2, JOY_SUPPORT_OFF);

    PAL_setPalette(0, palette_black);

    VDP_drawText("Bad apple PV demo - SGDK sample", 2, 0);

    VDP_drawText("Video: 320x224 2bpp @30FPS", 9, 3);
    VDP_drawText("Audio: 4bit ADPCM @ 13 Khz", 9, 4);
    VDP_drawText("Duration: 3m39s", 6, 5);
    VDP_drawText("Raw size: 120 MB", 6, 6);
    VDP_drawText("Optimized size: 23 MB", 0, 7);
    VDP_drawText("Packed size: 10 MB (LZ4W compression)", 3, 8);

    VDP_drawText("A --> change palette", 4, 10);
    VDP_drawText("B --> display/hide FPS meter", 4, 11);

    VDP_drawText("Press START to continue.", 2, 14);

    PAL_fadeInPalette(PAL0, font_pal_lib.data, 20, FALSE);

    JOY_waitPress(JOY_1, BUTTON_START);

    PAL_fadeOutAll(20, FALSE);

    VDP_clearTileMapRect(BG_A, 0, 0, 64, 64);
    VDP_clearTileMapRect(BG_B, 0, 0, 64, 64);

    reset();

    SYS_setVIntCallback(vblank);

    while(mov_frame < 3235)
    {
        // do nothing on pause
        while(paused);

        // audio sync
        if (mov_frame == 5)
            startPlay(music, sizeof(music), 0);

        // unpack data (synched with vblank process)
        while(ts_decomp) wait++;
        ts_trans = ts_unpack(mov_frame);
        ts_decomp = TRUE;
        while(tm_decomp) wait++;
        tm_unpack(mov_frame);
        tm_decomp = TRUE;

        // decompression completed
        mov_frame++;
    }

    while(tm_decomp);

    waitMs(1500);

    SYS_setVIntCallback(NULL);

    VDP_init();
    VDP_setPlaneSize(64, 64, TRUE);
    PAL_setPalette(PAL0, palette_black);

    VDP_drawText("Hope you enjoyed :)", 5, 10);
    VDP_drawText("@2021  Stephane Dallongeville", 9, 27);

    PAL_fadeInPalette(PAL0, font_pal_lib.data, 20, FALSE);

    SYS_setVIntCallback(vblank);

    while(TRUE);
}


static void reset()
{
    loadDriver();

    paused = FALSE;
    pal_step_index = 0;
    flip_done = TRUE;
    ts_decomp = FALSE;
    tm_decomp = FALSE;
    int_frame = 0;
    gen_frame = 0;
    mov_frame = 0;
    back = TRUE;
    transfer_phase = 0;

    set_palette();
    ts_init();
    tm_init();
}

static void joyEvent(u16 joy, u16 changed, u16 state)
{
    if (changed & state & BUTTON_A)
    {
        // change palette
        pal_step_index = (pal_step_index + 1) & 31;
        set_palette();
    }

    if (changed & state & BUTTON_B)
    {
        show_fps = !show_fps;

        if (!show_fps)
            VDP_clearText(0, 0, 2);
    }
}

static void vblank()
{
    SYS_doVBlankProcessEx(IMMEDIATELY);

    gen_frame++;
    if (int_frame != 0) int_frame--;

    if (int_frame == 2)
    {
        VDP_setPalette(PAL0, pal_even);
        fps = getFPS();
    }

    // we have data to transfer --> start transfer
    if (flip_done && ts_decomp && (transfer_phase == 0))
        transfer_phase = 1;

    // transfer ts
    if ((transfer_phase == 1) && ts_decomp)
    {
        ts_decomp = ts_transfer(ts_trans);
        ts_trans = 0;    // so we don't re-initiate tile transfer on next frame

        // transfer done ? --> next phase
        if (!ts_decomp) transfer_phase = 2;
    }
    // transfer tm
    else if ((transfer_phase == 2) && tm_decomp)
    {
        tm_decomp = tm_transfer();

        // all transfert done --> time to flip
        if (!tm_decomp)
        {
            transfer_phase = 0;
            flip_done = FALSE;
        }
    }

    // time to flip ?
    if ((int_frame == 0) && !flip_done)
    {
        // switch buffer view
        if (back)
        {
            VDP_setVerticalScroll(BG_B, 0 * 8);
            back = FALSE;
        }
        else
        {
            VDP_setVerticalScroll(BG_B, 32 * 8);
            back = TRUE;
        }

        // set first palette
        VDP_setPalette(PAL0, pal_odd);
        int_frame = 4;
        fps = getFPS();

        flip_done = TRUE;
    }

    if (show_fps)
    {
        uintToStr(fps, str1, 1);

        if (back)
        {
            VDP_clearTileMapRect(BG_B, 0, 0, 3, 1);
            VDP_drawText(str1, 0, 0);
        }
        else
        {
            VDP_clearTileMapRect(BG_B, 0, 32, 3, 1);
            VDP_drawText(str1, 0, 32);
        }
    }
}


static void set_palette()
{
    u16 v0, v1, v2, v3;

    if (pal_step_index > 15)
    {
        const u16 step = pal_step[pal_step_index & 15];

        v0 = 0xFFF;
        v1 = v0 - step;
        v2 = v1 - step;
        v3 = v2 - step;
    }
    else
    {
        const u16 step = pal_step[pal_step_index];

        v0 = 0x000;
        v1 = v0 + step;
        v2 = v1 + step;
        v3 = v2 + step;
    }


    u16 i;

    for(i = 0; i < 4; i++)
    {
        pal_odd[(i << 2) + 0] = v0;
        pal_odd[(i << 2) + 1] = v1;
        pal_odd[(i << 2) + 2] = v2;
        pal_odd[(i << 2) + 3] = v3;

        pal_even[0 + i] = v0;
        pal_even[4 + i] = v1;
        pal_even[8 + i] = v2;
        pal_even[12 + i] = v3;
    }
}
