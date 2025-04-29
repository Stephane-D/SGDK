#include <genesis.h>

#include "res/gfx.h"
#include "res/spr_res.h"

//#define TEST_SPR
#define MAX_OBJECT      80

// forward
static void init();
static void end();

static void initPartic(u16 num);
static void updatePartic(u16 num, u16 preloadedTiles, u16 realloc);
static u16 executePartic(u16 time, u16 numPartic, u16 preloadedTiles, u16 realloc);

static void updateDonut(u16 num, u16 preloadedTiles, u16 time);
static u16 executeDonut(u16 time, u16 preloadedTiles);

static void initPos(u16 num);
static void updatePos(u16 num);
static void updateAnim(u16 num);
static u16 execute(u16 time, u16 numSpr);
static void handleInput();


typedef struct
{
    Vect2D_f16 pos;
    Vect2D_f16 mov;
    u16 timer;
} MovingObject;


fix16 baseposx;
fix16 baseposy;
fix16 gravity;

static MovingObject objects[MAX_OBJECT];
static Sprite* sprites[MAX_OBJECT];
static u16 palette[64];
static u16 tileIndexes[64];

// sprites structure
static Sprite *guySprite;
static Sprite *codySprite;
static Sprite *haggarSprite;
static Sprite *andorSprite;
static Sprite *guySprite2;
static Sprite *codySprite2;
static Sprite *haggarSprite2;
static Sprite *andorSprite2;
#if !LEGACY_SPRITE_ENGINE
static Sprite *guySprite3;
static Sprite *codySprite3;
static Sprite *guySprite4;
static Sprite *codySprite4;
static Sprite *guySprite5;
static Sprite *codySprite5;
#endif

