#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "../inc/vgm.h"
#include "../inc/xgm.h"
#include "../inc/util.h"
#include "../inc/ym2612.h"
#include "../inc/psg.h"
#include "../inc/xgmtool.h"

#define SAMPLE_END_DELAY        400
#define SAMPLE_MIN_SIZE         100
#define SAMPLE_MIN_DYNAMIC      16
#define SAMPLE_ALLOWED_MARGE    64
#define SAMPLE_MIN_MEAN_DELTA   1.0


// forward
static void VGM_parse(VGM* vgm);
static void VGM_buildSamples(VGM* vgm, bool convert);
static LList* VGM_extractSampleFromSeek(VGM* vgm, LList* command, bool convert);
static SampleBank* VGM_getDataBank(VGM* vgm, int id);
static SampleBank* VGM_addDataBlock(VGM* vgm, VGMCommand* command);
static void VGM_cleanSeekCommands(VGM* vgm);
static void VGM_cleanPlayPCMCommands(VGM* vgm);
static void VGM_removeSeekAndPlayPCMCommands(VGM* vgm);
static int VGM_getSampleDataSize(VGM* vgm);
static int VGM_getSampleTotalLen(VGM* vgm);
static int VGM_getSampleNumber(VGM* vgm);
static int VGM_getMusicDataSize(VGM* vgm);

VGM* VGM_create(unsigned char* data, int dataSize, int offset, bool convert)
{
    int ver;

    if (strncasecmp(&data[offset + 0x00], "VGM ", 4))
    {
        printf("Error: VGM file not recognized !\n");
        return NULL;
    }

    // just check for sub version info (need version 1.50 at least)
    ver = data[offset + 8] & 0xFF;
    if (ver < 0x50)
    {
        printf("Error: VGM version 1.%2X not supported !\n", ver);
        return NULL;
    }

    if (!silent)
    {
        if (convert)
            printf("Optimizing VGM...\n");
        else
            printf("Parsing VGM file...\n");
    }

    VGM* result = malloc(sizeof(VGM));

    // set version
    result->version = ver;

    result->data = data;
    result->dataSize = dataSize;
    result->offset = offset;

    // start offset
    result->offsetStart = getInt(data, offset + 0x34) + (offset + 0x34);
    // end offset
    result->offsetEnd = getInt(data, offset + 0x04) + (offset + 0x04);

    // track len (in number of sample = 1/44100 sec)
    result->lenInSample = getInt(data, offset + 0x18);

    // loop start offset
    result->loopStart = getInt(data, offset + 0x1C);
    if (result->loopStart != 0)
        result->loopStart += (offset + 0x1C);
    // loop len (in number of sample = 1/44100 sec)
    result->loopLenInSample = getInt(data, offset + 0x20);

    // 50 or 60 Hz
    result->rate = getInt(data, offset + 0x24);
    if (result->rate != 50)
        result->rate = 60;

    if (!silent)
        printf("VGM duration: %d samples (%d seconds)\n", result->lenInSample, result->lenInSample / 44100);

    if (verbose)
    {
        printf("VGM data start: %6X   end: %6X\n", result->offsetStart, result->offsetEnd);
        printf("Loop start offset: %6X   lenght: %d (%d seconds)\n", result->loopStart, result->loopLenInSample, result->loopLenInSample / 44100);
    }

    result->sampleBanks = NULL;
    result->commands = NULL;

    // build command list
    VGM_parse(result);

    if (!silent)
        printf("Computed VGM duration: %d samples (%d seconds)\n", VGM_computeLen(result), VGM_computeLen(result) / 44100);

    // and build samples
    VGM_buildSamples(result, convert);

    // rebuild data blocks
    if (convert)
    {
        LList* c;
        LList* s;

        // remove previous data blocks
        c = result->commands;
        while(c != NULL)
        {
            VGMCommand* command = c->element;

            if (VGMCommand_isDataBlock(command) || VGMCommand_isStreamControl(command) || VGMCommand_isStreamData(command))
            {
                removeFromLList(c);
                // we removed the first command ? update list pointer
                if (c == result->commands)
                    result->commands = c->next;
            }

            c = c->next;
        }

        // rebuild data blocks
        c = NULL;
        s = result->sampleBanks;
        while(s != NULL)
        {
            SampleBank* bank = s->element;

            c = insertAfterLList(c, SampleBank_getDataBlockCommand(bank));
            c = insertAllAfterLList(c, SampleBank_getDeclarationCommands(bank));

            s = s->next;
        }

        // and re-insert them at beginning
        result->commands = insertAllBeforeLList(result->commands, getHeadLList(c));
    }

    if (verbose)
    {
        printf("VGM sample number: %d\n", VGM_getSampleNumber(result));
        printf("Sample data size: %d\n", VGM_getSampleDataSize(result));
        printf("Sample total len: %d\n", VGM_getSampleTotalLen(result));
    }

    return result;
}

VGM* VGM_create1(unsigned char* data, int dataSize, int offset)
{
    return VGM_create(data, dataSize, offset, false);
}

VGM* VGM_createFromVGM(VGM* vgm, bool convert)
{
    return VGM_create(vgm->data, vgm->dataSize, vgm->offset, convert);
}

