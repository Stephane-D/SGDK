#include <genesis.h>

#include "tracklist.h"
#include "xgm_tool.h"
#include "ym_state.h"
#include "psg_state.h"
#include "xd3.h"
#include "log_tab.h"

#include "smp_null.h"
#include "music.h"
#include "gfx.h"

#define MAX_TEMPO           300
#define MAX_LOOP            5
#define MAX_PRESSTIME       (60 * 5)

#define PROGRESS_MAX        (14*4)

#define	SCROLLV_LEN         224

#define REFRESH_TRACKINFO   (1 << 0)
#define REFRESH_TRACKLIST   (1 << 1)
#define REFRESH_SETTING     (1 << 2)
#define REFRESH_CONTROL     (1 << 3)
#define REFRESH_SHUFFLE     (1 << 4)

#define SET_PIXEL(x, y, c)              \
{                                       \
    u16 fx = (x) >> 1;                  \
    u16 fy = y;                         \
    u8 *dst = pcmTileBuffer;            \
    dst += fx & 3;                      \
    dst += 4 * fy;                      \
                                            \
    if ((x) & 1) *dst = (*dst & 0xF0) | c;  \
    else *dst = (*dst & 0x0F) | (c << 4);   \
}


const u16 gfx_palette[16] =
{
    0x0000,     // 0 - black
    0x0444,     // 1 - dark gray
    0x0AAA,     // 2 - light gray
    0x0EEE,     // 3 - white

    0x0008,     // 4 - dark red
    0x000E,     // 5 - red

    0x0400,     // 6 - dark blue
    0x0E00,     // 7 - blue

    0x0420,     // 8 - dark blue/cyan
    0x0E60,     // 9 - blue/cyan

    0x0440,     // A - dark cyan
    0x0EC0,     // B - cyan

    0x0040,     // C - dark green
    0x00E0,     // D - green

    0x0044,     // E - dark yellow
    0x00EE      // F - yellow
};


static void startPlay();
static void stopPlay();

static XD3* getTrackInfo(u16 trackIndex);
static void buildShuffledList();

static void initZ80CPUMeter();

static void drawStaticGUI();
static void drawPlayerControls();
static void drawPlayerSettings();
static void drawPlayingStates();
static void drawTrackInfo();
static void drawPlayList();
static void drawShortTrackInfo(s16 planIndex, u16 index);
static void drawTime(s32 time, u16 x, u16 y);

static void clearProgressBar();
static void updateProgressBar(u16 level);

static void drawChipsStates();
static void drawZ80Load();
static void drawYMState();
static u8* drawPCM(u8* dst, u16 addr);
static void drawPCMState();
static void drawPSGState();

static void drawHorizontalBarYM(u16 x, u16 y, u16 color);
static void drawHorizontalBarPSG(u16 x, u16 y, u32 color);
static void fillColumnYM(u16 x, u16 h1, u16 h2, u16 backColor, u16 firstColor, u16 secondColor);
static void fillColumnZ80(u16 h1, u16 h2, u32 backColor, u32 firstColor, u32 secondColor);
static void fillColumnPSG(u16 x, u16 heigth, u32 backColor, u32 frontColor);

static void initBGScroll();
static void updateBGScroll();

static void updateListScroll();

static void doJoyActions();
static void joyEvent(u16 joy, u16 changed, u16 state);
static void vint();
static void hint();


// get access to XGM driver timer
extern s16 xgmTempoCnt;

// make it in a volatile variable so compiler won't optimize to constant in code
vu16 numMusic = 13;

// track infos cache
static XD3 trackInfos[MAX_MUSIC];
static u16 shuffledIndexes[MAX_MUSIC];
static u16 inverseIndexes[MAX_MUSIC];

static u16 planTrackIndexesCache[64];

// current track info
static s16 trackPlayedRawIndex;
static s16 trackPlayed;
static s16 trackIndexList;
static XD3 *trackInfo;
static s32 elapsed;
static s32 total;

static u16 paused;
static s16 loop;
static u16 shuffle;
static u16 bgEnabled;
static u16 hidePlaylist;
static u16 tempo;
static u16 tempoDef;
vs16 frameToParse;
static vs16 frameToUpdate;
static u8* xgmData;
static u32 xgmOffset;
static YM ym;
static PSG psg;

// Z80 cpu meter
static u16 cpuload;
static u16 dmawaitload;

// left / right sprites
static Sprite* YMPanSprites[6];
static Sprite* trackListCursor;
static Sprite* trackListShadowTop[3];
static Sprite* trackListShadowBottom[3];

// BG starfield scrolling
static s16 scrollB[SCROLLV_LEN];
static fix16 scrollBF[SCROLLV_LEN];
static fix16 scrollSpeed[SCROLLV_LEN];

// internal
static u16 joyState;
static u16 refresh;
static u16 wantStartPlay;
static u16 buttonsPressed;
static u16 pressTime;
static u16 tileIndex;
static u16 tileIndexProgressBar;
static u16 tileIndexShadowMaskOdd;
static u16 tileIndexShadowMaskEven;
static u16 curTileIndexMesh;
static s16 trackListScrollPos;
static u16 playStateCnt;
static u16 evenCnt;
static u16 playIdleCnt;
static u16 fastForward;
static u16 progressBarLevel;
static u16 needProgressUpload;
static TileMap* bgTileMap;

// tiles buffer for chips state rendering
static u8 progressTileBuffer[32*15];
static u8 z80TileBuffer[32*1*4];
static u8 ymTileBuffer[32*(6*3)*4];
static u8 pcmTileBuffer[32*(4*3)*4];
static u8 psgTileBuffer[32*4*4];

static u16 palette[64];