u16 executeSpritesTest(u16 *scores)
{
    u16 globalScore;
    u16 ind, i;

    globalScore = 0;

    // init sprite engine
    init();

//    SYS_setVIntAligned(FALSE);

    SYS_disableInts();
    VDP_clearPlane(BG_A, TRUE);
    VDP_drawText("40 sprites 16x16 (all dynamic)", 1, 2);
    SYS_enableInts();

    waitMs(5000);
    SYS_disableInts();
    VDP_clearPlane(BG_A, TRUE);
    SYS_enableInts();

    // initialize sprites
    for(i = 0; i < 40; i++)
    {
        Sprite* spr;

        spr = SPR_addSprite(&flare_small, 0, 0, TILE_ATTR(PAL1, FALSE, FALSE, FALSE));
        sprites[i] = spr;

        // associate object to sprite
        spr->data = (u32) &objects[i];
    }

    // set palette
    PAL_setPalette(PAL1, flare_small.palette->data, CPU);

    // init positions
    baseposx = FIX16((VDP_getScreenWidth() / 2) - 8);
    baseposy = FIX16(100);
    gravity = FIX16(0.4);
    initPartic(40);

    // execute particle bench
    *scores = executePartic(15, 40, FALSE, TRUE) / 4;
    globalScore += *scores++;
    SPR_logProfil();

    SYS_disableInts();
    // reset sprite engine (release all allocated resources)
    SPR_reset();
    SPR_clear();
    VDP_clearPlane(BG_A, TRUE);
    VDP_drawText("40 sprites 16x16 (streamed animation)", 1, 2);
    SYS_enableInts();

    waitMs(5000);
    SYS_disableInts();
    VDP_clearPlane(BG_A, TRUE);
    SYS_enableInts();

    // initialize sprites
    for(i = 0; i < 40; i++)
    {
        Sprite* spr;

        spr = SPR_addSprite(&flare_small, 0, 0, TILE_ATTR(PAL1, FALSE, FALSE, FALSE));
        sprites[i] = spr;

        // associate object to sprite
        spr->data = (u32) &objects[i];
    }

    // set palette
    PAL_setPalette(PAL1, flare_small.palette->data, CPU);

    // init positions
    baseposx = FIX16((VDP_getScreenWidth() / 2) - 8);
    baseposy = FIX16(100);
    gravity = FIX16(0.4);
    initPartic(40);

    // execute particle bench
    *scores = executePartic(15, 40, FALSE, FALSE) / 4;
    globalScore += *scores++;
    SPR_logProfil();

    SYS_disableInts();
    // reset sprite engine (release all allocated resources)
    SPR_reset();
    SPR_clear();
    VDP_clearPlane(BG_A, TRUE);
    VDP_drawText("40 sprites 16x16 (preloaded animation)", 1, 2);
    SYS_enableInts();

    waitMs(5000);
    SYS_disableInts();
    VDP_clearPlane(BG_A, TRUE);
    SYS_enableInts();

    // initialize sprites
    for(i = 0; i < 40; i++)
    {
        Sprite* spr;

        spr = SPR_addSprite(&flare_small, 0, 0, TILE_ATTR(PAL1, FALSE, FALSE, FALSE));
        sprites[i] = spr;

        // associate object to sprite
        spr->data = (u32) &objects[i];
        // disable automatic tile upload and manual VRAM tile position
        SPR_setAutoTileUpload(spr, FALSE);
        SPR_setVRAMTileIndex(spr, TILE_USER_INDEX);
    }

    // preload animation tilesets
    ind = TILE_USER_INDEX;
    for(i = 0; i < flare_small.animations[0]->numFrame; i++)
    {
        TileSet* tileset = flare_small.animations[0]->frames[i]->tileset;

        VDP_loadTileSet(tileset, ind, TRUE);
        tileIndexes[i] = ind;
        ind += tileset->numTile;
    }

    // set palette
    PAL_setPalette(PAL1, flare_small.palette->data, CPU);

    // init positions
    baseposx = FIX16((VDP_getScreenWidth() / 2) - 8);
    baseposy = FIX16(100);
    gravity = FIX16(0.4);
    initPartic(40);

    // execute particle bench
    *scores = executePartic(15, 40, TRUE, FALSE) / 4;
    globalScore += *scores++;
    SPR_logProfil();

    SYS_disableInts();
    // reset sprite engine (release all allocated resources)
    SPR_reset();
    SPR_clear();
    VDP_clearPlane(BG_A, TRUE);
    VDP_drawText("80 sprites 16x16 (streamed animation)", 1, 2);
    SYS_enableInts();

    waitMs(5000);
    SYS_disableInts();
    VDP_clearPlane(BG_A, TRUE);
    SYS_enableInts();

    // initialize sprites
    for(i = 0; i < 79; i++)
    {
        Sprite* spr;

        spr = SPR_addSprite(&flare_small, 0, 0, TILE_ATTR(PAL1, FALSE, FALSE, FALSE));
        sprites[i] = spr;

        // associate object to sprite
        spr->data = (u32) &objects[i];
        // disable delayed frame update
        SPR_setDelayedFrameUpdate(spr, FALSE);
    }

    // set palette
    PAL_setPalette(PAL1, flare_small.palette->data, CPU);

    // init positions
    baseposx = FIX16((VDP_getScreenWidth() / 2) - 8);
    baseposy = FIX16(100);
    gravity = FIX16(0.4);
    initPartic(79);

    // execute particle bench
    *scores = executePartic(15, 79, FALSE, FALSE) * 1;
    globalScore += *scores++;
    SPR_logProfil();

    SYS_disableInts();
    // reset sprite engine (release all allocated resources)
    SPR_reset();
    SPR_clear();
    VDP_clearPlane(BG_A, TRUE);
    VDP_drawText("80 sprites 16x16 (preloaded animation)", 1, 2);
    SYS_enableInts();

    waitMs(5000);
    SYS_disableInts();
    VDP_clearPlane(BG_A, TRUE);
    SYS_enableInts();

    // initialize sprites
    for(i = 0; i < 79; i++)
    {
        Sprite* spr;

        spr = SPR_addSprite(&flare_small, 0, 0, TILE_ATTR(PAL1, FALSE, FALSE, FALSE));
        sprites[i] = spr;

        // associate object to sprite
        spr->data = (u32) &objects[i];
        // disable automatic tile upload and manual VRAM tile position
        SPR_setAutoTileUpload(spr, FALSE);
        SPR_setVRAMTileIndex(spr, TILE_USER_INDEX);
    }

    // preload animation tilesets
    ind = TILE_USER_INDEX;
    for(i = 0; i < flare_small.animations[0]->numFrame; i++)
    {
        TileSet* tileset = flare_small.animations[0]->frames[i]->tileset;

        VDP_loadTileSet(tileset, ind, TRUE);
        tileIndexes[i] = ind;
        ind += tileset->numTile;
    }

    // set palette
    PAL_setPalette(PAL1, flare_small.palette->data, CPU);

    // init positions
    baseposx = FIX16((VDP_getScreenWidth() / 2) - 8);
    baseposy = FIX16(100);
    gravity = FIX16(0.4);
    initPartic(79);

    // execute particle bench
    *scores = executePartic(15, 79, TRUE, FALSE) / 2;
    globalScore += *scores++;
    SPR_logProfil();

    SYS_disableInts();
    // reset sprite engine (release all allocated resources)
    SPR_reset();
    SPR_clear();
    VDP_clearPlane(BG_A, TRUE);
    VDP_drawText("40 sprites 32x32 (streamed)", 1, 2);
    SYS_enableInts();

    waitMs(5000);
    SYS_disableInts();
    VDP_clearPlane(BG_A, TRUE);
    SYS_enableInts();

    // initialize sprites
    for(i = 0; i < 40; i++)
    {
        Sprite* spr;

        spr = SPR_addSprite(&flare_big, 0, 0, TILE_ATTR(PAL1, FALSE, FALSE, FALSE));
        sprites[i] = spr;

        // associate object to sprite
        spr->data = (u32) &objects[i];
        // disable delayed frame update
        SPR_setDelayedFrameUpdate(spr, FALSE);
    }

    // set palette
    PAL_setPalette(PAL1, flare_big.palette->data, CPU);

    // init positions
    baseposx = FIX16((VDP_getScreenWidth() / 2) - 16);
    baseposy = FIX16(100);
    gravity = FIX16(0.4);
    initPartic(40);

    // execute particle bench
    *scores = executePartic(15, 40, FALSE, FALSE) * 1;
    globalScore += *scores++;
    SPR_logProfil();

    SYS_disableInts();
    // reset sprite engine (release all allocated resources)
    SPR_reset();
    SPR_clear();
    VDP_clearPlane(BG_A, TRUE);
    VDP_drawText("40 sprites 32x32 (preloaded)", 1, 2);
    SYS_enableInts();

    waitMs(5000);
    SYS_disableInts();
    VDP_clearPlane(BG_A, TRUE);
    SYS_enableInts();

    // initialize sprites
    for(i = 0; i < 40; i++)
    {
        Sprite* spr;

        spr = SPR_addSprite(&flare_big, 0, 0, TILE_ATTR(PAL1, FALSE, FALSE, FALSE));
        sprites[i] = spr;

        // associate object to sprite
        spr->data = (u32) &objects[i];
        // disable automatic tile upload and set manual VRAM tile position
        SPR_setAutoTileUpload(spr, FALSE);
        SPR_setVRAMTileIndex(spr, TILE_USER_INDEX);
    }

    // preload animation tilesets
    ind = TILE_USER_INDEX;
    for(i = 0; i < flare_big.animations[0]->numFrame; i++)
    {
        TileSet* tileset = flare_big.animations[0]->frames[i]->tileset;

        VDP_loadTileSet(tileset, ind, TRUE);
        tileIndexes[i] = ind;
        ind += tileset->numTile;
    }

    // set palette
    PAL_setPalette(PAL1, flare_big.palette->data, CPU);

    // init positions
    baseposx = FIX16((VDP_getScreenWidth() / 2) - 16);
    baseposy = FIX16(100);
    gravity = FIX16(0.4);
    initPartic(40);

    // execute particle bench
    *scores = executePartic(15, 40, TRUE, FALSE) / 4;
    globalScore += *scores++;
    SPR_logProfil();

    SYS_disableInts();
    // reset sprite engine (release all allocated resources)
    SPR_reset();
    SPR_clear();
    VDP_clearPlane(BG_A, TRUE);
    VDP_drawText("Donut animation (streamed)", 1, 2);
    SYS_enableInts();

    waitMs(5000);
    SYS_disableInts();
    VDP_clearPlane(BG_A, TRUE);
    SYS_enableInts();

    // set palette
    PAL_setPalette(PAL1, donut.palette->data, CPU);

    // execute donut bench
    *scores = executeDonut(20, FALSE) / 2;
    globalScore += *scores++;
    SPR_logProfil();

    SYS_disableInts();
    // reset sprite engine (release all allocated resources)
    SPR_reset();
    SPR_clear();
    VDP_clearPlane(BG_A, TRUE);
    VDP_drawText("Donut animation (preloaded)", 1, 2);
    SYS_enableInts();

    waitMs(5000);
    SYS_disableInts();
    VDP_clearPlane(BG_A, TRUE);
    SYS_enableInts();

    // preload animation tilesets
    ind = TILE_USER_INDEX;
    for(i = 0; i < donut.animations[0]->numFrame; i++)
    {
        TileSet* tileset = donut.animations[0]->frames[i]->tileset;

        VDP_loadTileSet(tileset, ind, TRUE);
        tileIndexes[i] = ind;
        ind += tileset->numTile;
    }

    // set palette
    PAL_setPalette(PAL1, donut.palette->data, CPU);

    // execute particle bench
    *scores = executeDonut(20, TRUE) / 4;
    globalScore += *scores++;
    SPR_logProfil();

    SYS_disableInts();
    // reset sprite engine (release all allocated resources)
    SPR_reset();
    SPR_clear();
    VDP_clearPlane(BG_A, TRUE);
    VDP_drawText("Compressed big sprites test (light)", 1, 2);
    SYS_enableInts();

    waitMs(5000);
    SYS_disableInts();
    VDP_clearPlane(BG_A, TRUE);
    PAL_setColors(0, palette_black, 64, CPU);
    SYS_enableInts();

    // create sprites structures
    guySprite = SPR_addSprite(&guy_packed_sprite, 0, 0, TILE_ATTR(PAL0, FALSE, FALSE, FALSE));
    codySprite = SPR_addSprite(&cody_packed_sprite, 0, 0, TILE_ATTR(PAL1, FALSE, FALSE, FALSE));
    haggarSprite = SPR_addSprite(&haggar_packed_sprite, 0, 0, TILE_ATTR(PAL2, FALSE, FALSE, FALSE));
    andorSprite = SPR_addSprite(&andor_packed_sprite, 0, 0, TILE_ATTR(PAL3, FALSE, FALSE, FALSE));

    SPR_update();
    SYS_doVBlankProcess();

    // desync frame timer so update happen on different frame
    haggarSprite->timer = 1;
    andorSprite->timer = 2;

    // we want to compute per hardware sprite visibility for these sprites
    SPR_setVisibility(guySprite, AUTO_SLOW);
    SPR_setVisibility(codySprite, AUTO_SLOW);
    SPR_setVisibility(haggarSprite, AUTO_SLOW);
    SPR_setVisibility(andorSprite, AUTO_SLOW);

    SPR_update();
    SYS_doVBlankProcess();

    sprites[0] = guySprite;
    sprites[1] = codySprite;
    sprites[2] = haggarSprite;
    sprites[3] = andorSprite;

    sprites[0]->data = (u32) &objects[0];
    sprites[1]->data = (u32) &objects[1];
    sprites[2]->data = (u32) &objects[2];
    sprites[3]->data = (u32) &objects[3];

    // init position for 4 sprites
    initPos(4);

    SPR_update();
    SYS_doVBlankProcess();

    // prepare palettes
    memcpy(&palette[0], guy_sprite.palette->data, 16 * 2);
    memcpy(&palette[16], cody_sprite.palette->data, 16 * 2);
    memcpy(&palette[32], haggar_sprite.palette->data, 16 * 2);
    memcpy(&palette[48], andor_sprite.palette->data, 16 * 2);
    // keep background color black
    palette[0] = 0;

    // set palette
    SYS_disableInts();
    PAL_setColors(0, palette, 64, CPU);
    SYS_enableInts();

    // execute sprite bench
    *scores = execute(25, 4) / 8;
    globalScore += *scores++;
    SPR_logProfil();

    SYS_disableInts();
    // reset sprite engine (release all allocated resources)
    SPR_reset();
    SPR_clear();
    VDP_clearPlane(BG_A, TRUE);
    VDP_drawText("Compressed big sprites test (heavy) ", 1, 2);
    SYS_enableInts();

    waitMs(5000);
    SYS_disableInts();
    VDP_clearPlane(BG_A, TRUE);
    PAL_setColors(0, palette_black, 64, CPU);
    SYS_enableInts();

    // create sprites structures
    guySprite = SPR_addSprite(&guy_packed_sprite, 0, 0, TILE_ATTR(PAL0, FALSE, FALSE, FALSE));
    codySprite = SPR_addSprite(&cody_packed_sprite, 0, 0, TILE_ATTR(PAL1, FALSE, FALSE, FALSE));
    haggarSprite = SPR_addSprite(&haggar_packed_sprite, 0, 0, TILE_ATTR(PAL2, FALSE, FALSE, FALSE));
    andorSprite = SPR_addSprite(&andor_packed_sprite, 0, 0, TILE_ATTR(PAL3, FALSE, FALSE, FALSE));
    guySprite2 = SPR_addSprite(&guy_packed_sprite, 0, 0, TILE_ATTR(PAL0, FALSE, FALSE, FALSE));
    codySprite2 = SPR_addSprite(&cody_packed_sprite, 0, 0, TILE_ATTR(PAL1, FALSE, FALSE, FALSE));
    haggarSprite2 = SPR_addSprite(&haggar_packed_sprite, 0, 0, TILE_ATTR(PAL2, FALSE, FALSE, FALSE));
    andorSprite2 = SPR_addSprite(&andor_packed_sprite, 0, 0, TILE_ATTR(PAL3, FALSE, FALSE, FALSE));

    SPR_update();
    SYS_doVBlankProcess();

    // desync frame timer so update happen on different frame
    guySprite2->timer = 1;
    codySprite2->timer = 1;
    haggarSprite2->timer = 1;
    andorSprite->timer = 2;
    andorSprite2->timer = 2;

    // we want to compute per hardware sprite visibility for these sprites
    SPR_setVisibility(guySprite, AUTO_SLOW);
    SPR_setVisibility(codySprite, AUTO_SLOW);
    SPR_setVisibility(haggarSprite, AUTO_SLOW);
    SPR_setVisibility(andorSprite, AUTO_SLOW);
    SPR_setVisibility(guySprite2, AUTO_SLOW);
    SPR_setVisibility(codySprite2, AUTO_SLOW);
    SPR_setVisibility(haggarSprite2, AUTO_SLOW);
    SPR_setVisibility(andorSprite2, AUTO_SLOW);

    SPR_update();
    SYS_doVBlankProcess();

    sprites[0] = guySprite;
    sprites[1] = codySprite;
    sprites[2] = haggarSprite;
    sprites[3] = andorSprite;
    sprites[4] = guySprite2;
    sprites[5] = codySprite2;
    sprites[6] = haggarSprite2;
    sprites[7] = andorSprite2;

    sprites[0]->data = (u32) &objects[0];
    sprites[1]->data = (u32) &objects[1];
    sprites[2]->data = (u32) &objects[2];
    sprites[3]->data = (u32) &objects[3];
    sprites[4]->data = (u32) &objects[4];
    sprites[5]->data = (u32) &objects[5];
    sprites[6]->data = (u32) &objects[6];
    sprites[7]->data = (u32) &objects[7];

    // init position for 8 sprites
    initPos(8);

    SPR_update();
    SYS_doVBlankProcess();

    // prepare palettes
    memcpy(&palette[0], guy_sprite.palette->data, 16 * 2);
    memcpy(&palette[16], cody_sprite.palette->data, 16 * 2);
    memcpy(&palette[32], haggar_sprite.palette->data, 16 * 2);
    memcpy(&palette[48], andor_sprite.palette->data, 16 * 2);
    // keep background color black
    palette[0] = 0;

    // set palette
    SYS_disableInts();
    PAL_setColors(0, palette, 64, CPU);
    SYS_enableInts();

    // execute sprite bench
    *scores = execute(25, 8) / 8;
    globalScore += *scores++;
    SPR_logProfil();

    SYS_disableInts();
    // reset sprite engine (release all allocated resources)
    SPR_reset();
    SPR_clear();
    VDP_clearPlane(BG_A, TRUE);
    VDP_drawText("Big sprites test", 1, 2);
    SYS_enableInts();

    waitMs(5000);
    SYS_disableInts();
    VDP_clearPlane(BG_A, TRUE);
    PAL_setColors(0, palette_black, 64, CPU);
    SYS_enableInts();

    // create sprites structures
    guySprite = SPR_addSprite(&guy_sprite, 0, 0, TILE_ATTR(PAL0, FALSE, FALSE, FALSE));
    codySprite = SPR_addSprite(&cody_sprite, 0, 0, TILE_ATTR(PAL1, FALSE, FALSE, FALSE));
    haggarSprite = SPR_addSprite(&haggar_sprite, 0, 0, TILE_ATTR(PAL2, FALSE, FALSE, FALSE));
    andorSprite = SPR_addSprite(&andor_sprite, 0, 0, TILE_ATTR(PAL3, FALSE, FALSE, FALSE));

    SPR_update();
    SYS_doVBlankProcess();

    // desync frame timer so update happen on different frame
    haggarSprite->timer = 1;
    andorSprite->timer = 2;

    // we want to compute per hardware sprite visibility for these sprites
    SPR_setVisibility(guySprite, AUTO_SLOW);
    SPR_setVisibility(codySprite, AUTO_SLOW);
    SPR_setVisibility(haggarSprite, AUTO_SLOW);
    SPR_setVisibility(andorSprite, AUTO_SLOW);

    SPR_update();
    SYS_doVBlankProcess();

    sprites[0] = guySprite;
    sprites[1] = codySprite;
    sprites[2] = haggarSprite;
    sprites[3] = andorSprite;

    sprites[0]->data = (u32) &objects[0];
    sprites[1]->data = (u32) &objects[1];
    sprites[2]->data = (u32) &objects[2];
    sprites[3]->data = (u32) &objects[3];

    // init position for 4 sprites
    initPos(4);

    SPR_update();
    SYS_doVBlankProcess();

    // prepare palettes
    memcpy(&palette[0], guy_sprite.palette->data, 16 * 2);
    memcpy(&palette[16], cody_sprite.palette->data, 16 * 2);
    memcpy(&palette[32], haggar_sprite.palette->data, 16 * 2);
    memcpy(&palette[48], andor_sprite.palette->data, 16 * 2);
    // keep background color black
    palette[0] = 0;

    // set palette
    SYS_disableInts();
    PAL_setColors(0, palette, 64, CPU);
    SYS_enableInts();

    // execute sprite bench
    *scores = execute(25, 4) / 8;
    globalScore += *scores++;
    SPR_logProfil();

    SYS_disableInts();
    // reset sprite engine (release all allocated resources)
    SPR_reset();
    SPR_clear();
    VDP_clearPlane(BG_A, TRUE);
    VDP_drawText("Big sprites test (heavy) ", 1, 2);
    SYS_enableInts();

    waitMs(5000);
    SYS_disableInts();
    VDP_clearPlane(BG_A, TRUE);
    PAL_setColors(0, palette_black, 64, CPU);
    SYS_enableInts();

    // create sprites structures
    guySprite = SPR_addSprite(&guy_sprite, 0, 0, TILE_ATTR(PAL0, FALSE, FALSE, FALSE));
    codySprite = SPR_addSprite(&cody_sprite, 0, 0, TILE_ATTR(PAL1, FALSE, FALSE, FALSE));
    haggarSprite = SPR_addSprite(&haggar_sprite, 0, 0, TILE_ATTR(PAL2, FALSE, FALSE, FALSE));
    andorSprite = SPR_addSprite(&andor_sprite, 0, 0, TILE_ATTR(PAL3, FALSE, FALSE, FALSE));
    guySprite2 = SPR_addSprite(&guy_sprite, 0, 0, TILE_ATTR(PAL0, FALSE, FALSE, FALSE));
    codySprite2 = SPR_addSprite(&cody_sprite, 0, 0, TILE_ATTR(PAL1, FALSE, FALSE, FALSE));
    haggarSprite2 = SPR_addSprite(&haggar_sprite, 0, 0, TILE_ATTR(PAL2, FALSE, FALSE, FALSE));
    andorSprite2 = SPR_addSprite(&andor_sprite, 0, 0, TILE_ATTR(PAL3, FALSE, FALSE, FALSE));

    SPR_update();
    SYS_doVBlankProcess();

    // desync frame timer so update happen on different frame
    guySprite2->timer = 1;
    codySprite2->timer = 1;
    haggarSprite2->timer = 1;
    andorSprite->timer = 2;
    andorSprite2->timer = 2;

    // we want to compute per hardware sprite visibility for these sprites
    SPR_setVisibility(guySprite, AUTO_SLOW);
    SPR_setVisibility(codySprite, AUTO_SLOW);
    SPR_setVisibility(haggarSprite, AUTO_SLOW);
    SPR_setVisibility(andorSprite, AUTO_SLOW);
    SPR_setVisibility(guySprite2, AUTO_SLOW);
    SPR_setVisibility(codySprite2, AUTO_SLOW);
    SPR_setVisibility(haggarSprite2, AUTO_SLOW);
    SPR_setVisibility(andorSprite2, AUTO_SLOW);

    SPR_update();
    SYS_doVBlankProcess();

    sprites[0] = guySprite;
    sprites[1] = codySprite;
    sprites[2] = haggarSprite;
    sprites[3] = andorSprite;
    sprites[4] = guySprite2;
    sprites[5] = codySprite2;
    sprites[6] = haggarSprite2;
    sprites[7] = andorSprite2;

    sprites[0]->data = (u32) &objects[0];
    sprites[1]->data = (u32) &objects[1];
    sprites[2]->data = (u32) &objects[2];
    sprites[3]->data = (u32) &objects[3];
    sprites[4]->data = (u32) &objects[4];
    sprites[5]->data = (u32) &objects[5];
    sprites[6]->data = (u32) &objects[6];
    sprites[7]->data = (u32) &objects[7];

    // init position for 8 sprites
    initPos(8);

    SPR_update();
    SYS_doVBlankProcess();

    // prepare palettes
    memcpy(&palette[0], guy_sprite.palette->data, 16 * 2);
    memcpy(&palette[16], cody_sprite.palette->data, 16 * 2);
    memcpy(&palette[32], haggar_sprite.palette->data, 16 * 2);
    memcpy(&palette[48], andor_sprite.palette->data, 16 * 2);
    // keep background color black
    palette[0] = 0;

    // set palette
    SYS_disableInts();
    PAL_setColors(0, palette, 64, CPU);
    SYS_enableInts();

    // execute sprite bench
    *scores = execute(25, 8) / 8;
    globalScore += *scores++;
    SPR_logProfil();


    SYS_disableInts();
    // reset sprite engine (release all allocated resources)
    SPR_reset();
    SPR_clear();
    VDP_clearPlane(BG_A, TRUE);
    VDP_drawText("Big sprites test (super heavy)", 1, 2);
#if LEGACY_SPRITE_ENGINE
    VDP_drawText("Not supported with legacy sprite engine", 1, 3);
#endif // LEGACY_SPRITE_ENGINE
    SYS_enableInts();

    waitMs(5000);
    SYS_disableInts();
    VDP_clearPlane(BG_A, TRUE);
    PAL_setColors(0, palette_black, 64, CPU);
    SYS_enableInts();

#if !LEGACY_SPRITE_ENGINE
    // create sprites structures
    guySprite = SPR_addSprite(&guy_slow_sprite, 0, 0, TILE_ATTR(PAL0, FALSE, FALSE, FALSE));
    codySprite = SPR_addSprite(&cody_slow_sprite, 0, 0, TILE_ATTR(PAL1, FALSE, FALSE, FALSE));
    haggarSprite = SPR_addSprite(&haggar_slow_sprite, 0, 0, TILE_ATTR(PAL2, FALSE, FALSE, FALSE));
    andorSprite = SPR_addSprite(&andor_slow_sprite, 0, 0, TILE_ATTR(PAL3, FALSE, FALSE, FALSE));
    guySprite2 = SPR_addSprite(&guy_slow_sprite, 0, 0, TILE_ATTR(PAL0, FALSE, FALSE, FALSE));
    codySprite2 = SPR_addSprite(&cody_slow_sprite, 0, 0, TILE_ATTR(PAL1, FALSE, FALSE, FALSE));
    guySprite3 = SPR_addSprite(&guy_slow_sprite, 0, 0, TILE_ATTR(PAL0, FALSE, FALSE, FALSE));
    codySprite3 = SPR_addSprite(&cody_slow_sprite, 0, 0, TILE_ATTR(PAL1, FALSE, FALSE, FALSE));
    guySprite4 = SPR_addSprite(&guy_slow_sprite, 0, 0, TILE_ATTR(PAL0, FALSE, FALSE, FALSE));
    codySprite4 = SPR_addSprite(&cody_slow_sprite, 0, 0, TILE_ATTR(PAL1, FALSE, FALSE, FALSE));
    guySprite5 = SPR_addSprite(&guy_slow_sprite, 0, 0, TILE_ATTR(PAL0, FALSE, FALSE, FALSE));
    codySprite5 = SPR_addSprite(&cody_slow_sprite, 0, 0, TILE_ATTR(PAL1, FALSE, FALSE, FALSE));

    SPR_update();
    SYS_doVBlankProcess();

    // desync frame timer so update happen on different frame
    andorSprite->timer = 1;
    guySprite2->timer = 1;
    guySprite3->timer = 1;
    codySprite2->timer = 2;
    codySprite3->timer = 2;
    guySprite4->timer = 2;
    codySprite4->timer = 3;
    guySprite5->timer = 3;
    codySprite5->timer = 3;

    // we want to compute per hardware sprite visibility for these sprites
    SPR_setVisibility(guySprite, AUTO_SLOW);
    SPR_setVisibility(codySprite, AUTO_SLOW);
    SPR_setVisibility(haggarSprite, AUTO_SLOW);
    SPR_setVisibility(andorSprite, AUTO_SLOW);
    SPR_setVisibility(guySprite2, AUTO_SLOW);
    SPR_setVisibility(codySprite2, AUTO_SLOW);
    SPR_setVisibility(guySprite3, AUTO_SLOW);
    SPR_setVisibility(codySprite3, AUTO_SLOW);
    SPR_setVisibility(guySprite4, AUTO_SLOW);
    SPR_setVisibility(codySprite4, AUTO_SLOW);
    SPR_setVisibility(guySprite5, AUTO_SLOW);
    SPR_setVisibility(codySprite5, AUTO_SLOW);

    SPR_update();
    SYS_doVBlankProcess();

    sprites[0] = guySprite;
    sprites[1] = codySprite;
    sprites[2] = haggarSprite;
    sprites[3] = andorSprite;
    sprites[4] = guySprite2;
    sprites[5] = codySprite2;
    sprites[6] = guySprite3;
    sprites[7] = codySprite3;
    sprites[8] = guySprite4;
    sprites[9] = codySprite4;
    sprites[10] = guySprite5;
    sprites[11] = codySprite5;

    sprites[0]->data = (u32) &objects[0];
    sprites[1]->data = (u32) &objects[1];
    sprites[2]->data = (u32) &objects[2];
    sprites[3]->data = (u32) &objects[3];
    sprites[4]->data = (u32) &objects[4];
    sprites[5]->data = (u32) &objects[5];
    sprites[6]->data = (u32) &objects[6];
    sprites[7]->data = (u32) &objects[7];
    sprites[8]->data = (u32) &objects[8];
    sprites[9]->data = (u32) &objects[9];
    sprites[10]->data = (u32) &objects[10];
    sprites[11]->data = (u32) &objects[11];

    // init position for 12 sprites
    initPos(12);

    SPR_update();
    SYS_doVBlankProcess();

    // prepare palettes
    memcpy(&palette[0], guy_sprite.palette->data, 16 * 2);
    memcpy(&palette[16], cody_sprite.palette->data, 16 * 2);
    memcpy(&palette[32], haggar_sprite.palette->data, 16 * 2);
    memcpy(&palette[48], andor_sprite.palette->data, 16 * 2);
    // keep background color black
    palette[0] = 0;

    // set palette
    SYS_disableInts();
    PAL_setColors(0, palette, 64, CPU);
    SYS_enableInts();

    // execute sprite bench (result not used in score)
    execute(25, 12);
#endif // LEGACY_SPRITE_ENGINE

    SPR_logProfil();

    SYS_disableInts();
    SPR_reset();
    SPR_clear();
    VDP_clearPlane(BG_A, TRUE);
    end();
    SYS_enableInts();

    return globalScore;
}


