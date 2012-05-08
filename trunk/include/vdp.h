/**
 * \file vdp.h
 * \brief VDP main
 * \author Stephane Dallongeville
 * \date 08/2011
 *
 * This unit provides general VDP methods :<br/>
 * - initialisation<br/>
 * - get / set register<br/>
 * - get / set resolution<br/>
 * - enable / disable VDP features<br/>
 */

#ifndef _VDP_H_
#define _VDP_H_


/**
 *  \def GFX_DATA_PORT
 *      VDP Data port address.
 */
#define GFX_DATA_PORT           0xC00000
/**
 *  \def GFX_CTRL_PORT
 *      VDP Control port address.
 */
#define GFX_CTRL_PORT           0xC00004
/**
 *  \def GFX_HVCOUNTER_PORT
 *      VDP HV counter port address.
 */
#define GFX_HVCOUNTER_PORT      0xC00008


/**
 *  \def VDP_FIFOEMPTY_FLAG
 *      VDP FIFO empty flag.
 */
#define VDP_FIFOEMPTY_FLAG      (1 << 9)
/**
 *  \def VDP_FIFOFULL_FLAG
 *      VDP FIFO full flag.
 */
#define VDP_FIFOFULL_FLAG       (1 << 8)
/**
 *  \def VDP_VINTPENDING_FLAG
 *      VDP Vertical interrupt pending flag.
 */
#define VDP_VINTPENDING_FLAG    (1 << 7)
/**
 *  \def VDP_SPROVERFLOW_FLAG
 *      VDP sprite overflow flag.
 */
#define VDP_SPROVERFLOW_FLAG    (1 << 6)
/**
 *  \def VDP_SPRCOLLISION_FLAG
 *      VDP sprite collision flag.
 */
#define VDP_SPRCOLLISION_FLAG   (1 << 5)
/**
 *  \def VDP_ODDFRAME_FLAG
 *      VDP odd frame flag.
 */
#define VDP_ODDFRAME_FLAG       (1 << 4)
/**
 *  \def VDP_VBLANK_FLAG
 *      VDP Vertical blanking flag.
 */
#define VDP_VBLANK_FLAG         (1 << 3)
/**
 *  \def VDP_HBLANK_FLAG
 *      VDP Horizontal blanking flag.
 */
#define VDP_HBLANK_FLAG         (1 << 2)
/**
 *  \def VDP_DMABUSY_FLAG
 *      VDP DMA busy flag.
 */
#define VDP_DMABUSY_FLAG        (1 << 1)
/**
 *  \def VDP_PALMODE_FLAG
 *      VDP PAL mode flag.
 */
#define VDP_PALMODE_FLAG        (1 << 0)

#define WPLAN                   0xB000
#define HSCRL                   0xB800
#define SLIST                   0xBC00
#define APLAN                   0xC000
#define BPLAN                   0xE000

/**
 *  \def VDP_PLAN_WINDOW
 *      VDP window plan tilemap address in VRAM.
 */
#define VDP_PLAN_WINDOW         WPLAN
/**
 *  \def VDP_SCROLL_H
 *      VDP horizontal scroll table address in VRAM.
 */
#define VDP_SCROLL_H            HSCRL
/**
 *  \def VDP_SPRITE_LIST
 *      VDP sprite list table address in VRAM.
 */
#define VDP_SPRITE_LIST         SLIST
/**
 *  \def VDP_PLAN_A
 *      VDP background A tilemap address in VRAM.
 */
#define VDP_PLAN_A              APLAN
/**
 *  \def VDP_PLAN_B
 *      VDP background B tilemap address in VRAM.
 */
#define VDP_PLAN_B              BPLAN


/**
 *  \def TILE_SIZE
 *      Size of a single tile in byte.
 */
#define TILE_SIZE               32
#define TILE_INDEX_MASK         (0xFFFF / TILE_SIZE)

