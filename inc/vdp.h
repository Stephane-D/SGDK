/**
 *  \file vdp.h
 *  \brief VDP main
 *  \author Stephane Dallongeville
 *  \date 08/2011
 *
 * This unit provides general VDP methods :<br>
 * - initialisation<br>
 * - get / set register<br>
 * - get / set resolution<br>
 * - enable / disable VDP features<br>
 * <br>
 * VRAM should always be organized in a way that tile data are always located before map in VRAM:<br>
 * 0000-XXXX = tile data<br>
 * XXXX-FFFF = maps & tables (H scroll table, sprite table, B/A plan and window plan).
 */

#ifndef _VDP_H_
#define _VDP_H_


/**
 *  \brief
 *      VDP Data port address.
 */
#define GFX_DATA_PORT           0xC00000
/**
 *  \brief
 *      VDP Control port address.
 */
#define GFX_CTRL_PORT           0xC00004
/**
 *  \brief
 *      VDP HV counter port address.
 */
#define GFX_HVCOUNTER_PORT      0xC00008

/**
 *  \brief
 *      VDP FIFO empty flag.
 */
#define VDP_FIFOEMPTY_FLAG      (1 << 9)
/**
 *  \brief
 *      VDP FIFO full flag.
 */
#define VDP_FIFOFULL_FLAG       (1 << 8)
/**
 *  \brief
 *      VDP Vertical interrupt pending flag.
 */
#define VDP_VINTPENDING_FLAG    (1 << 7)
/**
 *  \brief
 *      VDP sprite overflow flag.
 */
#define VDP_SPROVERFLOW_FLAG    (1 << 6)
/**
 *  \brief
 *      VDP sprite collision flag.
 */
#define VDP_SPRCOLLISION_FLAG   (1 << 5)
/**
 *  \brief
 *      VDP odd frame flag.
 */
#define VDP_ODDFRAME_FLAG       (1 << 4)
/**
 *  \brief
 *      VDP Vertical blanking flag.
 */
#define VDP_VBLANK_FLAG         (1 << 3)
/**
 *  \brief
 *      VDP Horizontal blanking flag.
 */
#define VDP_HBLANK_FLAG         (1 << 2)
/**
 *  \brief
 *      VDP DMA busy flag.
 */
#define VDP_DMABUSY_FLAG        (1 << 1)
/**
 *  \brief
 *      VDP PAL mode flag.
 */
#define VDP_PALMODE_FLAG        (1 << 0)

/**
 *  \brief
 *      VDP background A tilemap address in VRAM.
 */
#define VDP_PLAN_A              aplan_addr
/**
 *  \brief
 *      VDP background B tilemap address in VRAM.
 */
#define VDP_PLAN_B              bplan_addr
/**
 *  \brief
 *      VDP window tilemap address in VRAM.
 */
#define VDP_PLAN_WINDOW         window_addr
/**
 *  \brief
 *      VDP horizontal scroll table address in VRAM.
 */
#define VDP_HSCROLL_TABLE       hscrl_addr
/**
 *  \brief
 *      VDP sprite list table address in VRAM.
 */
#define VDP_SPRITE_TABLE        slist_addr
/**
 *  \brief
 *      Address in VRAM where maps start (= address of first map / table in VRAM).
 */
#define VDP_MAPS_START          maps_addr

/**
 *  \brief
 *      Definition to set horizontal scroll to mode plan.
 */
#define HSCROLL_PLANE           0
/**
 *  \brief
 *      Definition to set horizontal scroll to mode tile.
 */
#define HSCROLL_TILE            2
/**
 *  \brief
 *      Definition to set horizontal scroll to mode line.
 */
#define HSCROLL_LINE            3

/**
 *  \brief
 *      Definition to set vertical scroll to mode plan.
 */
#define VSCROLL_PLANE           0
/**
 *  \brief
 *      Definition to set vertical scroll to mode 2 tile.
 */