void init()
{
    // disable interrupt when accessing VDP
    SYS_disableInts();
    // DMA limit
//    DMA_setMaxTransferSize(7000);
//    DMA_setBufferSize(16000);
    // init sprites engine
//    SPR_initEx(16 * (32 + 16 + 8));
    // require about 1000 tiles to pass the last heavy big sprites test
    SPR_initEx(1024);
    // VDP process done, we can re enable interrupts
    SYS_enableInts();
}

void end()
{
    // end sprite engine
    SPR_end();
}


static void initPartic(u16 num)
{
    Sprite** sprite;
    u16 i;

    i = num;
    sprite = sprites;
    while(i--)
    {
        Sprite* s = *sprite;
        MovingObject* o = (MovingObject*) s->data;

        o->mov.x = FIX16(2) - (random() & (FIX16_FRAC_MASK << 2));
        o->mov.y = FIX16(2) + (random() & (FIX16_FRAC_MASK << 3));
        o->pos.x = baseposx + o->mov.x;
        o->pos.y = baseposy + o->mov.y;
        o->timer = i & 1;

        sprite++;
    }
}

static void updatePartic(u16 num, u16 preloadedTiles, u16 realloc)
{
    Sprite** sprite;
    fix16 minx;
    fix16 miny;
    fix16 maxx;
    u16 i;

    minx = FIX16(-20);
    maxx = FIX16(VDP_getScreenWidth() + 20);
    miny = FIX16(16);

    i = num;
    sprite = sprites;
    while(i--)
    {
        Sprite* s = *sprite;
        MovingObject* o = (MovingObject*) s->data;

        if ((o->pos.x < minx) || (o->pos.x > maxx) || (o->pos.y < miny))
        {
            // force realloc
            if (realloc)
            {
                SPR_releaseSprite(s);
                s = SPR_addSprite(&flare_small, 0, 0, TILE_ATTR(PAL1, FALSE, FALSE, FALSE));
                *sprite = s;
                // associate object to sprite
                s->data = (u32) o;
            }

            // re-init particle
            o->mov.x = FIX16(2) - (random() & (FIX16_FRAC_MASK << 2));
            o->mov.y = FIX16(2) + (random() & (FIX16_FRAC_MASK << 3));
            o->pos.x = baseposx + o->mov.x;
            o->pos.y = baseposy + o->mov.y;
        }
//        else if (o->pos.y < miny)
//        {
//            if (o->mov.y > -(gravity << 4))
//            {
//                // re-init particle
//                o->mov.x = FIX16(2) - (random() & (FIX16_FRAC_MASK << 2));
//                o->mov.y = FIX16(2) + (random() & (FIX16_FRAC_MASK << 3));
//                o->pos.x = baseposx + o->mov.x;
//                o->pos.y = baseposy + o->mov.y;
//            }
//            else
//            {
//                // handle re-bound
//                o->pos.x += o->mov.x;
//                o->pos.y -= o->mov.y;
//                o->mov.y = -o->mov.y;
//                o->mov.y >>= 1;
//            }
//        }
        else
        {
            o->pos.x += o->mov.x;
            o->pos.y += o->mov.y;
            o->mov.y -= gravity;
        }

        // increment timer
        o->timer++;
        if (o->timer >= 10) o->timer = 0;

        // update sprite position
        SPR_setPosition(s, F16_toInt(o->pos.x), 224 - F16_toInt(o->pos.y));
        // animate (30 FPS)
        if (o->timer & 1)
        {
            // use preloaded tileset
            if (preloadedTiles) SPR_setVRAMTileIndex(s, tileIndexes[o->timer >> 1]);
            // use sprite engine to deal with that
            else SPR_nextFrame(s);
        }

        sprite++;
    }
}

