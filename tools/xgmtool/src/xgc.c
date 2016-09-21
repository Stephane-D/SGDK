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
#include "../inc/gd3.h"


// forward
static void XGC_extractMusic(XGM* xgc, XGM* xgm);
static int XGC_computeLenInFrameOf(LList* commands);

XGM* XGC_create(XGM* xgm)
{
    XGM* result = XGM_create();
    LList* s;
    LList* d;

    if (!silent)
        printf("Converting to XGC...\n");

    // copy pal/ntsc information
    result->pal = xgm->pal;

    // simple copy for sample
    s = xgm->samples;
    d = result->samples;
    while(s != NULL)
    {
        XGMSample* sample = s->element;
        d = insertAfterLList(d, sample);
        s = s->next;
    }
    result->samples = getHeadLList(d);

    // and extract music data
    XGC_extractMusic(result, xgm);

    // shift all samples to 2 frames for PAL and 3 frames ahead for NTSC (because of PCM buffer length)
    if (result->pal)
        XGC_shiftSamples(result, 2);
    else
        XGC_shiftSamples(result, 3);

    // copy GD3 tags
    if (xgm->gd3)
    {
        LList* firstLoopCommand;
        int duration;
        int loopDuration;

        result->gd3 = xgm->gd3;

        duration = XGC_computeLenInFrame(result);
        firstLoopCommand = XGM_getLoopPointedCommandElement(result);

        if (firstLoopCommand != NULL) loopDuration = XGC_computeLenInFrameOf(firstLoopCommand);
        else loopDuration = 0;

        // convert to XD3 here
        result->xd3 = XD3_createFromGD3(xgm->gd3, duration, loopDuration);
    }

    // display play PCM command
//    if (verbose)
//    {
//        LList* curCom = result->commands;
//        while(curCom != NULL)
//        {
//            XGMCommand* command = curCom->element;
//
//            if (XGCCommand_isPCM(command))
//                printf("play sample %2X at frame %d\n", XGCCommand_getPCMId(command), XGC_getTimeInFrame(result, command));
//
//            curCom = curCom->next;
//        }
//    }

    if (verbose)
    {
        printf("Sample size: %d\n", XGM_getSampleDataSize(result));
        printf("Music data size: %d\n", XGM_getMusicDataSize(result));
        printf("Number of sample: %d\n", getSizeLList(result->samples));
    }
    if (!silent)
        printf("XGC duration: %d frames (%d seconds)\n", XGC_computeLenInFrame(result), XGC_computeLenInSecond(result));

    return result;
}

