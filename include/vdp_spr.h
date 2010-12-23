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
} _spritedef;


extern _spritedef spriteDefCache[MAX_SPRITE];


void VDP_resetSprites();

void VDP_setSprite(u16 index, u16 x, u16 y, u8 size, u16 tile_attr, u8 link);
void VDP_setSpriteP(u16 index, const _spritedef *sprite);
void VDP_setSpriteDirect(u16 index, u16 x, u16 y, u8 size, u16 tile_attr, u8 link);
void VDP_setSpriteDirectP(u16 index, const _spritedef *sprite);

void VDP_setSpritePosition(u16 index, u16 x, u16 y);

void VDP_setSprites(u16 index, const _spritedef *sprites, u16 num);

void VDP_updateSprites();



#endif // _VDP_SPR_H_