VGM* VGM_createFromXGM(XGM* xgm)
{
    VGM* result;
    LList* s;
    LList* d;
    unsigned char* data;
    int loopOffset;

    result = malloc(sizeof(VGM));

    result->data = NULL;
    result->dataSize = 0;
    result->offset = 0;

    result->sampleBanks = NULL;

    result->version = 0x60;
    result->offsetStart = 0;
    result->offsetEnd = 0;
    result->lenInSample = 0;
    result->loopStart = 0;
    result->loopLenInSample = 0;
    if (xgm->pal)
        result->rate = 50;
    else
        result->rate = 60;

    // convert XGM commands to VGM commands
    loopOffset = -1;
    s = xgm->commands;
    d = NULL;
    while(s != NULL)
    {
        int j, comsize;
        XGMCommand* command = s->element;

        switch (XGMCommand_getType(command))
        {
            case XGM_FRAME:
                data = malloc(1);
                if (xgm->pal)
                    data[0] = 0x63;
                else
                    data[0] = 0x62;
                d = insertAfterLList(d, VGMCommand_createEx(data, 0));
                break;

            case XGM_END:
                break;

            case XGM_LOOP:
                loopOffset = XGMCommand_getLoopOffset(command);
                break;

            case XGM_PCM:
                // not handled here
                break;

            case XGM_PSG:
                comsize = (command->data[0] & 0xF) + 1;
                for (j = 0; j < comsize; j++)
                {
                    data = malloc(2);
                    data[0] = 0x50;
                    data[1] = command->data[j + 1];
                    d = insertAfterLList(d, VGMCommand_createEx(data, 0));
                }
                break;

            case XGM_YM2612_PORT0:
                comsize = (command->data[0] & 0xF) + 1;
                for (j = 0; j < comsize; j++)
                {
                    data = malloc(3);
                    data[0] = 0x52;
                    data[1] = command->data[(j * 2) + 1];
                    data[2] = command->data[(j * 2) + 2];
                    d = insertAfterLList(d, VGMCommand_createEx(data, 0));
                }
                break;

            case XGM_YM2612_PORT1:
                comsize = (command->data[0] & 0xF) + 1;
                for (j = 0; j < comsize; j++)
                {
                    data = malloc(3);
                    data[0] = 0x53;
                    data[1] = command->data[(j * 2) + 1];
                    data[2] = command->data[(j * 2) + 2];
                    d = insertAfterLList(d, VGMCommand_createEx(data, 0));
                }
                break;

            case XGM_YM2612_REGKEY:
                comsize = (command->data[0] & 0xF) + 1;
                for (j = 0; j < comsize; j++)
                {
                    data = malloc(3);
                    data[0] = 0x52;
                    data[1] = 0x28;
                    data[2] = command->data[j + 1];
                    d = insertAfterLList(d, VGMCommand_createEx(data, 0));
                }
                break;
        }

        s = s->next;
    }

    // end bloc marker
    data = malloc(1);
    data[0] = 0x66;
    d = insertAfterLList(d, VGMCommand_createEx(data, 0));

    // store result
    result->commands = getHeadLList(d);

    // we had a loop command ?
    if (loopOffset != -1)
    {
        // find corresponding XGM command
        XGMCommand* command = XGM_getCommandAtOffset(xgm, loopOffset);

        // insert a VGM loop command at corresponding position
        if (command != NULL)
        {
            LList* c = VGM_getCommandElementAtTime(result, XGM_getTime(xgm, command));
            insertAfterLList(c, VGMCommand_create(VGM_LOOP));
        }
    }

    return result;
}

int VGM_computeLenEx(VGM* vgm, VGMCommand* from)
{
    LList* curCom = vgm->commands;
    int result = 0;
    bool count = (from == NULL);

    while(curCom != NULL)
    {
        VGMCommand* command = curCom->element;

        if (command == from)
            count = true;
        if (count)
            result += VGMCommand_getWaitValue(command);

        curCom = curCom->next;
    }

    return result;
}

int VGM_computeLen(VGM* vgm)
{
    return VGM_computeLenEx(vgm, NULL);
}

/**
 * Return the offset of the specified command
 */
int VGM_getOffset(VGM* vgm, VGMCommand* command)
{
    LList* curCom = vgm->commands;
    int result = 0;

    while(curCom != NULL)
    {
        VGMCommand* c = curCom->element;

        if (c == command)
            return result;
        result += c->size;

        curCom = curCom->next;
    }

    return -1;
}

/**
 * Return elapsed time (in 1/44100th of second) when specified command happen
 */
int VGM_getTime(VGM* vgm, VGMCommand* command)
{
    LList* curCom = vgm->commands;
    int result = 0;

    while(curCom != NULL)
    {
        VGMCommand* c = curCom->element;

        if (c == command)
            return result;
        result += VGMCommand_getWaitValue(c);

        curCom = curCom->next;
    }

    return 0;
}


/**
 * Return elapsed time (in frame) when specified command happen
 */
int VGM_getTimeInFrame(VGM* vgm, VGMCommand* command)
{
    return VGM_getTime(vgm, command) / (44100 / vgm->rate);
}

/**
 * Return command list element at specified time position
 */
LList* VGM_getCommandElementAtTime(VGM* vgm, int time)
{
    LList* c = vgm->commands;
    int result = 0;

    while(c != NULL)
    {
        VGMCommand* command = c->element;

        if (result >= time)
            return c;

        result += VGMCommand_getWaitValue(command);
        c = c->next;
    }

    return NULL;
}

/**
 * Return elapsed time when specified command happen
 */
VGMCommand* VGM_getCommandAtTime(VGM* vgm, int time)
{
    LList* c = VGM_getCommandElementAtTime(vgm, time);

    if (c != NULL) return c->element;

    return NULL;
}


