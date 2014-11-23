#ifndef XGC_H_
#define XGC_H_

#include "xgm.h"

XGM* XGC_create(XGM* xgm);

void XGC_shiftSamples(XGM* source, int sft);

#include "util.h"
#include "ym2612.h"

List* XGC_getStateChange(XGM* source, YM2612* current, YM2612* old);
void XGC_computeAllOffset(XGM* source);
void XGC_computeAllFrameSize(XGM* source);
int XGC_computeLenInFrame(XGM* source);
int XGC_getTime(XGM* source, XGMCommand* command);
int XGC_getCommandIndexAtTime(XGM* source, int time);
unsigned char* XGC_asByteArray(XGM* source, int *outSize);


#endif // XGC_H_