int main()
{
    u16 i;

    SYS_disableInts();
    SYS_setVIntAligned(FALSE);

    // clear all palette
    VDP_setPaletteColors(0, palette_black, 64);

    // initialization
    VDP_setScreenWidth320();
    // enable hilight / shadow
    VDP_setHilightShadow(TRUE);
	// set scrolling mode (line)
	VDP_setScrollingMode(HSCROLL_LINE, VSCROLL_PLANE);

    // need to use H Int at line 128 for window reconfiguration
    VDP_setHInterrupt(TRUE);
    VDP_setHIntCounter(128);

    // setup VRAM
    VDP_setPlaneSize(64, 64, FALSE);
    VDP_setSpriteListAddress(0xA800);
    VDP_setHScrollTableAddress(0xAC00);
    VDP_setWindowAddress(0xB000);
    VDP_setBGBAddress(0xC000);
    VDP_setBGAAddress(0xE000);

    // set window visible from first row up to row 13
    VDP_setWindowHPos(FALSE, 0);
    VDP_setWindowVPos(FALSE, 13);
    // by default we draw text in window plan and in high priority
    VDP_setTextPlane(WINDOW);
    VDP_setTextPriority(TRUE);

    // clear HScroll table
	DMA_doVRamFill(VDP_HSCROLL_TABLE, 224 * 4, 0, 1);
	VDP_waitDMACompletion();

    initZ80CPUMeter();

    // init track info structure
    memset(trackInfos, 0, sizeof(trackInfos));
    // init plan track cache
    memset(planTrackIndexesCache, 0xFF, sizeof(planTrackIndexesCache));

    trackPlayedRawIndex = -1;
    trackPlayed = -1;
    trackInfo = NULL;
    trackIndexList = 0;
    elapsed = -1;
    total = -1;
    paused = FALSE;
    shuffle = FALSE;
    bgEnabled = TRUE;
    hidePlaylist = FALSE;
    loop = 2;
    frameToParse = 0;
    frameToUpdate = 0;
    joyState = 0;
    refresh = 0;
    wantStartPlay = FALSE;
    buttonsPressed = 0;
    pressTime = 0;
    xgmData = NULL;
    trackListScrollPos = 0;
    playStateCnt = 0;
    evenCnt = 0;
    fastForward = FALSE;
    progressBarLevel = 0;
    needProgressUpload = FALSE;

    // force to load XGM driver
    XGM_setLoopNumber(loop);
    // get default tempo
    tempo = XGM_getMusicTempo();
    if (IS_PALSYSTEM) tempoDef = 50;
    else tempoDef = 60;
    // enable delay to avoid DMA contention when PSG is used in music
    XGM_setForceDelayDMA(TRUE);

    // init some GFX var
    tileIndexProgressBar = TILE_USERINDEX + bg.tileset->numTile + music_logo.tileset->numTile;
    tileIndex = TILE_USERINDEX + bg.tileset->numTile + music_logo.tileset->numTile + progress_bar.tileset->numTile + starfield.tileset->numTile;

    bgTileMap = unpackTileMap(bg.tilemap, NULL);

    // init shuffle list
    buildShuffledList();

    // draw static GUI & init BG scrolling
    drawStaticGUI();
    initBGScroll();

    // init Sprite engine
    SPR_initEx(80);

    // prepare sprites for panning
    YMPanSprites[0] = SPR_addSprite(&left_right, 32 + 0, 203, TILE_ATTR(PAL0, TRUE, FALSE, FALSE));
    YMPanSprites[1] = SPR_addSprite(&left_right, 32 + 24, 203, TILE_ATTR(PAL0, TRUE, FALSE, FALSE));
    YMPanSprites[2] = SPR_addSprite(&left_right, 32 + 48, 203, TILE_ATTR(PAL0, TRUE, FALSE, FALSE));
    YMPanSprites[3] = SPR_addSprite(&left_right, 32 + 72, 203, TILE_ATTR(PAL0, TRUE, FALSE, FALSE));
    YMPanSprites[4] = SPR_addSprite(&left_right, 32 + 95, 203, TILE_ATTR(PAL0, TRUE, FALSE, FALSE));
    YMPanSprites[5] = SPR_addSprite(&left_right, 32 + 119, 203, TILE_ATTR(PAL0, TRUE, FALSE, FALSE));
    for(i = 0; i < 6; i++)
        SPR_setVisibility(YMPanSprites[i], VISIBLE);
    // prepare track list cursor
    trackListCursor = SPR_addSprite(&cursor, 0, 108, TILE_ATTR(PAL0, TRUE, FALSE, FALSE));
    SPR_setVisibility(trackListCursor, VISIBLE);
    // prepare track list shadow
    curTileIndexMesh = tileIndexShadowMaskOdd;
    trackListShadowTop[0] = SPR_addSpriteEx(&shadow_mask_16, 8, 100, TILE_ATTR_FULL(PAL3, TRUE, FALSE, FALSE, curTileIndexMesh), 0, SPR_FLAG_AUTO_SPRITE_ALLOC);
    trackListShadowTop[1] = SPR_addSpriteEx(&shadow_mask_16, 8+128, 100, TILE_ATTR_FULL(PAL3, TRUE, FALSE, FALSE, curTileIndexMesh), 0, SPR_FLAG_AUTO_SPRITE_ALLOC);
    trackListShadowTop[2] = SPR_addSpriteEx(&shadow_mask_7, 8+256, 100, TILE_ATTR_FULL(PAL3, TRUE, FALSE, FALSE, curTileIndexMesh), 0, SPR_FLAG_AUTO_SPRITE_ALLOC);
    trackListShadowBottom[0] = SPR_addSpriteEx(&shadow_mask_16, 8, 132, TILE_ATTR_FULL(PAL3, TRUE, TRUE, FALSE, curTileIndexMesh), 0, SPR_FLAG_AUTO_SPRITE_ALLOC);
    trackListShadowBottom[1] = SPR_addSpriteEx(&shadow_mask_16, 8+128, 132, TILE_ATTR_FULL(PAL3, TRUE, TRUE, FALSE, curTileIndexMesh), 0, SPR_FLAG_AUTO_SPRITE_ALLOC);
    trackListShadowBottom[2] = SPR_addSpriteEx(&shadow_mask_7, 8+256, 132, TILE_ATTR_FULL(PAL3, TRUE, TRUE, FALSE, curTileIndexMesh), 0, SPR_FLAG_AUTO_SPRITE_ALLOC);
    for(i = 0; i < 3; i++)
    {
        SPR_setVisibility(trackListShadowTop[i], VISIBLE);
        SPR_setVisibility(trackListShadowBottom[i], VISIBLE);
    }

    // reset sound chip
    YM_init(&ym);
    PSGState_init(&psg);

    // force first refresh of screen
    SPR_update();
    drawChipsStates();
    updateBGScroll();
    drawTrackInfo();
    drawPlayingStates();
    drawPlayList();
    drawPlayerSettings();
    drawPlayerControls();

    SYS_enableInts();

    SYS_setVIntCallback(vint);
    SYS_setHIntCallback(hint);

    // prepare palettes
    memcpy(&palette[0 * 16], bg.palette->data, 15 * 2);
    // set white color for index 15
    palette[15] = 0xEEE;
    memcpy(&palette[1 * 16], gfx_palette, 16 * 2);
    memcpy(&palette[2 * 16], music_logo.palette->data, 16 * 2);
    memcpy(&palette[3 * 16], shadow_mask_16.palette->data, 16 * 2);
    // set hilight/shadow operators to black color
    palette[62] = 0x000;
    palette[63] = 0x000;

    // palette fading
    VDP_fadeAll(palette_black, palette, 30, FALSE);

    JOY_setEventHandler(joyEvent);

    while(1)
    {
        while (frameToParse > 0)
        {
            SYS_disableInts();

            u8* xgm = xgmData;

            // need to init xgm data ?
            if (xgm == NULL)
            {
                // get XGM music address
                xgm = (u8*) xgm_musics[shuffledIndexes[trackPlayed]];
                // add music block offset
                xgm += XGM_getMusicDataOffset(xgm);
                // save it to xgmData
                xgmData = xgm;

                xgmOffset = 0;
                YM_init(&ym);
                PSG_init(&psg);
            }

            SYS_enableInts();

            s32 offset = XGM_parseFrame(xgm + xgmOffset, &ym, &psg);

            // loop point
            if (offset < 0)
                xgmOffset = -offset;
            // simply add frame size to offset
            else
                xgmOffset += offset;

            frameToParse--;
        }

        while (frameToUpdate > 0)
        {
            // update YM envelop states
            YM_updateEnv(&ym);
            frameToUpdate--;
        }

        // alternate mesh
        if (curTileIndexMesh == tileIndexShadowMaskEven) curTileIndexMesh = tileIndexShadowMaskOdd;
        else curTileIndexMesh = tileIndexShadowMaskEven;
        for(i = 0; i < 3; i++)
        {
            SPR_setVRAMTileIndex(trackListShadowTop[i], curTileIndexMesh);
            SPR_setVRAMTileIndex(trackListShadowBottom[i], curTileIndexMesh);
        }

        SPR_update();
        drawChipsStates();
        if (bgEnabled) updateBGScroll();

//        SYS_disableInts();
//        VDP_showFPS(FALSE);
//        SYS_enableInts();

//        VDP_setBackgroundColor(3);
//        SYS_disableInts();
//        SYS_doVBlankProcess();
//        SYS_enableInts();
        VDP_waitVSync();
//        VDP_setBackgroundColor(0);

        // refresh shuffle list (should be before REFRESH_TRACKLIST)
        if (refresh & REFRESH_SHUFFLE)
        {
            refresh &= ~REFRESH_SHUFFLE;

            SYS_disableInts();

            buildShuffledList();
            // need to reset plan track cache
            memset(planTrackIndexesCache, 0xFF, sizeof(planTrackIndexesCache));

            // restart play from first track
            trackPlayedRawIndex = 0;
            trackPlayed = 0;
            wantStartPlay = TRUE;

            SYS_enableInts();
        }
        // refresh track info
        if (refresh & REFRESH_TRACKINFO)
        {
            refresh &= ~REFRESH_TRACKINFO;
            drawTrackInfo();
        }
        // refresh play list
        if (refresh & REFRESH_TRACKLIST)
        {
            refresh &= ~REFRESH_TRACKLIST;
            drawPlayList();
        }
        // refresh player settings
        if (refresh & REFRESH_SETTING)
        {
            refresh &= ~REFRESH_SETTING;
            drawPlayerSettings();
        }
        // refresh player controles
        if (refresh & REFRESH_CONTROL)
        {
            refresh &= ~REFRESH_CONTROL;
            drawPlayerControls();
        }

        // better to do that after VSync as it draws text (can take up to 30 lines !)
        drawPlayingStates();
        // do that last
        doJoyActions();
        // need to start music
        if (wantStartPlay) startPlay();
    }

    return 0;
}


