/**
 * \file vdp_spr.h
 * \brief VDP Sprite support
 * \author Stephane Dallongeville
 * \date 08/2011
 *
 * This unit provides methods to manipulate the VDP Sprites.
 * The Sega Genesis VDP can handle up to 80 simultanous sprites of 4x4 tiles (32x32 pixels).
 */

#ifndef _VDP_SPR_H_
#define _VDP_SPR_H_

#define MAX_SPRITE          80

#define SPRITE_SIZE(w, h)   ((((w) - 1) << 2) | ((h) - 1))


typedef struct
{
    s16 posx;
    s16 posy;
    u16 tile_attr;
    u8 size;
    u8 link;
} SpriteDef;

// GenRes support
typedef struct
{
    u16 *pal;               // pointer to pal data
    u32 **sprites;          // pointer to sprites data
    u16 count;              // nb sprites
    u16 width;              // width of each sprite in pixels (not tiles!)
    u16 height;             // height of each sprite in pixels (not tiles!)
    u16 size;               // since we use width/height in pixel, useful info on sprite size
                            // TODO : size is not SGDK compliant, you need to use size>>8
                            //        will be fixed in coming release
} GenResSprites;


extern SpriteDef spriteDefCache[MAX_SPRITE];


void VDP_resetSprites();

void VDP_setSprite(u16 index, u16 x, u16 y, u8 size, u16 tile_attr, u8 link);
void VDP_setSpriteP(u16 index, const SpriteDef *sprite);
void VDP_setSpriteDirect(u16 index, u16 x, u16 y, u8 size, u16 tile_attr, u8 link);
void VDP_setSpriteDirectP(u16 index, const SpriteDef *sprite);

void VDP_setSpritePosition(u16 index, u16 x, u16 y);

void VDP_setSprites(u16 index, const SpriteDef *sprites, u16 num);

void VDP_updateSprites();



#endif // _VDP_SPR_H_
