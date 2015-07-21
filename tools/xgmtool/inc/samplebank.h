#ifndef SAMPLEBANK_H_
#define SAMPLEBANK_H_

#include "util.h"


typedef struct
{
    int id;
    int dataOffset;
    int len;
    int rate;
} Sample;

typedef struct
{
    LList* samples;
    unsigned char* data;
    int offset;
    int len;
    int id;
} SampleBank;


#include "vgmcom.h"

SampleBank* SampleBank_create(VGMCommand* command);
void SampleBank_addBlock(SampleBank* bank, VGMCommand* command);
VGMCommand* SampleBank_getDataBlockCommand(SampleBank* bank);
LList* SampleBank_getDeclarationCommands(SampleBank* bank);
//Sample* SampleBank_getSampleByOffsetAndLen(SampleBank* bank, int dataOffset, int len);
Sample* SampleBank_getSampleByOffset(SampleBank* bank, int dataOffset);
Sample* SampleBank_getSampleById(SampleBank* bank, int id);
Sample* SampleBank_addSample(SampleBank* bank, int dataOffset, int len, int rate);
int Sample_getFrameSize(Sample* sample);

Sample* Sample_create(int id, int dataOffset, int len, int rate);
void Sample_setRate(Sample* sample, int value);
VGMCommand* Sample_getSetRateCommand(SampleBank* bank, Sample* sample, int value);
VGMCommand* Sample_getStartLongCommandEx(SampleBank* bank, Sample* sample, int value);
VGMCommand* Sample_getStartLongCommand(SampleBank* bank, Sample* sample);
VGMCommand* Sample_getStopCommand(SampleBank* bank, Sample* sample);


#endif // SAMPLEBANK_H_
