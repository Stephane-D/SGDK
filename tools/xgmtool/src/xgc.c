#include <stdlib.h>
#include <stdbool.h>
#include <memory.h>

#include "../inc/xgm.h"
#include "../inc/vgm.h"
#include "../inc/xgccom.h"
#include "../inc/xgc.h"
#include "../inc/ym2612.h"
#include "../inc/psg.h"
#include "../inc/xgmtool.h"


// forward
static void XGC_extractMusic(XGM* source, XGM* xgm);


XGM* XGC_create(XGM* xgm)
{
    int i;
    XGM* result = XGM_create();

    if (!silent)
        printf("Converting to XGC...\n");

    // simple copy for sample
    for(i = 0; i < xgm->samples->size; i++)
    {
        XGMSample* sample = getFromList(xgm->samples, i);
        addToList(result->samples, sample);
    }

    // and extract music data
    XGC_extractMusic(result, xgm);
    // shift all samples to 3 frames (~3.3 for driver buffer length)
    XGC_shiftSamples(result, 3);

    if (verbose)
    {
        printf("Sample size: %d\n", XGM_getSampleDataSize(result));
        printf("Music data size: %d\n", XGM_getMusicDataSize(result));
        printf("Number of sample: %d\n", result->samples->size);
    }
    if (!silent)
        printf("XGC duration: %d frames (%d seconds)\n", XGC_computeLenInFrame(result), XGC_computeLenInFrame(result) / 60);

    return result;
}

