/**
 *  \file vdp.h
 *  \brief VDP main
 *  \author Stephane Dallongeville
 *  \date 08/2011
 *
 * This unit provides general VDP (Video Display Processor) methods:<br>
 * - initialisation<br>
 * - get / set register<br>
 * - get / set resolution<br>
 * - enable / disable VDP features<br>
 * <br>
 * <b>WARNING:</b> It's very important that VRAM is organized with tile data being located before tilemaps and tables:<br>
 * 0000-XXXX = tile data<br>
 * XXXX-FFFF = tilemaps & tables (H scroll table, sprite table, B/A plane and window plane).<br>
 * <br>
 * If you don't respect that you may get in troubles as SGDK expect it ;)
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
 *  \deprecated
 *      Use VDP_BG_A instead
 */
#define VDP_PLAN_A              VDP_BG_A
/**
 *  \deprecated
 *      Use VDP_BG_B instead
 */
#define VDP_PLAN_B              VDP_BG_B
/**
 *  \deprecated
 *      Use VDP_WINDOW instead
 */
#define VDP_PLAN_WINDOW         VDP_WINDOW

/**
 *  \brief
 *      VDP background A tilemap address in VRAM.
 */
#define VDP_BG_A                bga_addr
/**
 *  \brief
 *      VDP background B tilemap address in VRAM.
 */
#define VDP_BG_B                bgb_addr
/**
 *  \brief
 *      VDP window tilemap address in VRAM.
 */
#define VDP_WINDOW              window_addr
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
 *      Address in VRAM where tilemaps start (= address of first tilemap / table in VRAM).
 */
#define VDP_MAPS_START          maps_addr

/**
 *  \brief
 *      Definition to set horizontal scroll to mode plane.
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
 *      Definition to set vertical scroll to mode plane.
 */
#define VSCROLL_PLANE           0
/**
 *  \brief
 *      Definition to set vertical scroll to mode column (2 tiles width).
 */
#define VSCROLL_COLUMN          1
/**
 *  \deprecated
 *      Use VSCROLL_COLUMN instead
 */
#define VSCROLL_2TILE           VSCROLL_COLUMN

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
 *      SGDK font length
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
 *      Space in byte for tile in VRAM (tile space ends where tilemaps starts)
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
 *      Sprite engine base tile index (equal TILE_FONTINDEX if Sprite Engine is not initialized).
 */
#define TILE_SPRITEINDEX        (TILE_FONTINDEX - spriteVramSize)
/**
 *  \brief
 *      Number of available user tile.
 */
#define TILE_USERLENGTH         ((userTileMaxIndex - TILE_USERINDEX) + 1)
/**
 *  \deprecated Use TILE_USERLENGTH instead.
 */
#define TILE_USERLENGHT         TILE_USERLENGTH
/**
 *  \brief
 *      Maximum tile index in VRAM reserved for user (for background and user managed sprites)
 */
#define TILE_USERMAXINDEX       userTileMaxIndex
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
#define GFX_READ_VRAM_ADDR(adr)     (((0x0000 + ((adr) & 0x3FFF)) << 16) + (((adr) >> 14) | 0x00))
/**
 *  \brief
 *      Set VDP command to read specified CRAM address.
 */
#define GFX_READ_CRAM_ADDR(adr)     (((0x0000 + ((adr) & 0x7F)) << 16) + 0x20)
/**
 *  \brief
 *      Set VDP command to read specified VSRAM address.
 */
#define GFX_READ_VSRAM_ADDR(adr)    (((0x0000 + ((adr) & 0x3F)) << 16) + 0x10)

/**
 *  \brief
 *      Set VDP command to write at specified VRAM address.
 */
#define GFX_WRITE_VRAM_ADDR(adr)    (((0x4000 + ((adr) & 0x3FFF)) << 16) + (((adr) >> 14) | 0x00))
/**
 *  \brief
 *      Set VDP command to write at specified CRAM address.
 */
