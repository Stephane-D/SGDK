#ifndef VGM_H_
#define VGM_H_

#include "samplebank.h"
#include "vgmcom.h"
#include "gd3.h"


typedef struct
{
    unsigned char* data;
    int offset;
    int dataSize;

    LList* sampleBanks;
    LList* commands;

    int version;

    int offsetStart;
    int offsetEnd;
    int lenInSample;

    int loopStart;
    int loopLenInSample;

    int rate;

    GD3* gd3;
} VGM;


VGM* VGM_create(unsigned char* data, int dataSize, int offset, bool convert);
//VGM* VGM_create1(unsigned char* data, int dataSize, int offset);
//VGM* VGM_createFromVGM(VGM* vgm, bool convert);

#include "xgm.h"

VGM* VGM_createFromXGM(XGM* xgm);

int VGM_computeLenEx(VGM* vgm, VGMCommand* from);
int VGM_computeLen(VGM* vgm);
int VGM_getOffset(VGM* vgm, VGMCommand* command);
int VGM_getTime(VGM* vgm, VGMCommand* command);
int VGM_getTimeInFrame(VGM* vgm, VGMCommand* command);
LList* VGM_getCommandElementAtTime(VGM* vgm, int time);
VGMCommand* VGM_getCommandAtTime(VGM* vgm, int time);
void VGM_cleanCommands(VGM* vgm);
void VGM_cleanSamples(VGM* vgm);
void VGM_fixKeyCommands(VGM* vgm);
//Sample* VGM_getSample(VGM* vgm, int sampleOffset, int len);
Sample* VGM_getSample(VGM* vgm, int sampleOffset);
void VGM_convertWaits(VGM* vgm);
//void VGM_shiftSamples(VGM* vgm, int sft);
unsigned char* VGM_asByteArray(VGM* vgm, int* outSize);


#endif // VGM_H_