static void XGC_extractMusic(XGM* source, XGM* xgm)
{
    List* vgmCommands = createList();
    List* frameCommands = createList();
    List* ymCommands = createList();
    List* ymKeyCommands = createList();
    List* psgCommands = createList();
    List* otherCommands = createList();
    List* newCommands = createList();
    List* stateChange = createList();

    XGMCommand* sizeCommand;
    YM2612* ymOldState;
    YM2612* ymState;
    int i, j, size;

    ymOldState = YM2612_create();
    ymState = YM2612_create();

    // reset frame
    addToList(source->commands, XGCCommand_createFrameSizeCommand(0));

    // TL / D1L / RR set to max
    clearList(vgmCommands);
    addAllToList(vgmCommands, VGMCommand_createYMCommands(0, 0x40, 0x7F));
    addAllToList(vgmCommands, VGMCommand_createYMCommands(0, 0x80, 0xFF));
    addAllToList(source->commands, XGCCommand_convert(XGMCommand_createYMPort0Commands(vgmCommands)));
    // set ym state
    for (i = 0; i < vgmCommands->size; i++)
    {
        VGMCommand* command = getFromList(vgmCommands, i);
        YM2612_set(ymState, VGMCommand_getYM2612Port(command), VGMCommand_getYM2612Register(command), VGMCommand_getYM2612Value(command));
    }

    clearList(vgmCommands);
    addAllToList(vgmCommands, VGMCommand_createYMCommands(1, 0x40, 0x7F));
    addAllToList(vgmCommands, VGMCommand_createYMCommands(1, 0x80, 0xFF));
    addAllToList(source->commands, XGCCommand_convert(XGMCommand_createYMPort1Commands(vgmCommands)));
    // set ym state
    for (i = 0; i < vgmCommands->size; i++)
    {
        VGMCommand* command = getFromList(vgmCommands, i);
        YM2612_set(ymState, VGMCommand_getYM2612Port(command), VGMCommand_getYM2612Register(command), VGMCommand_getYM2612Value(command));
    }

    // key off for all channels
    clearList(vgmCommands);
    addToList(vgmCommands, VGMCommand_createYMCommand(0, 0x28, 0x00));
    addToList(vgmCommands, VGMCommand_createYMCommand(0, 0x28, 0x01));
    addToList(vgmCommands, VGMCommand_createYMCommand(0, 0x28, 0x02));
    addToList(vgmCommands, VGMCommand_createYMCommand(0, 0x28, 0x04));
    addToList(vgmCommands, VGMCommand_createYMCommand(0, 0x28, 0x05));
    addToList(vgmCommands, VGMCommand_createYMCommand(0, 0x28, 0x06));
    addAllToList(source->commands, XGCCommand_convert(XGMCommand_createYMKeyCommands(vgmCommands)));
    // set ym state
    for (i = 0; i < vgmCommands->size; i++)
    {
        VGMCommand* command = getFromList(vgmCommands, i);
        YM2612_set(ymState, VGMCommand_getYM2612Port(command), VGMCommand_getYM2612Register(command), VGMCommand_getYM2612Value(command));
    }

    stateChange = XGC_getStateChange(source, ymState, ymOldState);
    // add the state commands if no empty
    if (stateChange->size > 0)
        addAllToList(source->commands, XGCCommand_createStateCommands(stateChange));

    // add 3 dummy frames (reserve frame space for PCM shift)
    addToList(source->commands, XGCCommand_createFrameSizeCommand(0));
    addToList(source->commands, XGCCommand_createFrameSizeCommand(0));
    addToList(source->commands, XGCCommand_createFrameSizeCommand(0));

    XGMCommand* loopCommand = XGM_getLoopPointedCommand(xgm);
    int loopOffset = -1;
    int index = 0;
    while (index < xgm->commands->size)
    {
        // get frame commands
        clearList(frameCommands);
        while (index < xgm->commands->size)
        {
            XGMCommand* command = getFromList(xgm->commands, index++);

            // this is the command where we loop
            if (command == loopCommand)
            {
                if (loopOffset == -1)
                    loopOffset = XGM_getMusicDataSize(source);
            }

            // loop information --> ignore
            if (XGMCommand_isLoop(command) || XGMCommand_isEnd(command))
                continue;
            // stop here
            if (XGMCommand_isFrame(command))
                break;

            // add command
            addToList(frameCommands, command);
        }

        // update state
        ymOldState = ymState;
        ymState = YM2612_copy(ymOldState);

        // prepare new commands for this frame
        clearList(newCommands);

        // add size command
        sizeCommand = XGCCommand_createFrameSizeCommand(0);
        addToList(newCommands, sizeCommand);

        // group commands
        clearList(ymCommands);
        clearList(ymKeyCommands);
        clearList(psgCommands);
        clearList(otherCommands);
        for (i = 0; i < frameCommands->size; i++)
        {
            XGMCommand* command = getFromList(frameCommands, i);

            if (XGMCommand_isPSGWrite(command))
                addToList(psgCommands, command);
            else if (XGMCommand_isYM2612RegKeyWrite(command))
                addToList(ymKeyCommands, command);
            else if (XGMCommand_isYM2612Write(command))
            {
                // update YM state
                for (j = 0; j < XGMCommand_getYM2612WriteCount(command); j++)
                {
                    if (XGMCommand_isYM2612Port0Write(command))
                        YM2612_set(ymState, 0, command->data[(j * 2) + 1] & 0xFF, command->data[(j * 2) + 2] & 0xFF);
                    else
                        YM2612_set(ymState, 1, command->data[(j * 2) + 1] & 0xFF, command->data[(j * 2) + 2] & 0xFF);
                }

                addToList(ymCommands, command);
            }
            else
                addToList(otherCommands, command);
        }

        // general YM commands first as key event were just done
        if (ymCommands->size > 0)
            addAllToList(newCommands, XGCCommand_convert(ymCommands));
        // then key commands
        if (ymKeyCommands->size > 0)
            addAllToList(newCommands, XGCCommand_convert(ymKeyCommands));
        // then PSG commands
        if (psgCommands->size > 0)
            addAllToList(newCommands, XGCCommand_convert(psgCommands));
        // and finally others commands
        if (otherCommands->size > 0)
            addAllToList(newCommands, XGCCommand_convert(otherCommands));

        // state change
        stateChange = XGC_getStateChange(source, ymState, ymOldState);
        // add the state command if no empty
        if (stateChange->size > 0)
            addAllToList(newCommands, XGCCommand_createStateCommands(stateChange));

        // last frame ?
        if (index >= xgm->commands->size)
        {
            // loop point ?
            if (loopOffset != -1)
                addToList(newCommands, XGMCommand_createLoopCommand(loopOffset));
            else
                // add end command
                addToList(newCommands, XGMCommand_createEndCommand());
        }

        // limit frame commands to 255 bytes max
        size = 0;
        int ind = 0;
        while (ind < newCommands->size)
        {
            XGMCommand* command = getFromList(newCommands, ind);

            // limit reached ?
            if ((size + command->size) >= 256)
            {
                // no firsts frame
                if (ind > 10)
                    printf("Warning: frame > 256 at frame %4X (need to split frame)\n", XGC_computeLenInFrame(source));

                // end previous frame
                XGCCommand_setFrameSizeSize(sizeCommand, size);

                // insert new frame size info
                sizeCommand = XGCCommand_createFrameSizeCommand(0);
                addToListEx(newCommands, ind, sizeCommand);

                // reset size and pass to next element
                size = 1;
                ind++;
            }

            size += command->size;
            ind++;
        }

        // set frame size
        XGCCommand_setFrameSizeSize(sizeCommand, size);

        // finally add the new commands
        addAllToList(source->commands, newCommands);
    }

    // compute offset & frame size
    XGC_computeAllOffset(source);
    XGC_computeAllFrameSize(source);

    if (!silent)
        printf("Number of command: %d\n", source->commands->size);
}