#define GFX_WRITE_CRAM_ADDR(adr)    (((0xC000 + ((adr) & 0x7F)) << 16) + 0x00)
/**
 *  \brief
 *      Set VDP command to write at specified VSRAM address.
 */
#define GFX_WRITE_VSRAM_ADDR(adr)   (((0x4000 + ((adr) & 0x3F)) << 16) + 0x10)

/**
 *  \brief
 *      Set VDP command to issue a DMA transfert to specified VRAM address.
 */
#define GFX_DMA_VRAM_ADDR(adr)      (((0x4000 + ((adr) & 0x3FFF)) << 16) + (((adr) >> 14) | 0x80))
/**
 *  \brief
 *      Set VDP command to issue a DMA transfert to specified CRAM address.
 */
#define GFX_DMA_CRAM_ADDR(adr)      (((0xC000 + ((adr) & 0x7F)) << 16) + 0x80)
/**
 *  \brief
 *      Set VDP command to issue a DMA transfert to specified VSRAM address.
 */
#define GFX_DMA_VSRAM_ADDR(adr)     (((0x4000 + ((adr) & 0x3F)) << 16) + 0x90)

/**
 *  \brief
 *      Set VDP command to issue a DMA VRAM copy to specified VRAM address.
 */
#define GFX_DMA_VRAMCOPY_ADDR(adr)  (((0x4000 + ((adr) & 0x3FFF)) << 16) + (((adr) >> 14) | 0xC0))

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
 *  \brief
 *      Type used to define on which plane to work (used by some methods).
 */
typedef enum
{
    BG_A = 0, BG_B = 1, WINDOW = 2
} VDPPlane;


// used by define
extern u16 window_addr;
extern u16 bga_addr;
extern u16 bgb_addr;
extern u16 hscrl_addr;
extern u16 slist_addr;
extern u16 maps_addr;
extern u16 userTileMaxIndex;

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
 *      Current background plane width (in tile)
 *
 *  Possible values are: 32, 64, 128
 */
extern u16 planeWidth;
/**
 *  \brief
 *      Current background plane height (in tile)
 *
 *  Possible values are: 32, 64, 128
 */
extern u16 planeHeight;
/**
 *  \brief
 *      Current window width (in tile)
 *
 *  Possible values are: 32, 64
 */
extern u16 windowWidth;
/**
 *  \brief
 *      Current background plane width bit shift
 *
 *  Possible values are: 5, 6 or 7 (corresponding to plane width 32, 64 and 128)
 */
extern u16 planeWidthSft;
/**
 *  \brief
 *      Current background plane height bit shift
 *
 *  Possible values are: 5, 6 or 7 (corresponding to plane height 32, 64 and 128)
 */
extern u16 planeHeightSft;
/**
 *  \brief
 *      Current window width bit shift
 *
 *  Possible values are: 5 or 6 (corresponding to window width 32 or 64)
 */
extern u16 windowWidthSft;


/**
 *  \brief
 *      Initialize the whole VDP sub system.
 *
 * Reset VDP registers, reset sprites then call #VDP_resetScreen() to reset BG and palettes.
 */
void VDP_init();

/**
 *  \brief
 *      Reset background planes and palettes.
 *
 *  Reset VRAM (clear BG planes and reload font), reset scrolls and reset palettes (set to default grey / red / green / blue ramps).
 */
void VDP_resetScreen();

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
 *      Return background plane width (in tile).
 */
u16  VDP_getPlaneWidth();
/**
 *  \brief
 *      Return background plane height (in tile).
 */
u16  VDP_getPlaneHeight();
/**
 *  \brief
 *      Set background plane size (in tile).<br>
 *      WARNING: take attention to properly setup VRAM so tilemaps has enough space.
 *
 *  \param w
 *      width in tile.<br>
 *      Possible values are 32, 64 or 128.
 *  \param h
 *      height in tile.<br>
 *      Possible values are 32, 64 or 128.
 *  \param setupVram
 *      If set to TRUE then tilemaps and tables will be automatically remapped in VRAM depending
 *      the plane size. If you don't know what that means then it's better to keep this value to TRUE :p<br>
 *      Be careful to redraw your backgrounds, also the sprite engine may need to re-allocate its VRAM region if location changed.
 */