static void startPlay()
{
    trackInfo = getTrackInfo(shuffledIndexes[trackPlayed]);
    // move to played track
    trackIndexList = trackPlayedRawIndex;
    elapsed = 0;
    paused = FALSE;
    playStateCnt = 0;
    playIdleCnt = 0;
    if (loop == MAX_LOOP) total = XD3_getDuration(trackInfo, -1);
    else total = XD3_getDuration(trackInfo, loop);
    xgmData = NULL;
    // start music
    XGM_startPlay(xgm_musics[shuffledIndexes[trackPlayed]]);
    refresh |= REFRESH_TRACKLIST | REFRESH_TRACKINFO | REFRESH_CONTROL;
    wantStartPlay = FALSE;
}

static void stopPlay()
{
    XGM_stopPlay();
    elapsed = -1;
    trackPlayedRawIndex = -1;
    trackPlayed = -1;
    trackInfo = NULL;
    paused = FALSE;
    playStateCnt = 0;
    total = -1;

    refresh |= REFRESH_TRACKLIST | REFRESH_TRACKINFO | REFRESH_CONTROL;
}

static XD3* getTrackInfo(u16 index)
{
    XD3* result = &trackInfos[index];

    // not yet initialized ?
    if (result->duration == 0)
        XGM_getXD3(xgm_musics[index], result);

    return result;
}

s16 compareList(void* obj1, void* obj2)
{
    if (obj1 < obj2) return -1;
    if (obj1 > obj2) return 1;
    return 0;
}

static void buildShuffledList()
{
    u16 indexes[MAX_MUSIC];
    u16 *src;
    u16 *dst;
    u16 *dstInv;
    u16 i;
    u16 remaining;

    src = indexes;
    for(i = 0; i < numMusic; i++)
        *src++ = i;

    dst = shuffledIndexes;
    dstInv = inverseIndexes;
    remaining = numMusic;

    if (shuffle)
    {
        while(remaining)
        {
            // random list copy
            u16* ind = &indexes[random() % remaining];

            dstInv[*ind] = dst - shuffledIndexes;
            *dst++ = *ind;

            remaining--;
            // copy last index in current
            *ind = indexes[remaining];
        }
    }
    else
    {
        src = indexes;
        // simple list copy
        while(remaining--) *dst++ = *src++;
    }

    refresh |= REFRESH_TRACKLIST;

/*
    // sort test
    void* list[numMusic];
    void** listDst;

    src = shuffledIndexes;
    listDst = list;
    for(i = 0; i < numMusic; i++)
        *listDst++ = *src++;

    KLog("Before sort:");
    for(i = 0; i < numMusic; i++)
        KLog_S2("Element ", i, " = ", list[i]);

    qsort(list, numMusic, compareList);

    KLog("After sort:");
    for(i = 0; i < numMusic; i++)
        KLog_S2("Element ", i, " = ", list[i]);
*/
}


static void initZ80CPUMeter()
{
    // init load
    cpuload = 0;
    dmawaitload = 0;
}

