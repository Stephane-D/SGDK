#include <stdlib.h>
#include <stdbool.h>
#include <memory.h>

#include "../inc/vgmcom.h"
#include "../inc/util.h"


VGMCommand* VGMCommand_create(int command, int time)
{
    VGMCommand* result;

    result = malloc(sizeof(VGMCommand));

    result->data = NULL;
    result->offset = 0;

    result->command = command;
    result->size = 1;
    result->time = time;

    return result;
}

VGMCommand* VGMCommand_createEx(unsigned char* data, int offset, int time)
{
    VGMCommand* result;

    result = malloc(sizeof(VGMCommand));

    result->data = data;
    result->offset = offset;

    result->command = data[offset] & 0xFF;
    result->size = VGMCommand_computeSize(result);
    result->time = time;

    return result;
}

bool VGMCommand_isDataBlock(VGMCommand* source)
{
    return source->command == VGM_DATA_BLOCK;
}

int VGMCommand_getDataBankId(VGMCommand* source)
{
    return source->data[source->offset + 2];
}

int VGMCommand_getDataBlockLen(VGMCommand* source)
{
    return getInt(source->data, source->offset + 3);
}

bool VGMCommand_isSeek(VGMCommand* source)
{
    return source->command == VGM_SEEK;
}

int VGMCommand_getSeekAddress(VGMCommand* source)
{
    if (VGMCommand_isSeek(source))
        return getInt(source->data, source->offset + 0x01);

    return -1;
}

bool VGMCommand_isEnd(VGMCommand* source)
{
    return source->command == VGM_END;
}

bool VGMCommand_isLoopStart(VGMCommand* source)
{
    return source->command == VGM_LOOP_START;
}

bool VGMCommand_isLoopEnd(VGMCommand* source)
{
    return source->command == VGM_LOOP_END;
}

bool VGMCommand_isPCM(VGMCommand* source)
{
    return (source->command & 0xF0) == 0x80;
}

bool VGMCommand_isWait(VGMCommand* source)
{
    if (VGMCommand_isShortWait(source))
        return true;

    return ((source->command >= 0x61) && (source->command <= 0x63));
}

bool VGMCommand_isWaitNTSC(VGMCommand* source)
{
    return (source->command == 0x62);
}

bool VGMCommand_isWaitPAL(VGMCommand* source)
{
    return (source->command == 0x63);
}

bool VGMCommand_isShortWait(VGMCommand* source)
{
    return (source->command & 0xF0) == 0x70;
}

int VGMCommand_getWaitValue(VGMCommand* source)
{
    if (VGMCommand_isShortWait(source))
        return (source->command & 0x0F) + 1;

    if (VGMCommand_isPCM(source))
        return source->command & 0x0F;

    switch (source->command)
    {
        case 0x61:
            return getInt16(source->data, source->offset + 0x01);

        case 0x62:
            return 0x2DF;

        case 0x63:
            return 0x372;
    }

    return 0;
}

int VGMCommand_computeSize(VGMCommand* source)
{
    switch (source->command)
    {
        case 0x4F:
        case 0x50:
            return 2;

        case 0x51:
        case 0x52:
        case 0x53:
        case 0x54:
        case 0x55:
        case 0x56:
        case 0x57:
        case 0x58:
        case 0x59:
        case 0x5A:
        case 0x5B:
        case 0x5C:
        case 0x5D:
        case 0x5E:
        case 0x5F:
            return 3;

        case 0x61:
            return 3;

        case 0x62:
        case 0x63:
            return 1;

        case 0x66:
            return 1;

        case 0x67:
            // data block start
            return 7 + getInt(source->data, source->offset + 0x03);

        case 0x68:
            // write data block start
            return 12 + getInt24(source->data, source->offset + 0x09);

        case 0x90:
            return 5;

        case 0x91:
            return 5;

        case 0x92:
            return 6;

        case 0x93:
            return 11;

        case 0x94:
            return 2;

        case 0x95:
            return 5;
    }

    switch (source->command >> 4)
    {
        case 0x3:
            return 2;

        case 0x4:
            return 3;

        case 0x7:
            return 1;

        case 0x8:
            return 1;

        case 0xA:
            return 3;

        case 0xB:
            return 3;

        case 0xC:
            return 4;

        case 0xD:
            return 4;

        case 0xE:
            return 5;

        case 0xF:
            return 5;
    }

    return 1;
}

unsigned char* VGMCommand_asByteArray(VGMCommand* source)
{
    unsigned char* result = malloc(source->size);

    if (source->data == NULL)
        result[0] = source->command;
    else
    {
        int i;

        for (i = 0; i < source->size; i++)
            result[i] = source->data[i + source->offset];
    }

    return result;
}