void VDP_setPlaneSize(u16 w, u16 h, bool setupVram);
/**
 *  \deprecated
 *      Use #VDP_setPlaneSize(..) instead.
 */
void VDP_setPlanSize(u16 w, u16 h);

/**
 *  \brief
 *      Returns plane horizontal scrolling mode.
 *
 *  Possible values are: HSCROLL_PLANE, HSCROLL_TILE, HSCROLL_LINE
 *
 *  \see VDP_setScrollingMode for more informations about scrolling mode.
 */
u8 VDP_getHorizontalScrollingMode();
/**
 *  \brief
 *      Returns plane vertical scrolling mode.
 *
 *  Possible values are: VSCROLL_PLANE, VSCROLL_2TILE
 *
 *  \see VDP_setScrollingMode for more informations about scrolling mode.
 */
u8 VDP_getVerticalScrollingMode();
/**
 *  \brief
 *      Set plane scrolling mode.
 *
 *  \param hscroll
 *      Horizontal scrolling mode :<br>
 *      <b>HSCROLL_PLANE</b> = Scroll offset is applied to the whole plane.<br>
 *      <b>HSCROLL_TILE</b> = Scroll offset is applied on a tile basis granularity (8 pixels bloc).<br>
 *      <b>HSCROLL_LINE</b> = Scroll offset is applied on a line basis granularity (1 pixel).<br>
 *  \param vscroll
 *      Vertical scrolling mode :<br>
 *      <b>VSCROLL_PLANE</b> = Scroll offset is applied to the whole plane.<br>
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
 *      Returns DMA enabled state
 */
u8 VDP_getDMAEnabled();
/**
 *  \brief
 *      Set DMA enabled state.
 *
 *  Note that by default SGDK always enable DMA (there is no reason to disable it)
 */
void VDP_setDMAEnabled(u8 value);
/**
 *  \brief
 *      Returns HV counter latching on INT2 (used for light gun)
 */
u8 VDP_getHVLatching();
/**
 *  \brief
 *      Set HV counter latching on INT2 (used for light gun)
 *
 *  You can ask the HV Counter to fix its value on INT2 for accurate light gun positionning.
 */
void VDP_setHVLatching(u8 value);
/**
 *  \brief
 *      Enable or Disable Horizontal interrupt.
 *
 *  \see VDP_setHIntCounter()
 */
void VDP_setHInterrupt(u8 value);
/**
 *  \brief
 *      Enable or Disable External interrupt.
 *
 *  \see VDP_setExtIntCounter()
 */
void VDP_setExtInterrupt(u8 value);
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
 *      Get VRAM address (location) of BG A tilemap.
 */
u16 VDP_getBGAAddress();
/**
 *  \brief
 *      Get VRAM address (location) of BG B tilemap.
 */
u16 VDP_getBGBAddress();
/**
 *  \deprecated
 *      Use #VDP_getBGAAddress(..) instead.
 */
u16 VDP_getAPlanAddress();
/**
 *  \deprecated
 *      Use #VDP_getBGBAddress(..) instead.
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
 *      Set VRAM address (location) of BG A tilemap.
 *      The address should be at multiple of $2000<br>
 *      <br>
 *      Ex:<br>
 *      VDP_setBGAAddress(0xC000)<br>
 *      Will set the BG A to at address 0xC000 in VRAM.
 */
void VDP_setBGAAddress(u16 value);
/**
 *  \brief
 *      Set VRAM address (location) of BG B tilemap.<br>
 *      The address should be at multiple of $2000<br>
 *      <br>
 *      Ex:<br>
 *      VDP_setBGBAddress(0xE000)<br>
 *      Will set the BG B tilemap at address 0xE000 in VRAM.
 */
