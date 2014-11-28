#include <stdlib.h>
#include <stdbool.h>
#include <memory.h>

#include "../inc/xgm.h"
#include "../inc/vgm.h"
#include "../inc/xgmtool.h"


// forward
static void XGM_parseMusic(XGM* xgm, unsigned char* data, int length);
static void XGM_extractSamples(XGM* xgm, VGM* vgm);
static void XGM_extractMusic(XGM* xgm, VGM* vgm);


XGM* XGM_create()
{
    XGM* result;

    result = malloc(sizeof(XGM));

    result->samples = createList();
    result->commands = createList();
    result->pal = -1;

    return result;
}

XGM* XGM_createFromData(unsigned char* data, int dataSize)
{
    int s;
    XGM* result = XGM_create();

    if (!silent)
        printf("Parsing XGM file...\n");

    if (strncasecmp(&data[0x00], "XGM ", 4))
    {
        printf("Error: XGM file not recognized !\n");
        return NULL;
    }

    // sample id table
    for (s = 1; s < 0x40; s++)
    {
        int offset = getInt16(data, (s * 4) + 0);
        int len = getInt16(data, (s * 4) + 2);

        // ignore empty sample
        if ((offset != 0xFFFF) && (len != 0x0100))
        {
            offset <<= 8;
            len <<= 8;

            // add sample
            addToList(result->samples, XGMSample_create(s, offset, data + (offset + 0x104), len));
        }
    }

    // calculate music data offset (sample block size + 0x104)
    int offset = (getInt16(data, 0x100) << 8) + 0x104;
    // int version = data[0x102];
    result->pal = data[0x103] & 1;

    // get music data length
    int len = getInt(data, offset);

    if (verbose)
    {
        printf("XGM sample number: %d\n", result->samples->size);
        printf("XGM start music data: %6X  len: %d\n", offset + 4, len);
    }

    // build command list
    XGM_parseMusic(result, data + offset + 4, len);

    if (!silent)
        printf("XGM duration: %d frames (%d seconds)\n", XGM_computeLenInFrame(result), XGM_computeLenInFrame(result) / 60);

    return result;
}

XGM* XGM_createFromVGM(VGM* vgm)
{
    XGM* result = XGM_create();

    if (!silent)
        printf("Converting VGM to XGM...\n");

    if (vgm->rate == 60)
        result->pal = 0;
    else if (vgm->rate == 50)
        result->pal = 1;
    else
        result->pal = -1;

    // extract samples from VGM
    XGM_extractSamples(result, vgm);
    // and extract music data
    XGM_extractMusic(result, vgm);

    if (verbose)
    {
        printf("XGM sample number: %d\n", result->samples->size);
        printf("Sample size: %d\n", XGM_getSampleDataSize(result));
        printf("Music data size: %d\n", XGM_getMusicDataSize(result));
    }
    if (!silent)
        printf("XGM duration: %d frames (%d seconds)\n", XGM_computeLenInFrame(result), XGM_computeLenInFrame(result) / 60);

    return result;
}


static void XGM_parseMusic(XGM* xgm, unsigned char* data, int length)
{
    // build command list
   int off;

    // parse all XGM commands
    off = 0;
    while (off < length)
    {
        // check for loop start
        XGMCommand* command = XGMCommand_createFromData(data + off);
        addToList(xgm->commands, command);
        off += command->size;

        // stop here
        if (XGMCommand_isEnd(command))
            break;
    }

    if (!silent)
        printf("Number of command: %d\n", xgm->commands->size);
}

static void XGM_extractSamples(XGM* xgm, VGM* vgm)
{
    int b, s;

    // extract samples
    for(b = 0; b < vgm->sampleBanks->size; b++)
    {
        SampleBank* sampleBank = getFromList(vgm->sampleBanks, b);

        for(s = 0; s < sampleBank->samples->size; s++)
            addToList(xgm->samples, XGMSample_createFromVGMSample(sampleBank, getFromList(sampleBank->samples, s)));
    }
}