static u16 executePartic(u16 time, u16 numPartic, u16 preloadedTiles, u16 realloc)
{
    u32 startTime;
    u32 endTime;
    u32 freeCpuTime;
    u16 cpuLoad;

    startTime = getTime(TRUE);
    endTime = startTime + (time << 8);
    freeCpuTime = 0;

    do
    {

        updatePartic(numPartic, preloadedTiles, realloc);
        // update sprites
        SPR_update();

        VDP_showFPS(FALSE, 1, 1);
        VDP_showCPULoad(1, 2);
        SYS_doVBlankProcess();
        cpuLoad = SYS_getCPULoad();

        // 100 + (0-100)
        if (cpuLoad < 100) freeCpuTime += 100 + (100 - cpuLoad);
        // 50 + (0-50)
        else if (cpuLoad < 200) freeCpuTime += 50 + ((200 - cpuLoad) >> 1);
        // (0-50)
        else if (cpuLoad < 300) freeCpuTime += (300 - cpuLoad) >> 1;
    } while(getTime(TRUE) < endTime);

    return freeCpuTime >> 8;
}

static void updateDonut(u16 num, u16 preloadedTiles, u16 time)
{
    Sprite** sprite;
    u16 i;
    u16 remaining;
    u16 ts, ti;

    remaining = num;
    sprite = sprites;

    ti = time;
    ts = ti >> 4;
    i = min(32, remaining);
    remaining -= i;
    while(i--)
    {
        Sprite* s = *sprite;

        u16 is = i << 5;
        fix16 x = cosFix16(ti + is);
        fix16 y = sinFix16(ti + is);

        x = (x << 1) + (x >> 2);
        y = (y << 0) + (y >> 1);

        SPR_setPosition(s, (160 - 16) + x, (112 - 16) + y);

        SPR_setFrame(s, (ts + i) & 0x7);
        if (preloadedTiles)
            SPR_setVRAMTileIndex(s, tileIndexes[(ts + i) & 0x7]);

        sprite++;
    }

    ti = -time;
    ts = ti >> 4;
    i = min(16, remaining);
    remaining -= i;
    while(i--)
    {
        Sprite* s = *sprite;

        u16 is = -i << 6;
        fix16 x = cosFix16(ti + is);
        fix16 y = sinFix16(ti + is);

        x = (x << 1) - (x >> 2);
        y = (y << 0) - 0;

        SPR_setPosition(s, (160 - 16) + x, (112 - 16) + y);

        SPR_setFrame(s, (ts + i) & 0x7);
        if (preloadedTiles)
            SPR_setVRAMTileIndex(s, tileIndexes[(ts + i) & 0x7]);

        sprite++;
    }

    ti = time;
    ts = ti >> 3;
    i = min(8, remaining);
    remaining -= i;
    while(i--)
    {
        Sprite* s = *sprite;

        u16 is = i << 7;
        fix16 x = cosFix16(time + is);
        fix16 y = sinFix16(time + is);

        x = (x << 0) - 0;
        y = (y >> 1) + 0;

        SPR_setPosition(s, (160 - 16) + x, (112 - 16) + y);

		SPR_setFrame(s, (ts + i) & 0x7);
        if (preloadedTiles)
            SPR_setVRAMTileIndex(s, tileIndexes[(ts + i) & 0x7]);

        sprite++;
    }
}