bool VGMCommand_isPSGWrite(VGMCommand* source)
{
    return (source->command == VGM_WRITE_SN76489);
}

bool VGMCommand_isPSGEnvWrite(VGMCommand* source)
{
    return (source->command == VGM_WRITE_SN76489) && ((VGMCommand_getPSGValue(source) & 0x91) == 0x91);
}

bool VGMCommand_isPSGToneWrite(VGMCommand* source)
{
    return VGMCommand_isPSGWrite(source) && !VGMCommand_isPSGEnvWrite(source);
}

int VGMCommand_getPSGValue(VGMCommand* source)
{
    if (VGMCommand_isPSGWrite(source))
        return source->data[source->offset + 1] & 0xFF;

    return -1;
}

bool VGMCommand_isYM2612Port0Write(VGMCommand* source)
{
    return (source->command == VGM_WRITE_YM2612_PORT0);
}

bool VGMCommand_isYM2612Port1Write(VGMCommand* source)
{
    return (source->command == VGM_WRITE_YM2612_PORT1);
}

bool VGMCommand_isYM2612Write(VGMCommand* source)
{
    return VGMCommand_isYM2612Port0Write(source) || VGMCommand_isYM2612Port1Write(source);
}

int VGMCommand_getYM2612Port(VGMCommand* source)
{
    if (VGMCommand_isYM2612Port0Write(source))
        return 0;

    if (VGMCommand_isYM2612Port1Write(source))
        return 1;

    return -1;
}

int VGMCommand_getYM2612Channel(VGMCommand* source)
{
    if (VGMCommand_isYM2612Port0Write(source))
        return (VGMCommand_getYM2612Register(source) & 3);

    if (VGMCommand_isYM2612Port1Write(source))
        return (VGMCommand_getYM2612Register(source) & 3) + 3;

    return -1;
}

int VGMCommand_getYM2612Register(VGMCommand* source)
{
    if (VGMCommand_isYM2612Write(source))
        return source->data[source->offset + 1] & 0xFF;

    return -1;
}

int VGMCommand_getYM2612Value(VGMCommand* source)
{
    if (VGMCommand_isYM2612Write(source))
        return source->data[source->offset + 2] & 0xFF;

    return -1;
}

bool VGMCommand_isYM2612KeyWrite(VGMCommand* source)
{
    return VGMCommand_isYM2612Port0Write(source) && (VGMCommand_getYM2612Register(source) == 0x28);
}

bool VGMCommand_isYM2612KeyOffWrite(VGMCommand* source)
{
    return VGMCommand_isYM2612KeyWrite(source) && ((VGMCommand_getYM2612Value(source) & 0xF0) == 0x00);
}

bool VGMCommand_isYM2612KeyOnWrite(VGMCommand* source)
{
    return VGMCommand_isYM2612KeyWrite(source) && ((VGMCommand_getYM2612Value(source) & 0xF0) != 0x00);
}

int VGMCommand_getYM2612KeyChannel(VGMCommand* source)
{
    if (VGMCommand_isYM2612KeyWrite(source))
    {
        int reg = VGMCommand_getYM2612Value(source) & 0x7;

        if ((reg == 3) || (reg == 7)) return -1;

        if (reg >= 4) reg--;

        return reg;
    }

    return -1;
}

bool VGMCommand_isYM26120x2XWrite(VGMCommand* source)
{
    return VGMCommand_isYM2612Port0Write(source) && ((VGMCommand_getYM2612Register(source) & 0xF0) == 0x20);
}

bool VGMCommand_isYM2612TimersWrite(VGMCommand* source)
{
    return VGMCommand_isYM2612Port0Write(source) && (VGMCommand_getYM2612Register(source) == 0x27);
}

bool VGMCommand_isYM2612TimersNoSpecialNoCSMWrite(VGMCommand* source)
{
    return VGMCommand_isYM2612TimersWrite(source) && ((VGMCommand_getYM2612Value(source) & 0xC0) == 0x00);
}

bool VGMCommand_isDACEnabled(VGMCommand* source)
{
    return VGMCommand_isYM2612Port0Write(source) && (VGMCommand_getYM2612Register(source) == 0x2B);
}

bool VGMCommand_isDACEnabledON(VGMCommand* source)
{
    return VGMCommand_isDACEnabled(source) && ((VGMCommand_getYM2612Value(source) & 0x80) == 0x80);
}

bool VGMCommand_isDACEnabledOFF(VGMCommand* source)
{
    return VGMCommand_isDACEnabled(source) && ((VGMCommand_getYM2612Value(source) & 0x80) == 0x00);
}

bool VGMCommand_isStream(VGMCommand* source)
{
    return VGMCommand_isStreamControl(source) || VGMCommand_isStreamData(source) || VGMCommand_isStreamFrequency(source) || VGMCommand_isStreamStart(source) || VGMCommand_isStreamStartLong(source)
           || VGMCommand_isStreamStop(source);
}