/**
 *  \def TILE_SPACE
 *      Space in byte for tile in VRAM.
 */
#define TILE_SPACE              0xB000

/**
 *  \def TILE_MAXNUM
 *      Maximum number of tile in VRAM (related to TILE_SPACE).
 */
#define TILE_MAXNUM             (TILE_SPACE / TILE_SIZE)
/**
 *  \def TILE_MAXINDEX
 *      Maximum tile index in VRAM (related to TILE_MAXNUM).
 */
#define TILE_MAXINDEX           (TILE_MAXNUM - 1)
/**
 *  \def TILE_SYSTEMINDEX
 *      System base tile index in VRAM.
 */
#define TILE_SYSTEMINDEX        0x0000
/**
 *  \def TILE_SYSTEMLENGTH
 *      Number of system tile.
 */
#define TILE_SYSTEMLENGTH       0x10
#define TILE_SYSTEMLENGHT       TILE_SYSTEMLENGTH
/**
 *  \def TILE_USERINDEX
 *      User base tile index.
 */
#define TILE_USERINDEX          (TILE_SYSTEMINDEX + TILE_SYSTEMLENGTH)
/**
 *  \def TILE_FONTINDEX
 *      Font base tile index.
 */
#define TILE_FONTINDEX          (TILE_MAXNUM - FONT_LEN)
/**
 *  \def TILE_USERLENGHT
 *      Number of available user tile.
 */
#define TILE_USERLENGHT         (TILE_FONTINDEX - TILE_USERINDEX)

/**
 *  \def TILE_SYSTEM
 *      System tile address in VRAM.
 */
#define TILE_SYSTEM             (TILE_SYSTEMINDEX * TILE_SIZE)
/**
 *  \def TILE_USER
 *      User tile address in VRAM.
 */
#define TILE_USER               (TILE_USERINDEX * TILE_SIZE)
/**
 *  \def TILE_FONT
 *      Font tile address in VRAM.
 */
#define TILE_FONT               (TILE_FONTINDEX * TILE_SIZE)

/**
 *  \def PAL0
 *      Palette 0
 */
#define PAL0                    0
/**
 *  \def PAL1
 *      Palette 1
 */
#define PAL1                    1
/**
 *  \def PAL2
 *      Palette 2
 */
#define PAL2                    2
/**
 *  \def PAL3
 *      Palette 3
 */
#define PAL3                    3

/**
 *  \def GFX_READ_VRAM_ADDR
 *      Set VDP command to read specified VRAM address.
 */
#define GFX_READ_VRAM_ADDR(adr)     ((0x0000 + ((adr) & 0x3FFF)) << 16) + (((adr) >> 14) | 0x00)
/**
 *  \def GFX_READ_CRAM_ADDR
 *      Set VDP command to read specified CRAM address.
 */
#define GFX_READ_CRAM_ADDR(adr)     ((0x0000 + ((adr) & 0x3FFF)) << 16) + (((adr) >> 14) | 0x20)
/**
 *  \def GFX_READ_VSRAM_ADDR
 *      Set VDP command to read specified VSRAM address.
 */
#define GFX_READ_VSRAM_ADDR(adr)    ((0x0000 + ((adr) & 0x3FFF)) << 16) + (((adr) >> 14) | 0x10)

/**
 *  \def GFX_WRITE_VRAM_ADDR
 *      Set VDP command to write at specified VRAM address.
 */
#if (VRAM_TABLE != 0)
#define GFX_WRITE_VRAM_ADDR(adr)    vramwrite_tab[adr]
#else
#define GFX_WRITE_VRAM_ADDR(adr)    ((0x4000 + ((adr) & 0x3FFF)) << 16) + (((adr) >> 14) | 0x00)
#endif

/**
 *  \def GFX_WRITE_CRAM_ADDR
 *      Set VDP command to write at specified CRAM address.
 */