#define VSCROLL_2TILE           1

/**
 *  \brief
 *      Interlaced scanning mode disabled.<br>
 *      That is the default mode for the VDP.
 */
#define INTERLACED_NONE         0
/**
 *  \brief
 *      Interlaced Scanning Mode 1 - 8x8 dots per cell (normal vertical resolution)<br>
 *      In Interlaced Mode 1, the same pattern will be displayed on the adjacent lines of even and odd numbered fields.
 */
#define INTERLACED_MODE1        1
/**
 *  \brief
 *      Interlaced Scanning Mode 2 - 8x16 dots per cell (double vertical resolution)<br>
 *      In Interlaced Mode 2, different patterns can be displayed on the adjacent lines of even and odd numbered fields.
 */
#define INTERLACED_MODE2        2

/**
 *  \brief
 *      SGDL font length
 */
#define FONT_LEN    96

/**
 *  \brief
 *      Size of a single tile in byte.
 */
#define TILE_SIZE               32
#define TILE_INDEX_MASK         (0xFFFF / TILE_SIZE)

/**
 *  \brief
 *      Space in byte for tile in VRAM (tile space ends where maps starts)
 */
#define TILE_SPACE              VDP_MAPS_START

/**
 *  \brief
 *      Maximum number of tile in VRAM (related to TILE_SPACE).
 */
#define TILE_MAXNUM             (TILE_SPACE / TILE_SIZE)
/**
 *  \brief
 *      Maximum tile index in VRAM (related to TILE_MAXNUM).
 */
#define TILE_MAXINDEX           (TILE_MAXNUM - 1)
/**
 *  \brief
 *      System base tile index in VRAM.
 */
#define TILE_SYSTEMINDEX        0x0000
/**
 *  \brief
 *      Number of system tile.
 */
#define TILE_SYSTEMLENGTH       16
/**
 *  \deprecated Use TILE_SYSTEMLENGTH instead.
 */
#define TILE_SYSTEMLENGHT       TILE_SYSTEMLENGTH
/**
 *  \brief
 *      User base tile index.
 */
#define TILE_USERINDEX          (TILE_SYSTEMINDEX + TILE_SYSTEMLENGTH)
/**
 *  \brief
 *      Font base tile index.
 */
#define TILE_FONTINDEX          (TILE_MAXNUM - FONT_LEN)
/**
 *  \brief
 *      Number of available user tile.
 */
#define TILE_USERLENGTH         (TILE_FONTINDEX - TILE_USERINDEX)
/**
 *  \deprecated Use TILE_USERLENGTH instead.
 */
#define TILE_USERLENGHT         TILE_USERLENGTH
/**
 *  \brief
 *      Maximum tile index in VRAM for user.
 */
#define TILE_USERMAXINDEX       (TILE_USERINDEX + TILE_USERLENGTH - 1)
/**
 *  \brief
 *      System tile address in VRAM.
 */
#define TILE_SYSTEM             (TILE_SYSTEMINDEX * TILE_SIZE)
/**
 *  \brief
 *      User tile address in VRAM.
 */
#define TILE_USER               (TILE_USERINDEX * TILE_SIZE)
/**
 *  \brief
 *      Font tile address in VRAM.
 */
#define TILE_FONT               (TILE_FONTINDEX * TILE_SIZE)

/**
 *  \brief
 *      Palette 0
 */
#define PAL0                    0
/**
 *  \brief
 *      Palette 1
 */
#define PAL1                    1
/**
 *  \brief
 *      Palette 2
 */
#define PAL2                    2
/**
 *  \brief
 *      Palette 3
 */
#define PAL3                    3

/**
 *  \brief
 *      Set VDP command to read specified VRAM address.
 */
#define GFX_READ_VRAM_ADDR(adr)     ((0x0000 + ((adr) & 0x3FFF)) << 16) + (((adr) >> 14) | 0x00)
/**
 *  \brief
 *      Set VDP command to read specified CRAM address.
 */