static void drawStaticGUI()
{
    TileSet* tileset;
    u16 x, sx, y;
    u16 i;

    SYS_disableInts();

    // General GUI & logo
    VDP_drawImageEx(WINDOW, &bg, TILE_ATTR_FULL(PAL0, TRUE, FALSE, FALSE, TILE_USERINDEX), 0, 0, FALSE, TRUE);
    VDP_drawImageEx(WINDOW, &music_logo, TILE_ATTR_FULL(PAL2, TRUE, FALSE, FALSE, TILE_USERINDEX + bg.tileset->numTile), 21, 0, FALSE, TRUE);
    VDP_drawImageEx(WINDOW, &progress_bar, TILE_ATTR_FULL(PAL0, TRUE, FALSE, FALSE, tileIndexProgressBar), 9, 8, FALSE, TRUE);
    // starfield
    VDP_drawImageEx(BG_B, &starfield, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, TILE_USERINDEX + bg.tileset->numTile + music_logo.tileset->numTile + progress_bar.tileset->numTile), 0, 0, FALSE, TRUE);

    // prepare 'bitTileMap' buffer for chips state rendering
    i = 0;

    // Z80 area
    for(y = 0; y < 4; y++, i++)
        VDP_setTileMapXY(WINDOW, TILE_ATTR_FULL(PAL1, TRUE, FALSE, FALSE, tileIndex + i), 1, 21 + y);
    // YM area
    for(x = 0; x < 6; x++)
        for(sx = 0; sx < 3; sx++)
            for(y = 0; y < 4; y++, i++)
                VDP_setTileMapXY(WINDOW, TILE_ATTR_FULL(PAL1, TRUE, FALSE, FALSE, tileIndex + i), 4 + (x * 3) + sx, 21 + y);
    // PCM area
    for(x = 0; x < 4; x++)
        for(sx = 0; sx < 3; sx++)
            for(y = 0; y < 4; y++, i++)
                VDP_setTileMapXY(WINDOW, TILE_ATTR_FULL(PAL1, TRUE, FALSE, FALSE, tileIndex + i), 23 + (x * 3) + sx, 21 + y);
    // PSG area
    for(x = 0; x < 4; x++)
        for(y = 0; y < 4; y++, i++)
            VDP_setTileMapXY(WINDOW, TILE_ATTR_FULL(PAL1, TRUE, FALSE, FALSE, tileIndex + i), 36 + x, 21 + y);

    // prepare progress bar rendering buffer
    tileset = unpackTileSet(progress_bar.tileset, NULL);
    memcpy(progressTileBuffer, tileset->tiles, tileset->numTile * 32);
    // release unpacked tileset
    MEM_free(tileset);

    // preload tilesets for shadow mask
    tileIndexShadowMaskOdd =  tileIndex + i;
    tileset = shadow_mask_16.animations[0]->frames[0]->tileset;
    VDP_loadTileSet(tileset, tileIndexShadowMaskOdd, TRUE);
    tileIndexShadowMaskEven = tileIndexShadowMaskOdd + tileset->numTile;
    tileset = shadow_mask_16.animations[1]->frames[0]->tileset;
    VDP_loadTileSet(tileset, tileIndexShadowMaskEven, TRUE);

    SYS_enableInts();
}

static void drawPlayerControls()
{
    SYS_disableInts();

    // alternate controls ?
    if (buttonsPressed & BUTTON_START)
    {
        // DPad control
        VDP_setTileMapEx(WINDOW, bgTileMap, TILE_ATTR_FULL(PAL0, TRUE, FALSE, FALSE, TILE_USERINDEX), 0, 2, 40 + 7, 6, 7, 6, CPU);

        // not playing
        if ((trackPlayed == -1) || paused)
            VDP_setTileMapEx(WINDOW, bgTileMap, TILE_ATTR_FULL(PAL0, TRUE, FALSE, FALSE, TILE_USERINDEX), 7, 2, 21 + (7 * 2), 0, 7, 6, CPU);
        else
            VDP_setTileMapEx(WINDOW, bgTileMap, TILE_ATTR_FULL(PAL0, TRUE, FALSE, FALSE, TILE_USERINDEX), 7, 2, 21 + (7 * 2) + 7, 0, 7, 6, CPU);
    }
    else
    {
        // DPad control
        VDP_setTileMapEx(WINDOW, bgTileMap, TILE_ATTR_FULL(PAL0, TRUE, FALSE, FALSE, TILE_USERINDEX), 0, 2, 40, 6, 7, 6, CPU);

        // not playing
        if ((trackPlayed == -1) || paused)
            VDP_setTileMapEx(WINDOW, bgTileMap, TILE_ATTR_FULL(PAL0, TRUE, FALSE, FALSE, TILE_USERINDEX), 7, 2, 21, 0, 7, 6, CPU);
        else
            VDP_setTileMapEx(WINDOW, bgTileMap, TILE_ATTR_FULL(PAL0, TRUE, FALSE, FALSE, TILE_USERINDEX), 7, 2, 21 + 7, 0, 7, 6, CPU);
    }

    SYS_enableInts();
}

static void drawPlayerSettings()
{
    char tempoStr[8];
    char loopStr[8];

    uintToStr(tempo, tempoStr, 3);
    if (loop != MAX_LOOP)
    {
        uintToStr(loop, loopStr, 1);
        strcat(loopStr, " ");
    }

    SYS_disableInts();

    VDP_drawText(tempoStr, 17, 4);
    // refresh shuffle state
    if (shuffle) VDP_setTileMapEx(WINDOW, bgTileMap, TILE_ATTR_FULL(PAL0, TRUE, FALSE, FALSE, TILE_USERINDEX), 26, 8, 25, 6, 2, 1, CPU);
    else VDP_setTileMapEx(WINDOW, bgTileMap, TILE_ATTR_FULL(PAL0, TRUE, FALSE, FALSE, TILE_USERINDEX), 26, 8, 23, 6, 2, 1, CPU);
    // refresh loop state
    if (loop != MAX_LOOP) VDP_drawText(loopStr, 18, 6);
    // use infinite symbol from original image
    else VDP_setTileMapEx(WINDOW, bgTileMap, TILE_ATTR_FULL(PAL0, TRUE, FALSE, FALSE, TILE_USERINDEX), 18, 6, 21, 6, 2, 1, CPU);

    SYS_enableInts();
}

static void drawPlayingStates()
{
    const u16 draw = playStateCnt & 0xF;

    if ((playStateCnt == 0) || (draw == 0))
    {
        SYS_disableInts();

        if (trackPlayed == -1) VDP_drawText("       ", 1, 8);
        else
        {
            if (paused)
            {
                if (playStateCnt & 0x20) VDP_drawText("       ", 1, 8);
                else VDP_drawText("Paused ", 1, 8);
            }
            else VDP_drawText("Playing", 1, 8);
        }

        SYS_enableInts();
    }

    if (elapsed != -1)
    {
        if (fastForward || (playStateCnt == 0) || (draw == 4))
        {
            drawTime(elapsed, 29, 8);
            // update progress bar
            if (total > 0)
            {
                u16 elapsedS = elapsed >> 5;
                u16 totalS = total >> 5;

                updateProgressBar(((elapsedS * PROGRESS_MAX) / totalS) + 2);
            }
        }
        if (fastForward || (playStateCnt == 0) || (draw == 8))
            drawTime(total, 35, 8);
    }
    else
    {
        if ((playStateCnt == 0) || (draw == 4))
        {
            drawTime(-1, 29, 8);
            drawTime(-1, 35, 8);

            clearProgressBar();
        }
    }

    playStateCnt++;
}

