#ifndef _VDP_BG_H_
#define _VDP_BG_H_


void VDP_setHorizontalScroll(u16 plan, u16 line, u16 value);
void VDP_setVerticalScroll(u16 plan, u16 cell, u16 value);

void VDP_clearPlan(u16 plan, u8 use_dma);

void VDP_drawText(const char *str, u16 x, u16 y);
void VDP_clearText(u16 x, u16 y, u16 w);
void VDP_clearTextLine(u16 y);

void VDP_drawTextBG(u16 plan, const char *str, u16 basetile, u16 x, u16 y);
void VDP_clearTextBG(u16 plan, u16 x, u16 y, u16 w);
void VDP_clearTextLineBG(u16 plan, u16 y);


#endif // _VDP_BG_H_
