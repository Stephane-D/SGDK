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

    if (verbose)
        printf("Initial bank sample added [%6X-%6X]   rate: %d Hz\n", 0, result->len - 1, 0);

    // consider the whole bank as a single sample by default
    result->samples = createElement(Sample_create(0, 0, result->len, 0));

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
        printf("Initial block sample added [%6X-%6X]   rate: %d Hz\n", bank->len, newLen - 1, 0);

    // add new sample corresponding to this data block
    insertAfterLList(bank->samples, Sample_create(getSizeLList(bank->samples), bank->len, VGMCommand_getDataBlockLen(command), 0));

    // set new data and len
    bank->data = newData;
    bank->offset = 0;
    bank->len = newLen;
}

VGMCommand* SampleBank_getDataBlockCommand(SampleBank* bank)
{
    // return command
    return VGMCommand_createEx(bank->data, bank->offset, -1);
}

LList* SampleBank_getDeclarationCommands(SampleBank* bank)
{
    unsigned char* data;
    LList* result;

    // rebuild data
    data = malloc(5);
    data[0] = VGM_STREAM_CONTROL;
    data[1] = bank->id;
    data[2] = 0x02;
    data[3] = 0x00;
    data[4] = 0x2A;
    result = createElement(VGMCommand_createEx(data, 0, -1));

    data = malloc(5);
    data[0] = VGM_STREAM_DATA;
    data[1] = bank->id;
    data[2] = bank->id;
    data[3] = 0x01;
    data[4] = 0x00;
    result = insertAfterLList(result, VGMCommand_createEx(data, 0, -1));

    return getHeadLList(result);
}

//Sample* SampleBank_getSampleByOffsetAndLen(SampleBank* bank, int dataOffset, int len)
//{
//    LList* list;
//
//    list = bank->samples;
//    while (list != NULL)
//    {
//        Sample* sample = list->element;
//        int frameSize = Sample_getFrameSize(sample);
//
//        // allow a margin of 1 frame for both offset and size
//        if ((abs(sample->dataOffset - dataOffset) < frameSize) && (abs(len - sample->len) < frameSize))
//            return sample;
//
//        list = list->next;
//    }
//
//    return NULL;
//}

Sample* SampleBank_getSampleByOffset(SampleBank* bank, int dataOffset)
{
    LList* list;

    list = bank->samples;
    while (list != NULL)
    {
        Sample* sample = list->element;
        int frameSize = Sample_getFrameSize(sample);

        // allow a margin of 1 frame for offset
        if (abs(sample->dataOffset - dataOffset) < frameSize)
            return sample;

        list = list->next;
    }

    return NULL;
}

Sample* SampleBank_getSampleById(SampleBank* bank, int id)
{
    LList* list;

    list = bank->samples;
    while (list != NULL)
    {
        Sample* sample = list->element;

        if (sample->id == id)
            return sample;

        list = list->next;
    }

    return NULL;
}

Sample* SampleBank_addSample(SampleBank* bank, int dataOffset, int len, int rate)
{
//    TODO: maybe use a switch here
//    Sample* result = SampleBank_getSampleByOffsetAndLen(bank, dataOffset, len);
    Sample* result = SampleBank_getSampleByOffset(bank, dataOffset);

    // not found --> create new sample
    if (result == NULL)
    {
        if (verbose)
            printf("Sample added     [%6X-%6X]  len: %6X  rate: %d Hz\n", dataOffset, dataOffset + (len - 1), len, rate);

        result = Sample_create(getSizeLList(bank->samples), dataOffset, len, rate);
        insertAfterLList(bank->samples, result);
    }
    // confirmation of sample
    else if (result->rate == 0)
    {
        if (verbose)
            printf("Sample confirmed [%6X-%6X]  len: %6X --> %6X   rate: %d --> %d Hz\n", dataOffset, dataOffset + (len - 1), result->len, len, result->rate, rate);

        result->rate = rate;
        result->len = len;
    }
    else
    {
        // adjust sample length if needed
        if (result->len < len)
        {
            if (verbose)
                printf("Sample modified  [%6X-%6X]  len: %6X --> %6X\n", dataOffset, dataOffset + (len - 1), result->len, len);

            result->len = len;
        }

//        // get size of sample frame
//        int frameLen = Sample_getFrameSize(result);
//
//        // less than 1 frame in size difference --> assume same sample
//        if (abs(len - result->len) < frameLen)
//        {
//            // adjust sample length if needed
//            if (result->len < len)
//            {
//                if (verbose)
//                    printf("Sample modified  [%6X-%6X]  len: %6X --> %6X\n", dataOffset, dataOffset + (len - 1), result->len, len);
//
//                result->len = len;
//            }
//        }
//        // too large difference in size --> assume we have a new sample here
//        else
//        {
//            if (verbose)
//                printf("Sample added     [%6X-%6X]  len: %6X  rate: %d Hz\n", dataOffset, dataOffset + (len - 1), len, rate);
//
//            result = Sample_create(getSizeLList(bank->samples), dataOffset, len, rate);
//            insertAfterLList(bank->samples, result);
//        }
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

int Sample_getFrameSize(Sample* sample)
{
    if (sample->rate > 0)
        return sample->rate / 60;

    // consider 4Khz by default for safety
    return 4000 / 60;
}

void Sample_setRate(Sample* sample, int value)
{
    if (value != 0)
    {
        // set sample rate if needed
        if (sample->rate != value)
        {
            if (verbose)
                printf("Sample modified  [%6X-%6X]  rate: %d --> %d Hz\n", sample->dataOffset, sample->dataOffset + (sample->len - 1), sample->rate, value);

            sample->rate = value;
        }
    }
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

    return VGMCommand_createEx(data, 0, -1);
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

    return VGMCommand_createEx(data, 0, -1);
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

    return VGMCommand_createEx(data, 0, -1);
}
