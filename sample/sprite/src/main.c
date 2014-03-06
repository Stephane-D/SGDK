#include <genesis.h>

#include "gfx.h"
#include "music.h"

#define ANIM_STAND      0
#define ANIM_WAIT       1
#define ANIM_WALK       2
#define ANIM_RUN        3
#define ANIM_BRAKE      4
#define ANIM_UP         5
#define ANIM_CROUNCH    6
#define ANIM_ROLL       7

#define MAX_SPEED       FIX16(8)
#define RUN_SPEED       FIX16(6)
#define BRAKE_SPEED     FIX16(2.5)

#define JUMP_SPEED      FIX16(-8)
#define GRAVITY         FIX16(0.4)
#define ACCEL           FIX16(0.1)
#define DE_ACCEL        FIX16(0.15)

#define MIN_POSX        FIX16(10)
#define MAX_POSX        FIX16(280)
#define MAX_POSY        FIX16(154)


// forward
static void handleInput();
static void joyEvent(u16 joy, u16 changed, u16 state);

static void updatePhysic();
static void updateAnim();

// sprite structure
Sprite sprite;

fix16 camposx;
fix16 camposy;
fix16 posx;
fix16 posy;
fix16 movx;
fix16 movy;
s16 xorder;
s16 yorder;

int main()
{
    u16 palette[64];
    u16 ind;

    // disable interrupt when accessing VDP
    SYS_disableInts();

    // initialization
    VDP_setScreenWidth320();

    // start music
    SND_startPlay_VGM(sonic_music);

    // init sprites engine
    SPR_init(256);

    // set all palette to black
    VDP_setPaletteColors(0, palette_black, 64);

    // load background
    ind = TILE_USERINDEX;
    VDP_drawImageEx(BPLAN, &bgb_image, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, ind), 0, 0, FALSE, TRUE);
    ind += bgb_image.tileset->numTile;
    VDP_drawImageEx(APLAN, &bga_image, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, ind), 0, 0, FALSE, TRUE);
    ind += bga_image.tileset->numTile;

    // VDP process done, we can re enable interrupts
    SYS_enableInts();

    camposx = FIX16(0);
    camposy = FIX16(0);
    posx = FIX16(48);
    posy = MAX_POSY;
    movx = FIX16(0);
    movy = FIX16(0);
    xorder = 0;
    yorder = 0;

    // init sonic sprite
    SPR_initSprite(&sprite, &sonic_sprite, fix16ToInt(posx), fix16ToInt(posy), TILE_ATTR(PAL2, TRUE, FALSE, FALSE));
    SPR_update(&sprite, 1);

    // prepare palettes
    memcpy(&palette[0], bgb_image.palette->data, 16 * 2);
    memcpy(&palette[16], bga_image.palette->data, 16 * 2);
    memcpy(&palette[32], sonic_sprite.palette->data, 16 * 2);

    // fade in (disable interrupts because of the passive fade in)
    SYS_disableInts();
    VDP_fadeIn(0, (3 * 16) - 1, palette, 20, FALSE);
    SYS_enableInts();

    JOY_setEventHandler(joyEvent);

    while(TRUE)
    {
        handleInput();

        updatePhysic();
        updateAnim();

        // update sprite
        SPR_update(&sprite, 1);

        VDP_waitVSync();
    }

    return 0;
}

static void updatePhysic()
{
    if (xorder > 0)
    {
        movx += ACCEL;
        // going opposite side, quick breaking
        if (movx < 0) movx += ACCEL;

        if (movx >= MAX_SPEED) movx = MAX_SPEED;
    }
    else if (xorder < 0)
    {
        movx -= ACCEL;
        // going opposite side, quick breaking
        if (movx > 0) movx -= ACCEL;

        if (movx <= -MAX_SPEED) movx = -MAX_SPEED;
    }
    else
    {
        if ((movx < FIX16(0.1)) && (movx > FIX16(-0.1)))
            movx = 0;
        if ((movx < FIX16(0.3)) && (movx > FIX16(-0.3)))
            movx -= movx >> 2;
        else
            movx -= movx >> 4;
    }

    posx += movx;
    posy += movy;

    if (movy)
    {

        if (posy > MAX_POSY)
        {
            posy = MAX_POSY;
            movy = 0;
        }
        else movy += GRAVITY;
    }

    if (posx >= MAX_POSX)
    {
        posx = MAX_POSX;
        movx = 0;
    }
    else if (posx <= MIN_POSX)
    {
        posx = MIN_POSX;
        movx = 0;
    }

    // set sprite position
    SPR_setPosition(&sprite, fix16ToInt(posx), fix16ToInt(posy));
}

static void updateAnim()
{
    // jumping
    if (movy) SPR_setAnim(&sprite, ANIM_ROLL);
    else
    {
        if (((movx >= BRAKE_SPEED) && (xorder < 0)) || ((movx <= -BRAKE_SPEED) && (xorder > 0)))
            SPR_setAnim(&sprite, ANIM_BRAKE);
        else if ((movx >= RUN_SPEED) || (movx <= -RUN_SPEED))
            SPR_setAnim(&sprite, ANIM_RUN);
        else if (movx != 0)
            SPR_setAnim(&sprite, ANIM_WALK);
        else
        {
            if (yorder < 0)
                SPR_setAnim(&sprite, ANIM_UP);
            else if (yorder > 0)
                SPR_setAnim(&sprite, ANIM_CROUNCH);
            else
                SPR_setAnim(&sprite, ANIM_STAND);
        }
    }

    if (movx > 0)
        SPR_setAttribut(&sprite, TILE_ATTR(PAL2, TRUE, FALSE, FALSE));
    else if (movx < 0)
        SPR_setAttribut(&sprite, TILE_ATTR(PAL2, TRUE, FALSE, TRUE));
}


static void handleInput()
{
    u16 value = JOY_readJoypad(JOY_1);

    if (value & BUTTON_UP) yorder = -1;
    else if (value & BUTTON_DOWN) yorder = +1;
    else yorder = 0;

    if (value & BUTTON_LEFT) xorder = -1;
    else if (value & BUTTON_RIGHT) xorder = +1;
    else xorder = 0;
}

static void joyEvent(u16 joy, u16 changed, u16 state)
{
    // START button state changed
    if (changed & BUTTON_START)
    {

    }

    if (changed & state & (BUTTON_A | BUTTON_B | BUTTON_C))
    {
        if (movy == 0) movy = JUMP_SPEED;
    }
}
