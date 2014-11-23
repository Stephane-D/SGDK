#include <stdlib.h>

#include "../inc/xgmsmp.h"


XGMSample* XGMSample_create(int id, int addr, unsigned char* data, int dataSize)
{
    XGMSample* result;

    result = malloc(sizeof(XGMSample));

    result->id = id;
    result->addr = addr;
    result->data = data;
    result->dataSize = dataSize;

    return result;
}

XGMSample* XGMSample_createFromVGMSample(SampleBank* bank, Sample* sample)
{
    int dataSize;
    unsigned char* data;

    data = resample(bank->data, bank->offset + sample->dataOffset + 7, sample->len - 1, sample->rate, 14000, 256, &dataSize);
    XGMSample* result = XGMSample_create(sample->id + 1, sample->dataOffset, data, dataSize);

    return result;
}
