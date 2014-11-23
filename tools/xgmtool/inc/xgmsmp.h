#ifndef XGMSMP_H_
#define XGMSMP_H_


typedef struct
{
    int id;
    int addr;
    unsigned char* data;
    int dataSize;
} XGMSample;


#include "samplebank.h"


XGMSample* XGMSample_create(int id, int addr, unsigned char* data, int dataSize);
XGMSample* XGMSample_createFromVGMSample(SampleBank* bank, Sample* sample);


#endif // XGMSMP_H_
