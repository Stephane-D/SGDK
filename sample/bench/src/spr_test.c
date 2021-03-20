#include <genesis.h>

#include "gfx.h"
#include "spr_res.h"

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
} Object;


fix16 baseposx;
fix16 baseposy;
fix16 gravity;

static Object objects[MAX_OBJECT];
static Sprite* sprites[MAX_OBJECT];
static u16 palette[64];
static u16 tileIndexes[64];

// sprites structure
static Sprite *guySprite;
static Sprite *codySprite;
static Sprite *haggarSprite;
static Sprite *andorSprite;


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
    VDP_setPalette(PAL1, flare_small.palette->data);

    // init positions
    baseposx = FIX16((VDP_getScreenWidth() / 2) - 8);
    baseposy = FIX16(100);
    gravity = FIX16(0.4);
    initPartic(40);

    // execute particle bench
    *scores++ = executePartic(15, 40, FALSE, TRUE);
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
    VDP_setPalette(PAL1, flare_small.palette->data);

    // init positions
    baseposx = FIX16((VDP_getScreenWidth() / 2) - 8);
    baseposy = FIX16(100);
    gravity = FIX16(0.4);
    initPartic(40);

    // execute particle bench
    *scores++ = executePartic(15, 40, FALSE, FALSE);
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
        SPR_setVRAMTileIndex(spr, TILE_USERINDEX);
    }

    // preload animation tilesets
    ind = TILE_USERINDEX;
    for(i = 0; i < flare_small.animations[0]->numFrame; i++)
    {
        TileSet* tileset = flare_small.animations[0]->frames[i]->tileset;

        VDP_loadTileSet(tileset, ind, TRUE);
        tileIndexes[i] = ind;
        ind += tileset->numTile;
    }

    // set palette
    VDP_setPalette(PAL1, flare_small.palette->data);

    // init positions
    baseposx = FIX16((VDP_getScreenWidth() / 2) - 8);
    baseposy = FIX16(100);
    gravity = FIX16(0.4);
    initPartic(40);

    // execute particle bench
    *scores = executePartic(15, 40, TRUE, FALSE);
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
    VDP_setPalette(PAL1, flare_small.palette->data);

    // init positions
    baseposx = FIX16((VDP_getScreenWidth() / 2) - 8);
    baseposy = FIX16(100);
    gravity = FIX16(0.4);
    initPartic(79);

    // execute particle bench
    *scores = executePartic(15, 79, FALSE, FALSE);
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
        SPR_setVRAMTileIndex(spr, TILE_USERINDEX);
    }

    // preload animation tilesets
    ind = TILE_USERINDEX;
    for(i = 0; i < flare_small.animations[0]->numFrame; i++)
    {
        TileSet* tileset = flare_small.animations[0]->frames[i]->tileset;

        VDP_loadTileSet(tileset, ind, TRUE);
        tileIndexes[i] = ind;
        ind += tileset->numTile;
    }

    // set palette
    VDP_setPalette(PAL1, flare_small.palette->data);

    // init positions
    baseposx = FIX16((VDP_getScreenWidth() / 2) - 8);
    baseposy = FIX16(100);
    gravity = FIX16(0.4);
    initPartic(79);

    // execute particle bench
    *scores = executePartic(15, 79, TRUE, FALSE);
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
    VDP_setPalette(PAL1, flare_big.palette->data);

    // init positions
    baseposx = FIX16((VDP_getScreenWidth() / 2) - 16);
    baseposy = FIX16(100);
    gravity = FIX16(0.4);
    initPartic(40);

    // execute particle bench
    *scores = executePartic(15, 40, FALSE, FALSE);
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
        SPR_setVRAMTileIndex(spr, TILE_USERINDEX);
    }

    // preload animation tilesets
    ind = TILE_USERINDEX;
    for(i = 0; i < flare_big.animations[0]->numFrame; i++)
    {
        TileSet* tileset = flare_big.animations[0]->frames[i]->tileset;

        VDP_loadTileSet(tileset, ind, TRUE);
        tileIndexes[i] = ind;
        ind += tileset->numTile;
    }

    // set palette
    VDP_setPalette(PAL1, flare_big.palette->data);

    // init positions
    baseposx = FIX16((VDP_getScreenWidth() / 2) - 16);
    baseposy = FIX16(100);
    gravity = FIX16(0.4);
    initPartic(40);

    // execute particle bench
    *scores = executePartic(15, 40, TRUE, FALSE);
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
    VDP_setPalette(PAL1, donut.palette->data);

    // execute donut bench
    *scores = executeDonut(20, FALSE);
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
    ind = TILE_USERINDEX;
    for(i = 0; i < donut.animations[0]->numFrame; i++)
    {
        TileSet* tileset = donut.animations[0]->frames[i]->tileset;

        VDP_loadTileSet(tileset, ind, TRUE);
        tileIndexes[i] = ind;
        ind += tileset->numTile;
    }

    // set palette
    VDP_setPalette(PAL1, donut.palette->data);

    // execute particle bench
    *scores = executeDonut(20, TRUE);
    globalScore += *scores++;
    SPR_logProfil();

    SYS_disableInts();
    // reset sprite engine (release all allocated resources)
    SPR_reset();
    SPR_clear();
    VDP_clearPlane(BG_A, TRUE);
    VDP_drawText("Big sprites test...", 1, 2);
    SYS_enableInts();

    waitMs(5000);
    SYS_disableInts();
    VDP_clearPlane(BG_A, TRUE);
    SYS_enableInts();

    // create sprites structures
    guySprite = SPR_addSprite(&guy_sprite, 350, 120, TILE_ATTR(PAL0, FALSE, FALSE, FALSE));
    codySprite = SPR_addSprite(&cody_sprite, 128, 300, TILE_ATTR(PAL1, FALSE, FALSE, FALSE));
    haggarSprite = SPR_addSprite(&haggar_sprite, 0, 0, TILE_ATTR(PAL2, FALSE, FALSE, FALSE));
    andorSprite = SPR_addSprite(&andor_sprite, 0, 0, TILE_ATTR(PAL3, FALSE, FALSE, FALSE));

    SPR_update();

    // desync frame timer so update happen on different frame
    haggarSprite->timer = 1;
    codySprite->timer = 1;

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

    // prepare palettes
    memcpy(&palette[0], guy_sprite.palette->data, 16 * 2);
    memcpy(&palette[16], cody_sprite.palette->data, 16 * 2);
    memcpy(&palette[32], haggar_sprite.palette->data, 16 * 2);
    memcpy(&palette[48], andor_sprite.palette->data, 16 * 2);
    // keep background color black
    palette[0] = 0;

    // set palette
    SYS_disableInts();
    VDP_setPaletteColors(0, palette, 64);
    SYS_enableInts();

    // execute sprite bench
    *scores = execute(50, 4);
    globalScore += *scores++;
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
    SPR_initEx(16 * (32 + 16 + 8));
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
        Object* o = (Object*) s->data;

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
        Object* o = (Object*) s->data;

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
        SPR_setPosition(s, fix16ToInt(o->pos.x), 224 - fix16ToInt(o->pos.y));
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

        VDP_showFPS(FALSE);
        VDP_showCPULoad();
        SYS_doVBlankProcess();
        cpuLoad = SYS_getCPULoad();

        if (cpuLoad < 100) freeCpuTime += 100 - cpuLoad;
        else if (cpuLoad < 200) freeCpuTime += (200 - cpuLoad) >> 1;
        else if (cpuLoad < 300) freeCpuTime += (300 - cpuLoad) >> 2;
    } while(getTime(TRUE) < endTime);

    return freeCpuTime >> 6;
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
                    SPR_setVRAMTileIndex(spr, TILE_USERINDEX);
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

        VDP_showFPS(FALSE);
        VDP_showCPULoad();

        SYS_doVBlankProcess();
        cpuLoad = SYS_getCPULoad();

        if (cpuLoad < 100) freeCpuTime += 100 - cpuLoad;
        else if (cpuLoad < 200) freeCpuTime += (200 - cpuLoad) >> 1;
        else if (cpuLoad < 300) freeCpuTime += (300 - cpuLoad) >> 2;

        t -= 4;
        frame++;
    } while(getTime(TRUE) < endTime);

    return freeCpuTime >> 6;
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
        Object* o = (Object*) s->data;

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
        Object* o = (Object*) s->data;

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
        SPR_setPosition(s, fix16ToInt(o->pos.x), fix16ToInt(o->pos.y));
        // set sprite depth
        SPR_setDepth(s, (300 - fix16ToInt(o->pos.y)) >> 4);

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
        Object* o = (Object*) s->data;
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

        VDP_showFPS(FALSE);
        VDP_showCPULoad();
        SYS_doVBlankProcess();
        cpuLoad = SYS_getCPULoad();

        if (cpuLoad < 100) freeCpuTime += 100 - cpuLoad;
        else if (cpuLoad < 200) freeCpuTime += (200 - cpuLoad) >> 1;
        else if (cpuLoad < 300) freeCpuTime += (300 - cpuLoad) >> 2;

    } while(getTime(TRUE) < endTime);

    return freeCpuTime >> 6;
}


static void handleInput()
{
#ifdef TEST_SPR
    Object* o = (Object*) sprites[0]->data;

    u16 value = JOY_readJoypad(JOY_1);

    if (value & BUTTON_UP) o->mov.y = FIX16(-1.4);
    else if (value & BUTTON_DOWN) o->mov.y = FIX16(+1.4);
    else o->mov.y = 0;

    if (value & BUTTON_LEFT) o->mov.x = FIX16(-2);
    else if (value & BUTTON_RIGHT) o->mov.x = FIX16(+2);
    else o->mov.x = 0;
#endif
}