static void VGM_parse(VGM* vgm)
{
    int off;
    LList* commands;

    // parse all VGM commands
    off = vgm->offsetStart;
    commands = getTailLList(vgm->commands);
    while (off < vgm->offsetEnd)
    {
        // check for loop start
        if ((vgm->loopStart != 0) && (off == vgm->loopStart))
            commands = insertAfterLList(commands, VGMCommand_create(VGM_LOOP));

        VGMCommand* command = VGMCommand_createEx(vgm->data, off);
        commands = insertAfterLList(commands, command);
        off += command->size;

        // stop here
        if (VGMCommand_isEnd(command))
            break;
    }

    vgm->commands = getHeadLList(commands);

    if (!silent)
        printf("Number of command: %d\n", getSizeLList(vgm->commands));
}

static void VGM_buildSamples(VGM* vgm, bool convert)
{
    int i;
    LList* curCom;

    // builds data blocks
    curCom = vgm->commands;
    while(curCom != NULL)
    {
        VGMCommand* command = curCom->element;

        if (VGMCommand_isDataBlock(command))
            VGM_addDataBlock(vgm, command);

        curCom = curCom->next;
    }

    // clean seek
    VGM_cleanSeekCommands(vgm);
    // clean useless PCM data
//    VGM_cleanPlayPCMCommands(vgm);

    // extract samples from seek command
    curCom = vgm->commands;
    while(curCom != NULL)
    {
        VGMCommand* command = curCom->element;

        if (VGMCommand_isSeek(command))
            curCom = VGM_extractSampleFromSeek(vgm, curCom, convert);
        else
            curCom = curCom->next;
    }

    // display play PCM command
//    if (convert && verbose)
//    {
//        curCom = vgm->commands;
//        while(curCom != NULL)
//        {
//            VGMCommand* command = curCom->element;
//
//            if (VGMCommand_isStreamStartLong(command))
//            {
//                int smpAddr = VGMCommand_getStreamSampleAddress(command);
//                int smpSize = VGMCommand_getStreamSampleSize(command);
//                printf("play sample long %.6X-%.6X at frame %d\n", smpAddr, smpAddr + (smpSize - 1), VGM_getTimeInFrame(vgm, command));
//            }
//            else if (VGMCommand_isStreamStart(command))
//            {
//                int id = VGMCommand_getStreamId(command);
//                int blockId = VGMCommand_getStreamBlockId(command);
//                printf("play sample short %d/%d at frame %d\n", id, blockId, VGM_getTimeInFrame(vgm, command));
//            }
//            else if (VGMCommand_isStreamStop(command))
//            {
//                int id = VGMCommand_getStreamId(command);
//                printf("stop play sample %d at frame %d\n", id, VGM_getTimeInFrame(vgm, command));
//            }
//
//            curCom = curCom->next;
//        }
//    }

    int sampleIdBanks[0x100];
    int sampleIdFrequencies[0x100];

    // set bank id and frequency to -1 by default
    for (i = 0; i < 0x100; i++)
    {
        sampleIdBanks[i] = -1;
        sampleIdFrequencies[i] = 0;
    }

    // adjust samples infos from stream command
    curCom = vgm->commands;
    while(curCom != NULL)
    {
        VGMCommand* command = curCom->element;

        // set bank id
        if (VGMCommand_isStreamData(command))
            sampleIdBanks[VGMCommand_getStreamId(command)] = VGMCommand_getStreamBankId(command);
        // set frequency
        if (VGMCommand_isStreamFrequency(command))
            sampleIdFrequencies[VGMCommand_getStreamId(command)] = VGMCommand_getStreamFrenquency(command);

        // short start command
        if (VGMCommand_isStreamStart(command))
        {
            int bankId = sampleIdBanks[VGMCommand_getStreamId(command)];
            SampleBank* bank = VGM_getDataBank(vgm, bankId);

            if (bank != NULL)
            {
                int sampleId = VGMCommand_getStreamBlockId(command);
                Sample* sample = SampleBank_getSampleById(bank, sampleId);

                // sample found --> adjust frequency
                if (sample != NULL)
                    Sample_setRate(sample, sampleIdFrequencies[VGMCommand_getStreamId(command)]);
                else if (!silent)
                    printf("Warning: sample id %2X not found !\n", sampleId);

                // convert to long command as we use single data block
                if (convert)
                    curCom->element = Sample_getStartLongCommandEx(bank, sample, sample->len);
            }
            else if (!silent)
                printf("Warning: sample bank id %2X not found !\n", bankId);
        }

        // long start command
        if (VGMCommand_isStreamStartLong(command))
        {
            int bankId = sampleIdBanks[VGMCommand_getStreamId(command)];
            SampleBank* bank = VGM_getDataBank(vgm, bankId);

            if (bank != NULL)
            {
                int sampleAddress = VGMCommand_getStreamSampleAddress(command);
                int sampleLen = VGMCommand_getStreamSampleSize(command);

                // add sample
                SampleBank_addSample(bank, sampleAddress, sampleLen, sampleIdFrequencies[VGMCommand_getStreamId(command)]);
            }
        }

        curCom = curCom->next;
    }

    if (convert)
        VGM_removeSeekAndPlayPCMCommands(vgm);
}

