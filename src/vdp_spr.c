#include "config.h"
#include "types.h"

#include "vdp.h"
#include "vdp_spr.h"


#include "tools.h"
#include "vdp_dma.h"
#include "vdp_tile.h"

#include "tab_vram.h"

// no static so it can be read
_spritedef spriteDefCache[MAX_SPRITE];

static u16 spriteNum;


void VDP_resetSprites()
{
    spriteDefCache[0].posx = -0x80;
    spriteDefCache[0].link = 0;

    // needed to send the null sprite to the VDP
    spriteNum = 1;
}


void VDP_setSpriteDirect(u16 index, u16 x, u16 y, u8 size, u16 tile_attr, u8 link)
{
    vu32 *plctrl;
    vu16 *pwdata;
    u32 addr;

    VDP_setAutoInc(2);

    addr = SLIST + (index * 8);

    /* Point to vdp port */
    plctrl = (u32 *) GFX_CTRL_PORT;
    pwdata = (u16 *) GFX_DATA_PORT;

    *plctrl = GFX_WRITE_VRAM_ADDR(addr);

    // y position
    *pwdata = 0x80 + y;
    // size & link
    *pwdata = (size << 8) | link;
    // tile attribut
    *pwdata = tile_attr;
    // x position
    *pwdata = 0X80 + x;
}

void VDP_setSpriteDirectP(u16 index, const _spritedef *sprite)
{
    vu32 *plctrl;
    vu16 *pwdata;
    u32 addr;

    VDP_setAutoInc(2);

    addr = SLIST + (index * 8);

    /* Point to vdp port */
    plctrl = (u32 *) GFX_CTRL_PORT;
    pwdata = (u16 *) GFX_DATA_PORT;

    *plctrl = GFX_WRITE_VRAM_ADDR(addr);

    // y position
    *pwdata = 0x80 + sprite->posy;
    // size & link
    *pwdata = (sprite->size << 8) | sprite->link;
    // tile attribut
    *pwdata = sprite->tile_attr;
    // x position
    *pwdata = 0X80 + sprite->posx;
}


void VDP_setSprite(u16 index, u16 x, u16 y, u8 size, u16 tile_attr, u8 link)
{
    _spritedef *sprite;

    if (index >= spriteNum) spriteNum = index + 1;

    sprite = &spriteDefCache[index];

    sprite->posx = x;
    sprite->posy = y;
    sprite->tile_attr = tile_attr;
    sprite->size = size;
    sprite->link = link;
}

void VDP_setSpriteP(u16 index, const _spritedef *sprite)
{
    _spritedef *spriteDst;

    if (index >= spriteNum) spriteNum = index + 1;

    spriteDst = &spriteDefCache[index];

    spriteDst->posx = sprite->posx;
    spriteDst->posy = sprite->posy;
    spriteDst->tile_attr = sprite->tile_attr;
    spriteDst->size = sprite->size;
    spriteDst->link = sprite->link;
}

void VDP_setSprites(u16 index, const _spritedef *sprites, u16 num)
{
    if ((index + num) > spriteNum) spriteNum = index + num;

    fastmemcpy(&spriteDefCache[index], sprites, sizeof(_spritedef) * num);
}

void VDP_setSpritePosition(u16 index, u16 x, u16 y)
{
    _spritedef *sprite;

    if (index >= spriteNum) spriteNum = index + 1;

    sprite = &spriteDefCache[index];

    sprite->posx = x;
    sprite->posy = y;
}


void VDP_updateSprites()
{
    vu32 *plctrl;
    vu16 *pwdata;

    if (spriteNum == 0) return;

    VDP_setAutoInc(2);

    /* Point to vdp port */
    plctrl = (u32 *) GFX_CTRL_PORT;
    pwdata = (u16 *) GFX_DATA_PORT;

    *plctrl = GFX_WRITE_VRAM_ADDR(SLIST);

    {
        _spritedef *sprite;
        u16 i;

        sprite = &spriteDefCache[0];
        i = spriteNum;
        while(i--)
        {
            // y position
            *pwdata = 0x80 + sprite->posy;
            // size & link
            *pwdata = (sprite->size << 8) | sprite->link;
            // tile attribut
            *pwdata = sprite->tile_attr;
            // x position
            *pwdata = 0X80 + sprite->posx;

            // next sprite
            sprite++;
        }
    }

    // we won't upload unmodified sprite
    spriteNum = 0;
}
