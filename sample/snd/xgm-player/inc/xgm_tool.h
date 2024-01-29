#ifndef XGM_TOOL_H
#define XGM_TOOL_H

#include "ym_state.h"
#include "psg_state.h"
#include "xd3.h"


u32 XGM_getMusicDataOffset(const u8* xgm);
//u32 XGM_getDuration(const u8* xgm, u16 loopCnt);
void XGM_getXD3(const u8* xgm, XD3* xd3);

s32 XGM_parseFrame(u8* frameData, YM* ym, PSG *psg);


#endif // XGM_TOOL_H
