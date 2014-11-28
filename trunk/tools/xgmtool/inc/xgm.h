#ifndef XGM_H_
#define XGM_H_

#include "util.h"


typedef struct
{
    List* samples;
    List* commands;
    int pal;
} XGM;


#include "vgm.h"

XGM* XGM_create();
XGM* XGM_createFromData(unsigned char* data, int dataSize);
XGM* XGM_createFromVGM(VGM* vgm);

#include "xgmcom.h"

XGMCommand* XGM_getLoopCommand(XGM* xgm);
int XGM_getLoopPointedCommandIndex(XGM* xgm);
XGMCommand* XGM_getLoopPointedCommand(XGM* xgm);
int XGM_computeLenInFrame(XGM* xgm);
int XGM_getOffset(XGM* xgm, XGMCommand* command);
int XGM_getTime(XGM* xgm, XGMCommand* command);
int XGM_getCommandIndexAtTime(XGM* xgm, int time);
int XGM_getCommandIndexAtOffset(XGM* xgm, int offset);
XGMCommand* XGM_getCommandAtOffset(XGM* xgm, int offset);
XGMCommand* XGM_getCommandAtTime(XGM* xgm, int time);

#include "xgmsmp.h"

XGMSample* XGM_getSampleById(XGM* xgm, int id);
XGMSample* XGM_getSampleByAddress(XGM* xgm, int addr);
unsigned char* XGM_asByteArray(XGM* xgm, int *outSize);
int XGM_getSampleDataSize(XGM* xgm);
int XGM_getMusicDataSize(XGM* xgm);


#endif // XGM_H_