static void XGM_extractMusic(XGM* xgm, VGM* vgm)
{
    List* frameCommands = createList();
    List* ymKeyOffCommands = createList();
    List* ymKeyOtherCommands = createList();
    List* ymPort0Commands = createList();
    List* ymPort1Commands = createList();
    List* psgCommands = createList();
    List* sampleCommands = createList();

    List* xgmCommands = createList();

    int com;
    int index = 0;
    int loopOffset = -1;
    bool lastCommWasKey;

    while (index < vgm->commands->size)
    {
        // get frame commands
        clearList(frameCommands);
        while (index < vgm->commands->size)
        {
            VGMCommand* command = getFromList(vgm->commands, index++);

            if (VGMCommand_isLoop(command))
            {
                if (loopOffset == -1)
                    loopOffset = XGM_getMusicDataSize(xgm);
                continue;
            }
            // ignore data block
            if (VGMCommand_isDataBlock(command))
                continue;
            // stop here
            if (VGMCommand_isWait(command))
            {
                // set PAL flag if not already set
                if (xgm->pal == -1)
                {
                    if (VGMCommand_isWaitPAL(command))
                        xgm->pal = 1;
                    else if (VGMCommand_isWaitNTSC(command))
                        xgm->pal = 0;
                }

                break;
            }

            if (VGMCommand_isEnd(command))
                break;

            // add command
            addToList(frameCommands, command);
        }

        // prepare new commands for this frame
        clearList(xgmCommands);

        // group commands
        clearList(ymKeyOffCommands);
        clearList(ymKeyOtherCommands);
        clearList(ymPort0Commands);
        clearList(ymPort1Commands);
        clearList(psgCommands);
        clearList(sampleCommands);
        lastCommWasKey = false;

        for (com = 0; com < frameCommands->size; com++)
        {
            VGMCommand* command = getFromList(frameCommands, com);

            if (VGMCommand_isStream(command))
                addToList(sampleCommands, command);
            else if (VGMCommand_isPSGWrite(command))
                addToList(psgCommands, command);
            else if (VGMCommand_isYM2612KeyWrite(command))
            {
                if (VGMCommand_isYM2612KeyOffWrite(command))
                {
                    if (!VGMCommand_contains(ymKeyOffCommands, command))
                        addToList(ymKeyOffCommands, command);
                }
                else
                {
                    VGMCommand* previousCommand = VGMCommand_getKeyCommand(ymKeyOtherCommands,
                            VGMCommand_getYM2612KeyChannel(command));

                    // change command with last one
                    if (previousCommand != NULL)
                        previousCommand->data[previousCommand->offset + 2] = command->data[command->offset + 2];
                    else
                        addToList(ymKeyOtherCommands, command);
                }

                lastCommWasKey = true;
            }
            else if (VGMCommand_isYM2612Write(command))
            {
                // need accurate key event so we transfer commands now
                if (lastCommWasKey)
                {
                    // general YM commands first as key event were just done
                    if (ymPort0Commands->size > 0)
                        addAllToList(xgmCommands, XGMCommand_createYMPort0Commands(ymPort0Commands));
                    if (ymPort1Commands->size > 0)
                        addAllToList(xgmCommands, XGMCommand_createYMPort1Commands(ymPort1Commands));
                    // then key off first
                    if (ymKeyOffCommands->size > 0)
                        addAllToList(xgmCommands, XGMCommand_createYMKeyCommands(ymKeyOffCommands));
                    // followed by key on commands
                    if (ymKeyOtherCommands->size > 0)
                        addAllToList(xgmCommands, XGMCommand_createYMKeyCommands(ymKeyOtherCommands));
                    // then PSG commands
                    if (psgCommands->size > 0)
                        addAllToList(xgmCommands, XGMCommand_createPSGCommands(psgCommands));
                    // and finally PCM commands
                    if (sampleCommands->size > 0)
                        addAllToList(xgmCommands, XGMCommand_createPCMCommands(xgm, sampleCommands));

                    clearList(ymKeyOffCommands);
                    clearList(ymKeyOtherCommands);
                    clearList(ymPort0Commands);
                    clearList(ymPort1Commands);
                    clearList(psgCommands);
                    clearList(sampleCommands);
                    lastCommWasKey = false;
                }

                if (VGMCommand_isYM2612Port0Write(command))
                    addToList(ymPort0Commands, command);
                else
                    addToList(ymPort1Commands, command);
            }
            else
            {
                if (verbose)
                    printf("Command %d ignored\n", command->command);
            }
        }

        // general YM commands first as key event were just done
        if (ymPort0Commands->size > 0)
            addAllToList(xgmCommands, XGMCommand_createYMPort0Commands(ymPort0Commands));
        if (ymPort1Commands->size > 0)
            addAllToList(xgmCommands, XGMCommand_createYMPort1Commands(ymPort1Commands));
        // then key off first
        if (ymKeyOffCommands->size > 0)
            addAllToList(xgmCommands, XGMCommand_createYMKeyCommands(ymKeyOffCommands));
        // followed by key on commands
        if (ymKeyOtherCommands->size > 0)
            addAllToList(xgmCommands, XGMCommand_createYMKeyCommands(ymKeyOtherCommands));
        // then PSG commands
        if (psgCommands->size > 0)
            addAllToList(xgmCommands, XGMCommand_createPSGCommands(psgCommands));
        // and finally PCM commands
        if (sampleCommands->size > 0)
            addAllToList(xgmCommands, XGMCommand_createPCMCommands(xgm, sampleCommands));

        // last frame ?
        if (index >= vgm->commands->size)
        {
            // loop point ?
            if (loopOffset != -1)
                addToList(xgmCommands, XGMCommand_createLoopCommand(loopOffset));
            else
                // add end command
                addToList(xgmCommands, XGMCommand_createEndCommand());
        }

        // end frame
        addToList(xgmCommands, XGMCommand_createFrameCommand());

        // finally add the new commands
        addAllToList(xgm->commands, xgmCommands);
    }

    // compute offset
    int offset = 0;
    for (com = 0; com < xgm->commands->size; com++)
    {
        XGMCommand* command = getFromList(xgm->commands, com);

        XGMCommand_setOffset(command, offset);
        offset += command->size;
    }

    if (!silent)
        printf("Number of command: %d\n", xgm->commands->size);
}

