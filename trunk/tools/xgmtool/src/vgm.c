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

// forward
static void VGM_parse(VGM* vgm);
static void VGM_buildSamples(VGM* vgm, bool convert);
static int VGM_extractSampleFromSeek(VGM* vgm, int index, bool convert);
static SampleBank* VGM_getDataBank(VGM* vgm, int id);
static SampleBank* VGM_addDataBlock(VGM* vgm, VGMCommand* command);
static void VGM_cleanSeekCommands(VGM* vgm);
static void VGM_removePlayPCMCommands(VGM* vgm);
static int VGM_getSampleDataSize(VGM* vgm);
static int VGM_getSampleTotalLen(VGM* vgm);
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

    result->sampleBanks = createList();
    result->commands = createList();

    // build command list
    VGM_parse(result);

    if (!silent)
        printf("Computed VGM duration: %d samples (%d seconds)\n", VGM_computeLen(result), VGM_computeLen(result) / 44100);

    // and build samples
    VGM_buildSamples(result, convert);

    // rebuild data blocks
    if (convert)
    {
        int i, ind;

        // remove previous data blocks
        for (i = result->commands->size - 1; i >= 0; i--)
        {
            VGMCommand* command = getFromList(result->commands, i);

            if (VGMCommand_isDataBlock(command) || VGMCommand_isStreamControl(command) || VGMCommand_isStreamData(command))
                removeFromList(result->commands, i);
        }

        ind = 0;
        // add data block for each sample
        for (i = 0; i < result->sampleBanks->size; i++)
        {
            SampleBank* bank = getFromList(result->sampleBanks, i);

            addToListEx(result->commands, ind++, SampleBank_getDataBlockCommand(bank));
            addAllToListEx(result->commands, ind, SampleBank_getDeclarationCommands(bank));
        }
    }

    if (verbose)
    {
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
    unsigned char* data;
    int loopOffset, i;

    result = malloc(sizeof(VGM));

    result->data = NULL;
    result->dataSize = 0;
    result->offset = 0;

    result->sampleBanks = createList();
    result->commands = createList();

    result->version = 0x60;
    result->offsetStart = 0;
    result->offsetEnd = 0;
    result->lenInSample = 0;
    result->loopStart = 0;
    result->loopLenInSample = 0;

    loopOffset = -1;
    for (i = 0; i < xgm->commands->size; i++)
    {
        XGMCommand* command = getFromList(xgm->commands, i);
        int j, comsize;

        switch (XGMCommand_getType(command))
        {
            case XGM_FRAME:
                data = malloc(1);
                data[0] = 0x62;
                addToList(result->commands, VGMCommand_createEx(data, 0));
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
                    addToList(result->commands, VGMCommand_createEx(data, 0));
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
                    addToList(result->commands, VGMCommand_createEx(data, 0));
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
                    addToList(result->commands, VGMCommand_createEx(data, 0));
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
                    addToList(result->commands, VGMCommand_createEx(data, 0));
                }
                break;
        }
    }

    data = malloc(1);
    data[0] = 0x66;
    addToList(result->commands, VGMCommand_createEx(data, 0));

    // we had a loop command ?
    if (loopOffset != -1)
    {
        // find corresponding XGM command
        XGMCommand* command = XGM_getCommandAtOffset(xgm, loopOffset);

        // insert a VGM loop command at corresponding position
        if (command != NULL)
            addToListEx(result->commands, VGM_getCommandIndexAtTime(result, XGM_getTime(xgm, command)), VGMCommand_create(VGM_LOOP));
    }

    return result;
}