static LList* VGM_extractSampleFromSeek(VGM* vgm, LList* command, bool convert)
{
    LList* seekCom = command;
    // get sample address in data bank
    int sampleAddr = VGMCommand_getSeekAddress(seekCom->element);

    LList* curCom;
    LList* startPlayCom;
    LList* endPlayCom;
    SampleBank* bank;
    int len;
    int wait;
    int delta;
    double deltaMean;
    int endPlayWait;

    // sample stats
    int sampleData;
    int sampleMinData;
    int sampleMaxData;
    double sampleMeanDelta;

    // use the last bank (FIXME: not really nice to do that)
    if (vgm->sampleBanks != NULL)
        bank = getTailLList(vgm->sampleBanks)->element;
    else
        bank = NULL;

    // then find seek command to extract sample
    len = 0;
    wait = -1;
    delta = 0;
    deltaMean = 0;
    endPlayWait = 0;

    sampleData = 128;
    sampleMinData = 128;
    sampleMaxData = 128;
    sampleMeanDelta = 0;

    startPlayCom = NULL;
    endPlayCom = NULL;
    curCom = seekCom->next;
    while (curCom != NULL)
    {
        VGMCommand* command = curCom->element;

        // sample done !
        if (VGMCommand_isDataBlock(command) || VGMCommand_isEnd(command))
            break;
        if (VGMCommand_isSeek(command))
        {
            int seekAddr = VGMCommand_getSeekAddress(command);
            int curAddr = sampleAddr + len;

            // seek on different address --> interrupt current play
            if (((curAddr + SAMPLE_ALLOWED_MARGE) < seekAddr) || ((curAddr - SAMPLE_ALLOWED_MARGE) > seekAddr))
                break;
            else if (verbose)
                printf("Seek command found with small offset change (%d) --> considering continue play\n", curAddr - seekAddr);
        }

        // playing ?
        if (wait != -1)
        {
            delta = wait - endPlayWait;
            // delta >= 20 means rate < 2200 Hz --> very unlikely, discard it from mean computation
            if (delta < 20)
            {
                // compute delta mean for futher correction
                if (deltaMean == 0) deltaMean = delta;
                else deltaMean = (delta * 0.1) + (deltaMean * 0.9);
            }

            // delta > SAMPLE_END_DELAY samples --> consider sample ended
            if (delta > SAMPLE_END_DELAY)
            {
                // found a sample --> add it
                if ((len > 0) && (endPlayWait > 0) && (startPlayCom != endPlayCom))
                {
                    // ignore too short sample
                    if ((len < SAMPLE_MIN_SIZE) && sampleIgnore)
                    {
                        if (verbose)
                            printf("Sample at %6X is too small (%d) --> ignored\n", sampleAddr, len);
                    }
                    // ignore sample with too small dynamic
                    else if (((sampleMaxData - sampleMinData) < SAMPLE_MIN_DYNAMIC) && sampleIgnore)
                    {
                        if (verbose)
                            printf("Sample at %6X has a too quiet global dynamic (%d) --> ignored\n", sampleAddr, sampleMaxData - sampleMinData);
                    }
                    // ignore sample too quiet
                    else if (((sampleMeanDelta / len) < SAMPLE_MIN_MEAN_DELTA) && sampleIgnore)
                    {
                        if (verbose)
                            printf("Sample at %6X is too quiet (mean delta value = %g) --> ignored\n", sampleAddr, (sampleMeanDelta / len));
                    }
                    else if (bank != NULL)
                    {
                        int rate = (int) round(((double) 44100 * len) / (double) endPlayWait);
                        Sample* sample = SampleBank_addSample(bank, sampleAddr, len, rate);

                        if (convert)
                        {
                            // insert stream play command
                            insertBeforeLList(startPlayCom, Sample_getSetRateCommand(bank, sample, sample->rate));
                            insertBeforeLList(startPlayCom, Sample_getStartLongCommandEx(bank, sample, len));

                            // always insert sample stop as sample len can change
//                            if ((sample->len + SAMPLE_ALLOWED_MARGE) < len)
                            {
                                // insert stream stop command
                                insertAfterLList(endPlayCom, Sample_getStopCommand(bank, sample));
                            }
                        }
                    }
                }

                // reset
                sampleAddr += len;
                len = 0;
                wait = -1;
                delta = 0;
                deltaMean = 0;
                endPlayWait = 0;

                sampleData = 128;
                sampleMinData = 128;
                sampleMaxData = 128;
                sampleMeanDelta = 0;

                startPlayCom = NULL;
                endPlayCom = NULL;
            }
        }

        // compute sample len
        if (VGMCommand_isPCM(command))
        {
            // start play --> init wait
            if (wait == -1)
            {
                wait = 0;
                startPlayCom = curCom;
            }

            // simple fix by using mean
            if (sampleRateFix)
            {
                // need a minimal length before applying correction
    //            if ((len > 100) && (wait > 200))
    //            {
    //                int mean = wait / len;
    //
    //                // correct abnormal delta
    //                if (delta < (mean - 2))
    //                    wait += (mean - delta);
    //                else if (delta > (mean + 2))
    //                    wait -= (delta - mean);
    //            }

                // can correct ?
                if (deltaMean != 0)
                {
                    int mean = round(deltaMean);
                    if (delta < (mean - 2))
                        wait += mean - delta;
                    else if (delta > (mean + 2))
                        wait -= delta - mean;
                }
            }

            // keep trace of last play wait value
            endPlayWait = wait;
            endPlayCom = curCom;

            // get current sample value
            if (bank != NULL)
            {
                unsigned char data = bank->data[bank->offset + sampleAddr + 7 + len];

                sampleMeanDelta += abs(data - sampleData);
                if (sampleMinData > data) sampleMinData = data;
                if (sampleMaxData < data) sampleMaxData = data;
                sampleData = data;
            }

            wait += VGMCommand_getWaitValue(command);
            len++;
        }
        // playing ?
        else if (wait != -1)
            wait += VGMCommand_getWaitValue(command);

        curCom = curCom->next;
    }

    // found a sample --> add it
    if ((len > 0) && (endPlayWait > 0) && (startPlayCom != endPlayCom))
    {
        // ignore too short sample
        if ((len < SAMPLE_MIN_SIZE) && sampleIgnore)
        {
            if (verbose)
                printf("Sample at %6X is too small (%d) --> ignored\n", sampleAddr, len);
        }
        // ignore sample with too small dynamic
        else if (((sampleMaxData - sampleMinData) < SAMPLE_MIN_DYNAMIC) && sampleIgnore)
        {
            if (verbose)
                printf("Sample at %6X has a too quiet global dynamic (%d) --> ignored\n", sampleAddr, sampleMaxData - sampleMinData);
        }
        // ignore sample too quiet
        else if (((sampleMeanDelta / len) < SAMPLE_MIN_MEAN_DELTA) && sampleIgnore)
        {
            if (verbose)
                printf("Sample at %6X is too quiet (mean delta value = %g) --> ignored\n", sampleAddr, (sampleMeanDelta / len));
        }
        else if (bank != NULL)
        {
            int rate = (int) round(((double) 44100 * len) / (double) endPlayWait);
            Sample* sample = SampleBank_addSample(bank, sampleAddr, len, rate);

            if (convert)
            {
                // insert stream play command
                insertBeforeLList(startPlayCom, Sample_getSetRateCommand(bank, sample, sample->rate));
                insertBeforeLList(startPlayCom, Sample_getStartLongCommandEx(bank, sample, len));

                // always insert sample stop as sample len can change
//              if ((sample->len + SAMPLE_ALLOWED_MARGE) < len)
                {
                    // insert stream stop command
                    insertAfterLList(endPlayCom, Sample_getStopCommand(bank, sample));
                }
            }
        }
    }

    return curCom;
}