static void XGC_extractMusic(XGM* xgc, XGM* xgm)
{
    LList* frameCommands = NULL;
    LList* ymOtherCommands = NULL;
    LList* ymKeyCommands = NULL;
    LList* ymCommands = NULL;
    LList* psgCommands = NULL;
    LList* otherCommands = NULL;
    LList* newCommands = NULL;
    LList* stateChange = NULL;
    LList* xgcCommands;
    LList* com;
    LList* tmpCom;

    XGMCommand* sizeCommand;
    YM2612* ymLoopState;
    YM2612* ymOldState;
    YM2612* ymState;
    int j, size;
    int time;
    bool hasKeyCom;

    time = 0;
    ymLoopState = NULL;
    ymOldState = NULL;
    ymState = YM2612_create();

    // add 3 dummy frames (reserve frame space for PCM shift)
    xgcCommands = createElement(XGCCommand_createFrameSizeCommand(0));
    xgcCommands = insertAfterLList(xgcCommands, XGCCommand_createFrameSizeCommand(0));
    xgcCommands = insertAfterLList(xgcCommands, XGCCommand_createFrameSizeCommand(0));

    XGMCommand* loopCommand = XGM_getLoopPointedCommand(xgm);
    int loopOffset = -1;
    bool loopEnd = false;

    com = xgm->commands;
    while(com != NULL)
    {
        // build frame commands
        deleteLList(frameCommands);
        frameCommands = NULL;

        while(com != NULL)
        {
            // get command and pass to next one
            XGMCommand* command = com->element;
            com = com->next;

            // this is the command where we start loop
            if (command == loopCommand)
            {
                if (loopOffset == -1)
                {
                    loopOffset = XGM_getMusicDataSizeOf(getHeadLList(xgcCommands));
                    // keep YM state on loop
                    ymLoopState = YM2612_copy(ymState);
                }
            }

            // end information --> ignore
            if (XGMCommand_isEnd(command))
                continue;
            // loop end information --> store it
            if (XGMCommand_isLoop(command))
            {
                loopEnd = true;
                continue;
            }
            // stop here
            if (XGMCommand_isFrame(command))
                break;

            // add command
            frameCommands = insertAfterLList(frameCommands, command);
        }

        // get back to head
        frameCommands = getHeadLList(frameCommands);

        // update state
        if (ymOldState) free(ymOldState);
        ymOldState = ymState;
        ymState = YM2612_copy(ymOldState);

        // prepare new commands for this frame
        deleteLList(newCommands);
        // add size command
        sizeCommand = XGCCommand_createFrameSizeCommand(0);
        newCommands = createElement(sizeCommand);

        // group commands
        deleteLList(ymOtherCommands);
        deleteLList(ymKeyCommands);
        deleteLList(ymCommands);
        deleteLList(psgCommands);
        deleteLList(otherCommands);
        ymOtherCommands = NULL;
        ymKeyCommands = NULL;
        ymCommands = NULL;
        psgCommands = NULL;
        otherCommands = NULL;

        hasKeyCom = false;

        tmpCom = frameCommands;
        while(tmpCom != NULL)
        {
            XGMCommand* command = tmpCom->element;

            if (XGMCommand_isPSGWrite(command))
                psgCommands = insertAfterLList(psgCommands, command);
            else if (XGMCommand_isYM2612RegKeyWrite(command))
            {
                ymKeyCommands = insertAfterLList(ymKeyCommands, command);
                hasKeyCom = true;
            }
            else if (XGMCommand_isYM2612Write(command))
            {
                // need accurate order of key event / register write so we cumulate YM commands now
                if (hasKeyCom)
                {
                    ymOtherCommands = getHeadLList(ymOtherCommands);
                    ymKeyCommands = getHeadLList(ymKeyCommands);

                    // general YM commands first as key event were just done
                    if (ymOtherCommands != NULL)
                        ymCommands = insertAllAfterLList(ymCommands, XGCCommand_convert(ymOtherCommands));
                    // then key commands
                    if (ymKeyCommands != NULL)
                        ymCommands = insertAllAfterLList(ymCommands, XGCCommand_convert(ymKeyCommands));

                    deleteLList(ymOtherCommands);
                    deleteLList(ymKeyCommands);
                    ymOtherCommands = NULL;
                    ymKeyCommands = NULL;

                    hasKeyCom = false;
                }

                // update YM state
                for (j = 0; j < XGMCommand_getYM2612WriteCount(command); j++)
                {
                    if (XGMCommand_isYM2612Port0Write(command))
                        YM2612_set(ymState, 0, command->data[(j * 2) + 1] & 0xFF, command->data[(j * 2) + 2] & 0xFF);
                    else
                        YM2612_set(ymState, 1, command->data[(j * 2) + 1] & 0xFF, command->data[(j * 2) + 2] & 0xFF);
                }

                // remove all $2B register writes (DAC enable is done automatically)
                if (XGMCommand_removeYM2612RegWrite(command, 0, 0x2B))
                    ymOtherCommands = insertAfterLList(ymOtherCommands, command);
            }
            else
                otherCommands = insertAfterLList(otherCommands, command);

            tmpCom = tmpCom->next;
        }

        XGMCommand* pcmComCH[4];
        pcmComCH[0] = NULL;
        pcmComCH[1] = NULL;
        pcmComCH[2] = NULL;
        pcmComCH[3] = NULL;

        // discard multi PCM command in a single frame
        tmpCom = otherCommands;
        while(tmpCom != NULL)
        {
            XGMCommand* command = tmpCom->element;

            if (XGMCommand_isPCM(command))
            {
                // get channel
                int ch = XGMCommand_getPCMChannel(command);

                // already have a PCM command for this channel ?
                if (pcmComCH[ch] != NULL)
                {
                    // remove current PCM command
                    removeFromLList(tmpCom);
                    // we removed the last command --> update pointer
                    if (tmpCom == otherCommands)
                        otherCommands = tmpCom->prev;

                    if (!silent)
                    {
                        int frameInd = XGC_computeLenInFrameOf(getHeadLList(xgcCommands));
                        int id = XGMCommand_getPCMId(command);

                        // we are ignoring a real play command --> display it
                        if (id != 0)
                            printf("Warning: multiple PCM command on %d --> play %2X removed\n", frameInd, id);
                    }
                }
                else
                    pcmComCH[ch] = command;
            }

            tmpCom = tmpCom->prev;
        }

        // merge YM commands
        ymOtherCommands = getHeadLList(ymOtherCommands);
        ymKeyCommands = getHeadLList(ymKeyCommands);

        // general YM commands first as key event were just done
        if (ymOtherCommands != NULL)
            ymCommands = insertAllAfterLList(ymCommands, XGCCommand_convert(ymOtherCommands));
        // then key commands
        if (ymKeyCommands != NULL)
            ymCommands = insertAllAfterLList(ymCommands, XGCCommand_convert(ymKeyCommands));

        psgCommands = getHeadLList(psgCommands);
        ymCommands = getHeadLList(ymCommands);
        otherCommands = getHeadLList(otherCommands);

        // PSG commands first as PSG require main BUS access (DMA contention)
        if (psgCommands != NULL)
            newCommands = insertAllAfterLList(newCommands, XGCCommand_convert(psgCommands));
        // then YM commands (already transformed in XGC command)
        if (ymCommands != NULL)
            newCommands = insertAllAfterLList(newCommands, ymCommands);
        // and finally others commands (PCM)
        if (otherCommands != NULL)
            newCommands = insertAllAfterLList(newCommands, XGCCommand_convert(otherCommands));

        // state change
        stateChange = XGC_getStateChange(ymState, ymOldState);
        // add the state command if no empty
        if (stateChange != NULL)
            newCommands = insertAllAfterLList(newCommands, XGCCommand_createStateCommands(stateChange));

        // loop point ?
        if (loopEnd)
        {
            if (loopOffset != -1)
            {
                // TODO: try to fix YM state restoration on loop

                // and frame skip command as we force end frame after loop from XGM
                newCommands = insertAfterLList(newCommands, XGCCommand_createFrameSkipCommand());
                // then insert loop command
                newCommands = insertAfterLList(newCommands, XGMCommand_createLoopCommand(loopOffset));
                loopOffset = -1;
            }
        }

        // is it the last frame ?
        if (com == NULL)
        {
            // loop point ?
            if (loopOffset != -1)
            {
                // TODO: try to fix YM state restoration on loop

                // and frame skip command as we force end frame after loop
                newCommands = insertAfterLList(newCommands, XGCCommand_createFrameSkipCommand());
                // then insert loop command
                newCommands = insertAfterLList(newCommands, XGMCommand_createLoopCommand(loopOffset));
                loopOffset = -1;
            }
            else
                // add end command
                newCommands = insertAfterLList(newCommands, XGMCommand_createEndCommand());
        }

        // get back to head
        newCommands = getHeadLList(newCommands);

        // limit frame commands to 255 bytes max
        size = 0;
        tmpCom = newCommands;
        while(tmpCom != NULL)
        {
            XGMCommand* command = tmpCom->element;

            // limit reached (use 250 for safe sample shift operation) ?
            if ((size + command->size) >= 250)
            {
//                if ((frameInd > 10) && (!silent))
                if (!silent)
                {
                    int frameInd = XGC_computeLenInFrameOf(getHeadLList(xgcCommands)) + (XGC_computeLenInFrameOf(newCommands) - 1);
                    printf("Warning: frame >= 256 at frame %4X (need to split frame)\n", frameInd);
                }

                // insert frame skip command so driver will parse 2 frames together
                insertBeforeLList(tmpCom, XGCCommand_createFrameSkipCommand());
                // end previous frame (current size + frame skip command size)
                XGCCommand_setFrameSizeSize(sizeCommand, size + 1);

                // insert new frame size info
                sizeCommand = XGCCommand_createFrameSizeCommand(0);
                insertBeforeLList(tmpCom, sizeCommand);

                // reset size and pass to next element
                size = 1;
            }

            size += command->size;
            tmpCom = tmpCom->next;
        }

        // set frame size
        XGCCommand_setFrameSizeSize(sizeCommand, size);

        // finally add the new commands
        xgcCommands = insertAllAfterLList(xgcCommands, newCommands);
    }

    xgc->commands = getHeadLList(xgcCommands);

    // compute offset & frame size
    XGM_computeAllOffset(xgc);
    XGC_computeAllFrameSize(xgc);

    if (!silent)
        printf("Number of command: %d\n", getSizeLList(xgc->commands));
}