void VDP_setBGBAddress(u16 value);
/**
 *  \deprecated
 *      Use #VDP_setBGAAddress(..) instead.
 */
void VDP_setAPlanAddress(u16 value);
/**
 *  \deprecated
 *      Use #VDP_setBGBAddress(..) instead.
 */
void VDP_setBPlanAddress(u16 value);
/**
 *  \brief
 *      Set VRAM address (location) of Window tilemap.<br>
 *      The address should be at multiple of $1000 in H40 and $800 in H32<br>
 *      <br>
 *      Ex:<br>
 *      VDP_setWindowAddress(0xA000)<br>
 *      Will set the Window tilemap at address 0xA000 in VRAM.
 */
void VDP_setWindowAddress(u16 value);
/**
 *  \deprecated
 *      Use #VDP_setWindowAddress(..) instead.
 */
void VDP_setWindowPlanAddress(u16 value);
/**
 *  \brief
 *      Set VRAM address (location) of Sprite list.<br>
 *      The address should be at multiple of $400 in H40 and $200 in H32<br>
 *      <br>
 *      Ex:<br>
 *      VDP_setSpriteListAddress(0xD800)<br>
 *      Will set the Sprite list to at address 0xD800 in VRAM.
 */
void VDP_setSpriteListAddress(u16 value);
/**
 *  \brief
 *      Set VRAM address (location) of H Scroll table.<br>
 *      The address should be at multiple of $400<br>
 *      <br>
 *      Ex:<br>
 *      VDP_setHScrollTableAddress(0xD400)<br>
 *      Will set the HScroll table to at address 0xD400 in VRAM.
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
 *      Wait for next Vertical Interruption.
 *  \return
 *      TRUE if a frame miss was detected (more than 1 frame elapsed since last call)
 *
 *  The method actually wait for the start of Vertical Interruption.
 *  It returns immediately if we are already in V-Int handler.
 */
bool VDP_waitVInt();
/**
 *  \brief
 *      Wait for next vertical blank period (same as #VDP_waitVSync())
 *  \return
 *      TRUE if a frame miss was detected (more than 1 frame elapsed since last call)
 *
 *  \param forceNext
 *      Force waiting for next start of VBlank if we are already in VBlank period when calling the method.
 *
 *  The method wait until we are in Vertical blanking area/period.
 */
bool VDP_waitVBlank(bool forceNext);
/**
 *  \brief
 *      Wait for Vertical Synchro.
 *  \return
 *      TRUE if a frame miss was detected (more than 1 frame elapsed since last call)
 *
 *  The method actually wait for the *next* start of Vertical blanking.
 */
bool VDP_waitVSync();
/**
 *  \brief
 *      Wait for next vertical active area (end of vertical blank period)
 *
 *  \param forceNext
 *      Force waiting for next start of V-Active if we are already in V-Active period when calling the method.
 *
 *  The method wait until we are in Vertical active area/period.
 */
void VDP_waitVActive(bool forceNext);

/**
 *  \brief
 *      Return an enhanced V Counter representation.
 *
 *  Using direct V counter from VDP may give troubles as the VDP V-Counter rollback during V-Blank period.<br>
 *  This function aim to make ease the use of V-Counter by adjusting it to a [0-255] range where 0 is the start of VBlank area and 255 the end of active display area.
 */
u16 VDP_getAdjustedVCounter();

/**
 *  \brief
 *      Display number of Frame Per Second.
 *
 *  \param asFloat
 *      Display in float number format.
 *
 * This function actually display the number of time it was called in the last second.<br>
 * i.e: for benchmarking you should call this method only once per frame update.
 *
 * \see #getFPS(..)
 */
void VDP_showFPS(u16 asFloat);
/**
 *  \brief
 *      Display the estimated CPU load (in %).
 *
 * This function actually display an estimation of the CPU load (in %) for the last frame.
 *
 * \see #SYS_getCPULoad()
 */
void VDP_showCPULoad();


#endif // _VDP_H_