static SampleBank* VGM_getDataBank(VGM* vgm, int id)
{
    LList* curBank;

    curBank = vgm->sampleBanks;
    while(curBank != NULL)
    {
        SampleBank* bank = curBank->element;

        if (bank->id == id)
            return bank;

        curBank = curBank->next;
    }

    return NULL;
}

static SampleBank* VGM_addDataBlock(VGM* vgm, VGMCommand* command)
{
    SampleBank* result;

    result = VGM_getDataBank(vgm, VGMCommand_getDataBankId(command));
    // different id --> new bank
    if (result == NULL)
    {
        LList* curBank = getTailLList(vgm->sampleBanks);

        if (verbose)
            printf("Add data bank %6X:%2X\n", command->offset, VGMCommand_getDataBankId(command));

        result = SampleBank_create(command);
        vgm->sampleBanks = getHeadLList(insertAfterLList(curBank, result));
    }
    // same id --> concat block
    else
    {
        if (verbose)
            printf("Add data block %6X to bank %2X\n", command->offset, VGMCommand_getDataBankId(command));

        SampleBank_addBlock(result, command);
    }

    return result;
}

static void VGM_cleanSeekCommands(VGM* vgm)
{
    LList* curCom;
    bool samplePlayed;

    curCom = getTailLList(vgm->commands);
    samplePlayed = false;
    while(curCom != NULL)
    {
        VGMCommand* command = curCom->element;

        // seek command ?
        if (VGMCommand_isSeek(command))
        {
            // no sample played after this seek command --> remove it
            if (!samplePlayed)
            {
                if (!silent)
                    printf("Useless seek command found at %6X", command->offset);

                removeFromLList(curCom);
            }

            samplePlayed = false;
        }
        else if (VGMCommand_isPCM(command))
            samplePlayed = true;

        curCom = curCom->prev;
    }
}

static void VGM_cleanPlayPCMCommands(VGM* vgm)
{
    LList* curCom;
    bool dacEnabled = false;

    curCom = vgm->commands;
    while(curCom != NULL)
    {
        VGMCommand* command = curCom->element;

        if (VGMCommand_isDACEnabledON(command))
            dacEnabled = true;
        else if (VGMCommand_isDACEnabledOFF(command))
            dacEnabled = false;
        else
        {
            // DAC is currently disabled
            if (!dacEnabled)
            {
                // replace PCM command by simple wait command
                if (VGMCommand_isPCM(command))
                {
                    const int wait = VGMCommand_getWaitValue(command);

                    // remove or just replace by wait command
                    if (wait == 0)
                        removeFromLList(curCom);
                    else
                        curCom->element = VGMCommand_create(0x70 + (wait - 1));
                }
            }
        }

        curCom = curCom->next;
    }

    if (!silent)
        printf("Number of command after PCM command cleaning: %d\n", getSizeLList(vgm->commands));
    if (verbose)
        printf("Computed VGM duration: %d samples (%d seconds)\n", VGM_computeLen(vgm), VGM_computeLen(vgm) / 44100);
}

static void VGM_removeSeekAndPlayPCMCommands(VGM* vgm)
{
    LList* curCom;

    curCom = vgm->commands;
    while(curCom != NULL)
    {
        VGMCommand* command = curCom->element;

        // remove Seek command
        if (VGMCommand_isSeek(command))
            removeFromLList(curCom);
        // replace PCM command by simple wait command
        else if (VGMCommand_isPCM(command))
        {
            const int wait = VGMCommand_getWaitValue(command);

            // remove or just replace by wait command
            if (wait == 0)
                removeFromLList(curCom);
            else
                curCom->element = VGMCommand_create(0x70 + (wait - 1));
        }

        curCom = curCom->next;
    }

    if (!silent)
        printf("Number of command after PCM command remove: %d\n", getSizeLList(vgm->commands));
    if (verbose)
        printf("Computed VGM duration: %d samples (%d seconds)\n", VGM_computeLen(vgm), VGM_computeLen(vgm) / 44100);
}