int VGM_computeLenEx(VGM* vgm, VGMCommand* from)
{
    int i;
    int result = 0;
    bool count = (from == NULL);

    for (i = 0; i < vgm->commands->size; i++)
    {
        VGMCommand* command = getFromList(vgm->commands, i);

        if (command == from)
            count = true;
        if (count)
            result += VGMCommand_getWaitValue(command);
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
    int i;
    int result = 0;

    for (i = 0; i < vgm->commands->size; i++)
    {
        VGMCommand* c = getFromList(vgm->commands, i);

        if (c == command)
            return result;
        result += c->size;
    }

    return -1;
}

/**
 * Return elapsed time when specified command happen
 */
int VGM_getTime(VGM* vgm, VGMCommand* command)
{
    int i;
    int result = 0;

    for (i = 0; i < vgm->commands->size; i++)
    {
        VGMCommand* c = getFromList(vgm->commands, i);

        if (c == command)
            return result;
        result += VGMCommand_getWaitValue(c);
    }

    return 0;
}

/**
 * Return elapsed time when specified command happen
 */
int VGM_getCommandIndexAtTime(VGM* vgm, int time)
{
    int c;
    int result = 0;

    for (c = 0; c < vgm->commands->size; c++)
    {
        VGMCommand* command = getFromList(vgm->commands, c);

        if (result >= time)
            return c;

        result += VGMCommand_getWaitValue(command);
    }

    return vgm->commands->size - 1;
}

/**
 * Return elapsed time when specified command happen
 */
VGMCommand* VGM_getCommandAtTime(VGM* vgm, int time)
{
    return getFromList(vgm->commands, VGM_getCommandIndexAtTime(vgm, time));
}

static void VGM_parse(VGM* vgm)
{
    int off;

    // parse all VGM commands
    off = vgm->offsetStart;
    while (off < vgm->offsetEnd)
    {
        // check for loop start
        if ((vgm->loopStart != 0) && (off == vgm->loopStart))
            addToList(vgm->commands, VGMCommand_create(VGM_LOOP));

        VGMCommand* command = VGMCommand_createEx(vgm->data, off);
        addToList(vgm->commands, command);
        off += command->size;

        // stop here
        if (VGMCommand_isEnd(command))
            break;
    }

    if (!silent)
        printf("Number of command: %d\n", vgm->commands->size);
}

static void VGM_buildSamples(VGM* vgm, bool convert)
{
    int i, ind;

    // builds data blocks
    for (i = 0; i < vgm->commands->size; i++)
    {
        VGMCommand* command = getFromList(vgm->commands, i);

        if (VGMCommand_isDataBlock(command))
            VGM_addDataBlock(vgm, command);
    }

    // clean seek
    VGM_cleanSeekCommands(vgm);

    // extract samples from seek command
    ind = 0;
    while (ind < vgm->commands->size)
    {
        VGMCommand* command = getFromList(vgm->commands, ind);

        if (VGMCommand_isSeek(command))
            ind = VGM_extractSampleFromSeek(vgm, ind, convert);
        else
            ind++;
    }

    int sampleIdBanks[0x100];
    int sampleIdFrequencies[0x100];

    // set bank id and frequency to -1 by default
    for (i = 0; i < 0x100; i++)
    {
        sampleIdBanks[i] = -1;
        sampleIdFrequencies[i] = 0;
    }

    // adjust samples infos from stream command
    ind = 0;
    while (ind < vgm->commands->size)
    {
        VGMCommand* command = getFromList(vgm->commands, ind);

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
                else
                    printf("Warning: sample id %2X not found !\n", sampleId);

                // convert to long command as we use single data block
                if (convert)
                    setToList(vgm->commands, ind, Sample_getStartLongCommandEx(bank, sample, sample->len));
            }
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

        ind++;
    }

    if (convert)
        VGM_removePlayPCMCommands(vgm);
}

