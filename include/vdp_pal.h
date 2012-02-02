/**
 * \file vdp_pal.h
 * \brief VDP Palette support
 * \author Stephane Dallongeville
 * \date 08/2011
 *
 * This unit provides methods to manipulate the VDP Color Palettes.
 * The Sega Genesis VDP has 4 palettes of 16 colors.
 * Color is defined with 3 bits for each component : 0RRR00GGG00BBB00
 */

#ifndef _VDP_PAL_H_
#define _VDP_PAL_H_


#define VDPPALETTE_REDSFT           9
#define VDPPALETTE_GREENSFT         5
#define VDPPALETTE_BLUESFT          1

#define VDPPALETTE_REDMASK          0x0E00
#define VDPPALETTE_GREENMASK        0x00E0
#define VDPPALETTE_BLUEMASK         0x000E
#define VDPPALETTE_COLORMASK        0x0EEE

#define RGB24_TO_VDPCOLOR(color)    (((color >> ((2 * 4) + 4)) & VDPPALETTE_REDMASK) | ((color >> ((1 * 4) + 4)) & VDPPALETTE_GREENMASK) | ((color >> ((0 * 4) + 4)) & VDPPALETTE_BLUEMASK))


extern const u16 palette_black[16];
extern const u16 palette_grey[16];
extern const u16 palette_red[16];
extern const u16 palette_green[16];
extern const u16 palette_blue[16];


u16  VDP_getPaletteColor(u16 numpal, u16 numcol);
void VDP_getPalette(u16 num, u16 *pal);

void VDP_setPaletteColor(u16 numpal, u16 numcol, u16 value);
void VDP_setPalette(u16 num, const u16 *pal);


// these functions should be private as they are called by VDP_fadeXXX functions internally
// but they can be useful sometime for better control on the fading processus
u16  VDP_doStepFading();
u16  VDP_initFading(u16 fromcol, u16 tocol, const u16 *palsrc, const u16 *paldst, u16 numframe);


void VDP_fade(u16 fromcol, u16 tocol, const u16 *palsrc, const u16 *paldst, u16 numframe, u8 async);
void VDP_fadeTo(u16 fromcol, u16 tocol, const u16 *pal, u16 numframe, u8 async);
void VDP_fadeOut(u16 fromcol, u16 tocol, u16 numframe, u8 async);
void VDP_fadeIn(u16 fromcol, u16 tocol, const u16 *pal, u16 numframe, u8 async);

void VDP_fadePal(u16 numpal, const u16 *palsrc, const u16 *paldst, u16 numframe, u8 async);
void VDP_fadePalTo(u16 numpal, const u16 *pal, u16 numframe, u8 async);
void VDP_fadePalOut(u16 numpal, u16 numframe, u8 async);
void VDP_fadePalIn(u16 numpal, const u16 *pal, u16 numframe, u8 async);

void VDP_fadeAll(const u16 *palsrc, const u16 *paldst, u16 numframe, u8 async);
void VDP_fadeAllTo(const u16 *pal, u16 numframe, u8 async);
void VDP_fadeOutAll(u16 numframe, u8 async);
void VDP_fadeInAll(const u16 *pal, u16 numframe, u8 async);

void VDP_waitFadeCompletion();


#endif // _VDP_PAL_H_