void XGC_shiftSamples(XGM* source, int sft)
{
    int i;

    if (sft == 0)
        return;

    XGMCommand* loopCommand = XGM_getLoopCommand(source);
    XGMCommand* loopPointedCommand = XGM_getLoopPointedCommand(source);

    LList* sampleCommands[sft];
    LList* loopSampleCommands[sft];

    for (i = 0; i < sft; i++)
    {
        sampleCommands[i] = NULL;
        loopSampleCommands[i] = NULL;
    }

    int loopFrameIndex = sft;
    int frameRead = 0;
    int frameWrite = 0;
    LList* com = getTailLList(source->commands);

    while (com != NULL)
    {
        XGMCommand* command = com->element;

        // this is the command pointed by the loop
        if (command == loopPointedCommand)
            loopFrameIndex = 0;

        if (XGMCommand_isPCM(command))
        {
            sampleCommands[frameRead] = insertAfterLList(sampleCommands[frameRead], command);
            removeFromLList(com);
        }
        else if (XGCCommand_isFrameSize(command))
        {
            frameRead = (frameRead + 1) % sft;
            frameWrite = (frameWrite + 1) % sft;

            // add sample commands stored for this frame
            while (sampleCommands[frameWrite] != NULL)
            {
                // get current sample
                XGMCommand* sampleCommand = sampleCommands[frameWrite]->element;
                // next sample to store
                sampleCommands[frameWrite] = sampleCommands[frameWrite]->prev;

                // insert sample command just before current frame (and bypass it)
                com = insertBeforeLList(com, sampleCommand);
                // store for the loop samples restore
                if (loopFrameIndex < sft)
                    loopSampleCommands[loopFrameIndex] = insertAfterLList(loopSampleCommands[loopFrameIndex], XGCCommand_createFromCommand(sampleCommand));
            }

            loopFrameIndex++;
        }

        com = com->prev;
    }

    // add last remaining samples
    com = source->commands;
    for (i = 0; i < sft; i++)
    {
        while (sampleCommands[i] != NULL)
        {
            insertAfterLList(com, sampleCommands[i]->element);
            // next sample to store
            sampleCommands[i] = sampleCommands[i]->prev;
        }
    }

    com = getTailLList(source->commands);
    // avoid the last command (end or loop)
    if (com != NULL)
        com = com->prev;
    loopFrameIndex = 0;
    while (loopFrameIndex < sft)
    {
        XGMCommand* command = com->element;

        if (XGCCommand_isFrameSize(command))
        {
            // add sample command to previous frame
            while (loopSampleCommands[loopFrameIndex] != NULL)
            {
                XGMCommand* sampleCommand = loopSampleCommands[loopFrameIndex]->element;
                // next sample to store
                loopSampleCommands[loopFrameIndex] = loopSampleCommands[loopFrameIndex]->prev;

                // add sample command to current frame
                insertAfterLList(com, sampleCommand);
            }

            loopFrameIndex++;
        }

        com = com->prev;
    }

    // recompute offset & frame size
    XGM_computeAllOffset(source);
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

LList* XGC_getStateChange(YM2612* current, YM2612* old)
{
    int port;
    LList* result = NULL;
    int addr;

    addr = 0x44;
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
                result = insertAfterLList(result, (void*) addr);
                result = insertAfterLList(result, (void*) YM2612_get(current, port, reg));
            }

            addr++;
            if ((reg & 3) == 2)
                reg += 2;
            else
                reg++;
        }
    }

    // DAC Enable change
    if (YM2612_isDiff(current, old, 0, 0x2B))
    {
        // write state for current register
        result = insertAfterLList(result, (void*) 0x44 + 0x1C);
        result = insertAfterLList(result, (void*) YM2612_get(current, 0, 0x2B));
    }

    return getHeadLList(result);
}

