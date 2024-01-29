#ifndef _HUD_H_
#define _HUD_H_


u16 HUD_init(u16 vramIndex);

void HUD_setVisibility(bool visible);
void HUD_handleInput(u16 value);


#endif // _HUD_H_
