/**
 * Original author: Astrofa
 *
 * modified by Stef
 */

#include <genesis.h>
#include <gfx.h>

#define	TABLE_LEN       220
#define MAX_DONUT       24
#define MAX_DONUT_ANIM  8

static void fastStarFieldFX();
//static s16 compareSprite(Sprite* s1, Sprite* s2);
static void joyEvent(u16 joy, u16 changed, u16 state);

s16 scroll_PLAN_B[TABLE_LEN];
fix16 scroll_PLAN_B_F[TABLE_LEN];
fix16 scroll_speed[TABLE_LEN];
Sprite* sprites[MAX_DONUT];
u16 animVramIndexes[MAX_DONUT_ANIM];

int main()
{
	fastStarFieldFX();
	return 0;
}

static void fastStarFieldFX()
{
	u16 vramIndex = TILE_USERINDEX;
	s16 i, ns, s;

	SYS_disableInts();

	VDP_clearPlan(PLAN_A, 0);
	VDP_clearPlan(PLAN_B, 0);
	VDP_setPlanSize(32, 32);

	/* Draw the foreground */
	VDP_drawImageEx(PLAN_B, &starfield, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, vramIndex), 0, 0, TRUE, FALSE);
	vramIndex += starfield.tileset->numTile;

	/*	Set the proper scrolling mode (line by line) */
	VDP_setScrollingMode(HSCROLL_LINE, VSCROLL_PLANE);

	/*	Create the scrolling offset table */
	s = 1;
	for(i = 0; i < TABLE_LEN; i++)
	{
		scroll_PLAN_B_F[i] = FIX16(0);
		do
		{
			ns = -((random() & 0x3F) + 10);
		}
		while (ns == s);
		scroll_speed[i] = ns;
		s = ns;
	}

	/* Setup the sprites */
	SPR_init(MAX_DONUT, 0, 0);
	for(i = 0; i < MAX_DONUT; i++)
    {
	    sprites[i] = SPR_addSprite(&donut, 0, 0, TILE_ATTR_FULL(PAL2, TRUE, FALSE, FALSE, 0));
	    /* Disable auto tile upload */
	    SPR_setAutoTileUpload(sprites[i], FALSE);
	    /* Enable Y sorting */
//	    SPR_setYSorting(sprites[i], TRUE);
	    /* Manually set VRAM index */
	    SPR_setVRAMTileIndex(sprites[i], vramIndex);
    }

    Animation* anim = donut.animations[0];
    for(i = 0; i < anim->numFrame; i++)
    {
        TileSet* tileset = anim->frames[i]->tileset;
        VDP_loadTileSet(tileset, vramIndex, TRUE);
        animVramIndexes[i] = vramIndex;
        vramIndex += tileset->numTile;
    }

	SPR_update();

	VDP_setPalette(PAL2, donut.palette->data);

	SYS_enableInts();

	JOY_setEventHandler(&joyEvent);

	/*	Start !!!! */
	s = 0;
	while (TRUE)
	{
		VDP_waitVSync();

        SYS_disableInts();

		VDP_showFPS(TRUE);
		/* 	Scroll the starfield */
		VDP_setHorizontalScrollLine(PLAN_B, 2, scroll_PLAN_B, TABLE_LEN, TRUE);

		SYS_enableInts();

		for(i = 0; i < TABLE_LEN; i++)
        {
			scroll_PLAN_B_F[i] += scroll_speed[i];
			scroll_PLAN_B[i] = fix16ToInt(scroll_PLAN_B_F[i]) & 0xFF;
        }

		/*	Animate the donuts */
		for(i = 0; i < MAX_DONUT; i++)
		{
	        SPR_setPosition(sprites[i], (cosFix16(s + (i << 5)) << 1) + 160 - 16, sinFix16(s + (i << 5)) + 112 - 16);
			SPR_setVRAMTileIndex(sprites[i], animVramIndexes[((s >> 4) + i) & 0x7]);
		}

		s += 4;
		SPR_update();

        SPR_sort(NULL);

		if ((s & 0x007F) == 0x0070)
            SPR_logProfil();
//		if ((s & 0x03FF) == 0x0370)
//            SPR_sort(&compareSprite);
	}
}


//static s16 compareSprite(Sprite* s1, Sprite* s2)
//{
//    const s16 y1 = s1->y;
//    const s16 y2 = s2->y;
//
//    if (y1 > y2) return 1;
//    else if (y1 < y2) return -1;
//
//    return 0;
//}

static void joyEvent(u16 joy, u16 changed, u16 state)
{
//    u16 pressed = changed & state;
}