void XGC_computeAllFrameSize(XGM* source)
{
    XGMCommand* sizeCommand;
    int size, frame;
    LList* com;

    sizeCommand = NULL;
    size = 0;
    frame = 0;
    com = source->commands;
    while(com != NULL)
    {
        XGMCommand* command = com->element;

        if (XGCCommand_isFrameSize(command))
        {
            if (sizeCommand != NULL)
            {
                if ((size > 255) && (!silent))
                    printf("Error: frame %4X has a size > 255 ! Can't continue...\n", frame);

                XGCCommand_setFrameSizeSize(sizeCommand, size);
            }

            sizeCommand = command;
            size = 1;
            frame++;
        }
        else
            size += command->size;

        com = com->next;
    }

    // last size command
    if (sizeCommand != NULL)
    {
        if ((size > 255) && (!silent))
            printf("Error: frame %4X has a size > 255 ! Can't continue...\n", frame);

        XGCCommand_setFrameSizeSize(sizeCommand, size);
    }
}

static int XGC_computeLenInFrameOf(LList* commands)
{
    LList* com = commands;
    int result = 0;

    while(com != NULL)
    {
        XGMCommand* command = com->element;

        if (XGCCommand_isFrameSize(command))
            result++;
        else if (XGCCommand_isFrameSkip(command))
            result--;

        com = com->next;
    }

    return result;
}