static int VGM_extractSampleFromSeek(VGM* vgm, int index, bool convert)
{
    int seekIndex = index;
    VGMCommand* command = getFromList(vgm->commands, seekIndex);
    // get sample address in data bank
    int sampleAddr = VGMCommand_getSeekAddress(command);

    int ind;
    int len;
    int wait;
    int delta;
    int endPlayWait;
    int startPlayInd;
    int endPlayInd;

    // then find seek command to extract sample
    len = 0;
    wait = -1;
    delta = 0;
    endPlayWait = 0;
    startPlayInd = -1;
    endPlayInd = -1;
    ind = seekIndex + 1;
    while (ind < vgm->commands->size)
    {
        VGMCommand* command = getFromList(vgm->commands, ind);

        // sample done !
        if (VGMCommand_isSeek(command) || VGMCommand_isDataBlock(command) || VGMCommand_isEnd(command))
            break;

        // playing ?
        if (wait != -1)
        {
            delta = wait - endPlayWait;

            // delta > 200 samples --> sample ended
            if (delta > 200)
            {
                // found a sample --> add it
                if ((len > 0) && (endPlayWait > 0) && (startPlayInd != endPlayInd))
                {
                    if (vgm->sampleBanks->size > 0)
                    {
                        // get last bank
                        SampleBank* bank = getFromList(vgm->sampleBanks, vgm->sampleBanks->size - 1);
                        int rate = (int) round(((double) 44100 * len) / (double) endPlayWait);
                        Sample* sample = SampleBank_addSample(bank, sampleAddr, len, rate);

                        if (convert)
                        {
                            // insert stream play command
                            addToListEx(vgm->commands, startPlayInd + 0, Sample_getSetRateCommand(bank, sample, sample->rate));
                            addToListEx(vgm->commands, startPlayInd + 1, Sample_getStartLongCommandEx(bank, sample, len));

                            // remove seek command
                            if (seekIndex != -1)
                            {
                                removeFromList(vgm->commands, seekIndex);
                                seekIndex = -1;
                                // adjust index
                                ind += 1;
                            }
                            else
                                ind += 2;

                            // sample stopped before end ?
                            if (sample->len != len)
                            {
                                // insert stream stop command
                                addToListEx(vgm->commands, ind, Sample_getStopCommand(bank, sample));
                                ind++;
                            }
                        }
                    }
                }

                // reset
                sampleAddr += len;
                len = 0;
                wait = -1;
                delta = 0;
                endPlayWait = 0;
                startPlayInd = -1;
                endPlayInd = -1;
            }
        }

        // compute sample len
        if (VGMCommand_isPCM(command))
        {
            // start play --> init wait
            if (wait == -1)
            {
                wait = 0;
                startPlayInd = ind;
            }

            // need a minimal length before applying correction
            if ((len > 100) && (wait > 200))
            {
                int mean = wait / len;

                // correct abnormal delta
                if (delta < (mean - 2))
                    wait += mean - delta;
                else if (delta > (mean + 2))
                    wait -= delta - mean;
            }

            // keep trace of last play wait value
            endPlayWait = wait;
            endPlayInd = ind;

            wait += VGMCommand_getWaitValue(command);
            len++;
        }
        // playing ?
        else if (wait != -1)
            wait += VGMCommand_getWaitValue(command);

        ind++;
    }

    // found a sample --> add it
    if ((len > 0) && (endPlayWait > 0) && (startPlayInd != endPlayInd))
    {
        if (vgm->sampleBanks->size > 0)
        {
            // get last bank
            SampleBank* bank = getFromList(vgm->sampleBanks, vgm->sampleBanks->size - 1);
            int rate = (int) round(((double) 44100 * len) / (double) endPlayWait);
            Sample* sample = SampleBank_addSample(bank, sampleAddr, len, rate);

            if (convert)
            {
                // insert stream play command
                addToListEx(vgm->commands, startPlayInd + 0, Sample_getSetRateCommand(bank, sample, sample->rate));
                addToListEx(vgm->commands, startPlayInd + 1, Sample_getStartLongCommandEx(bank, sample, len));

                // remove seek command
                if (seekIndex != -1)
                {
                    removeFromList(vgm->commands, seekIndex);
                    seekIndex = -1;
                    // adjust index
                    ind += 1;
                }
                else
                    ind += 2;

                // insert stream stop command
                addToListEx(vgm->commands, ind, Sample_getStopCommand(bank, sample));
                ind++;
            }
        }
    }

    return ind;
}