static u16 executeDonut(u16 time, u16 preloadedTiles)
{
    u32 startTime;
    u32 endTime;
    u32 freeCpuTime;
    u16 cpuLoad;
    u16 frame;
    u16 num;
    u16 t;

    startTime = getTime(TRUE);
    endTime = startTime + (time << 8);
    freeCpuTime = 0;
    frame = 0;
    num = 0;
    t = 0;

    do
    {
        // disable ints
        SYS_disableInts();

        if (!(frame & 0x7))
        {
            // sprite limit not yet raised
            if (num < 56)
            {
                // add a new sprite
                Sprite* spr;

                spr = SPR_addSprite(&donut, 0, 0, TILE_ATTR(PAL1, FALSE, FALSE, FALSE));
                sprites[num] = spr;

                if (preloadedTiles)
                {
                    // disable automatic tile upload and manual VRAM tile position
                    SPR_setAutoTileUpload(spr, FALSE);
                    SPR_setVRAMTileIndex(spr, TILE_USER_INDEX);
                }

                // associate object to sprite
                spr->data = (u32) &objects[num];
                num++;
            }
        }

        updateDonut(num, preloadedTiles, t);

        // enable ints
        SYS_enableInts();

        // update sprites
        SPR_update();

        VDP_showFPS(FALSE, 1, 1);
        VDP_showCPULoad(1, 2);

        SYS_doVBlankProcess();
        cpuLoad = SYS_getCPULoad();

        // 100 + (0-100)
        if (cpuLoad < 100) freeCpuTime += 100 + (100 - cpuLoad);
        // 50 + (0-50)
        else if (cpuLoad < 200) freeCpuTime += 50 + ((200 - cpuLoad) >> 1);
        // (0-50)
        else if (cpuLoad < 300) freeCpuTime += (300 - cpuLoad) >> 1;

        t -= 4;
        frame++;
    } while(getTime(TRUE) < endTime);

    return freeCpuTime >> 8;
}

