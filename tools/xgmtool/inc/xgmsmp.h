#ifndef XGMSMP_H_
#define XGMSMP_H_


typedef struct
{
    int index;
    unsigned char* data;
    int dataSize;
    int originAddr;
} XGMSample;


#include "samplebank.h"


XGMSample* XGMSample_create(int index, unsigned char* data, int dataSize, int originAddr);
XGMSample* XGMSample_createFromVGMSample(SampleBank* bank, Sample* sample);

#endif // XGMSMP_H_