static SampleBank* VGM_getDataBank(VGM* vgm, int id)
{
    int i;

    for (i = 0; i < vgm->sampleBanks->size; i++)
    {
        SampleBank* bank = getFromList(vgm->sampleBanks, i);
        if (bank->id == id)
            return bank;
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
        if (verbose)
            printf("Add data bank %6X:%2X\n", command->offset, VGMCommand_getDataBankId(command));

        result = SampleBank_create(command);
        addToList(vgm->sampleBanks, result);
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
    int ind;
    bool samplePlayed;

    samplePlayed = false;
    for (ind = vgm->commands->size - 1; ind >= 0; ind--)
    {
        VGMCommand* command = getFromList(vgm->commands, ind);

        // seek command ?
        if (VGMCommand_isSeek(command))
        {
            // no sample played after this seek command --> remove it
            if (!samplePlayed)
            {
                if (!silent)
                    printf("Useless seek command found at %6X", command->offset);

                removeFromList(vgm->commands, ind);
            }

            samplePlayed = false;
        }
        else if (VGMCommand_isPCM(command))
            samplePlayed = true;
    }
}

static void VGM_removePlayPCMCommands(VGM* vgm)
{
    int ind;

    for (ind = vgm->commands->size - 1; ind >= 0; ind--)
    {
        VGMCommand* command = getFromList(vgm->commands, ind);

        // replace PCM command by simple wait command
        if (VGMCommand_isPCM(command))
        {
            const int wait = VGMCommand_getWaitValue(command);

            // remove or just replace by wait command
            if (wait == 0)
                removeFromList(vgm->commands, ind);
            else
                setToList(vgm->commands, ind, VGMCommand_create(0x70 + (wait - 1)));
        }
    }

    if (!silent)
        printf("Number of command after PCM command remove: %d\n", vgm->commands->size);
    if (verbose)
        printf("Computed VGM duration: %d samples (%d seconds)\n", VGM_computeLen(vgm), VGM_computeLen(vgm) / 44100);
}

void VGM_cleanCommands(VGM* vgm)
{
    List* newCommands = createList();
    List* normalCommands = createList();
    List* optimizedCommands = createList();
    List* keyOnOffCommands = createList();
    List* lastCommands = createList();

    YM2612* ymOldState;
    YM2612* ymState;
    PSG* psgOldState;
    PSG* psgState;

    ymOldState = YM2612_create();
    psgOldState = PSG_create();

    VGMCommand* command;
    int startInd;
    int endInd;

    startInd = 0;
    do
    {
        int ind;

        endInd = startInd;

        do
        {
            command = getFromList(vgm->commands, endInd);
            endInd++;
        }
        while ((endInd < vgm->commands->size) && !VGMCommand_isWait(command) && !VGMCommand_isEnd(command));

        psgState = PSG_copy(psgOldState);
        ymState = YM2612_copy(ymOldState);

        // clear frame sets
        clearList(normalCommands);
        clearList(optimizedCommands);
        clearList(keyOnOffCommands);
        clearList(lastCommands);

        // startInd --> endInd contains commands for a single frame
        for (ind = startInd; ind < endInd; ind++)
        {
            command = getFromList(vgm->commands, ind);

            // keep trace of originals commands
            if (!VGMCommand_isYM2612TimersWrite(command))
                addToList(normalCommands, command);

            // keep data block and stream commands
            if (VGMCommand_isDataBlock(command) || VGMCommand_isStream(command) || VGMCommand_isLoop(command))
                addToList(optimizedCommands, command);
            else if (VGMCommand_isPSGWrite(command))
                PSG_write(psgState, VGMCommand_getPSGValue(command));
            else if (VGMCommand_isYM2612Write(command))
            {
                // write to YM state and store key on/off command
                if (YM2612_set(ymState, VGMCommand_getYM2612Port(command), VGMCommand_getYM2612Register(command), VGMCommand_getYM2612Value(command)))
                    addToList(keyOnOffCommands, command);
            }
            else if (VGMCommand_isWait(command) || VGMCommand_isSeek(command))
                addToList(lastCommands, command);
            else
            {
                if (verbose)
                    printf("Command ignored: %d\n", command->command);
            }
        }

        bool hasStreamStart = false;
        bool hasStreamRate = false;
        // check we have single stream per frame
        for (ind = optimizedCommands->size - 1; ind >= 0; ind--)
        {
            command = getFromList(optimizedCommands, ind);

            if (VGMCommand_isStreamStartLong(command))
            {
                if (hasStreamStart)
                {
                    if (!silent)
                    {
                        printf("Warning: more than 1 PCM command in a single frame !\n");
                        printf("Command stream start removed at %g\n", (double) VGM_getTime(vgm, command) / 44100);
                    }

                    removeFromList(optimizedCommands, ind);
                }

                hasStreamStart = true;
            }
            else if (VGMCommand_isStreamFrequency(command))
            {
                if (hasStreamRate)
                {
                    if (!silent)
                        printf("Command stream rate removed at %g\n", (double) VGM_getTime(vgm, command) / 44100);

                    removeFromList(optimizedCommands, ind);
                }

                hasStreamRate = true;
            }
        }

        // add frame commands for delta YM
        addAllToList(optimizedCommands, YM2612_getDelta(ymOldState, ymState));
        // add frame commands for key on/off
        addAllToList(optimizedCommands, keyOnOffCommands);
        // add frame commands for delta PSG
        addAllToList(optimizedCommands, PSG_getDelta(psgOldState, psgState));
        // add frame const commands
        addAllToList(optimizedCommands, lastCommands);

        // add frame optimized set to new commands
        addAllToList(newCommands, optimizedCommands);

        // update states
        ymOldState = ymState;
        psgOldState = psgState;
        startInd = endInd;
    }
    while ((endInd < vgm->commands->size) && !VGMCommand_isEnd(command));

    addToList(newCommands, VGMCommand_create(VGM_END));

    vgm->commands = newCommands;

    if (verbose)
        printf("Music data size: %d\n", VGM_getMusicDataSize(vgm));
    if (!silent)
    {
        printf("Computed VGM duration: %d samples (%d seconds)\n", VGM_computeLen(vgm), VGM_computeLen(vgm) / 44100);
        printf("Number of command after commands clean: %d\n", vgm->commands->size);
    }
}

void VGM_cleanSamples(VGM* vgm)
{
    int b, s, c;

    for (b = vgm->sampleBanks->size - 1; b >= 0; b--)
    {
        SampleBank* bank = getFromList(vgm->sampleBanks, b);
        int bankId = bank->id;

        for (s = bank->samples->size - 1; s >= 0; s--)
        {
            Sample* sample = getFromList(bank->samples, s);
            int sampleId = sample->id;
            int sampleAddress = sample->dataOffset;
            bool used = false;
            int currentBankId = -1;

            for (c = 0; c < vgm->commands->size - 1; c++)
            {
                VGMCommand* command = getFromList(vgm->commands, c);

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
                        if (sampleAddress == VGMCommand_getStreamSampleAddress(command))
                        {
                            used = true;
                            break;
                        }
                    }
                }
            }

            // sample not used --> remove it
            if (!used)
            {
                if (verbose)
                    printf("Sample at offset %6X is not used --> removed\n", sampleAddress);

                removeFromList(bank->samples, s);
            }
        }
    }
}