static void initPos(u16 num)
{
    Sprite** sprite;
    u16 i;

    i = num;
    sprite = sprites;
    while(i--)
    {
        Sprite* s = *sprite;
        MovingObject* o = (MovingObject*) s->data;

        o->pos.x = FIX16(32 + (random() & 0xFF));
        o->pos.y = FIX16((random() & 0x7F));
        if (random() & 1) o->mov.x = (random() & 0xF) + FIX16(1);
        else o->mov.x = -((random() & 0xF) + FIX16(1));
        if (random() & 1) o->mov.y = (random() & 0x7) + FIX16(0.7);
        else o->mov.y = -((random() & 0x7) + FIX16(0.7));

        sprite++;
    }
}

static void updatePos(u16 num)
{
    Sprite** sprite;
    fix16 minx;
    fix16 miny;
    fix16 maxx;
    fix16 maxy;
    u16 i;

    minx = FIX16(-48);
    maxx = FIX16(VDP_getScreenWidth() - 32);
    miny = FIX16(-48);
    maxy = FIX16(VDP_getScreenHeight() - 32);

    i = num;
    sprite = sprites;
    while(i--)
    {
        Sprite* s = *sprite;
        MovingObject* o = (MovingObject*) s->data;

        o->pos.x += o->mov.x;
        o->pos.y += o->mov.y;

#ifdef TEST_SPR
        if (i != (num - 1))
        {
#endif
            if ((o->pos.x < minx) || (o->pos.x > maxx)) o->mov.x = - o->mov.x;
            if ((o->pos.y < miny) || (o->pos.y > maxy)) o->mov.y = - o->mov.y;
#ifdef TEST_SPR
        }
#endif

        // set sprite position
        SPR_setPosition(s, F16_toInt(o->pos.x), F16_toInt(o->pos.y));
        // set sprite depth
        SPR_setDepth(s, (300 - F16_toInt(o->pos.y)) >> 4);

        sprite++;
    }
}