void XGC_shiftSamples(XGM* source, int sft)
{
    int i;

    if (sft == 0)
        return;

    XGMCommand* loopCommand = XGM_getLoopCommand(source);
    XGMCommand* loopPointedCommand = XGM_getLoopPointedCommand(source);

    List* sampleCommands[sft];
    List* loopSampleCommands[sft];

    for (i = 0; i < sft; i++)
    {
        sampleCommands[i] = createList();
        loopSampleCommands[i] = createList();
    }

    int loopFrameIndex = sft;
    int frameRead = 0;
    int frameWrite = 0;
    int index = source->commands->size - 1;
    while (index >= 0)
    {
        XGMCommand* command = getFromList(source->commands, index);

        // this is the command pointer by the loop
        if (command == loopPointedCommand)
            loopFrameIndex = 0;

        if (XGMCommand_isPCM(command))
        {
            addToList(sampleCommands[frameRead], command);
            removeFromList(source->commands, index);
        }
        else if (XGCCommand_isFrameSize(command))
        {
            frameRead = (frameRead + 1) % sft;
            frameWrite = (frameWrite + 1) % sft;

            // add sample command to this frame
            while (sampleCommands[frameWrite]->size > 0)
            {
                // get last and remove
                XGMCommand* sampleCommand = removeFromList(sampleCommands[frameWrite], sampleCommands[frameWrite]->size - 1);

                // add sample command to previous frame
                addToListEx(source->commands, max(1, index), sampleCommand);
                // store for the loop samples restore
                if (loopFrameIndex < sft)
                    addToList(loopSampleCommands[loopFrameIndex], XGCCommand_createFromCommand(sampleCommand));
            }

            loopFrameIndex++;
        }

        index--;
    }

    // add last remaining samples
    for (i = 0; i < sft; i++)
        while (sampleCommands[i]->size > 0)
            addToListEx(source->commands, i + 1, removeFromList(sampleCommands[i], sampleCommands[i]->size - 1));

    // avoid end or loop command
    index = source->commands->size - 2;
    loopFrameIndex = 0;
    while ((index >= 0) && (loopFrameIndex < sft))
    {
        XGMCommand* command = getFromList(source->commands, index);

        if (XGCCommand_isFrameSize(command))
        {
            // add sample command to previous frame
            while (loopSampleCommands[loopFrameIndex]->size > 0)
            {
                XGMCommand* sampleCommand = removeFromList(loopSampleCommands[loopFrameIndex], loopSampleCommands[loopFrameIndex]->size - 1);

                // add sample command to current frame
                addToListEx(source->commands, index + 1, sampleCommand);
            }

            loopFrameIndex++;
        }

        index--;
    }

    // recompute offset & frame size
    XGC_computeAllOffset(source);
    XGC_computeAllFrameSize(source);

    // fix address in loop command
    if ((loopCommand != NULL) && (loopPointedCommand != NULL))
    {
        int offset = loopPointedCommand->offset;

        loopCommand->data[1] = offset >> 0;
        loopCommand->data[2] = offset >> 8;
        loopCommand->data[3] = offset >> 16;
    }
}

List* XGC_getStateChange(XGM* source, YM2612* current, YM2612* old)
{
    int port;
    List* result = createList();

    int addr = 0x44;
    // ym2612
    for (port = 0; port < 2; port++)
    {
        // D1L / RR registers state change
        int reg = 0x80;
        while (reg < 0x90)
        {
            // value is different
            if (YM2612_isDiff(current, old, port, reg))
            {
                // write state for current register
                addToList(result, (void*) addr);
                addToList(result, (void*) YM2612_get(current, port, reg));
            }

            addr++;
            if ((reg & 3) == 2)
                reg += 2;
            else
                reg++;
        }
    }

    return result;
}

void XGC_computeAllOffset(XGM* source)
{
    int i;

    // compute offset
    int offset = 0;
    for (i = 0; i < source->commands->size; i++)
    {
        XGMCommand* command = getFromList(source->commands, i);

        XGMCommand_setOffset(command, offset);
        offset += command->size;
    }
}