bool VGMCommand_isStreamControl(VGMCommand* source)
{
    return (source->command == VGM_STREAM_CONTROL);
}

bool VGMCommand_isStreamData(VGMCommand* source)
{
    return (source->command == VGM_STREAM_DATA);
}

bool VGMCommand_isStreamFrequency(VGMCommand* source)
{
    return (source->command == VGM_STREAM_FREQUENCY);
}

bool VGMCommand_isStreamStart(VGMCommand* source)
{
    return (source->command == VGM_STREAM_START);
}

bool VGMCommand_isStreamStartLong(VGMCommand* source)
{
    return (source->command == VGM_STREAM_START_LONG);
}

bool VGMCommand_isStreamStop(VGMCommand* source)
{
    return (source->command == VGM_STREAM_STOP);
}

int VGMCommand_getStreamId(VGMCommand* source)
{
    if (VGMCommand_isStream(source))
        return source->data[source->offset + 1] & 0xFF;

    return -1;
}

int VGMCommand_getStreamBankId(VGMCommand* source)
{
    if (VGMCommand_isStreamData(source))
        return source->data[source->offset + 2] & 0xFF;

    return -1;
}

int VGMCommand_getStreamBlockId(VGMCommand* source)
{
    if (VGMCommand_isStreamStart(source))
        return source->data[source->offset + 2] & 0xFF;

    return -1;
}

int VGMCommand_getStreamFrenquency(VGMCommand* source)
{
    if (VGMCommand_isStreamFrequency(source))
        return getInt(source->data, source->offset + 2);

    return -1;
}

int VGMCommand_getStreamSampleAddress(VGMCommand* source)
{
    if (VGMCommand_isStreamStartLong(source))
        return getInt(source->data, source->offset + 2);

    return -1;
}

int VGMCommand_getStreamSampleSize(VGMCommand* source)
{
    if (VGMCommand_isStreamStartLong(source))
        return getInt(source->data, source->offset + 7);

    return -1;
}

bool VGMCommand_isSame(VGMCommand* source, VGMCommand* com)
{
    if (source->data == NULL)
        return (com->data == NULL) && (source->command == com->command);

    if (com->data == NULL)
        return false;

    if (source->size != com->size)
        return false;

    // compare data
    return !memcmp(&(source->data[source->offset]), &(com->data[com->offset]), source->size);
}

bool VGMCommand_contains(LList* commands, VGMCommand* command)
{
    LList* l = commands;

    while(l != NULL)
    {
        if (VGMCommand_isSame(l->element, command))
            return true;

        l = l->next;
    }

    return false;
}

VGMCommand* VGMCommand_getKeyOnCommand(LList* commands, int channel)
{
    LList* l = commands;

    while(l != NULL)
    {
        VGMCommand* command = l->element;

        if (VGMCommand_isYM2612KeyOnWrite(command) && (VGMCommand_getYM2612KeyChannel(command) == channel))
            return command;

        l = l->next;
    }

    return NULL;
}

VGMCommand* VGMCommand_getKeyOffCommand(LList* commands, int channel)
{
    LList* l = commands;

    while(l != NULL)
    {
        VGMCommand* command = l->element;

        if (VGMCommand_isYM2612KeyOffWrite(command) && (VGMCommand_getYM2612KeyChannel(command) == channel))
            return command;

        l = l->next;
    }

    return NULL;
}

VGMCommand* VGMCommand_getKeyCommand(LList* commands, int channel)
{
    LList* l = commands;

    while(l != NULL)
    {
        VGMCommand* command = l->element;

        if (VGMCommand_isYM2612KeyWrite(command) && (VGMCommand_getYM2612KeyChannel(command) == channel))
            return command;

        l = l->next;
    }

    return NULL;
}

VGMCommand* VGMCommand_createYMCommand(int port, int reg, int value)
{
    VGMCommand* result = malloc(sizeof(VGMCommand));

    if (port == 0)
        result->command = VGM_WRITE_YM2612_PORT0;
    else
        result->command = VGM_WRITE_YM2612_PORT1;

    result->data = malloc(2);
    result->data[0] = result->command;
    result->data[1] = reg;
    result->data[2] = value;
    result->offset = 0;
    result->size = 3;

    return result;
}

LList* VGMCommand_createYMCommands(int port, int baseReg, int value)
{
    LList* result;
    int ch, op;

    result = NULL;

    for (ch = 0; ch < 3; ch++)
        for (op = 0; op < 4; op++)
            result = insertAfterLList(result, VGMCommand_createYMCommand(port, baseReg + ((op & 3) << 2) + (ch & 3), value));

    return getHeadLList(result);
}
