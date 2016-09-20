#include <stdlib.h>
#include <stdbool.h>
#include <memory.h>
#include <math.h>

#include "../inc/xgmtool.h"
#include "../inc/xgmcom.h"
#include "../inc/xgm.h"
#include "../inc/xgccom.h"
#include "../inc/xgc.h"
#include "../inc/util.h"


XGMCommand* XGCCommand_createFrameSizeCommand(int size)
{
    unsigned char *data = malloc(1);

    data[0] = size;

    return XGMCommand_createEx(XGC_FRAME_SIZE, data, 1);
}

XGMCommand* XGCCommand_createFrameSkipCommand()
{
    unsigned char *data = malloc(1);

    data[0] = XGC_FRAME_SKIP;

    return XGMCommand_create(data, 1);
}

XGMCommand* XGCCommand_createFromCommand(XGMCommand* command)
{
    return XGMCommand_create(command->data, command->size);
}

XGMCommand* XGCCommand_createFromData(unsigned char* data)
{
    XGMCommand* result;

    result = malloc(sizeof(XGMCommand));

    // convert XGC --> XGM
    data[0] >>= 1;

    result->data = data;
    result->offset = 0;

    int command = data[0];

    // LOOP
    if (command == XGM_LOOP)
    {
        result->size = 4;
    }
    // END
    else if (command == XGM_END)
    {
        result->size = 1;
    }
    // FRAME SKIP
    else if (command == XGC_FRAME_SKIP)
    {
        result->size = 1;
    }
    // OTHERS
    else switch(command & 0xF0)
    {
        case XGM_PSG:
            if (command & 0x8)
            {
                command &= ~8;
                result->size = 1 + ((command & 0x3) + 1);
            }
            else
                result->size = 1 + ((command & 0x7) + 1);
            break;

        case XGM_YM2612_REGKEY:
            result->size = 1 + ((command & 0xF) + 1);
            break;

        case XGM_YM2612_PORT0:
        case XGM_YM2612_PORT1:
        case XGC_STATE:
            result->size = 1 + (2 * ((command & 0xF) + 1));
            break;

        case XGM_PCM:
            result->size = 2;
            break;
    }

    data[0] = command;
    result->command = command;

    return result;
}

int XGCCommand_getFrameSizeSize(XGMCommand* source)
{
    return source->data[0] & 0xFF;
}

void XGCCommand_setFrameSizeSize(XGMCommand* source, int value)
{
    source->data[0] = value;
}

int XGCCommand_getType(XGMCommand* source)
{
    if (source->command == XGC_FRAME_SIZE)
        return XGC_FRAME_SIZE;
    if (source->command == XGM_LOOP)
        return XGM_LOOP;
    if (source->command == XGM_END)
        return XGM_END;

    return source->command & 0xF0;
}

bool XGCCommand_isFrameSize(XGMCommand* source)
{
    return source->command == XGC_FRAME_SIZE;
}

bool XGCCommand_isFrameSkip(XGMCommand* source)
{
    return source->command == XGC_FRAME_SKIP;
}

bool XGCCommand_isPSGEnvWrite(XGMCommand* source)
{
    return (source->command & 0xF8) == XGC_PSG_ENV;
}

bool XGCCommand_isPSGToneWrite(XGMCommand* source)
{
    return (source->command & 0xF8) == XGC_PSG_TONE;
}

bool XGCCommand_isPCM(XGMCommand* source)
{
    return (source->command & 0xF0) == XGC_PCM;
}

int XGCCommand_getPCMId(XGMCommand* source)
{
    if (XGCCommand_isPCM(source))
        return source->data[1];

    return -1;
}

bool XGCCommand_isState(XGMCommand* source)
{
    return (source->command & 0xF0) == XGC_STATE;
}


static XGMCommand* XGCCommand_createPSGEnvCommand(LList** pcommands)
{
    LList* curCom = *pcommands;
    const int size = min(4, getSizeLList(curCom));
    unsigned char* data = malloc(size + 1);
    int i, off;

    data[0] = XGC_PSG_ENV | (size - 1);

    off = 1;
    for (i = 0; i < size; i++)
    {
        data[off++] = VGMCommand_getPSGValue(curCom->element);
        curCom = curCom->next;
    }

    // update list pointer to remove elements we have done
    *pcommands = curCom;

    return XGMCommand_create(data, size + 1);
}

static XGMCommand* XGCCommand_createPSGToneCommand(LList** pcommands)
{
    LList* curCom = *pcommands;
    const int size = min(8, getSizeLList(curCom));
    unsigned char* data = malloc(size + 1);
    int i, off;

    data[0] = XGC_PSG_TONE | (size - 1);

    off = 1;
    for (i = 0; i < size; i++)
    {
        data[off++] = VGMCommand_getPSGValue(curCom->element);
        curCom = curCom->next;
    }

    // update list pointer to remove elements we have done
    *pcommands = curCom;

    return XGMCommand_create(data, size + 1);
}