int XGC_computeLenInFrame(XGM* source)
{
    return XGC_computeLenInFrameOf(source->commands);
}

int XGC_computeLenInSecond(XGM* source)
{
    return XGC_computeLenInFrameOf(source->commands) / (source->pal ? 50 : 60);
}

/**
 * Return elapsed time when specified command happen (in 1/44100 of second)
 */
int XGC_getTime(XGM* source, XGMCommand* command)
{
    LList* com = source->commands;
    int result = -1;

    while(com != NULL)
    {
        XGMCommand* c = com->element;

        if (XGCCommand_isFrameSize(c))
            result++;
        if (c == command)
            break;

        com = com->next;
    }

    // convert in sample (44100 Hz)
    return (result * 44100) / (source->pal ? 50 : 60);
}

/**
 * Return elapsed time when specified command happen (in frame)
 */
int XGC_getTimeInFrame(XGM* xgm, XGMCommand* command)
{
    return XGC_getTime(xgm, command) / (44100 / (xgm->pal ? 50 : 60));
}


/**
 * Return elapsed time when specified command happen
 */
LList* XGC_getCommandElementAtTime(XGM* source, int time)
{
    LList* com = source->commands;
    const int adjTime = (time * 60) / 44100;
    int result = 0;

    while(com != NULL)
    {
        XGMCommand* command = com->element;

        if (result >= adjTime)
            return com;
        if (XGCCommand_isFrameSize(command))
            result++;

        com = com->next;
    }

    return com;
}

unsigned char* XGC_asByteArray(XGM* source, int *outSize)
{
    int s;
    int offset;
    unsigned char byte;
    FILE* f = fopen("tmp.bin", "wb+");
    LList* l;

    if (f == NULL)
    {
        printf("Error: cannot open file tmp.bin\n");
        return NULL;
    }

    // 0000-00FB: sample id table
    // fixed size : 252 bytes, limit music to 63 samples max
    offset = 0;
    s = 0;
    l = source->samples;
    while(l != NULL)
    {
        XGMSample* sample = l->element;
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
        s++;
        l = l->next;
    }
    for (; s < 0x3F; s++)
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
    // 00FF
    byte = 0x00;
    // b0=NTSC/PAL
    byte |= source->pal?1:0;
    // b1=XD3 tags
    byte |= (source->xd3 != NULL)?2:0;
    // b2=multi track, others=reserved
    fwrite(&byte, 1, 1, f);

    // 0100-XXXX: sample data
    l = source->samples;
    while(l != NULL)
    {
        XGMSample* sample = l->element;
        fwrite(sample->data, 1, sample->dataSize, f);
        l = l->next;
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
    l = source->commands;
    while(l != NULL)
    {
        XGMCommand* command = l->element;

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
        l = l->next;
    }

    // XXXX+0004+MLEN: XD3 tags if present
    if (source->xd3)
    {
        unsigned char* data = XD3_asByteArray(source->xd3, &s);
        fwrite(data, 1, s, f);
    }

    unsigned char* result = inEx(f, 0, getFileSizeEx(f), outSize);

    fclose(f);

    return result;
}