void VGM_cleanCommands(VGM* vgm)
{
    LList* newCommands = NULL;
    LList* optimizedCommands = NULL;
    LList* keyOnOffCommands = NULL;
    LList* lastCommands = NULL;

    YM2612* ymOldState;
    YM2612* ymState;
    PSG* psgOldState;
    PSG* psgState;

    ymOldState = YM2612_create();
    psgOldState = PSG_create();

    VGMCommand* command;
    LList* startCom;
    LList* endCom;
    LList* com;

    int cnt = 0;

    startCom = vgm->commands;
    do
    {
        endCom = startCom;

        do
        {
            command = endCom->element;
            endCom = endCom->next;
            cnt++;
        }
        while ((endCom != NULL) && !VGMCommand_isWait(command) && !VGMCommand_isEnd(command));

        psgState = PSG_copy(psgOldState);
        ymState = YM2612_copy(ymOldState);

        // clear frame sets
        deleteLList(optimizedCommands);
        deleteLList(keyOnOffCommands);
        deleteLList(lastCommands);
        optimizedCommands = NULL;
        keyOnOffCommands = NULL;
        lastCommands = NULL;

        // startCom --> endCom contains commands for a single frame
        com = startCom;
        while (com != endCom)
        {
            command = com->element;

            // keep data block and stream commands
            if (VGMCommand_isDataBlock(command) || VGMCommand_isStream(command) || VGMCommand_isLoop(command))
                optimizedCommands = insertAfterLList(optimizedCommands, command);
            else if (VGMCommand_isPSGWrite(command))
                PSG_write(psgState, VGMCommand_getPSGValue(command));
            else if (VGMCommand_isYM2612Write(command))
            {
                // write to YM state and store key on/off command
                if (YM2612_set(ymState, VGMCommand_getYM2612Port(command), VGMCommand_getYM2612Register(command), VGMCommand_getYM2612Value(command)))
                    keyOnOffCommands = insertAfterLList(keyOnOffCommands, command);
            }
            else if (VGMCommand_isWait(command) || VGMCommand_isSeek(command))
                lastCommands = insertAfterLList(lastCommands, command);
            else
            {
                if (verbose)
                    printf("Command ignored: %2X\n", command->command);
            }

            com = com->next;
        }

        bool hasStreamStart = false;
        bool hasStreamRate = false;
        // start at end of optimized commands
        com = optimizedCommands;
        // check we have single stream per frame
        while(com != NULL)
        {
            command = com->element;

            if (VGMCommand_isStreamStartLong(command))
            {
                if (hasStreamStart)
                {
                    if (!silent)
                    {
                        printf("Warning: more than 1 PCM command in a single frame !\n");
                        printf("Command stream start removed at %g\n", (double) VGM_getTime(vgm, command) / 44100);
                    }

                    // remove the command
                    removeFromLList(com);
                }

                hasStreamStart = true;
            }
            else if (VGMCommand_isStreamFrequency(command))
            {
                if (hasStreamRate)
                {
                    if (!silent)
                        printf("Command stream rate removed at %g\n", (double) VGM_getTime(vgm, command) / 44100);

                    // remove the command
                    removeFromLList(com);
                }

                hasStreamRate = true;
            }

            com = com->prev;
        }

        // get back to head of list
        keyOnOffCommands = getHeadLList(keyOnOffCommands);
        lastCommands = getHeadLList(lastCommands);

        // add frame commands for delta YM
        optimizedCommands = insertAllAfterLList(optimizedCommands, YM2612_getDelta(ymOldState, ymState));
        // add frame commands for key on/off
        optimizedCommands = insertAllAfterLList(optimizedCommands, keyOnOffCommands);
        // add frame commands for delta PSG
        optimizedCommands = insertAllAfterLList(optimizedCommands, PSG_getDelta(psgOldState, psgState));
        // add frame const commands
        optimizedCommands = insertAllAfterLList(optimizedCommands, lastCommands);

        // get back to head
        optimizedCommands = getHeadLList(optimizedCommands);

        // add frame optimized set to new commands
        newCommands = insertAllAfterLList(newCommands, optimizedCommands);

        // update states
        ymOldState = ymState;
        psgOldState = psgState;
        startCom = endCom;
    }
    while ((endCom != NULL) && !VGMCommand_isEnd(command));

    newCommands = insertAfterLList(newCommands, VGMCommand_create(VGM_END));

    vgm->commands = getHeadLList(newCommands);

    if (verbose)
        printf("Music data size: %d\n", VGM_getMusicDataSize(vgm));
    if (!silent)
    {
        printf("Computed VGM duration: %d samples (%d seconds)\n", VGM_computeLen(vgm), VGM_computeLen(vgm) / 44100);
        printf("Number of command after commands clean: %d\n", getSizeLList(vgm->commands));
    }
}

void VGM_cleanSamples(VGM* vgm)
{
    LList* b;
    LList* s;
    LList* c;

    b = getTailLList(vgm->sampleBanks);
    while(b != NULL)
    {
        SampleBank* bank = b->element;
        int bankId = bank->id;

        s = getTailLList(bank->samples);
        while(s != NULL)
        {
            Sample* sample = s->element;
            int sampleId = sample->id;
            int sampleAddress = sample->dataOffset;
            int minLen = max(0, sample->len - 50);
            int maxLen = sample->len + 50;
            bool used = false;
            int currentBankId = -1;

            c = vgm->commands;
            while(c != NULL)
            {
                VGMCommand* command = c->element;

                if (VGMCommand_isStreamData(command))
                    currentBankId = VGMCommand_getStreamBankId(command);

                if (bankId == currentBankId)
                {
                    if (VGMCommand_isStreamStart(command))
                    {
                        if (sampleId == VGMCommand_getStreamBlockId(command))
                        {
                            used = true;
                            break;
                        }
                    }
                    else if (VGMCommand_isStreamStartLong(command))
                    {
                        int sampleLen = VGMCommand_getStreamSampleSize(command);

                        if ((sampleAddress == VGMCommand_getStreamSampleAddress(command)) && (sampleLen >= minLen) && (sampleLen <= maxLen))
                        {
                            used = true;
                            break;
                        }
                    }
                }

                c = c->next;
            }

            // sample not used --> remove it
            if (!used)
            {
                if (verbose)
                    printf("Sample at offset %6X (len = %d) is not used --> removed\n", sampleAddress, sample->len);

                // remove sample
                removeFromLList(s);
                // special case where we removed first sample
                if (s == bank->samples)
                    bank->samples = s->next;
            }

            s = s->prev;
        }

        b = b->prev;
    }
}