#define GFX_READ_CRAM_ADDR(adr)     ((0x0000 + ((adr) & 0x3FFF)) << 16) + (((adr) >> 14) | 0x20)
/**
 *  \brief
 *      Set VDP command to read specified VSRAM address.
 */
#define GFX_READ_VSRAM_ADDR(adr)    ((0x0000 + ((adr) & 0x3FFF)) << 16) + (((adr) >> 14) | 0x10)

/**
 *  \brief
 *      Set VDP command to write at specified VRAM address.
 */
#define GFX_WRITE_VRAM_ADDR(adr)    ((0x4000 + ((adr) & 0x3FFF)) << 16) + (((adr) >> 14) | 0x00)
/**
 *  \brief
 *      Set VDP command to write at specified CRAM address.
 */
#define GFX_WRITE_CRAM_ADDR(adr)    ((0xC000 + ((adr) & 0x3FFF)) << 16) + (((adr) >> 14) | 0x00)
/**
 *  \brief
 *      Set VDP command to write at specified VSRAM address.
 */
#define GFX_WRITE_VSRAM_ADDR(adr)   ((0x4000 + ((adr) & 0x3FFF)) << 16) + (((adr) >> 14) | 0x10)

/**
 *  \brief
 *      Set VDP command to issue a DMA transfert to specified VRAM address.
 */
#define GFX_DMA_VRAM_ADDR(adr)      ((0x4000 + ((adr) & 0x3FFF)) << 16) + (((adr) >> 14) | 0x80)
/**
 *  \brief
 *      Set VDP command to issue a DMA transfert to specified CRAM address.
 */
#define GFX_DMA_CRAM_ADDR(adr)      ((0xC000 + ((adr) & 0x3FFF)) << 16) + (((adr) >> 14) | 0x80)
/**
 *  \brief
 *      Set VDP command to issue a DMA transfert to specified VSRAM address.
 */
#define GFX_DMA_VSRAM_ADDR(adr)     ((0x4000 + ((adr) & 0x3FFF)) << 16) + (((adr) >> 14) | 0x90)

/**
 *  \brief
 *      Set VDP command to issue a DMA VRAM copy to specified VRAM address.
 */
#define GFX_DMA_VRAMCOPY_ADDR(adr)  ((0x4000 + ((adr) & 0x3FFF)) << 16) + (((adr) >> 14) | 0xC0)

/**
 *  \brief
 *      Helper to write in vertical scroll table (same as GFX_WRITE_VSRAM_ADDR).
 */
#define GFX_VERT_SCROLL(adr)        GFX_WRITE_VSRAM_ADDR(adr)
/**
 *  \brief
 *      Helper to write in horizontal scroll table (same as GFX_WRITE_VRAM_ADDR(VDP_SCROLL_H + adr)).
 */
#define GFX_HORZ_SCROLL(adr)        GFX_WRITE_VRAM_ADDR(VDP_SCROLL_H + (adr))

/**
 *  \brief
 *      Tests VDP status against specified flag (see VDP_XXX_FLAG).
 */
#define GET_VDPSTATUS(flag)         ((*(vu16*)(GFX_CTRL_PORT)) & (flag))
/**
 *  \brief
 *      Tests if current system is a PAL system (50 Hz).
 */
#define IS_PALSYSTEM                GET_VDPSTATUS(VDP_PALMODE_FLAG)

/**
 *  \brief
 *      Returns HV counter.
 */
#define GET_HVCOUNTER               (*(vu16*)(GFX_HVCOUNTER_PORT))
/**
 *  \brief
 *      Returns Horizontal counter.
 */
#define GET_HCOUNTER                (GET_HVCOUNTER & 0xFF)
/**
 *  \brief
 *      Returns Vertical counter.
 */
#define GET_VCOUNTER                (GET_HVCOUNTER >> 8)