static XGMCommand* XGCCommand_createStateCommand(LList** pstates)
{
    LList* curState = *pstates;
    const int size = min(16, (getSizeLList(curState) / 2));
    unsigned char* data = malloc((size * 2) + 1);
    int i, off;

    data[0] = XGC_STATE | (size - 1);

    off = 1;
    for (i = 0; i < size; i++)
    {
        data[off++] = (int) curState->element;
        curState = curState->next;
        data[off++] = (int) curState->element;
        curState = curState->next;
    }

    // update list pointer to remove elements we have done
    *pstates = curState;

    return XGMCommand_create(data, (size * 2) + 1);
}

LList* XGCCommand_createPSGEnvCommands(LList* commands)
{
    LList* result;
    LList* src;

    result = NULL;
    src = commands;

    if (getSizeLList(src) > 4)
    {
        if (!silent)
            printf("Warning: more than 4 PSG env command in a single frame !\n");
    }

    while (src != NULL)
        result = insertAfterLList(result, XGCCommand_createPSGEnvCommand(&src));

    return getHeadLList(result);
}

LList* XGCCommand_createPSGToneCommands(LList* commands)
{
    LList* result;
    LList* src;

    result = NULL;
    src = commands;

    while (src != NULL)
        result = insertAfterLList(result, XGCCommand_createPSGToneCommand(&src));

    return getHeadLList(result);
}

LList* XGCCommand_createYMKeyCommands(LList* commands)
{
    LList* result;
    LList* src;

    result = NULL;
    src = commands;

    if (getSizeLList(src) > 6)
    {
        if (!silent)
            printf("Warning: more than 6 Key off or Key on command in a single frame !\n");
    }

    while (src != NULL)
        result = insertAfterLList(result, XGMCommand_createYMKeyCommand(&src, 6));

    return getHeadLList(result);
}

LList* XGCCommand_createStateCommands(LList* commands)
{
    LList* result;
    LList* src;

    result = NULL;
    src = commands;

    while (src != NULL)
        result = insertAfterLList(result, XGCCommand_createStateCommand(&src));

    return getHeadLList(result);
}

LList* XGCCommand_convertSingle(XGMCommand* command)
{
    int i, size;
    LList* result;
    LList* comm1;
    LList* comm2;
    unsigned char* data;

    result = NULL;
    comm1 = NULL;
    comm2 = NULL;

    switch (XGMCommand_getType(command))
    {
        default:
            result = insertAfterLList(result, command);
            break;

        case XGM_PSG:
            size = (command->data[0] & 0xF) + 1;

            for (i = 0; i < size; i++)
            {
                VGMCommand* vgmCommand;

                // create VGM PSG command
                data = malloc(2);
                data[0] = 0x50;
                data[1] = command->data[i + 1];
                vgmCommand = VGMCommand_createEx(data, 0, -1);

                // env register write ?
                if (VGMCommand_isPSGEnvWrite(vgmCommand))
                    comm1 = insertAfterLList(comm1, vgmCommand);
                else
                    comm2 = insertAfterLList(comm2, vgmCommand);
            }

            if (comm1 != NULL)
                result = insertAllAfterLList(result, XGCCommand_createPSGEnvCommands(getHeadLList(comm1)));
            if (comm2 != NULL)
                result = insertAllAfterLList(result, XGCCommand_createPSGToneCommands(getHeadLList(comm2)));
            break;

        case XGM_YM2612_REGKEY:
            size = (command->data[0] & 0xF) + 1;

            for (i = 0; i < size; i++)
            {
                VGMCommand* vgmCommand;

                // create VGM YM command
                data = malloc(3);
                data[0] = 0x52;
                data[1] = 0x28;
                data[2] = command->data[i + 1];
                vgmCommand = VGMCommand_createEx(data, 0, -1);

                comm1 = insertAfterLList(comm1, vgmCommand);
            }

            if (comm1 != NULL)
                result = insertAllAfterLList(result, XGCCommand_createYMKeyCommands(getHeadLList(comm1)));
            break;
    }

    deleteLList(comm1);
    deleteLList(comm2);

    return getHeadLList(result);
}

LList* XGCCommand_convert(LList* commands)
{
    LList* result;
    LList* src;

    result = NULL;

    src = commands;
    while(src != NULL)
    {
        result = insertAllAfterLList(result, XGCCommand_convertSingle(src->element));
        src = src->next;
    }

    return getHeadLList(result);
}