static void clearProgressBar()
{
    u16 z16 = getZeroU16();
    u32 z32 = getZeroU32();
    u16* dst16 = (u16*) progressTileBuffer;
    u32* dst32 = (u32*) progressTileBuffer;

    // clear progress bar
    dst16[(16 * 0) + 7] = z16;
    dst16[(16 * 0) + 9] = z16;
    dst32[(8 * 1) + 3] = z32;
    dst32[(8 * 1) + 4] = z32;
    dst32[(8 * 2) + 3] = z32;
    dst32[(8 * 2) + 4] = z32;
    dst32[(8 * 3) + 3] = z32;
    dst32[(8 * 3) + 4] = z32;
    dst32[(8 * 4) + 3] = z32;
    dst32[(8 * 4) + 4] = z32;
    dst32[(8 * 5) + 3] = z32;
    dst32[(8 * 5) + 4] = z32;
    dst32[(8 * 6) + 3] = z32;
    dst32[(8 * 6) + 4] = z32;
    dst32[(8 * 7) + 3] = z32;
    dst32[(8 * 7) + 4] = z32;
    dst32[(8 * 8) + 3] = z32;
    dst32[(8 * 8) + 4] = z32;
    dst32[(8 * 9) + 3] = z32;
    dst32[(8 * 9) + 4] = z32;
    dst32[(8 * 10) + 3] = z32;
    dst32[(8 * 10) + 4] = z32;
    dst32[(8 * 11) + 3] = z32;
    dst32[(8 * 11) + 4] = z32;
    dst32[(8 * 12) + 3] = z32;
    dst32[(8 * 12) + 4] = z32;
    dst32[(8 * 13) + 3] = z32;
    dst32[(8 * 13) + 4] = z32;
    dst16[(16 * 14) + 6] = z16;
    dst16[(16 * 14) + 8] = z16;

    progressBarLevel = 2;
    needProgressUpload = TRUE;
}

static void updateProgressBar(u16 level)
{
    // need to clear first
    if (progressBarLevel > level) clearProgressBar();

    // min level = 2, max level = 58
    u16 curLevel = progressBarLevel;
    u8 col = 0x99;
    u8* dst = &progressTileBuffer[12];

    dst += (curLevel >> 2) << 5;
    dst += curLevel & 3;
    // safe
    if (level > 58) level = 58;

    while(curLevel < level)
    {
        dst[4] = col;
        *dst++ = col;
        curLevel++;
        if ((curLevel & 3) == 0) dst += 32 - 4;
    }

    progressBarLevel = curLevel;
    needProgressUpload = TRUE;
}

static void drawTrackInfo()
{
    // not playing ?
    if (trackInfo == NULL)
    {
        SYS_disableInts();

        // just erase infos
        VDP_clearText(8, 9, 32);
        VDP_clearText(8, 10, 17);
        VDP_clearText(35, 10, 5);
        VDP_clearText(8, 11, 16);
        VDP_clearText(32, 11, 8);

        SYS_enableInts();
    }
    else
    {
        u16 len;
        char str[41];

        len = strlen(trackInfo->trackName);
        if (len > 32)
        {
            strncpy(str, trackInfo->trackName, min(len, 32-1));
            strcat(str, ".");
        }
        else strcpy(str, trackInfo->trackName);

        // track name
        SYS_disableInts();
        VDP_clearText(8, 9, 32);
        VDP_drawText(str, 8, 9);
        SYS_enableInts();

        len = strlen(trackInfo->gameName);
        if (len > 17)
        {
            strncpy(str, trackInfo->gameName, min(len, 17-1));
            strcat(str, ".");
        }
        else strcpy(str, trackInfo->gameName);

        // game name
        SYS_disableInts();
        VDP_clearText(8, 10, 17);
        VDP_drawText(str, 8, 10);
        SYS_enableInts();

        // duration
        drawTime(trackInfo->duration, 35, 10);

        len = strlen(trackInfo->authorName);
        if (len > 16)
        {
            strncpy(str, trackInfo->authorName, min(len, 16-1));
            strcat(str, ".");
        }
        else strcpy(str, trackInfo->authorName);

        // author name
        SYS_disableInts();
        VDP_clearText(8, 11, 16);
        VDP_drawText(str, 8, 11);
        SYS_enableInts();

        len = strlen(trackInfo->date);
        if (len > 8)
        {
            strncpy(str, trackInfo->date, min(len, 8-1));
            strcat(str, ".");
        }
        else strcpy(str, trackInfo->date);

        // date
        SYS_disableInts();
        VDP_clearText(32, 11, 8);
        VDP_drawText(str, 32, 11);
        SYS_enableInts();
    }
}

static void drawPlayList()
{
    s16 planInd;
    s16 trackInd;
    u16 num;

    // play list is draw in plan A to use scrolling capabilities
    VDP_setTextPlane(BG_A);

    planInd = trackIndexList - 3;
    trackInd = planInd % numMusic;
    if (trackInd < 0) trackInd += numMusic;

    num = 7;
    while(num--)
    {
        drawShortTrackInfo(planInd, trackInd);

        planInd++;
        trackInd++;
        if (trackInd >= numMusic) trackInd = 0;
    }

    // restore previous value
    VDP_setTextPlane(WINDOW);
}

static void drawShortTrackInfo(s16 planIndex, u16 index)
{
    u16 posY;
    u16 cachedIndex;
    u16 i;
    u16 len;
    XD3 *info;
    char str[80];
    char *src;
    char *dst;

    posY = planIndex & 63;
    cachedIndex = index;
    if (index == trackPlayed) cachedIndex |= 0x8000;

    // plan is up to date here --> exit
    if (planTrackIndexesCache[posY] == cachedIndex) return;

    // get track infos
    info = getTrackInfo(shuffledIndexes[index]);

    // draw track number
    uintToStr(index, str, 2);
    // separation
    str[2] = ' ';

    src = info->trackName;
    dst = &str[3];

    // copy track name (limit to 61 characters max)
    len = min(strlen(src), 61);
    i = len;
    while(i--) *dst++ = *src++;

    // track name > 60 --> replace last character by '.'
    if (len > 60)
    {
        *--dst = '.';
        dst++;
    }
    else
    {
        // complete with blank
        i = 61 - len;
        while(i--) *dst++ = ' ';
    }
    // end of text
    *dst = 0;

    SYS_disableInts();

    if (index == trackPlayed)
    {
        VDP_setTextPalette(PAL1);
        VDP_drawText(str, 1, posY);
        VDP_setTextPalette(PAL0);
    }
    else VDP_drawText(str, 1, posY);

    SYS_enableInts();

    // plan updated
    planTrackIndexesCache[posY] = cachedIndex;
}

static void drawTime(s32 time, u16 x, u16 y)
{
    char str1[16];
    char str2[4];

    if (time == -1) strcpy(str1, "--:--");
    else
    {
        const u16 divider = (IS_PALSYSTEM) ? 50 : 60;
        u16 mn = time / (60 * divider);
        u16 sec = (time / divider) % 60;

        uintToStr(mn, str1, 2);
        strcat(str1, ":");
        uintToStr(sec, str2, 2);
        strcat(str1, str2);
    }

    SYS_disableInts();
    VDP_drawText(str1, x, y);
    SYS_enableInts();
}

