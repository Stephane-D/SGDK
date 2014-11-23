#include <stdlib.h>

#include "../inc/psg.h"
#include "../inc/vgmcom.h"
#include "../inc/util.h"


// forward
static void PSG_writeLow(PSG* psg, int value);
static void PSG_writeHigh(PSG* psg, int value);
static VGMCommand* PSG_createLowWriteCommand(PSG* psg, int ind, int typ, int value);
static List* PSG_createWriteCommands(PSG* psg, int ind, int typ, int value);


PSG* PSG_create()
{
    PSG* result;

    result = malloc(sizeof(PSG));

    result->index = -1;
    result->type = -1;

    PSG_clear(result);

    return result;
}

PSG* PSG_copy(PSG* state)
{
    PSG* result;
    int i;

    result = malloc(sizeof(PSG));

    result->index = -1;
    result->type = -1;

    for (i = 0; i < 4; i++)
    {
        result->registers[i][0] = state->registers[i][0];
        result->registers[i][1] = state->registers[i][1];
        result->init[i][0] = state->init[i][0];
        result->init[i][1] = state->init[i][1];
    }

    return result;
}

void PSG_clear(PSG* psg)
{
    int i;

    for (i = 0; i < 4; i++)
    {
        psg->registers[i][0] = -1;
        psg->registers[i][1] = -1;
        psg->init[i][0] = false;
        psg->init[i][1] = false;
    }
}

int PSG_get(PSG* psg, int ind, int typ)
{
    switch (typ)
    {
        case 0:
            // tone / noise
            if (ind == 3)
                return psg->registers[ind][typ] & 0x7;
            else
                return psg->registers[ind][typ] & 0x3FF;

        case 1:
            // volume
            return psg->registers[ind][typ] & 0xF;
    }

    return 0;
}

void PSG_write(PSG* psg, int value)
{
    if ((value & 0x80) != 0)
        PSG_writeLow(psg, value & 0x7F);
    else
        PSG_writeHigh(psg, value & 0x7F);
}

static void PSG_writeLow(PSG* psg, int value)
{
    psg->index = (value >> 5) & 0x03;
    psg->type = (value >> 4) & 0X01;

    if ((psg->type == 0) && (psg->index == 3))
    {
        psg->registers[psg->index][psg->type] &= ~0x7;
        psg->registers[psg->index][psg->type] |= value & 0x7;
    }
    else
    {
        psg->registers[psg->index][psg->type] &= ~0xF;
        psg->registers[psg->index][psg->type] |= value & 0xF;
    }

    psg->init[psg->index][psg->type] = true;
}

static void PSG_writeHigh(PSG* psg, int value)
{
    if ((psg->type == 0) && (psg->index == 3))
    {
        psg->registers[psg->index][psg->type] &= ~0x7;
        psg->registers[psg->index][psg->type] |= value & 0x7;
    }
    else if (psg->type == 1)
    {
        psg->registers[psg->index][psg->type] &= ~0xF;
        psg->registers[psg->index][psg->type] |= value & 0xF;
    }
    else
    {
        psg->registers[psg->index][psg->type] &= ~0x3F0;
        psg->registers[psg->index][psg->type] |= (value & 0x3F) << 4;
    }

    psg->init[psg->index][psg->type] = true;
}

bool PSG_isSame(PSG* psg, PSG* state, int ind, int typ)
{
    if ((psg->init[ind][typ] == false) && (state->init[ind][typ] == false))
        return true;

    return psg->init[ind][typ] && (PSG_get(state, ind, typ) == PSG_get(psg, ind, typ));
}

bool PSG_isLowSame(PSG* psg, PSG* state, int ind, int typ)
{
    if ((psg->init[ind][typ] == false) && (state->init[ind][typ] == false))
        return true;

    return psg->init[ind][typ] && ((PSG_get(state, ind, typ) & 0xF) == (PSG_get(psg, ind, typ) & 0xF));
}

bool PSG_isHighSame(PSG* psg, PSG* state, int ind, int typ)
{
    if ((psg->init[ind][typ] == false) && (state->init[ind][typ] == false))
        return true;

    return psg->init[ind][typ] && ((PSG_get(state, ind, typ) & 0x3F0) == (PSG_get(psg, ind, typ) & 0x3F0));
}

bool PSG_isDiff(PSG* psg, PSG* state, int ind, int typ)
{
    return !PSG_isSame(psg, state, ind, typ);
}

bool PSG_isLowDiffOnly(PSG* psg, PSG* state, int ind, int typ)
{
    return !PSG_isLowSame(psg, state, ind, typ) && PSG_isHighSame(psg, state, ind, typ);
}

static VGMCommand* PSG_createLowWriteCommand(PSG* psg, int ind, int typ, int value)
{
    unsigned char* data = malloc(2);
    data[0] = VGM_WRITE_SN76489;
    data[1] = 0x80 | (ind << 5) | (typ << 4) | (value & 0xF);
    return VGMCommand_createEx(data, 0);
}

static List* PSG_createWriteCommands(PSG* psg, int ind, int typ, int value)
{
    unsigned char* data;
    List* result = createList();

    // rebuild data
    data = malloc(2);
    data[0] = VGM_WRITE_SN76489;
    data[1] = 0x80 | (ind << 5) | (typ << 4) | (value & 0xF);
    addToList(result, VGMCommand_createEx(data, 0));

    if ((typ == 0) && (ind != 3))
    {
        data = malloc(2);
        data[0] = VGM_WRITE_SN76489;
        data[1] = 0x00 | ((value >> 4) & 0x3F);
        addToList(result, VGMCommand_createEx(data, 0));
    }

    return result;
}

/**
 * Returns commands list to update to the specified PSG state
 */
List* PSG_getDelta(PSG* psg, PSG* state)
{
    int ind, typ;
    List* result = createList();

    for (ind = 0; ind < 4; ind++)
    {
        for (typ = 0; typ < 2; typ++)
        {
            if (typ == 0)
            {
                // value different on low bits only --> add single command
                if (PSG_isLowDiffOnly(psg, state, ind, typ))
                    addToList(result, PSG_createLowWriteCommand(psg, ind, typ, PSG_get(state, ind, typ)));
                // value is different --> add commands
                else if (PSG_isDiff(psg, state, ind, typ))
                    addAllToList(result, PSG_createWriteCommands(psg, ind, typ, PSG_get(state, ind, typ)));
            }
            else
            {
                // value is different --> add commands
                if (PSG_isDiff(psg, state, ind, typ))
                    addAllToList(result, PSG_createWriteCommands(psg, ind, typ, PSG_get(state, ind, typ)));
            }
        }
    }

    return result;
}