//Sample* VGM_getSample(VGM* vgm, int sampleOffset, int len)
//{
//    LList* b;
//
//    b = vgm->sampleBanks;
//    while(b != NULL)
//    {
//        SampleBank* bank = b->element;
//        Sample* sample = SampleBank_getSampleByOffsetAndLen(bank, sampleOffset, len);
//
//        if (sample != NULL)
//            return sample;
//
//        b = b->next;
//    }
//
//    return NULL;
//}

Sample* VGM_getSample(VGM* vgm, int sampleOffset)
{
    LList* b;

    b = vgm->sampleBanks;
    while(b != NULL)
    {
        SampleBank* bank = b->element;
        Sample* sample = SampleBank_getSampleByOffset(bank, sampleOffset);

        if (sample != NULL)
            return sample;

        b = b->next;
    }

    return NULL;
}

void VGM_convertWaits(VGM* vgm)
{
    LList* newCommands = NULL;
    // number of sample per frame
    const double limit = (double) 44100 / (double) vgm->rate;
    // -15%
    const double minLimit = limit - ((limit * 15) / 100);
    const int comWait = (vgm->rate == 60) ? VGM_WAIT_NTSC_FRAME : VGM_WAIT_PAL_FRAME;

    double sampleCnt = 0;
    LList* c = vgm->commands;
    while(c != NULL)
    {
        VGMCommand* command = c->element;

        // add no wait command
        if (!VGMCommand_isWait(command))
            newCommands = insertAfterLList(newCommands, command);
        else
            sampleCnt += VGMCommand_getWaitValue(command);

        while (sampleCnt > minLimit)
        {
            newCommands = insertAfterLList(newCommands, VGMCommand_create(comWait));
            sampleCnt -= limit;
        }

        c = c->next;
    }

    // set new commands
    vgm->commands = getHeadLList(newCommands);

    if (!silent)
    {
        printf("VGM duration after wait command conversion: %d samples (%d seconds)\n", VGM_computeLen(vgm), VGM_computeLen(vgm) / 44100);
        printf("Number of command: %d\n", getSizeLList(vgm->commands));
    }
}

//void VGM_shiftSamples(VGM* vgm, int sft)
//{
//    int i;
//
//    if (sft == 0)
//        return;
//
//    LList* sampleCommands[sft];
//
//    for (i = 0; i < sft; i++)
//        sampleCommands[i] = NULL;
//
//    int frameRead = 0;
//    int frameWrite = 1;
//    LList* c = getTailLList(vgm->commands);
//    while (c != NULL)
//    {
//        VGMCommand* command = c->element;
//
//        if (VGMCommand_isStream(command))
//        {
//            sampleCommands[frameRead] = insertAfterLList(sampleCommands[frameRead], command);
//            removeFromLList(c);
//        }
//        else if (VGMCommand_isWait(command) || VGMCommand_isEnd(command))
//        {
//            frameRead = (frameRead + 1) % sft;
//            frameWrite = (frameWrite + 1) % sft;
//
//            // add sample command to this frame
//            while (sampleCommands[frameWrite] != NULL)
//            {
//                // insert and by-pass
//                c = insertBeforeLList(c, sampleCommands[frameWrite]->element);
//                sampleCommands[frameWrite] = removeFromLList(sampleCommands[frameWrite]);
//            }
//        }
//
//        c = c->prev;
//    }
//
//    // add last remaining samples
//    for (i = 0; i < sft; i++)
//    {
//        while (sampleCommands[i] != NULL)
//        {
//            // insert and update list head
//            vgm->commands = insertBeforeLList(vgm->commands, sampleCommands[i]->element);
//            sampleCommands[i] = removeFromLList(sampleCommands[frameWrite]);
//        }
//    }
//}

static int VGM_getSampleDataSize(VGM* vgm)
{
    LList* b;
    int result = 0;

    b = vgm->sampleBanks;
    while(b != NULL)
    {
        SampleBank* bank = b->element;
        result += bank->len;
        b = b->next;
    }

    return result;
}

static int VGM_getSampleTotalLen(VGM* vgm)
{
    LList* b;
    int result = 0;

    b = vgm->sampleBanks;
    while(b != NULL)
    {
        SampleBank* bank = b->element;
        LList* s = bank->samples;

        while(s != NULL)
        {
            Sample* sample = s->element;
            result += sample->len;
            s = s->next;
        }

        b = b->next;
    }

    return result;
}

static int VGM_getSampleNumber(VGM* vgm)
{
    LList* b;
    int result = 0;

    b = vgm->sampleBanks;
    while(b != NULL)
    {
        SampleBank* bank = b->element;
        result += getSizeLList(bank->samples);
        b = b->next;
    }

    return result;
}

static int VGM_getMusicDataSize(VGM* vgm)
{
    LList* c;
    int result = 0;

    c = vgm->commands;
    while(c != NULL)
    {
        VGMCommand* command = c->element;

        if (!VGMCommand_isDataBlock(command))
            result += command->size;

        c = c->next;
    }

    return result;
}