Sample* VGM_getSample(VGM* vgm, int sampleOffset)
{
    int i;

    for (i = 0; i < vgm->sampleBanks->size; i++)
    {
        SampleBank* bank = getFromList(vgm->sampleBanks, i);
        Sample* sample = SampleBank_getSampleByOffset(bank, sampleOffset);

        if (sample != NULL)
            return sample;
    }

    return NULL;
}

void VGM_convertWaits(VGM* vgm)
{
    List* newCommands = createList();
    // number of sample per frame
    const double limit = (double) 44100 / (double) vgm->rate;
    // -15%
    const double minLimit = limit - ((limit * 15) / 100);
    const int comWait = (vgm->rate == 60) ? VGM_WAIT_NTSC_FRAME : VGM_WAIT_PAL_FRAME;
    int i;

    double sampleCnt = 0;
    for (i = 0; i < vgm->commands->size; i++)
    {
        VGMCommand* command = getFromList(vgm->commands, i);

        // add no wait command
        if (!VGMCommand_isWait(command))
            addToList(newCommands, command);
        else
            sampleCnt += VGMCommand_getWaitValue(command);

        while (sampleCnt > minLimit)
        {
            addToList(newCommands, VGMCommand_create(comWait));
            sampleCnt -= limit;
        }
    }

    // set new commands
    vgm->commands = newCommands;

    if (!silent)
    {
        printf("VGM duration after wait command conversion: %d samples (%d seconds)\n", VGM_computeLen(vgm), VGM_computeLen(vgm) / 44100);
        printf("Number of command: %d\n", vgm->commands->size);
    }
}

