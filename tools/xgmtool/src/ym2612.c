#include <stdlib.h>
#include <stdbool.h>

#include "../inc/vgmcom.h"
#include "../inc/ym2612.h"

#define DUALS_SIZE      7


const static int duals[DUALS_SIZE][2] =
{
    {0x24, 0x25}, {0xA4, 0xA0}, {0xA5, 0xA1}, {0xA6, 0xA2}, {0xAC, 0xA8}, {0xAD, 0xA9},
    {0xAE, 0xAA}
};


YM2612* YM2612_create()
{
    YM2612* result;

    result = malloc(sizeof(YM2612));

    YM2612_clear(result);

    return result;
}

YM2612* YM2612_copy(YM2612* source)
{
    YM2612* result;
    int i;

    result = YM2612_create();

    for (i = 0; i < 0x100; i++)
    {
        result->registers[0][i] = source->registers[0][i];
        result->registers[1][i] = source->registers[1][i];
        result->init[0][i] = source->init[0][i];
        result->init[1][i] = source->init[1][i];
    }

    return result;
}

void YM2612_clear(YM2612* source)
{
    int i;

    for (i = 0; i < 0x20; i++)
    {
        source->registers[0][i] = -1;
        source->registers[1][i] = -1;
        source->init[0][i] = false;
        source->init[1][i] = false;
    }
    for (i = 0x20; i < 0x100; i++)
    {
        source->registers[0][i] = -1;
        source->registers[1][i] = -1;
        source->init[0][i] = false;
        source->init[1][i] = false;
    }
}

void YM2612_initialize(YM2612* source)
{
    int i;

    for (i = 0; i < 0x20; i++)
    {
        source->registers[0][i] = 0;
        source->registers[1][i] = 0;
        source->init[0][i] = true;
        source->init[1][i] = true;
    }
    for (i = 0x20; i < 0x100; i++)
    {
        source->registers[0][i] = 0xFF;
        source->registers[1][i] = 0xFF;
        source->init[0][i] = true;
        source->init[1][i] = true;
    }
}

int YM2612_get(YM2612* source, int port, int reg)
{
    if (YM2612_canIgnore(port, reg))
        return 0;

    return source->registers[port][reg];
}

bool YM2612_set(YM2612* source, int port, int reg, int value)
{
    if (YM2612_canIgnore(port, reg))
        return false;

    int newValue = value;

    if (port == 0)
    {
        // special case of KEY ON/OFF register
        if (reg == 0x28)
        {
            int oldValue = source->registers[port][value & 7];
            newValue &= 0xF0;

            if (oldValue != newValue)
            {
                source->registers[port][value & 7] = newValue;
//                source->init[port][value & 7] = true;

                // always write when key state change
                return true;
            }

            return false;
        }

        // special case of Timer register --> ignore useless bits
        if (reg == 0x27)
            newValue &= 0xC0;
    }

    source->registers[port][reg] = newValue;
    source->init[port][reg] = true;

    return false;
}

bool YM2612_isSame(YM2612* source, YM2612* state, int port, int reg)
{
    if ((source->init[port][reg] == false) && (state->init[port][reg] == false))
        return true;

    return (source->init[port][reg] || YM2612_canIgnore(port, reg)) && (YM2612_get(source, port, reg) == YM2612_get(state, port, reg));
}

bool YM2612_isDiff(YM2612* source, YM2612* state, int port, int reg)
{
    return !YM2612_isSame(source, state, port, reg);
}


/**
 * Returns commands list to update to the specified YM2612 state
 */
List* YM2612_getDelta(YM2612* source, YM2612* state)
{
    List* result;
    int i, port, reg;

    result = createList();

    // do dual reg first
    for (i = 0; i < DUALS_SIZE; i++)
    {
        const int* dual = duals[i];
        const int reg0 = dual[0];
        const int reg1 = dual[1];

        // value is different
        if (YM2612_isDiff(source, state, 0, reg0) || YM2612_isDiff(source, state, 0, reg1))
        {
            // add commands
            addToList(result, VGMCommand_createYMCommand(0, reg0, YM2612_get(state, 0, reg0)));
            addToList(result, VGMCommand_createYMCommand(0, reg1, YM2612_get(state, 0, reg1)));
        }
        // port 1 too ?
        if (dual[0] > 0x30)
        {
            if (YM2612_isDiff(source, state, 1, reg0) || YM2612_isDiff(source, state, 1, reg1))
            {
                // add commands
                addToList(result, VGMCommand_createYMCommand(1, reg0, YM2612_get(state, 1, reg0)));
                addToList(result, VGMCommand_createYMCommand(1, reg1, YM2612_get(state, 1, reg1)));
            }
        }
    }

    for (port = 0; port < 2; port++)
    {
        for (reg = 0; reg < 0x100; reg++)
        {
            // can ignore or special case of KEY ON/OFF register
            if (YM2612_canIgnore(port, reg) || ((port == 0) && (reg == 0x28)))
                continue;

            // ignore dual reg
            if (YM2612_getDualReg(reg) != NULL)
                continue;

            // value is different --> add command
            if (YM2612_isDiff(source, state, port, reg))
                addToList(result, VGMCommand_createYMCommand(port, reg, YM2612_get(state, port, reg)));
        }
    }

    return result;
}

/**
 * Return true if write can be ignored
 */
bool YM2612_canIgnore(int port, int reg)
{
    switch (reg)
    {
        case 0x22:
        case 0x24:
        case 0x25:
        case 0x26:
        case 0x27:
        case 0x28:
        case 0x2B:
            return (port == 1);
    }

    if ((reg >= 0x30) && (reg < 0xB8))
        return ((reg & 3) == 3);

    return true;
}


/**
 * Return the dual entry for this reg (or NULL if no dual entry)
 */
int* YM2612_getDualReg(int reg)
{
    int i;

    for (i = 0; i < DUALS_SIZE; i++)
    {
        const int* dual = duals[i];

        if ((dual[0] == reg) || (dual[1] == reg))
            return (int*) dual;
    }

    return NULL;
}