unsigned char* VGM_asByteArray(VGM* vgm, int* outSize)
{
    int i;
    unsigned char byte;
    FILE* f = fopen("tmp.bin", "wb+");

    if (f == NULL)
    {
        printf("Error: cannot open file tmp.bin\n");
        return NULL;
    }

    // 00: VGM
    fwrite("Vgm ", 1, 4, f);

    // 04: len (reserve 4 bytes)
    byte = 0x00;
    fwrite(&byte, 1, 1, f);
    fwrite(&byte, 1, 1, f);
    fwrite(&byte, 1, 1, f);
    fwrite(&byte, 1, 1, f);
    // 08: version 1.60
    byte = 0x60;
    fwrite(&byte, 1, 1, f);
    byte = 0x01;
    fwrite(&byte, 1, 1, f);
    byte = 0x00;
    fwrite(&byte, 1, 1, f);
    byte = 0x00;
    fwrite(&byte, 1, 1, f);
    // 0C: SN76489 clock
    byte = 0x99;
    fwrite(&byte, 1, 1, f);
    byte = 0x9E;
    fwrite(&byte, 1, 1, f);
    byte = 0x36;
    fwrite(&byte, 1, 1, f);
    byte = 0x00;
    fwrite(&byte, 1, 1, f);
    // 10: YM2413 clock
    byte = 0x00;
    fwrite(&byte, 1, 1, f);
    fwrite(&byte, 1, 1, f);
    fwrite(&byte, 1, 1, f);
    fwrite(&byte, 1, 1, f);
    // 14: GD3 offset
    byte = 0x00;
    fwrite(&byte, 1, 1, f);
    fwrite(&byte, 1, 1, f);
    fwrite(&byte, 1, 1, f);
    fwrite(&byte, 1, 1, f);
    // 18: total number of sample (44100 samples per second)
    byte = 0x00;
    fwrite(&byte, 1, 1, f);
    fwrite(&byte, 1, 1, f);
    fwrite(&byte, 1, 1, f);
    fwrite(&byte, 1, 1, f);
    // 1C: loop offset
    byte = 0x00;
    fwrite(&byte, 1, 1, f);
    fwrite(&byte, 1, 1, f);
    fwrite(&byte, 1, 1, f);
    fwrite(&byte, 1, 1, f);
    // 20: loop number of samples (44100 samples per second)
    byte = 0x00;
    fwrite(&byte, 1, 1, f);
    fwrite(&byte, 1, 1, f);
    fwrite(&byte, 1, 1, f);
    fwrite(&byte, 1, 1, f);
    // 24: rate (50 or 60 Hz)
    byte = vgm->rate;
    fwrite(&byte, 1, 1, f);
    byte = 0x00;
    fwrite(&byte, 1, 1, f);
    fwrite(&byte, 1, 1, f);
    fwrite(&byte, 1, 1, f);
    // 28: SN76489 flags
    byte = 0x09;
    fwrite(&byte, 1, 1, f);
    byte = 0x00;
    fwrite(&byte, 1, 1, f);
    byte = 0x10;
    fwrite(&byte, 1, 1, f);
    byte = 0x00;
    fwrite(&byte, 1, 1, f);
    // 2C: YM2612 clock
    byte = 0xB5;
    fwrite(&byte, 1, 1, f);
    byte = 0x0A;
    fwrite(&byte, 1, 1, f);
    byte = 0x75;
    fwrite(&byte, 1, 1, f);
    byte = 0x00;
    fwrite(&byte, 1, 1, f);
    // 30: YM2151 clock
    byte = 0x00;
    fwrite(&byte, 1, 1, f);
    fwrite(&byte, 1, 1, f);
    fwrite(&byte, 1, 1, f);
    fwrite(&byte, 1, 1, f);
    // 34: VGM data offset
    byte = 0x4C;
    fwrite(&byte, 1, 1, f);
    byte = 0x00;
    fwrite(&byte, 1, 1, f);
    fwrite(&byte, 1, 1, f);
    fwrite(&byte, 1, 1, f);
    // 38: Sega PCM clock
    byte = 0x00;
    fwrite(&byte, 1, 1, f);
    fwrite(&byte, 1, 1, f);
    fwrite(&byte, 1, 1, f);
    fwrite(&byte, 1, 1, f);
    // 3C: Sega PCM interface
    byte = 0x00;
    fwrite(&byte, 1, 1, f);
    fwrite(&byte, 1, 1, f);
    fwrite(&byte, 1, 1, f);
    fwrite(&byte, 1, 1, f);
    // 40-80
    byte = 0x00;
    for (i = 0x40; i < 0x80; i++)
        fwrite(&byte, 1, 1, f);

    VGMCommand* loopCommand = NULL;
    int loopOffset = 0;
    LList* l;

    // write command (ignore loop marker)
    l = vgm->commands;
    while(l != NULL)
    {
        VGMCommand* command = l->element;

        if (!VGMCommand_isLoop(command))
            fwrite(VGMCommand_asByteArray(command), 1, command->size, f);
        else
        {
            loopCommand = command;
            loopOffset = getFileSizeEx(f) - 0x1C;
        }

        l = l->next;
    }

    unsigned char* array = inEx(f, 0, getFileSizeEx(f), outSize);

    fclose(f);

    // set loop offset
    if (loopCommand != NULL)
    {
        setInt(array, 0x1C, loopOffset);
        setInt(array, 0x20, VGM_computeLenEx(vgm, loopCommand));
    }
    // set file size
    setInt(array, 0x04, *outSize - 4);
    // set len in sample
    setInt(array, 0x18, VGM_computeLen(vgm) - 1);

    return array;
}
