#ifndef XGM_H_
#define XGM_H_

#include "util.h"
#include "gd3.h"


typedef struct
{
    LList* samples;
    LList* commands;
    GD3* gd3;
    XD3* xd3;
    int pal;
} XGM;


#include "vgm.h"

XGM* XGM_create();
XGM* XGM_createFromData(unsigned char* data, int dataSize);
XGM* XGM_createFromXGCData(unsigned char* data, int dataSize);
XGM* XGM_createFromVGM(VGM* vgm);

#include "xgm.h"
#include "xgmcom.h"

XGMCommand* XGM_getLoopCommand(XGM* xgm);
int XGM_getLoopPointedCommandIndex(XGM* xgm);
LList* XGM_getLoopPointedCommandElement(XGM* xgm);
XGMCommand* XGM_getLoopPointedCommand(XGM* xgm);
void XGM_computeAllOffset(XGM* xgm);
int XGM_computeLenInFrame(XGM* xgm);
int XGM_computeLenInSecond(XGM* xgm);
int XGM_getOffset(XGM* xgm, XGMCommand* command);
int XGM_getTime(XGM* xgm, XGMCommand* command);
int XGM_getTimeInFrame(XGM* xgm, XGMCommand* command);
LList* XGM_getCommandElementAtOffset(XGM* xgm, int offset);
LList* XGM_getCommandElementAtTime(XGM* xgm, int time);
XGMCommand* XGM_getCommandAtOffset(XGM* xgm, int offset);
XGMCommand* XGM_getCommandAtTime(XGM* xgm, int time);

#include "xgmsmp.h"

XGMSample* XGM_getSampleByIndex(XGM* xgm, int index);
//XGMSample* XGM_getSampleByAddressAndLen(XGM* xgm, int originAddr, int originSize);
XGMSample* XGM_getSampleByAddress(XGM* xgm, int originAddr);
unsigned char* XGM_asByteArray(XGM* xgm, int *outSize);
int XGM_getSampleDataSize(XGM* xgm);
int XGM_getMusicDataSizeOf(LList* commands);
int XGM_getMusicDataSize(XGM* xgm);


#endif // XGM_H_