static void drawChipsStates()
{
    drawZ80Load();
    drawYMState();
    drawPCMState();
    drawPSGState();

    SYS_disableInts();
    if (needProgressUpload)
    {
        DMA_queueDma(DMA_VRAM, progressTileBuffer, tileIndexProgressBar * TILE_SIZE, sizeof(progressTileBuffer) / 2, 2);
        needProgressUpload = FALSE;
    }
    DMA_queueDma(DMA_VRAM, z80TileBuffer, (tileIndex + (0 * 4)) * TILE_SIZE, sizeof(z80TileBuffer) / 2, 2);
    // first part
    if (evenCnt & 1)
    {
        DMA_queueDma(DMA_VRAM, ymTileBuffer, (tileIndex + (1 * 4)) * TILE_SIZE, sizeof(ymTileBuffer) / 4, 2);
        DMA_queueDma(DMA_VRAM, pcmTileBuffer, (tileIndex + ((1 + (6 * 3)) * 4)) * TILE_SIZE, sizeof(pcmTileBuffer) / 4, 2);
        DMA_queueDma(DMA_VRAM, psgTileBuffer, (tileIndex + ((1 + (6 * 3) + (4 * 3)) * 4)) * TILE_SIZE, sizeof(psgTileBuffer) / 4, 2);
    }
    // second part
    else
    {
        DMA_queueDma(DMA_VRAM, ymTileBuffer + (32 * 4 * 3 * 3), (tileIndex + ((1 + (3 * 3)) * 4)) * TILE_SIZE, sizeof(ymTileBuffer) / 4, 2);
        DMA_queueDma(DMA_VRAM, pcmTileBuffer + (32 * 4 * 3 * 2), (tileIndex + ((1 + (6 * 3) + (2 * 3)) * 4)) * TILE_SIZE, sizeof(pcmTileBuffer) / 4, 2);
        DMA_queueDma(DMA_VRAM, psgTileBuffer + (32 * 4 * 1 * 2), (tileIndex + ((1 + (6 * 3) + (4 * 3) + (2 * 1)) * 4)) * TILE_SIZE, sizeof(psgTileBuffer) / 4, 2);
    }
    DMA_queueDma(DMA_VRAM, scrollB, VDP_HSCROLL_TABLE + 2, SCROLLV_LEN, 4);
    SYS_enableInts();
}

static void drawZ80Load()
{
    const u16 h1 = (cpuload - 25) >> 1;
    const u16 h2 = dmawaitload >> 1;
    u32 c1, c2;

    if (h1 > 26)
    {
        c1 = 0x44444444;
        c2 = 0x55555555;
    }
    else
    {
        c1 = 0xCCCCCCCC;
        c2 = 0xDDDDDDDD;
    }

    fillColumnZ80(h1, h2, c1, c2, 0xFFFFFFFF);
}

static void drawYMState()
{
    const YM_CH *ch;
    Sprite** panSpr;
    u16 c;
    u16 x;

    // second part
    if (evenCnt & 1)
    {
        ch = &(ym.channels[3]);
        panSpr = &YMPanSprites[3];
        x = 1 + ((4 + 2) * 3);
    }
    // first part
    else
    {
        ch = &(ym.channels[0]);
        panSpr = &YMPanSprites[0];
        x = 1;
    }

    c = 3;
    while(c--)
    {
        const YM_SLOT *sl = &(ch->slots[0]);
        const u16 pan = (ch->pan >> 6) & 3;
        u16 s;

        // set LEFT/RIGHT panning info
        if (pan)
        {
            SPR_setVisibility(*panSpr, VISIBLE);
            SPR_setAnim(*panSpr, pan - 1);
        }
        else
            SPR_setVisibility(*panSpr, HIDDEN);
        panSpr++;

        s = 4;
        while(s--)
        {
            u16 color1, color2;
            u16 freq;

            switch(sl->out)
            {
                case 0:
                    color1 = 0x6666;
                    color2 = 0x7777;
                    break;

                case 1:
                    color1 = 0x8888;
                    color2 = 0x9999;
                    break;

                case 2:
                    color1 = 0xAAAA;
                    color2 = 0xBBBB;
                    break;

                case 3:
                default:
                    color1 = 0xCCCC;
                    color2 = 0xDDDD;
                    break;
            }

            // bright = current env, dark = TL
            fillColumnYM(x, 0x1F - (sl->tl >> 8), 0x1F - (sl->env >> 8), 0x0000, color1, color2);

            // special mode
            if (ym.ch3_special && (ch == &(ym.channels[2])))
            {
                if (s == 3) freq = ym.channels[2].freq >> 4;
                else freq = ym.channels[2 - s].ext_freq >> 4;
            }
            else
            {
                u16 mul = sl->mul;
                if (mul) freq = (ch->freq >> 4) * mul;
                else freq = ch->freq >> 5;
            }

            // we assume "too high" frequency
            if (freq > 0x3FFF) freq = 0x3FFF;

            // compute approximed log2
//            freq = getApproximatedLog2(freq << 16) >> 14;
//            freq -= freq >> 2;
//            if (freq > 0x1F) freq = 0x1F;
            freq = log_tab[freq >> 4];

            drawHorizontalBarYM(x, freq, 0x5555);
            if (sl->sl > sl->tl)
                drawHorizontalBarYM(x, 0x1F - (sl->sl >> 8), 0x2222);

            x++;
            sl++;
        }

        x += 2;
        ch++;
    }
}

static u8* drawPCM(u8* dst, u16 addr)
{
    s8 *pcm;
    u16 i;
    u16 x;

    pcm = (s8*) (addr << 8);

    // draw waveform (3 column per channel)
    i = 3;
    while(i--)
    {
        // column
        x = 8;
        while(x--)
        {
            u8* d;
            u16 y;
            s8 v;

            // want a value between [-15..+15]
            v = *pcm >> 3;
            pcm += 10;
            // y between [0..31]
            y = 16 + v;

            d = dst + (y * 4);
            if (x & 1) *d = (*d & 0x0F) | 0x30;
            else
            {
                *d = (*d & 0xF0) | 0x03;
                dst++;
            }
        }

        dst += (32 * 4) - 4;
    }

    return dst;
}

static void drawPCMState()
{
    u16 nullsmp;
    u16 addr[2];
    vu8 *pb;
    u8 *dst;

    // second part
    if (evenCnt & 1)
    {
        // point on PCM address
        pb = (u8 *) (Z80_DRV_PARAMS + 0x12 + (8 * 2));
        // pcm tile buffer
        dst = (u8*) pcmTileBuffer + (32 * 4 * 3 * 2);
    }
    // first part
    else
    {
        // point on PCM address
        pb = (u8 *) (Z80_DRV_PARAMS + 0x12);
        // pcm tile buffer
        dst = (u8*) pcmTileBuffer;
    }

    SYS_disableInts();
    Z80_requestBus(TRUE);

    // store addr
    addr[0] = pb[0] | (pb[1] << 8);
    addr[1] = pb[8] | (pb[9] << 8);

    Z80_releaseBus();
    SYS_enableInts();

    // null sample addr / 256
    nullsmp = ((u32) smp_null) >> 8;
    // next
    nullsmp++;

    // fix possible null sample end read
    if (addr[0] == nullsmp) addr[0] = nullsmp - 1;
    if (addr[1] == nullsmp) addr[1] = nullsmp - 1;

    // clear pcm buffer
    memset(dst, 0, sizeof(pcmTileBuffer) / 2);

    // draw PCM
    dst = drawPCM(dst, addr[0]);
    drawPCM(dst, addr[1]);

    evenCnt++;
}