void XGC_computeAllFrameSize(XGM* source)
{
    XGMCommand* sizeCommand;
    int i, size, frame;

    sizeCommand = NULL;
    size = 0;
    frame = 0;
    for (i = 0; i < source->commands->size; i++)
    {
        XGMCommand* command = getFromList(source->commands, i);

        if (XGCCommand_isFrameSize(command))
        {
            if (sizeCommand != NULL)
            {
                if (size > 255)
                    printf("Error: frame %4X has a size > 255 ! Can't continue...\n", frame);

                XGCCommand_setFrameSizeSize(sizeCommand, size);
            }

            sizeCommand = command;
            size = 1;
            frame++;
        }
        else
            size += command->size;
    }
}

int XGC_computeLenInFrame(XGM* source)
{
    int i;
    int result = 0;

    for (i = 0; i < source->commands->size; i++)
    {
        XGMCommand* command = getFromList(source->commands, i);

        if (XGCCommand_isFrameSize(command))
            result++;
    }

    return result;
}

/**
 * Return elapsed time when specified command happen (in 1/44100 of second)
 */
int XGC_getTime(XGM* source, XGMCommand* command)
{
    int i;
    int result = -1;

    for (i = 0; i < source->commands->size; i++)
    {
        XGMCommand* c = getFromList(source->commands, i);

        if (XGCCommand_isFrameSize(c))
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
int XGC_getCommandIndexAtTime(XGM* source, int time)
{
    const int adjTime = (time * 60) / 44100;
    int c;
    int result = 0;

    for (c = 0; c < source->commands->size; c++)
    {
        XGMCommand* command = getFromList(source->commands, c);

        if (result >= adjTime)
            return c;
        if (XGCCommand_isFrameSize(command))
            result++;
    }

    return source->commands->size - 1;
}

unsigned char* XGC_asByteArray(XGM* source, int *outSize)
{
    int c, s;
    int offset;
    unsigned char byte;
    FILE* f = fopen("tmp.bin", "wb+");

    if (f == NULL)
    {
        printf("Error: cannot open file tmp.bin\n");
        return NULL;
    }

    // 0000-00FB: sample id table
    // fixed size : 252 bytes, limit music to 63 samples max
    offset = 0;
    for (s = 0; s < source->samples->size; s++)
    {
        XGMSample* sample = getFromList(source->samples, s);
        int len = sample->dataSize;

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
    for (s = source->samples->size; s < 0x3F; s++)
    {
        // special mark for silent sample
        byte = 0xFF;
        fwrite(&byte, 1, 1, f);
        byte = 0xFF;
        fwrite(&byte, 1, 1, f);
        byte = 0x01;
        fwrite(&byte, 1, 1, f);
        byte = 0x00;
        fwrite(&byte, 1, 1, f);
    }

    // 00FC-00FD: sample block size *256 (2 bytes)
    byte = offset >> 8;
    fwrite(&byte, 1, 1, f);
    byte = offset >> 16;
    fwrite(&byte, 1, 1, f);

    // 00FE: XGM version
    byte = 0x00;
    fwrite(&byte, 1, 1, f);
    // 00FF: PAL/NTSC flag + reserved
    byte = 0x00;
    fwrite(&byte, 1, 1, f);

    // 0100-XXXX: sample data
    for (s = 0; s < source->samples->size; s++)
    {
        XGMSample* sample = getFromList(source->samples, s);
        fwrite(sample->data, 1, sample->dataSize, f);
    }

    // compute XGM music data size in byte
    int len = XGM_getMusicDataSize(source);

    // XXXX+0000: music data size (in byte)
    byte = len >> 0;
    fwrite(&byte, 1, 1, f);
    byte = len >> 8;
    fwrite(&byte, 1, 1, f);
    byte = len >> 16;
    fwrite(&byte, 1, 1, f);
    byte = len >> 24;
    fwrite(&byte, 1, 1, f);

    offset = 0;
    // XXXX+0004: music data
    for (c = 0; c < source->commands->size; c++)
    {
        XGMCommand* command = getFromList(source->commands, c);

        if (XGCCommand_isFrameSize(command))
            fwrite(command->data, 1, command->size, f);
        else
        {
            // for easier Z80 16 bits jump table
            byte = command->data[0] << 1;
            fwrite(&byte, 1, 1, f);

            if (command->size > 1)
                fwrite(&(command->data[1]), 1, command->size - 1, f);
        }

        if (command->offset != offset)
            printf("Error: command offset is incorrect !\n");

        offset += command->size;
    }

    unsigned char* result = inEx(f, 0, getFileSizeEx(f), outSize);

    fclose(f);

    return result;
}
