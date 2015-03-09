#include <stdlib.h>
#include <memory.h>

#include "../inc/samplebank.h"
#include "../inc/vgmcom.h"
#include "../inc/xgmtool.h"


SampleBank* SampleBank_create(VGMCommand* command)
{
    if (!VGMCommand_isDataBlock(command))
    {
        printf("Error: incorrect sample data declaration at %6X !\n", command->offset);
        return NULL;
    }

    SampleBank* result;

    result = malloc(sizeof(SampleBank));

    result->data = command->data;
    result->offset = command->offset;

    // id
    result->id = VGMCommand_getDataBankId(command);
    // len
    result->len = VGMCommand_getDataBlockLen(command);

    result->samples = createList();

    if (verbose)
        printf("Sample added %6X  len: %6X   rate: %d Hz\n", 0, result->len, 0);

    // consider the whole bank as a single sample by default
    addToList(result->samples, Sample_create(0, 0, result->len, 0));

    return result;
}

void SampleBank_addBlock(SampleBank* bank, VGMCommand* command)
{
    if (!VGMCommand_isDataBlock(command))
        return;

    // concat data block
    int newLen = bank->len + VGMCommand_getDataBlockLen(command);
    unsigned char* newData = malloc(newLen + 7);

    memcpy(&newData[0], &bank->data[bank->offset], bank->len + 7);
    memcpy(&newData[bank->len + 7], &command->data[command->offset + 7], VGMCommand_getDataBlockLen(command));
    // adjust len
    setInt(newData, 0 + 3, newLen);

    if (verbose)
        printf("Sample added %6X  len: %6X   rate: %d Hz\n", bank->len, VGMCommand_getDataBlockLen(command), 0);

    // add new sample corresponding to this data block
    addToList(bank->samples, Sample_create(bank->samples->size, bank->len, VGMCommand_getDataBlockLen(command), 0));

    // set new data and len
    bank->data = newData;
    bank->offset = 0;
    bank->len = newLen;
}

VGMCommand* SampleBank_getDataBlockCommand(SampleBank* bank)
{
    // return command
    return VGMCommand_createEx(bank->data, bank->offset);
}

List* SampleBank_getDeclarationCommands(SampleBank* bank)
{
    unsigned char* data;
    List* result = createList();

    // rebuild data
    data = malloc(5);
    data[0] = VGM_STREAM_CONTROL;
    data[1] = bank->id;
    data[2] = 0x02;
    data[3] = 0x00;
    data[4] = 0x2A;
    addToList(result, VGMCommand_createEx(data, 0));

    data = malloc(5);
    data[0] = VGM_STREAM_DATA;
    data[1] = bank->id;
    data[2] = bank->id;
    data[3] = 0x01;
    data[4] = 0x00;
    addToList(result, VGMCommand_createEx(data, 0));

    return result;
}

//Sample* SampleBank_getSampleByOffsetAndLen(SampleBank* bank, int dataOffset, int len)
//{
//    int i;
//    int minOffset;
//    int maxOffset;
//    int minLen;
//    int maxLen;
//
//    minOffset = max(0, dataOffset - 50);
//    maxOffset = dataOffset + 50;
//    minLen = max(0, len - 50);
//    maxLen = len + 50;
//
//    for (i = 0; i < bank->samples->size; i++)
//    {
//        Sample* sample = getFromList(bank->samples, i);
//
//        // allow a small margin
//        if ((sample->dataOffset >= minOffset) && (sample->dataOffset <= maxOffset) && (sample->len >= minLen) && (sample->len <= maxLen))
//            return sample;
//    }
//
//    return NULL;
//}

Sample* SampleBank_getSampleByOffset(SampleBank* bank, int dataOffset)
{
    int i;
    int minOffset;
    int maxOffset;

    minOffset = max(0, dataOffset - 50);
    maxOffset = dataOffset + 50;

    for (i = 0; i < bank->samples->size; i++)
    {
        Sample* sample = getFromList(bank->samples, i);

        // allow a small margin
        if ((sample->dataOffset >= minOffset) && (sample->dataOffset <= maxOffset))
            return sample;
    }

    return NULL;
}

Sample* SampleBank_getSampleById(SampleBank* bank, int id)
{
    int i;

    for (i = 0; i < bank->samples->size; i++)
    {
        Sample* sample = getFromList(bank->samples, i);

        if (sample->id == id)
            return sample;
    }

    return NULL;
}

Sample* SampleBank_addSample(SampleBank* bank, int dataOffset, int len, int rate)
{
//    Sample* result = SampleBank_getSampleByOffsetAndLen(bank, dataOffset, len);
    Sample* result = SampleBank_getSampleByOffset(bank, dataOffset);

    // not found --> create new sample
    if (result == NULL)
    {
        if (verbose)
            printf("Sample added [%6X]  len: %6X   rate: %d Hz\n", dataOffset, len, rate);

        result = Sample_create(bank->samples->size, dataOffset, len, rate);
        addToList(bank->samples, result);
    }
    else
    {
        // confirmation of sample
        if (result->rate == 0)
        {
            if (verbose)
                printf("Sample confirmation [%6X]  len: %6X --> %6X   rate: %d --> %d Hz\n", dataOffset, result->len, len, result->rate, rate);

            result->rate = rate;
            result->len = len;
        }
        // adjust sample length if needed
        else if (result->len < len)
        {
            if (verbose)
                printf("Sample modified [%6X]  len: %6X --> %6X\n", dataOffset, result->len, len);

            result->len = len;
        }
    }

    return result;
}


Sample* Sample_create(int id, int dataOffset, int len, int rate)
{
    Sample* result;

    result = malloc(sizeof(Sample));

    result->id = id;
    result->dataOffset = dataOffset;
    result->len = len;
    result->rate = rate;

    return result;
}

void Sample_setRate(Sample* sample, int value)
{
    if (value != 0)
        sample->rate = value;
}

VGMCommand* Sample_getSetRateCommand(SampleBank* bank, Sample* sample, int value)
{
    unsigned char* data;

    // build command
    data = malloc(6);
    data[0] = VGM_STREAM_FREQUENCY;
    data[1] = bank->id;
    data[2] = (value >> 0) & 0xFF;
    data[3] = (value >> 8) & 0xFF;
    data[4] = 0x00;
    data[5] = 0x00;

    return VGMCommand_createEx(data, 0);
}

VGMCommand* Sample_getStartLongCommandEx(SampleBank* bank, Sample* sample, int value)
{
    unsigned char* data;
    int adjLen = min(value, sample->len);

    // build command
    data = malloc(11);
    data[0] = VGM_STREAM_START_LONG;
    data[1] = bank->id;
    data[2] = (sample->dataOffset >> 0) & 0xFF;
    data[3] = (sample->dataOffset >> 8) & 0xFF;
    data[4] = (sample->dataOffset >> 16) & 0xFF;
    data[5] = 0x00;
    data[6] = 0x01;
    data[7] = (adjLen >> 0) & 0xFF;
    data[8] = (adjLen >> 8) & 0xFF;
    data[9] = (adjLen >> 16) & 0xFF;
    data[10] = 0x00;

    return VGMCommand_createEx(data, 0);
}

VGMCommand* Sample_getStartLongCommand(SampleBank* bank, Sample* sample)
{
    return Sample_getStartLongCommandEx(bank, sample, sample->len);
}

VGMCommand* Sample_getStopCommand(SampleBank* bank, Sample* sample)
{
    unsigned char* data;

    // build command
    data = malloc(2);
    data[0] = VGM_STREAM_STOP;
    data[1] = bank->id;

    return VGMCommand_createEx(data, 0);
}