static void drawPSGState()
{
    u16 x;
    u16 h;
    u16 tone;
    u16 *src_env;
    u16 *src_tone;

    // second part
    if (evenCnt & 1)
    {
        src_env = &psg.env[2];
        src_tone = &psg.tone[2];
        x = 2;
    }
    // first part
    else
    {
        src_env = &psg.env[0];
        src_tone = &psg.tone[0];
        x = 0;
    }

    // update 2 channels
    h = 0x1E - ((*src_env++ & 0xF) << 1);
    tone = 0x1F - (*src_tone++ >> 5);
    fillColumnPSG(x, h, 0xCCCCCCCC, 0xDDDDDDDD);
    drawHorizontalBarPSG(x++, tone, 0x55555555);
    h = 0x1E - ((*src_env & 0xF) << 1);
    tone = 0x1F - (*src_tone >> 5);
    fillColumnPSG(x, h, 0xCCCCCCCC, 0xDDDDDDDD);
    drawHorizontalBarPSG(x, tone, 0x55555555);
}

static void drawHorizontalBarYM(u16 x, u16 y, u16 color)
{
    u16 *dst;
    u16 adjy = 0x1F - y;

    dst = (u16*) ymTileBuffer;
    dst += x & 1;
    dst += 16 * 4 * (x >> 1);
    dst += 2 * adjy;

    *dst = color;
}

static void drawHorizontalBarPSG(u16 x, u16 y, u32 color)
{
    u32 *dst;
    u16 adjy = 0x1F - y;

    dst = (u32*) psgTileBuffer;
    dst += 8 * 4 * x;
    dst += adjy;

    *dst = color;
}

static void fillColumnYM(u16 x, u16 h1, u16 h2, u16 backColor, u16 firstColor, u16 secondColor)
{
    u16 *dst;
    u16 color;
    u16 h;

    dst = (u16*) ymTileBuffer;
    dst += x & 1;
    dst += 16 * 4 * (x >> 1);

    h = 4 * 8;

    color = backColor;
    while(h > h1)
    {
        *dst = color;
        dst += 2;
        h--;
    }

    color = firstColor;
    while(h > h2)
    {
        *dst = color;
        dst += 2;
        h--;
    }

    color = secondColor;
    while(h > 0)
    {
        *dst = color;
        dst += 2;
        h--;
    }
}

static void fillColumnZ80(u16 h1, u16 h2, u32 backColor, u32 firstColor, u32 secondColor)
{
    u32 *dst;
    u32 color;
    u16 h;

    dst = (u32*) z80TileBuffer;
    h = 4 * 8;

    color = backColor;
    while(h > h1)
    {
        *dst++ = color;
        h--;
    }

    color = firstColor;
    while(h > h2)
    {
        *dst++ = color;
        h--;
    }

    color = secondColor;
    while(h > 0)
    {
        *dst++ = color;
        h--;
    }
}

static void fillColumnPSG(u16 x, u16 heigth, u32 backColor, u32 frontColor)
{
    u32 *dst;
    u32 color;
    u16 h;

    dst = (u32*) psgTileBuffer;
    dst += 8 * 4 * x;
    h = 4 * 8;

    color = backColor;
    while(h > heigth)
    {
        *dst++ = color;
        h--;
    }

    color = frontColor;
    while(h > 0)
    {
        *dst++ = color;
        h--;
    }
}

static void initBGScroll()
{
    s16 i, ns, s;
    fix16* sf = scrollBF;
    fix16* ss = scrollSpeed;

	// create the scrolling offset table
	s = 1;
	i = SCROLLV_LEN;
	while(i--)
	{
		*sf++ = FIX16(0);
		do ns = -((random() & 0x3F) + 10);
		while(ns == s);
		*ss++ = ns;
		s = ns;
	}
}

static void updateBGScroll()
{
    s16 i;
    fix16* sf = scrollBF;
    s16* s = scrollB;
    fix16* ss = scrollSpeed;

    i = SCROLLV_LEN;
	while(i--)
	{
        *sf += *ss++;
        *s++ = fix16ToInt(*sf++);
    }
}

static void updateListScroll()
{
    s16 wantedPos = (trackIndexList - 16) * 8;

    // need to change scrolling ?
    if (trackListScrollPos != wantedPos)
    {
        s16 delta = (wantedPos - trackListScrollPos);
        s16 step = delta >> 3;
        s16 cursorPos;

        if (step == 0)
        {
            if (wantedPos < trackListScrollPos) step = -1;
            else step = 1;
        }

        trackListScrollPos += step;

        // set new track list scroll position
        VDP_setVerticalScroll(BG_A, trackListScrollPos);

        cursorPos = trackIndexList;
        cursorPos %= numMusic;
        if (cursorPos < 0) cursorPos += numMusic;
        cursorPos *= 47;
        cursorPos /= numMusic;

        // set cursor position
        SPR_setPosition(trackListCursor, 0, 101 + cursorPos);
    }
}


static void doJoyActions()
{
    u16 state = joyState;

    // start pressed
    if (state & BUTTON_START)
    {
        if (state & (BUTTON_UP | BUTTON_DOWN))
        {
            u16 repeat = 0;

            // handle repeat
            if (pressTime >= 50)
                repeat = 1;
            else if ((pressTime >= 30) && (!(pressTime & 1)))
                repeat = 1;
            else if ((pressTime >= 15) && (!(pressTime & 3)))
                repeat = 1;
            else if ((pressTime >= 7) && (!(pressTime & 7)))
                repeat = 1;

            if (repeat)
            {
                if (state & BUTTON_UP) tempo += repeat;
                else tempo -= repeat;

                if (tempo < 1) tempo = 1;
                else if (tempo > MAX_TEMPO) tempo = MAX_TEMPO;

                XGM_setMusicTempo(tempo);
                if (IS_PALSYSTEM) tempoDef = 50;
                else tempoDef = 60;

                // refresh player settings
                refresh |= REFRESH_SETTING;
            }
        }

        fastForward = FALSE;
    }
    else
    {
        // track index change in list
        if (state & (BUTTON_UP | BUTTON_DOWN))
        {
            u16 repeat = 0;

            // handle repeat
            if ((pressTime >= 40) && (!(pressTime & 1)))
                repeat = 1;
            else if ((pressTime >= 20) && (!(pressTime & 3)))
                repeat = 1;
            else if ((pressTime >= 7) && (!(pressTime & 7)))
                repeat = 1;

            if (repeat)
            {
                if (state & BUTTON_UP) trackIndexList -= repeat;
                else trackIndexList += repeat;

                // refresh track list
                refresh |= REFRESH_TRACKLIST;
            }
        }

        // fast forward ?
        if (state & BUTTON_C)
        {
            // directly alter xgm driver timer
            if (XGM_isPlaying()) xgmTempoCnt -= 800;
            fastForward = TRUE;
        }
        else
        {
            // stop immediately fast forward
            if (fastForward) xgmTempoCnt = 0;
            fastForward = FALSE;
        }
    }

    // no need to count over 5 seconds
    if (++pressTime >= MAX_PRESSTIME)
        pressTime -= (MAX_PRESSTIME / 2);
}