#define GFX_WRITE_CRAM_ADDR(adr)    ((0xC000 + ((adr) & 0x3FFF)) << 16) + (((adr) >> 14) | 0x00)
/**
 *  \def GFX_WRITE_VSRAM_ADDR
 *      Set VDP command to write at specified VSRAM address.
 */
#define GFX_WRITE_VSRAM_ADDR(adr)   ((0x4000 + ((adr) & 0x3FFF)) << 16) + (((adr) >> 14) | 0x10)

/**
 *  \def GFX_DMA_VRAM_ADDR
 *      Set VDP command to issue a DMA transfert to specified VRAM address.
 */
#define GFX_DMA_VRAM_ADDR(adr)      ((0x4000 + ((adr) & 0x3FFF)) << 16) + (((adr) >> 14) | 0x80)
/**
 *  \def GFX_DMA_CRAM_ADDR
 *      Set VDP command to issue a DMA transfert to specified CRAM address.
 */
#define GFX_DMA_CRAM_ADDR(adr)      ((0xC000 + ((adr) & 0x3FFF)) << 16) + (((adr) >> 14) | 0x80)
/**
 *  \def GFX_DMA_VSRAM_ADDR
 *      Set VDP command to issue a DMA transfert to specified VSRAM address.
 */
#define GFX_DMA_VSRAM_ADDR(adr)     ((0x4000 + ((adr) & 0x3FFF)) << 16) + (((adr) >> 14) | 0x90)

/**
 *  \def GFX_VERT_SCROLL
 *      Helper to write in vertical scroll table (same as GFX_WRITE_VSRAM_ADDR).
 */
#define GFX_VERT_SCROLL(adr)        GFX_WRITE_VSRAM_ADDR(adr)
/**
 *  \def GFX_HORZ_SCROLL
 *      Helper to write in horizontal scroll table (same as GFX_WRITE_VRAM_ADDR(VDP_SCROLL_H + adr)).
 */
#define GFX_HORZ_SCROLL(adr)        GFX_WRITE_VRAM_ADDR(VDP_SCROLL_H + (adr))

/**
 *  \def GET_VDPSTATUS
 *      Tests VDP status against specified flag (see VDP_XXX_FLAG).
 */
#define GET_VDPSTATUS(flag)         ((*(vu16*)(GFX_CTRL_PORT)) & (flag))
/**
 *  \def IS_PALSYSTEM
 *      Tests if current system is a PAL system (50 Hz).
 */
#define IS_PALSYSTEM                GET_VDPSTATUS(VDP_PALMODE_FLAG)

/**
 *  \def GET_HVCOUNTER
 *      Returns HV counter.
 */
#define GET_HVCOUNTER               (*(vu16*)(GFX_HVCOUNTER_PORT))
/**
 *  \def GET_HCOUNTER
 *      Returns Horizontal counter.
 */
#define GET_HCOUNTER                (GET_HVCOUNTER & 0xFF)
/**
 *  \def GET_VCOUNTER
 *      Returns Vertical counter.
 */
#define GET_VCOUNTER                (GET_HVCOUNTER >> 8)


/**
 *  \brief
 *      Initialize the VDP sub system.
 *
 * Reset VDP registers, clear VRAM, set defaults grey, red, green & blue palette.
 */
void VDP_init();

/**
 *  \brief
 *      Get VDP register value.
 *
 *  \param reg
 *      Register number we want to retrieve value.
 *  \return specified register value.
 */
u8   VDP_getReg(u16 reg);
/**
 *  \brief
 *      Set VDP register value.
 *
 *  \param reg
 *      Register number we want to set value.
 *  \param value
 *      value to set.
 */
void VDP_setReg(u16 reg, u8 value);

/**
 *  \brief
 *      Returns VDP enable state.
 */
u8   VDP_getEnable();
/**
 *  \brief
 *      Set VDP enable state.
 *
 *  You can temporary disable VDP to speed up VDP memory transfert.
 */