static void updateAnim(u16 num)
{
    Sprite** sprite;
    u16 i;

    i = num;
    sprite = sprites;
    while(i--)
    {
        Sprite* s = *sprite;
        MovingObject* o = (MovingObject*) s->data;
        fix16 mx = o->mov.x;

        if ((mx != 0) || (o->mov.y != 0)) SPR_setAnim(s, 1);
        else SPR_setAnim(s, 0);

        if (mx > 0) SPR_setHFlip(s, FALSE);
        else if (mx < 0) SPR_setHFlip(s, TRUE);

        sprite++;
    }
}

static u16 execute(u16 time, u16 numSpr)
{
    u32 startTime;
    u32 endTime;
    u32 freeCpuTime;
    u16 cpuLoad;

    startTime = getTime(TRUE);
    endTime = startTime + (time << 8);
    freeCpuTime = 0;

    do
    {
        handleInput();

        updateAnim(numSpr);
        updatePos(numSpr);
        // update sprites
        SPR_update();

        VDP_showFPS(FALSE, 1, 1);
        VDP_showCPULoad(1, 2);
        SYS_doVBlankProcess();
        cpuLoad = SYS_getCPULoad();

        // 100 + (0-100)
        if (cpuLoad < 100) freeCpuTime += 100 + (100 - cpuLoad);
        // 50 + (0-50)
        else if (cpuLoad < 200) freeCpuTime += 50 + ((200 - cpuLoad) >> 1);
        // (0-50)
        else if (cpuLoad < 300) freeCpuTime += (300 - cpuLoad) >> 1;

    } while(getTime(TRUE) < endTime);

    return freeCpuTime >> 8;
}


static void handleInput()
{
#ifdef TEST_SPR
    MovingObject* o = (MovingObject*) sprites[0]->data;

    u16 value = JOY_readJoypad(JOY_1);

    if (value & BUTTON_UP) o->mov.y = FIX16(-1.4);
    else if (value & BUTTON_DOWN) o->mov.y = FIX16(+1.4);
    else o->mov.y = 0;

    if (value & BUTTON_LEFT) o->mov.x = FIX16(-2);
    else if (value & BUTTON_RIGHT) o->mov.x = FIX16(+2);
    else o->mov.x = 0;
#endif
}