/**
 * Internal use
 */
#define CONST_PLAN_A                0
#define CONST_PLAN_B                1
#define CONST_PLAN_WINDOW           2

/**
 *  \brief
 *      Type used to define on which plan to work (used by some methods).
 */
typedef struct
{
    u16 value;
} VDPPlan;


// used by define
extern u16 window_addr;
extern u16 aplan_addr;
extern u16 bplan_addr;
extern u16 hscrl_addr;
extern u16 slist_addr;
extern u16 maps_addr;

/**
 *  \brief
 *      Current screen width (horizontale resolution)
 */
extern u16 screenWidth;
/**
 *  \brief
 *      Current screen height (verticale resolution)
 */
extern u16 screenHeight;
/**
 *  \brief
 *      Current background plan width (in tile)
 *
 *  Possible values are: 32, 64, 128
 */
extern u16 planWidth;
/**
 *  \brief
 *      Current background plan height (in tile)
 *
 *  Possible values are: 32, 64, 128
 */
extern u16 planHeight;
/**
 *  \brief
 *      Current window width (in tile)
 *
 *  Possible values are: 32, 64
 */
extern u16 windowWidth;
/**
 *  \brief
 *      Current background plan width bit shift
 *
 *  Possible values are: 5, 6 or 7 (corresponding to plan width 32, 64 and 128)
 */
extern u16 planWidthSft;
/**
 *  \brief
 *      Current background plan height bit shift
 *
 *  Possible values are: 5, 6 or 7 (corresponding to plan height 32, 64 and 128)
 */
extern u16 planHeightSft;
/**
 *  \brief
 *      Current window width bit shift
 *
 *  Possible values are: 5 or 6 (corresponding to window width 32 or 64)
 */
extern u16 windowWidthSft;


/**
 *  \brief
 *      Constante to represent VDP background A plan (used by some methods)
 */
extern const VDPPlan PLAN_A;
/**
 *  \brief
 *      Constante to represent VDP background B plan (used by some methods)
 */
extern const VDPPlan PLAN_B;
/**
 *  \brief
 *      Constante to represent VDP window plan (used by some methods)
 */
extern const VDPPlan PLAN_WINDOW;


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
 *  \deprecated Use the <i>planWidth</i> variable directly.
 */
u16  VDP_getPlanWidth();
/**
 *  \deprecated Use the <i>planHeight</i> variable directly.
 */
u16  VDP_getPlanHeight();
/**
 *  \brief
 *      Set background plan size (in tile).
 *
 *  \param w
 *      width in tile.<br>
 *      Possible values are 32, 64 or 128.
 *  \param h
 *      height in tile.<br>
 *      Possible values are 32, 64 or 128.
 */
void VDP_setPlanSize(u16 w, u16 h);

/**
 *  \brief
 *      Returns plan horizontal scrolling mode.
 *
 *  Possible values are: HSCROLL_PLANE, HSCROLL_TILE, HSCROLL_LINE
 *
 *  \see VDP_setScrollingMode for more informations about scrolling mode.
 */
u8 VDP_getHorizontalScrollingMode();
/**
 *  \brief
 *      Returns plan vertical scrolling mode.
 *
 *  Possible values are: VSCROLL_PLANE, VSCROLL_2TILE
 *
 *  \see VDP_setScrollingMode for more informations about scrolling mode.
 */
