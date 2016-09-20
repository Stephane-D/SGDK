#include <stdlib.h>
#include <stdbool.h>
#include <memory.h>
#include <math.h>

#include "../inc/xgmtool.h"
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

int XGMCommand_getSize(XGMCommand* source)
{
    int command = source->command;
    unsigned char size = command & 0xF;

    // default
    int result = 1;

    switch(command & 0xF0)
    {
        case XGM_PSG:
        case XGM_YM2612_REGKEY:
            result += size + 1;
            break;

        case XGM_YM2612_PORT0:
        case XGM_YM2612_PORT1:
            result += (size + 1) * 2;
            break;

        case XGM_PCM:
            result = 2;
            break;

        case 0x70:
            // LOOP
            if (size == 0xE)
                result = 4;
            break;
    }

    return result;
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

int XGMCommand_getPCMId(XGMCommand* source)
{
    if (XGMCommand_isPCM(source))
        return source->data[1];

    return -1;
}

int XGMCommand_getPCMChannel(XGMCommand* source)
{
    if (XGMCommand_isPCM(source))
        return source->data[0] & 3;

    return -1;
}

int XGMCommand_getPCMPrio(XGMCommand* source)
{
    if (XGMCommand_isPCM(source))
        return (source->data[0] >> 2) & 3;

    return -1;
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
    if (XGMCommand_isYM2612Write(source) || XGMCommand_isYM2612RegKeyWrite(source))
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
        int r = source->data[(i * 2) + 1];
        int v = source->data[(i * 2) + 2];

        // register accepted ?
        if (r != reg)
        {
            data[off++] = r;
            data[off++] = v;
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


XGMCommand* XGMCommand_createYMKeyCommand(LList** pcommands, int max)
{
    LList* curCom = *pcommands;
    const int size = min(max, getSizeLList(curCom));
    unsigned char* data = malloc(size + 1);
    int i, off;

    data[0] = XGM_YM2612_REGKEY | (size - 1);

    off = 1;
    for (i = 0; i < size; i++)
    {
        data[off++] = VGMCommand_getYM2612Value(curCom->element);
        curCom = curCom->next;
    }

    // update list pointer to remove elements we have done
    *pcommands = curCom;

    return XGMCommand_create(data, size + 1);
}

static XGMCommand* XGMCommand_createYMPort0Command(LList** pcommands)
{
    LList* curCom = *pcommands;
    const int size = min(16, getSizeLList(curCom));
    unsigned char* data = malloc((size * 2) + 1);
    int i, off;

    data[0] = XGM_YM2612_PORT0 | (size - 1);

    off = 1;
    for (i = 0; i < size; i++)
    {
        VGMCommand* command = curCom->element;

        data[off++] = VGMCommand_getYM2612Register(command);
        data[off++] = VGMCommand_getYM2612Value(command);

        curCom = curCom->next;
    }

    // update list pointer to remove elements we have done
    *pcommands = curCom;

    return XGMCommand_create(data, (size * 2) + 1);
}

static XGMCommand* XGMCommand_createYMPort1Command(LList** pcommands)
{
    LList* curCom = *pcommands;
    const int size = min(16, getSizeLList(curCom));
    unsigned char* data = malloc((size * 2) + 1);
    int i, off;

    data[0] = XGM_YM2612_PORT1 | (size - 1);

    off = 1;
    for (i = 0; i < size; i++)
    {
        VGMCommand* command = curCom->element;

        data[off++] = VGMCommand_getYM2612Register(command);
        data[off++] = VGMCommand_getYM2612Value(command);

        curCom = curCom->next;
    }

    // update list pointer to remove elements we have done
    *pcommands = curCom;

    return XGMCommand_create(data, (size * 2) + 1);
}

static XGMCommand* XGMCommand_createPSGCommand(LList** pcommands)
{
    LList* curCom = *pcommands;
    const int size = min(16, getSizeLList(curCom));
    unsigned char* data = malloc(size + 1);
    int i, off;

    data[0] = XGM_PSG | (size - 1);

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

static XGMCommand* XGMCommand_createPCMCommand(XGM* xgm, VGM* vgm, VGMCommand* command, int channel)
{
    unsigned char* data = malloc(2);
    XGMSample* xgmSample;
    unsigned char prio;

    data[0] = XGM_PCM;
    prio = 0;

    if (VGMCommand_isStreamStartLong(command))
    {
        int address;
        int size;
        Sample* vgmSample;

        address = VGMCommand_getStreamSampleAddress(command);
        size = VGMCommand_getStreamSampleSize(command);
//        vgmSample = VGM_getSample(vgm, address, size);
        vgmSample = VGM_getSample(vgm, address);

        // use stream id as priority
        prio = 3 - (VGMCommand_getStreamId(command) & 0x3);
        xgmSample = XGM_getSampleByAddress(xgm, address);

        if (vgmSample != NULL)
        {
            double len;
            int lenInFrame;

            len = VGMCommand_getStreamSampleSize(command);
            lenInFrame = ceil(len / ((double) vgmSample->rate / (double) vgm->rate));
        }
        else if (!silent)
            printf("Warning: can't find original VGM sample for VGM command at offset %6X\n", command->offset);
    }
    else if (VGMCommand_isStreamStart(command))
    {
        // use stream id as priority
        prio = 3 - (VGMCommand_getStreamId(command) & 0x3);
        xgmSample = XGM_getSampleByIndex(xgm, VGMCommand_getStreamBlockId(command) + 1);
        if (xgmSample == NULL)
            xgmSample = XGM_getSampleByIndex(xgm, prio + 1);
    }
    else if (VGMCommand_isStreamStop(command))
    {
        // use stream id as priority
        prio = 3 - (VGMCommand_getStreamId(command) & 0x3);
        // stop command
        data[1] = 0;
        return XGMCommand_create(data, 2);
    }
    else
    {
        // assume stop command by default
        prio = 3;
        data[1] = 0;
        return XGMCommand_create(data, 2);
    }

    // no sample found (can arrive if we reached 63 samples limit) --> use stop sample
    if (xgmSample == NULL)
    {
        if (!silent)
            printf("Warning: no corresponding sample found for VGM command at offset %6X in XGM\n", command->offset);
        // assume stop command by default
        data[1] = 0;
    }
    else
        data[1] = xgmSample->index;

    // channel == -1 --> we use inverse of priority for channel
    if (channel == -1)
        data[0] |= 3 - prio;
    else
        data[0] |= (channel & 0x3);
    // set prio
    data[0] |= prio << 2;

    return XGMCommand_create(data, 2);
}


LList* XGMCommand_createYMKeyCommands(LList* commands)
{
    LList* result;
    LList* src;

    result = NULL;
    src = commands;

    while (src != NULL)
        result = insertAfterLList(result, XGMCommand_createYMKeyCommand(&src, 16));

    return getHeadLList(result);
}

LList* XGMCommand_createYMPort0Commands(LList* commands)
{
    LList* result;
    LList* src;

    result = NULL;
    src = commands;

    while (src != NULL)
        result = insertAfterLList(result, XGMCommand_createYMPort0Command(&src));

    return getHeadLList(result);
}

LList* XGMCommand_createYMPort1Commands(LList* commands)
{
    LList* result;
    LList* src;

    result = NULL;
    src = commands;

    while (src != NULL)
        result = insertAfterLList(result, XGMCommand_createYMPort1Command(&src));

    return getHeadLList(result);
}

LList* XGMCommand_createPSGCommands(LList* commands)
{
    LList* result;
    LList* src;

    result = NULL;
    src = commands;

    while (src != NULL)
        result = insertAfterLList(result, XGMCommand_createPSGCommand(&src));

    return getHeadLList(result);
}

LList* XGMCommand_createPCMCommands(XGM* xgm, VGM* vgm, LList* commands)
{
    LList* result;
    LList* src;

    result = NULL;
    src = commands;
    while(src != NULL)
    {
        VGMCommand* command = src->element;

        if (VGMCommand_isStreamStartLong(command) || VGMCommand_isStreamStart(command) || VGMCommand_isStreamStop(command))
            result = insertAfterLList(result, XGMCommand_createPCMCommand(xgm, vgm, command, -1));

        src = src->next;
    }

    return getHeadLList(result);
}

char* XGMCommand_toString(XGMCommand* command)
{
    static char str[32];

    if (XGMCommand_isFrame(command)) sprintf(str, "Frame command");
    else if (XGMCommand_isEnd(command)) sprintf(str, "Frame end");
    else if (XGMCommand_isLoop(command)) sprintf(str, "Frame loop: %8X", XGMCommand_getLoopOffset(command));
    else if (XGMCommand_isPCM(command)) sprintf(str, "PCM: ch%d id=%d prio=%d", XGMCommand_getPCMChannel(command), XGMCommand_getPCMId(command), XGMCommand_getPCMPrio(command));
    else if (XGMCommand_isPSGWrite(command)) sprintf(str, "PSG: num=%d", XGMCommand_getPSGWriteCount(command));
    else if (XGMCommand_isYM2612Port0Write(command)) sprintf(str, "YM0: num=%d", XGMCommand_getYM2612WriteCount(command));
    else if (XGMCommand_isYM2612Port1Write(command)) sprintf(str, "YM1: num=%d", XGMCommand_getYM2612WriteCount(command));
    else if (XGMCommand_isYM2612RegKeyWrite(command)) sprintf(str, "YM Key: num=%d", XGMCommand_getYM2612WriteCount(command));
    else sprintf(str, "Other command id=%2X", XGMCommand_getType(command));

    return str;
}

void XGMCommand_logCommand(FILE *file, XGMCommand* command)
{
    int size;
    unsigned char *data;

    fprintf(file, "%s", XGMCommand_toString(command));

    size = XGMCommand_getSize(command) - 1;
    data = command->data + 1;

    fprintf(file, " - %02X", command->command);
    while(size--) fprintf(file, ":%02X", *data++);
    fprintf(file, "\n");
}

bool XGMCommand_logCommands(char* fileName, LList* commands)
{
    FILE *f;
    LList* com;

    f = fopen(fileName, "w");

    if (!f)
    {
        printf("Error: couldn't open output file %s\n", fileName);
        // error
        return false;
    }

    com = commands;
    while(com != NULL)
    {
        XGMCommand_logCommand(f, com->element);
        com = com->next;
    }

    fclose(f);

    return true;
}
