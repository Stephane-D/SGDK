#ifndef _VDP_H_
#define _VDP_H_


#define GFX_DATA_PORT           0xC00000
#define GFX_CTRL_PORT           0xC00004
#define GFX_HVCOUNTER_PORT      0xC00008

#define VDP_FIFOEMPTY_FLAG      (1 << 9)
#define VDP_FIFOFULL_FLAG       (1 << 8)
#define VDP_VINTPENDING_FLAG    (1 << 7)
#define VDP_SPROVERFLOW_FLAG    (1 << 6)
#define VDP_SPRCOLLISION_FLAG   (1 << 5)
#define VDP_ODDFRAME_FLAG       (1 << 4)
#define VDP_VBLANK_FLAG         (1 << 3)
#define VDP_HBLANK_FLAG         (1 << 2)
#define VDP_DMABUSY_FLAG        (1 << 1)
#define VDP_PALMODE_FLAG        (1 << 0)

#define WPLAN                   0xB000
#define HSCRL                   0xB800
#define SLIST                   0xBC00
#define APLAN                   0xC000
#define BPLAN                   0xE000

#define TILE_SPACE              0xB000

#define TILE_MAXNUM             (TILE_SPACE / 32)
#define TILE_MAXINDEX           (TILE_MAXNUM - 1)
#define TILE_SYSTEMINDEX        0x0000
#define TILE_SYSTEMLENGHT       0x10
#define TILE_USERINDEX          (TILE_SYSTEMINDEX + TILE_SYSTEMLENGHT)
#define TILE_FONTINDEX          (TILE_MAXNUM - FONT_LEN)
#define TILE_USERLENGHT         (TILE_FONTINDEX - TILE_USERINDEX)
#define TILE_SYSTEM             (TILE_SYSTEMINDEX * 32)
#define TILE_USER               (TILE_USERINDEX * 32)
#define TILE_FONT               (TILE_FONTINDEX * 32)

#define PAL0                    0
#define PAL1                    1
#define PAL2                    2
#define PAL3                    3

#define GFX_READ_VRAM_ADDR(adr)     ((0x0000 + ((adr) & 0x3FFF)) << 16) + (((adr) >> 14) | 0x00)
#define GFX_READ_CRAM_ADDR(adr)     ((0x0000 + ((adr) & 0x3FFF)) << 16) + (((adr) >> 14) | 0x20)
#define GFX_READ_VSRAM_ADDR(adr)    ((0x0000 + ((adr) & 0x3FFF)) << 16) + (((adr) >> 14) | 0x10)

#ifdef FAST_WRITE_VRAM_ADDR
#define GFX_WRITE_VRAM_ADDR(adr)    vramwrite_tab[adr]
#else
#define GFX_WRITE_VRAM_ADDR(adr)    ((0x4000 + ((adr) & 0x3FFF)) << 16) + (((adr) >> 14) | 0x00)
#endif
#define GFX_WRITE_CRAM_ADDR(adr)    ((0xC000 + ((adr) & 0x3FFF)) << 16) + (((adr) >> 14) | 0x00)
#define GFX_WRITE_VSRAM_ADDR(adr)   ((0x4000 + ((adr) & 0x3FFF)) << 16) + (((adr) >> 14) | 0x10)

#define GFX_DMA_VRAM_ADDR(adr)      ((0x4000 + ((adr) & 0x3FFF)) << 16) + (((adr) >> 14) | 0x80)
#define GFX_DMA_CRAM_ADDR(adr)      ((0xC000 + ((adr) & 0x3FFF)) << 16) + (((adr) >> 14) | 0x80)
#define GFX_DMA_VSRAM_ADDR(adr)     ((0x4000 + ((adr) & 0x3FFF)) << 16) + (((adr) >> 14) | 0x90)

#define GFX_VERT_SCROLL(adr)        GFX_WRITE_VSRAM_ADDR(adr)
#define GFX_HORZ_SCROLL(adr)        GFX_WRITE_VRAM_ADDR(adr)

#define GET_VDPSTATUS(flag)         ((*(vu16*)(GFX_CTRL_PORT)) & (flag))
#define IS_PALSYSTEM                GET_VDPSTATUS(VDP_PALMODE_FLAG)

#define GET_HVCOUNTER               (*(vu16*)(GFX_HVCOUNTER_PORT))
#define GET_HCOUNTER                (GET_HVCOUNTER & 0xFF)
#define GET_VCOUNTER                (GET_HVCOUNTER >> 8)


void VDP_init();

u8   VDP_getReg(u16 reg);
void VDP_setReg(u16 reg, u8 value);

u8   VDP_getEnable();
void VDP_setEnable(u8 value);

u16  VDP_getScanlineNumber();
u16  VDP_getScreenHeight();
void VDP_setScreenHeight224();
void VDP_setScreenHeight240();
u16  VDP_getScreenWidth();
void VDP_setScreenWidth256();
void VDP_setScreenWidth320();

u16  VDP_getPlanWidth();
u16  VDP_getPlanHeight();
void VDP_setPlanSize(u16 w, u16 h);

u8   VDP_getAutoInc();
void VDP_setAutoInc(u8 value);

void VDP_setHInterrupt(u8 value);
void VDP_setHilightShadow(u8 value);

u8   VDP_getHIntCounter();
void VDP_setHIntCounter(u8 value);

void VDP_waitDMACompletion();
void VDP_waitFIFOEmpty();

void VDP_waitVSync();

void VDP_resetScreen();
void VDP_blit();


#endif // _VDP_H_