void VGM_shiftSamples(VGM* vgm, int sft)
{
    int i;

    if (sft == 0)
        return;

    List* sampleCommands[sft];

    for (i = 0; i < sft; i++)
        sampleCommands[i] = createList();

    int frameRead = 0;
    int frameWrite = 1;
    int index = vgm->commands->size - 1;
    while (index >= 0)
    {
        VGMCommand* command = getFromList(vgm->commands, index);

        if (VGMCommand_isStream(command))
        {
            addToList(sampleCommands[frameRead], command);
            removeFromList(vgm->commands, index);
        }
        else if (VGMCommand_isWait(command) || VGMCommand_isEnd(command))
        {
            frameRead = (frameRead + 1) % sft;
            frameWrite = (frameWrite + 1) % sft;

            // add sample command to this frame
            while (sampleCommands[frameWrite]->size > 0)
                addToListEx(vgm->commands, index, removeFromList(sampleCommands[frameWrite], 0));
        }

        index--;
    }

    // add last remaining samples
    for (i = 0; i < sft; i++)
        while (sampleCommands[i]-> size > 0)
            addToListEx(vgm->commands, 0, removeFromList(sampleCommands[i], 0));
}

static int VGM_getSampleDataSize(VGM* vgm)
{
    int i;
    int result = 0;

    for (i = 0; i < vgm->sampleBanks->size; i++)
    {
        SampleBank* bank = getFromList(vgm->sampleBanks, i);
        result += bank->len;
    }

    return result;
}

static int VGM_getSampleTotalLen(VGM* vgm)
{
    int i, j;
    int result = 0;

    for (i = 0; i < vgm->sampleBanks->size; i++)
    {
        SampleBank* bank = getFromList(vgm->sampleBanks, i);

        for (j = 0; j < bank->samples->size; j++)
        {
            Sample* sample = getFromList(bank->samples, j);
            result += sample->len;
        }
    }

    return result;
}

static int VGM_getMusicDataSize(VGM* vgm)
{
    int i;
    int result = 0;

    for (i = 0; i < vgm->commands->size; i++)
    {
        VGMCommand* command = getFromList(vgm->commands, i);

        if (!VGMCommand_isDataBlock(command))
            result += command->size;
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

    // write command (ignore loop marker)
    for (i = 0; i < vgm->commands->size; i++)
    {
        VGMCommand* command = getFromList(vgm->commands, i);

        if (!VGMCommand_isLoop(command))
            fwrite(VGMCommand_asByteArray(command), 1, command->size, f);
        else
        {
            loopCommand = command;
            loopOffset = getFileSizeEx(f) - 0x1C;
        }
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