void VDP_setEnable(u8 value);

/**
 *  \brief
 *      Returns number of total scanline.
 *
 *  312 for PAL system and 262 for NTSC system.
 */
u16  VDP_getScanlineNumber();
/**
 *  \brief
 *      Returns vertical screen resolution.
 *
 *  Always returns 224 on NTSC system as they only support this mode.<br>
 *  PAL system supports 240 pixels mode.
 */
u16  VDP_getScreenHeight();
/**
 *  \brief
 *      Set vertical resolution to 224 pixels.
 *
 *  This is the only accepted mode for NTSC system.
 */
void VDP_setScreenHeight224();
/**
 *  \brief
 *      Set vertical resolution to 240 pixels.
 *
 *  Only work on PAL system.
 */
void VDP_setScreenHeight240();
/**
 *  \brief
 *      Returns horizontal screen resolution.
 *
 *  Returns 320 or 256 depending current horizontal resolution mode.
 */
u16  VDP_getScreenWidth();
/**
 *  \brief
 *      Set horizontal resolution to 256 pixels.
 */
void VDP_setScreenWidth256();
/**
 *  \brief
 *      Set horizontal resolution to 320 pixels.
 */
void VDP_setScreenWidth320();

/**
 *  \brief
 *      Returns background plan width (in tile).
 *
 *  Possible values are: 32, 64, 128
 */
u16  VDP_getPlanWidth();
/**
 *  \brief
 *      Returns background plan height (in tile).
 *
 *  Possible values are: 32, 64, 128
 */
u16  VDP_getPlanHeight();
/**
 *  \brief
 *      Set background plan size (in tile).
 *
 *  \param w
 *      width in tile.<br/>
 *      Possible values are 32, 64 or 128.
 *  \param h
 *      height in tile.<br/>
 *      Possible values are 32, 64 or 128.
 */

void VDP_setPlanSize(u16 w, u16 h);

/**
 *  \brief
 *      Returns auto increment register value.
 */
u8   VDP_getAutoInc();
/**
 *  \brief
 *      Set auto increment register value.
 */
void VDP_setAutoInc(u8 value);

/**
 *  \brief
 *      Enable or Disable Horizontal interrupt.
 *
 *  When Horizontal interrupt is enabled, setting 5 here means that H int will occurs each 5+1 scanline.<br/>
 *  Set value 0 to get H int at each scanline.
 */
void VDP_setHInterrupt(u8 value);
/**
 *  \brief
 *      Enable or Disable Hilight / Shadow effect.
 */
void VDP_setHilightShadow(u8 value);

/**
 *  \brief
 *      Get Horizontal interrupt counter value.
 */
u8   VDP_getHIntCounter();
/**
 *  \brief
 *      Set Horizontal interrupt counter value.
 *
 *  When Horizontal interrupt is enabled, setting 5 here means that H int will occurs each 5+1 scanline.<br/>
 *  Set value 0 to get H int at each scanline.
 */
void VDP_setHIntCounter(u8 value);

/**
 *  \brief
 *      Wait for DMA operation to complete.
 */
void VDP_waitDMACompletion();
/**
 *  \brief
 *      Wait for VDP FIFO to be empty.
 */
void VDP_waitFIFOEmpty();

/**
 *  \brief
 *      Wait for Vertical Synchro.
 *
 *  The method actually wait for the next start of Vertical blanking.
 */
void VDP_waitVSync();

/**
 *  \brief
 *      Reset background plan and palette.
 *
 *  Clear background plans and reset palette to grey / red / green / blue.
 */
void VDP_resetScreen();


/**
 * \brief
 *      Display number of Frame Per Second.
 *
 * \param float_display
 *      Display as float number.
 *
 * This function actually display the number of time it was called in the last second.<br>
 * i.e: for benchmarking you should call this method only once per frame update.
 */
void VDP_showFPS(u16 float_display);


#endif // _VDP_H_