/**
 * Find the LOOP command
 */
XGMCommand* XGM_getLoopCommand(XGM* xgm)
{
    int i;

    for (i = 0; i < xgm->commands->size; i++)
    {
        XGMCommand* command = getFromList(xgm->commands, i);
        if (XGMCommand_isLoop(command))
            return command;
    }

    return NULL;
}

/**
 * Return the position of the command pointed by the loop
 */
int XGM_getLoopPointedCommandIndex(XGM* xgm)
{
    XGMCommand* loopCommand = XGM_getLoopCommand(xgm);

    if (loopCommand != NULL)
        return XGM_getCommandIndexAtOffset(xgm, XGMCommand_getLoopOffset(loopCommand));

    return -1;
}

/**
 * Return the command pointed by the loop
 */
XGMCommand* XGM_getLoopPointedCommand(XGM* xgm)
{
    int index = XGM_getLoopPointedCommandIndex(xgm);

    if (index != -1)
        return getFromList(xgm->commands, index);

    return NULL;
}

int XGM_computeLenInFrame(XGM* xgm)
{
    int i;
    int result = 0;

    for (i = 0; i < xgm->commands->size; i++)
        if (XGMCommand_isFrame(getFromList(xgm->commands, i)))
            result++;

    return result;
}

/**
 * Return the offset of the specified command
 */
int XGM_getOffset(XGM* xgm, XGMCommand* command)
{
    int i;
    int result = 0;

    for (i = 0; i < xgm->commands->size; i++)
    {
        XGMCommand* c = getFromList(xgm->commands, i);

        if (c == command)
            return result;

        result += c->size;
    }

    return -1;
}

/**
 * Return elapsed time when specified command happen (in 1/44100 of second)
 */
int XGM_getTime(XGM* xgm, XGMCommand* command)
{
    int i;
    int result = -1;

    for (i = 0; i < xgm->commands->size; i++)
    {
        XGMCommand* c = getFromList(xgm->commands, i);

        if (XGMCommand_isFrame(c))
            result++;
        if (c == command)
            break;
    }

    // convert in sample (44100 Hz)
    return (result * 44100) / 60;
}

/**
 * Return elapsed time when specified command happen
 */
int XGM_getCommandIndexAtTime(XGM* xgm, int time)
{
    int i;
    int adjTime = (time * 60) / 44100;
    int result = 0;

    for (i = 0; i < xgm->commands->size; i++)
    {
        XGMCommand* command = getFromList(xgm->commands, i);

        if (result >= adjTime)
            return i;
        if (XGMCommand_isFrame(command))
            result++;
    }

    return xgm->commands->size - 1;
}

int XGM_getCommandIndexAtOffset(XGM* xgm, int offset)
{
    int i;
    int curOffset = 0;

    for (i = 0; i < xgm->commands->size; i++)
    {
        XGMCommand* command = getFromList(xgm->commands, i);

        if (curOffset == offset)
            return i;

        curOffset += command->size;
    }

    return -1;
}

XGMCommand* XGM_getCommandAtOffset(XGM* xgm, int offset)
{
    const int index = XGM_getCommandIndexAtOffset(xgm, offset);

    if (index != -1)
        return getFromList(xgm->commands, index);

    return NULL;
}