u8 VDP_getVerticalScrollingMode();
/**
 *  \brief
 *      Set plan scrolling mode.
 *
 *  \param hscroll
 *      Horizontal scrolling mode :<br>
 *      <b>HSCROLL_PLANE</b> = Scroll offset is applied to the whole plan.<br>
 *      <b>HSCROLL_TILE</b> = Scroll offset is applied on a tile basis granularity (8 pixels bloc).<br>
 *      <b>HSCROLL_LINE</b> = Scroll offset is applied on a line basis granularity (1 pixel).<br>
 *  \param vscroll
 *      Vertical scrolling mode :<br>
 *      <b>VSCROLL_PLANE</b> = Scroll offset is applied to the whole plan.<br>
 *      <b>VSCROLL_2TILE</b> = Scroll offset is applied on 2 tiles basis granularity (16 pixels bloc).<br>
 *
 *  \see VDP_setHorizontalScroll() to set horizontal scroll offset in mode plane.<br>
 *  \see VDP_setHorizontalScrollTile() to set horizontal scroll offset(s) in mode tile.<br>
 *  \see VDP_setHorizontalScrollLine() to set horizontal scroll offset(s) in mode line.<br>
 *  \see VDP_setVerticalScroll() to set vertical scroll offset in mode plane.<br>
 *  \see VDP_setVerticalScrollTile() to set vertical scroll offset(s) in mode 2-tile.<br>
 */
void VDP_setScrollingMode(u16 hscroll, u16 vscroll);


/**
 *  \brief
 *      Returns the background color index.
 */
u8 VDP_getBackgroundColor();
/**
 *  \brief
 *      Set the background color index.
 */
void VDP_setBackgroundColor(u8 value);

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
 *  \see VDP_setHIntCounter()
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
 *  When Horizontal interrupt is enabled, setting 5 here means that H int will occurs each (5+1) scanline.<br>
 *  Set value 0 to get H int at each scanline.
 */
void VDP_setHIntCounter(u8 value);

/**
 *  \brief
 *      Get VRAM address (location) of Plan A tilemap.
 */
u16 VDP_getAPlanAddress();
/**
 *  \brief
 *      Get VRAM address (location) of Plan B tilemap.
 */
u16 VDP_getBPlanAddress();
/**
 *  \brief
 *      Get VRAM address (location) of Window tilemap.
 */
u16 VDP_getWindowAddress();
/**
 *  \deprecated
 *      Use #VDP_getWindowAddress(..) instead.
 */
u16 VDP_getWindowPlanAddress();
/**
 *  \brief
 *      Get VRAM address (location) of Sprite list.
 */
u16 VDP_getSpriteListAddress();
/**
 *  \brief
 *      Get VRAM address (location) of H SCroll table.
 */
u16 VDP_getHScrollTableAddress();

/**
 *  \brief
 *      Set VRAM address (location) of Plan A tilemap.
 *      <br>
 *      WARNING: the window tilemap should always be the first object attribute in VRAM:<br>
 *      | system tiles<br>
 *      | user tiles<br>
 *      | window plan<br>
 *      v others (plan a, plan b, ...)<br>
 *      <br>
 *      The window tilemap address is used internally to calculated how much space is available for tiles.
 *
 *  EX:<br>
 *      VDP_setAPlanAddress(0xC000)<br>
 *      Will set the Plan A to at address 0xC000 in VRAM.
 */
void VDP_setAPlanAddress(u16 value);
/**
 *  \brief
 *      Set VRAM address (location) of Window tilemap.<br>
 *      <br>
 *      WARNING: the window tilemap should always be the first object attribute in VRAM:<br>
 *      | system tiles<br>
 *      | user tiles<br>
 *      | window plan<br>
 *      v others (plan a, plan b, ...)<br>
 *      <br>
 *      The window tilemap address is used internally to calculated how much space is available for tiles.
 *
 *  EX:<br>
 *      VDP_setWindowAddress(0xA000)<br>
 *      Will set the Window tilemap to at address 0xA000 in VRAM.
 */
void VDP_setWindowAddress(u16 value);
/**
 *  \deprecated
 *      Use #VDP_setWindowAddress(..) instead.
 */
