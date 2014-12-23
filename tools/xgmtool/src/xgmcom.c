#include <stdlib.h>
#include <stdbool.h>
#include <memory.h>
#include <math.h>

#include "../inc/xgmcom.h"
#include "../inc/xgm.h"
#include "../inc/util.h"


XGMCommand* XGMCommand_create(unsigned char* data, int size)
{
    return XGMCommand_createEx(data[0] & 0xFF, data, size);
}

XGMCommand* XGMCommand_createEx(int command, unsigned char* data, int size)
{
    XGMCommand* result;

    result = malloc(sizeof(XGMCommand));

    result->command = command;
    result->data = data;
    result->size = size;
    result->offset = -1;

    return result;
}

XGMCommand* XGMCommand_createLoopCommand(int offset)
{
    unsigned char* data = malloc(4);

    data[0] = XGM_LOOP;
    data[1] = offset >> 0;
    data[2] = offset >> 8;
    data[3] = offset >> 16;

    return XGMCommand_create(data, 4);
}

XGMCommand* XGMCommand_createFrameCommand()
{
    unsigned char* data = malloc(1);

    data[0] = XGM_FRAME;

    return XGMCommand_create(data, 1);
}

XGMCommand* XGMCommand_createEndCommand()
{
    unsigned char* data = malloc(1);

    data[0] = XGM_END;

    return XGMCommand_create(data, 1);
}

XGMCommand* XGMCommand_createFromData(unsigned char* data)
{
    unsigned char command = *data;
    unsigned char size = command & 0xF;

    // default
    int comSize = 1;

    switch(command & 0xF0)
    {
        case XGM_PSG:
        case XGM_YM2612_REGKEY:
            comSize += size + 1;
            break;

        case XGM_YM2612_PORT0:
        case XGM_YM2612_PORT1:
            comSize += (size + 1) * 2;
            break;

        case XGM_PCM:
            comSize = 2;
            break;

        case 0x70:
            // LOOP
            if (size == 0xE)
                comSize = 4;
            break;
    }

    return XGMCommand_create(data, comSize);
}

int XGMCommand_getType(XGMCommand* source)
{
    if (source->command == XGM_FRAME)
        return XGM_FRAME;
    if (source->command == XGM_LOOP)
        return XGM_LOOP;
    if (source->command == XGM_END)
        return XGM_END;

    return source->command & 0xF0;
}

bool XGMCommand_isFrame(XGMCommand* source)
{
    return source->command == XGM_FRAME;
}

bool XGMCommand_isLoop(XGMCommand* source)
{
    return source->command == XGM_LOOP;
}

int XGMCommand_getLoopOffset(XGMCommand* source)
{
    return getInt24(source->data, 1);
}

bool XGMCommand_isEnd(XGMCommand* source)
{
    return source->command == XGM_END;
}

bool XGMCommand_isPCM(XGMCommand* source)
{
    return (source->command & 0xF0) == XGM_PCM;
}

bool XGMCommand_isPSGWrite(XGMCommand* source)
{
    return (source->command & 0xF0) == XGM_PSG;
}

int XGMCommand_getPSGWriteCount(XGMCommand* source)
{
    if (XGMCommand_isPSGWrite(source))
        return (source->data[0] & 0x0F) + 1;

    return -1;
}

bool XGMCommand_isYM2612Port0Write(XGMCommand* source)
{
    return (source->command & 0xF0) == XGM_YM2612_PORT0;
}

bool XGMCommand_isYM2612Port1Write(XGMCommand* source)
{
    return (source->command & 0xF0) == XGM_YM2612_PORT1;
}

bool XGMCommand_isYM2612Write(XGMCommand* source)
{
    return XGMCommand_isYM2612Port0Write(source) || XGMCommand_isYM2612Port1Write(source);
}

int XGMCommand_getYM2612Port(XGMCommand* source)
{
    if (XGMCommand_isYM2612Port0Write(source))
        return 0;
    if (XGMCommand_isYM2612Port1Write(source))
        return 1;

    return -1;
}

int XGMCommand_getYM2612WriteCount(XGMCommand* source)
{
    if (XGMCommand_isYM2612Write(source))
        return (source->data[0] & 0x0F) + 1;

    return -1;
}

bool XGMCommand_isYM2612RegKeyWrite(XGMCommand* source)
{
    return (source->command & 0xF0) == XGM_YM2612_REGKEY;
}

void XGMCommand_setOffset(XGMCommand* source, int value)
{
    source->offset = value;
}


bool XGMCommand_removeYM2612RegWrite(XGMCommand* source, int port, int reg)
{
    switch(port)
    {
        case -1:
            if (!XGMCommand_isYM2612Write(source)) return true;
            break;

        case 0:
            if (!XGMCommand_isYM2612Port0Write(source)) return true;
            break;

        case 1:
            if (!XGMCommand_isYM2612Port1Write(source)) return true;
            break;

        default:
            return true;
    }

    const int size = XGMCommand_getYM2612WriteCount(source);
    unsigned char* data = malloc((size * 2) + 1);
    int i, off;

    off = 1;
    for (i = 0; i < size; i++)
    {
        // register accepted ?
        if (source->data[(i * 2) + 1] != reg)
        {
            data[off++] = source->data[(i * 2) + 1];
            data[off++] = source->data[(i * 2) + 2];
        }
    }

    const int newSize = off / 2;

    // changed ?
    if (newSize != size)
    {
        // no more command !
        if (newSize == 0) return false;

        // set command and size
        data[0] = (source->data[0] & 0xF0) | (newSize - 1);
        // replace data and size
        source->data = data;
        source->size = (newSize * 2) + 1;
    }
    else free(data);

    return true;
}


