#include <stdlib.h>

#include "../inc/xgmsmp.h"


XGMSample* XGMSample_create(int index, unsigned char* data, int dataSize, int originAddr)
{
    XGMSample* result;

    result = malloc(sizeof(XGMSample));

    result->index = index;
    result->data = data;
    result->dataSize = dataSize;
    result->originAddr = originAddr;

    return result;
}

XGMSample* XGMSample_createFromVGMSample(SampleBank* bank, Sample* sample)
{
    int dataSize;
    unsigned char* data;

    // invalid sample
    if (sample->rate == 0)
        return NULL;

    data = resample(bank->data, bank->offset + sample->dataOffset + 7, sample->len - 1, sample->rate, 14000, 256, &dataSize);
    // index should be modified when inserted in sample list
    XGMSample* result = XGMSample_create(0, data, dataSize, sample->dataOffset);

    return result;
}