void VDP_setWindowPlanAddress(u16 value);
/**
 *  \brief
 *      Set VRAM address (location) of Plan B tilemap.<br>
 *      <br>
 *      WARNING: the window tilemap should always be the first object attribute in VRAM:<br>
 *      | system tiles<br>
 *      | user tiles<br>
 *      | window plan<br>
 *      v others (plan a, plan b, ...)<br>
 *      <br>
 *      The window tilemap address is used internally to calculated how much space is available for tiles.
 *
 *  EX:<br>
 *      VDP_setBPlanAddress(0xE000)<br>
 *      Will set the Plan B to at address 0xE000 in VRAM.
 */
void VDP_setBPlanAddress(u16 value);
/**
 *  \brief
 *      Set VRAM address (location) of Sprite list.<br>
 *      <br>
 *      WARNING: the window tilemap should always be the first object attribute in VRAM:<br>
 *      | system tiles<br>
 *      | user tiles<br>
 *      | window plan<br>
 *      v others (plan a, plan b, ...)<br>
 *      <br>
 *      The window tilemap address is used internally to calculated how much space is available for tiles.
 *
 *  EX:<br>
 *      VDP_setSpriteListAddress(0xB800)<br>
 *      Will set the Sprite list to at address 0xB800 in VRAM.
 */
void VDP_setSpriteListAddress(u16 value);
/**
 *  \brief
 *      Set VRAM address (location) of H Scroll table.<br>
 *      <br>
 *      WARNING: the window tilemap should always be the first object attribute in VRAM:<br>
 *      | system tiles<br>
 *      | user tiles<br>
 *      | window plan<br>
 *      v others (plan a, plan b, ...)<br>
 *      <br>
 *      The the window tilemap address is used internally to calculated how much space is available for tiles.
 *
 *  EX:<br>
 *      VDP_setHScrollTableAddress(0xB400)<br>
 *      Will set the HScroll table to at address 0xB400 in VRAM.
 */
void VDP_setHScrollTableAddress(u16 value);

/**
 *  \brief
 *      Sets the scan mode of the display.
 *
 *  \param mode
 *      Accepted values : #INTERLACED_NONE, #INTERLACED_MODE1, #INTERLACED_MODE2
 *
 * This function changes the scanning mode on the next display blanking period.<br>
 * In Interlaced Mode 1, the same pattern will be displayed on the adjacent lines of even and odd numbered fields.<br>
 * In Interlaced Mode 2, different patterns can be displayed on the adjacent lines of even and odd numbered fields.<br>
 * The number of cells on the screen stays the same regardless of which scanning mode is active.
 */
void VDP_setScanMode(u16 mode);

/**
 *  \brief
 *      Sets the window Horizontal position.
 *
 *  \param right
 *      If set to <i>FALSE</i> the window is displayed from column 0 up to column <i>pos</i>
 *      If set to <i>TRUE</i> the window is displayed from column <i>pos</i> up to last column
 *  \param pos
 *      The Horizontal position of the window in 2 tiles unit (16 pixels).
 */
void VDP_setWindowHPos(u16 right, u16 pos);
/**
 *  \brief
 *      Sets the window Vertical position.
 *
 *  \param down
 *      If set to <i>FALSE</i> the window is displayed from row 0 up to row <i>pos</i>
 *      If set to <i>TRUE</i> the window is displayed from row <i>pos</i> up to last row
 *  \param pos
 *      The Vertical position of the window in 1 tile unit (8 pixels).
 */
void VDP_setWindowVPos(u16 down, u16 pos);

/**
 *  \brief
 *      Wait for DMA operation to complete.
 *  \deprecated Use #DMA_waitCompletion() instead
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
 *  Clear background plans, reset palette to grey / red / green / blue and reset scrolls.
 */
void VDP_resetScreen();

/**
 *  \brief
 *      Display number of Frame Per Second.
 *
 *  \param float_display
 *      Display as float number.
 *
 * This function actually display the number of time it was called in the last second.<br>
 * i.e: for benchmarking you should call this method only once per frame update.
 */
void VDP_showFPS(u16 float_display);


#endif // _VDP_H_
