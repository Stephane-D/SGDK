#include <stdlib.h>
#include <stdbool.h>
#include <memory.h>
#include <math.h>

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

XGMCommand* XGCCommand_createFromCommand(XGMCommand* command)
{
    return XGMCommand_create(command->data, command->size);
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

bool XGCCommand_isPSGEnvWrite(XGMCommand* source)
{
    return (source->command & 0xF8) == XGC_PSG_ENV;
}

bool XGCCommand_isPSGToneWrite(XGMCommand* source)
{
    return (source->command & 0xF8) == XGC_PSG_TONE;
}


static XGMCommand* XGCCommand_createPSGEnvCommand(List* commands, int* offset)
{
    const int size = min(4, commands->size - *offset);
    unsigned char* data = malloc(size + 1);
    int i, off;

    data[0] = XGC_PSG_ENV | (size - 1);

    off = 1;
    for (i = 0; i < size; i++)
        data[off++] = VGMCommand_getPSGValue(getFromList(commands, i + *offset));

    // remove elements we have done
    *offset += size;

    return XGMCommand_create(data, size + 1);
}

static XGMCommand* XGCCommand_createPSGToneCommand(List* commands, int* offset)
{
    const int size = min(8, commands->size - *offset);
    unsigned char* data = malloc(size + 1);
    int i, off;

    data[0] = XGC_PSG_TONE | (size - 1);

    off = 1;
    for (i = 0; i < size; i++)
        data[off++] = VGMCommand_getPSGValue(getFromList(commands, i + *offset));

    // remove elements we have done
    *offset += size;

    return XGMCommand_create(data, size + 1);
}

static XGMCommand* XGCCommand_createStateCommand(List* states, int* offset)
{
    const int size = min(16, (states->size / 2) - *offset);
    unsigned char* data = malloc((size * 2) + 1);
    int i, off;

    data[0] = XGC_STATE | (size - 1);

    off = 1;
    for (i = 0; i < size; i++)
    {
        data[off++] = (int) getFromList(states, ((i + *offset) * 2) + 0);
        data[off++] = (int) getFromList(states, ((i + *offset) * 2) + 1);
    }

    // remove elements we have done
    *offset += size;

    return XGMCommand_create(data, (size * 2) + 1);
}


List* XGCCommand_createPSGEnvCommands(List* commands)
{
    List* result;
    int offset;

    // allocate
    result = createList();

    if (commands->size > 4)
        printf("Warning: more then 4 PSG env command in a single frame !\n");

    offset = 0;
    while (offset < commands->size)
        addToList(result, XGCCommand_createPSGEnvCommand(commands, &offset));

    return result;
}

List* XGCCommand_createPSGToneCommands(List* commands)
{
    List* result;
    int offset;

    // allocate
    result = createList();

    offset = 0;
    while (offset < commands->size)
        addToList(result, XGCCommand_createPSGToneCommand(commands, &offset));

    return result;
}

List* XGCCommand_createYMKeyCommands(List* commands)
{
    List* result;
    int offset;

    // allocate
    result = createList();

    if (commands->size > 6)
        printf("Warning: more then 6 Key off or Key on command in a single frame !\n");

    offset = 0;
    while (offset < commands->size)
        addToList(result, XGMCommand_createYMKeyCommand(commands, &offset, 6));

    return result;
}

List* XGCCommand_createStateCommands(List* commands)
{
    List* result;
    int offset;

    // allocate
    result = createList();

    offset = 0;
    while (offset < (commands->size / 2))
        addToList(result, XGCCommand_createStateCommand(commands, &offset));

    return result;
}

List* XGCCommand_convertSingle(XGMCommand* command)
{
    int i, size;
    List* result;
    List* comm1;
    List* comm2;
    unsigned char* data;

    // allocate
    result = createList();

    comm1 = createList();
    comm2 = createList();

    switch (XGMCommand_getType(command))
    {
        default:
            addToList(result, command);
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
                vgmCommand = VGMCommand_createEx(data, 0);

                // env register write ?
                if (VGMCommand_isPSGEnvWrite(vgmCommand))
                    addToList(comm1, vgmCommand);
                else
                    addToList(comm2, vgmCommand);
            }

            if (comm1->size > 0)
                addAllToList(result, XGCCommand_createPSGEnvCommands(comm1));
            if (comm2->size > 0)
                addAllToList(result, XGCCommand_createPSGToneCommands(comm2));
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
                vgmCommand = VGMCommand_createEx(data, 0);

                addToList(comm1, vgmCommand);
            }

            if (comm1->size > 0)
                addAllToList(result, XGCCommand_createYMKeyCommands(comm1));
            break;
    }

    deleteList(comm1);
    deleteList(comm2);

    return result;
}

List* XGCCommand_convert(List* commands)
{
    int i;
    List* result;

    // allocate
    result = createList();

    for (i = 0; i < commands->size; i++)
        addAllToList(result, XGCCommand_convertSingle(getFromList(commands, i)));

    return result;
}