/**
 * Return elapsed time when specified command happen
 */
XGMCommand* XGM_getCommandAtTime(XGM* xgm, int time)
{
    return getFromList(xgm->commands, XGM_getCommandIndexAtTime(xgm, time));
}

XGMSample* XGM_getSampleById(XGM* xgm, int id)
{
    int i;

    for (i = 0; i < xgm->samples->size; i++)
    {
        XGMSample* sample = getFromList(xgm->samples, i);

        if (sample->id == id)
            return sample;
    }

    return NULL;
}

XGMSample* XGM_getSampleByAddress(XGM* xgm, int addr)
{
    int i;

    for (i = 0; i < xgm->samples->size; i++)
    {
        XGMSample* sample = getFromList(xgm->samples, i);

        if (sample->addr == addr)
            return sample;
    }

    return NULL;
}

int XGM_getSampleDataSize(XGM* xgm)
{
    int i;
    int result = 0;

    for (i = 0; i < xgm->samples->size; i++)
    {
        XGMSample* sample = getFromList(xgm->samples, i);
        result += sample->dataSize;
    }

    return result;
}

int XGM_getMusicDataSize(XGM* xgm)
{
    int i;
    int result = 0;

    for (i = 0; i < xgm->commands->size; i++)
    {
        XGMCommand* command = getFromList(xgm->commands, i);
        result += command->size;
    }

    return result;
}

unsigned char* XGM_asByteArray(XGM* xgm, int *outSize)
{
    int i;
    int offset;
    unsigned char byte;
    FILE* f = fopen("tmp.bin", "wb+");

    if (f == NULL)
    {
        printf("Error: cannot open file tmp.bin\n");
        return NULL;
    }

    // 0000: XGM (should be ignored in ROM resource)
    fwrite("XGM ", 1, 4, f);

    // 0004-0100: sample id table
    // fixed size : 252 bytes, limit music to 63 samples max
    offset = 0;
    for (i = 0; i < xgm->samples->size; i++)
    {
        XGMSample* sample = getFromList(xgm->samples, i);
        const int len = sample->dataSize;

        byte = offset >> 8;
        fwrite(&byte, 1, 1, f);
        byte = offset >> 16;
        fwrite(&byte, 1, 1, f);
        byte = len >> 8;
        fwrite(&byte, 1, 1, f);
        byte = len >> 16;
        fwrite(&byte, 1, 1, f);
        offset += len;
    }
    for (i = xgm->samples->size; i < 0x3F; i++)
    {
        // special mark for silent sample
        byte = 0xFF;
        fwrite(&byte, 1, 1, f);
        fwrite(&byte, 1, 1, f);
        byte = 0x00;
        fwrite(&byte, 1, 1, f);
        fwrite(&byte, 1, 1, f);
    }

    // 0100-0101: sample block size *256 (2 bytes)
    byte = offset >> 8;
    fwrite(&byte, 1, 1, f);
    byte = offset >> 16;
    fwrite(&byte, 1, 1, f);

    // init PAL flag if needed (default is NTSC)
    if (xgm->pal == -1)
        xgm->pal = 0;

    // 0102: XGM version
    byte = 0x00;
    fwrite(&byte, 1, 1, f);
    // 0103: b0=NTSC/PAL others=reserved
    byte = xgm->pal & 1;
    fwrite(&byte, 1, 1, f);

    // 0104-XXXX: sample data
    for (i = 0; i < xgm->samples->size; i++)
    {
        XGMSample* sample = getFromList(xgm->samples, i);
        fwrite(sample->data, 1, sample->dataSize, f);
    }

    // compute XGM music data size in byte
    const int len = XGM_getMusicDataSize(xgm);

    // XXXX+0000: music data size (in byte)
    byte = len >> 0;
    fwrite(&byte, 1, 1, f);
    byte = len >> 8;
    fwrite(&byte, 1, 1, f);
    byte = len >> 16;
    fwrite(&byte, 1, 1, f);
    byte = len >> 24;
    fwrite(&byte, 1, 1, f);

    // XXXX+0004: music data
    for (i = 0; i < xgm->commands->size; i++)
    {
        XGMCommand* command = getFromList(xgm->commands, i);
        fwrite(command->data, 1, command->size, f);
    }

    unsigned char* result = inEx(f, 0, getFileSizeEx(f), outSize);

    fclose(f);

    return result;
}