static void joyEvent(u16 joy, u16 changed, u16 state)
{
    u16 pressed = changed & state;

    joyState = state;

    // keep buttons state
    if (buttonsPressed != state)
    {
        buttonsPressed = state;
        // reset press time
        pressTime = 0;
    }

    if (state & BUTTON_START)
    {
        // alternate action
        if (pressed & (BUTTON_UP | BUTTON_DOWN))
        {
            if (pressed & BUTTON_UP) tempo++;
            else tempo--;

            if (tempo < 1) tempo = 1;
            else if (tempo > MAX_TEMPO) tempo = MAX_TEMPO;

            XGM_setMusicTempo(tempo);
            if (IS_PALSYSTEM) tempoDef = 50;
            else tempoDef = 60;

            refresh |= REFRESH_SETTING;
        }
        else if (pressed & (BUTTON_LEFT | BUTTON_RIGHT))
        {
            if (pressed & BUTTON_RIGHT) loop++;
            else loop--;

            if (loop < 1) loop = 0;
            else if (loop > MAX_LOOP) loop = MAX_LOOP;

            if (loop == MAX_LOOP) XGM_setLoopNumber(-1);
            else XGM_setLoopNumber(loop);

            refresh |= REFRESH_SETTING;
        }

        // enable/disable starfield
        if (pressed & BUTTON_A)
        {
            if (bgEnabled)
            {
                bgEnabled = FALSE;
                VDP_setVerticalScroll(BG_B, 32 * 8);
            }
            else
            {
                bgEnabled = TRUE;
                VDP_setVerticalScroll(BG_B, 0 * 8);
            }
        }
        // hide/show playlist
        if (pressed & BUTTON_B)
        {
            if (hidePlaylist)
            {
                hidePlaylist = FALSE;
                VDP_setHilightShadow(TRUE);
            }
            else
            {
                hidePlaylist = TRUE;
                VDP_setHilightShadow(FALSE);
            }
            // refresh track list
            refresh |= REFRESH_TRACKLIST;
        }
        // enable/disable shuffle mode
        if (pressed & BUTTON_C)
        {
            if (shuffle) shuffle = FALSE;
            else shuffle = TRUE;

            // rebuild shuffle list each time we enable shuffle
            refresh |= REFRESH_SHUFFLE | REFRESH_SETTING;
        }
    }
    else
    {
        // normal action
        if (pressed & BUTTON_UP)
        {
            trackIndexList--;
            refresh |= REFRESH_TRACKLIST;
        }
        else if (pressed & BUTTON_DOWN)
        {
            trackIndexList++;
            refresh |= REFRESH_TRACKLIST;
        }

        if (pressed & BUTTON_LEFT)
        {
            // it was stopped ?
            if (trackPlayed == -1)
            {
                // start from last
                trackPlayedRawIndex = -1;
                trackPlayed = numMusic - 1;
            }
            else
            {
                trackPlayedRawIndex--;
                trackPlayed--;
                if (trackPlayed < 0) trackPlayed = numMusic - 1;
            }
            wantStartPlay = TRUE;
        }
        else if (pressed & BUTTON_RIGHT)
        {
            trackPlayedRawIndex++;
            trackPlayed++;
            if (trackPlayed >= numMusic) trackPlayed = 0;
            wantStartPlay = TRUE;
        }

        // start / pause / resume
        if (pressed & BUTTON_A)
        {
            // start play ?
            if (trackPlayed == -1)
            {
                // set play track and start play
                trackPlayedRawIndex = trackIndexList;
                trackPlayed = trackIndexList % numMusic;
                if (trackPlayed < 0) trackPlayed += numMusic;
                wantStartPlay = TRUE;
            }
            else
            {
                if (paused)
                {
                    paused = FALSE;
                    playStateCnt = 0;
                    XGM_resumePlay();
                }
                else
                {
                    paused = TRUE;
                    playStateCnt = 0;
                    XGM_pausePlay();
                }

                refresh |= REFRESH_CONTROL;
            }
        }
        // stop
        else if (pressed & BUTTON_B)
        {
            stopPlay();
        }
    }

    // START pressed or released --> refresh control
    if (changed & BUTTON_START)
        refresh |= REFRESH_CONTROL;

    // PCM 1
    if (pressed & BUTTON_X)
    {
        XGM_setPCM(64, pcm_hat2, sizeof(pcm_hat2));
        XGM_startPlayPCM(64, 1, SOUND_PCM_CH2);
    }
    // PCM 2
    if (pressed & BUTTON_Y)
    {
        XGM_setPCM(65, pcm_snare2, sizeof(pcm_snare2));
        XGM_startPlayPCM(65, 1, SOUND_PCM_CH3);
    }
    // PCM 3
    if (pressed & BUTTON_Z)
    {
        XGM_setPCM(66, pcm_voice, sizeof(pcm_voice));
        XGM_startPlayPCM(66, 1, SOUND_PCM_CH4);
    }
}


void vint()
{
    // do vblank process directly from the vint callback (easier to manage here)
    SYS_doVBlankProcessEx(IMMEDIATELY);

    // set window visible from first row up to row 13
    VDP_setWindowVPos(FALSE, 13);
    // update track list scroll position
    updateListScroll();

    // music is playing ?
    if ((trackPlayed != -1) && !paused)
    {
        // music not yet finish...
        if (XGM_isPlaying())
        {
            // update elapsed and frameToParse
            u32 newElapsed = XGM_getElapsed();
            s32 delta;

            if (elapsed == -1) delta = newElapsed;
            else delta = newElapsed - elapsed;

            frameToParse += delta;
            elapsed = newElapsed;
            playIdleCnt = 0;
        }
        // music finished ?
        else
        {
            // more than 1 second finished ?
            if (playIdleCnt > 60)
            {
                // pass to next track
                trackPlayedRawIndex++;
                trackPlayed++;
                if (trackPlayed >= numMusic) trackPlayed = 0;
                wantStartPlay = TRUE;
            }
            else playIdleCnt++;
        }
    }

    // 1 more frame to update for YM2612 env calculation
    frameToUpdate++;

    // update Z80 cpu meter
    u32 load = XGM_getCPULoad();

    cpuload = load & 0xFFFF;
    dmawaitload = load >> 16;
}

void hint()
{
    // set window visible from row 20 up to last row
    VDP_setWindowVPos(TRUE, 20);
}