XGMCommand* XGMCommand_createYMKeyCommand(List* commands, int* offset, int max)
{
    const int size = min(max, commands->size - *offset);
    unsigned char* data = malloc(size + 1);
    int i, off;

    data[0] = XGM_YM2612_REGKEY | (size - 1);

    off = 1;
    for (i = 0; i < size; i++)
        data[off++] = VGMCommand_getYM2612Value(getFromList(commands, i + *offset));

    // remove elements we have done
    *offset += size;

    return XGMCommand_create(data, size + 1);
}

static XGMCommand* XGMCommand_createYMPort0Command(List* commands, int* offset)
{
    const int size = min(16, commands->size - *offset);
    unsigned char* data = malloc((size * 2) + 1);
    int i, off;

    data[0] = XGM_YM2612_PORT0 | (size - 1);

    off = 1;
    for (i = 0; i < size; i++)
    {
        VGMCommand* command = getFromList(commands, i + *offset);

        data[off++] = VGMCommand_getYM2612Register(command);
        data[off++] = VGMCommand_getYM2612Value(command);
    }

    // remove elements we have done
    *offset += size;

    return XGMCommand_create(data, (size * 2) + 1);
}

static XGMCommand* XGMCommand_createYMPort1Command(List* commands, int* offset)
{
    const int size = min(16, commands->size - *offset);
    unsigned char* data = malloc((size * 2) + 1);
    int i, off;

    data[0] = XGM_YM2612_PORT1 | (size - 1);

    off = 1;
    for (i = 0; i < size; i++)
    {
        VGMCommand* command = getFromList(commands, i + *offset);

        data[off++] = VGMCommand_getYM2612Register(command);
        data[off++] = VGMCommand_getYM2612Value(command);
    }

    // remove elements we have done
    *offset += size;

    return XGMCommand_create(data, (size * 2) + 1);
}

static XGMCommand* XGMCommand_createPSGCommand(List* commands, int* offset)
{
    const int size = min(16, commands->size - *offset);
    unsigned char* data = malloc(size + 1);
    int i, off;

    data[0] = XGM_PSG | (size - 1);

    off = 1;
    for (i = 0; i < size; i++)
        data[off++] = VGMCommand_getPSGValue(getFromList(commands, i + *offset));

    // remove elements we have done
    *offset += size;

    return XGMCommand_create(data, size + 1);
}

static XGMCommand* XGMCommand_createPCMCommand(XGM* xgm, VGMCommand* command, int channel)
{
    unsigned char* data = malloc(2);
    XGMSample* sample;
    unsigned char prio;

    data[0] = XGM_PCM;
    prio = 0;

    if (VGMCommand_isStreamStartLong(command))
    {
        // use stream id as priority
        prio = 3 - (VGMCommand_getStreamId(command) & 0x3);
        sample = XGM_getSampleByAddress(xgm, VGMCommand_getStreamSampleAddress(command));
    }
    else if (VGMCommand_isStreamStart(command))
    {
        // use stream id as priority
        prio = 3 - (VGMCommand_getStreamId(command) & 0x3);
        sample = XGM_getSampleById(xgm, VGMCommand_getStreamBlockId(command) + 1);
        if (sample == NULL)
            sample = XGM_getSampleById(xgm, prio + 1);
    }
    else
    {
        // stop command
        data[1] = 0;
        return XGMCommand_create(data, 2);
    }

    // no sample found --> use empty sample
    if (sample == NULL)
    {
        printf("Warning: no corresponding sample found for VGM command at offset %6X in XGM\n", command->offset);
        data[1] = 0;
    }
    else
        data[1] = sample->id;

    // channel == -1 --> we use inverse of priority for channel
    if (channel == -1)
        data[0] |= 3 - prio;
    else
        data[0] |= (channel & 0x3);
    // set prio
    data[0] |= prio << 2;

    return XGMCommand_create(data, 2);
}


List* XGMCommand_createYMKeyCommands(List* commands)
{
    List* result;
    int offset;

    // allocate
    result = createList();

    offset = 0;
    while (offset < commands->size)
        addToList(result, XGMCommand_createYMKeyCommand(commands, &offset, 16));

    return result;
}

List* XGMCommand_createYMPort0Commands(List* commands)
{
    List* result;
    int offset;

    // allocate
    result = createList();

    offset = 0;
    while (offset < commands->size)
        addToList(result, XGMCommand_createYMPort0Command(commands, &offset));

    return result;
}

List* XGMCommand_createYMPort1Commands(List* commands)
{
    List* result;
    int offset;

    // allocate
    result = createList();

    offset = 0;
    while (offset < commands->size)
        addToList(result, XGMCommand_createYMPort1Command(commands, &offset));

    return result;
}

List* XGMCommand_createPSGCommands(List* commands)
{
    List* result;
    int offset;

    // allocate
    result = createList();

    offset = 0;
    while (offset < commands->size)
        addToList(result, XGMCommand_createPSGCommand(commands, &offset));

    return result;
}

List* XGMCommand_createPCMCommands(XGM* xgm, List* commands)
{
    List* result;
    int i;

    // allocate
    result = createList();

    for(i = 0; i < commands->size; i++)
    {
        VGMCommand* command = getFromList(commands, i);

        if (VGMCommand_isStreamStartLong(command) || VGMCommand_isStreamStart(command) || VGMCommand_isStreamStop(command))
            addToList(result, XGMCommand_createPCMCommand(xgm, command, -1));
    }

    return result;
}
