#ifndef HEADER_INPUT
#define HEADER_INPUT

#include <types.h>

#define JOY_1_INDEX                             0

struct Dpad
{
    s8 dirX;
    s8 dirY;
};

struct Button
{
    s8 Left, Right, Up, Down, A, B, C, X, Y, Z, Mode, Start;
};

typedef struct
{
    struct Dpad dpad;
    struct Button pressed;
    struct Button released;
    struct Button holding;
    struct Button charged;
    struct Button doubleTaped;
    struct Button ticksOfCharge_;
    struct Button ticksOfRelease_;
} Joypad;

void OnMenuInputCallback(u16 joy, u16 changed, u16 state);
void OnInputCallback_InGame(u16 joypad, u16 changed, u16 state);

bool Input_IsEnabled();
void Input_Enable();
void Input_Disable();
void Input_HoldingUpdate();
void Input_ResetAllJoyStates(u8 joyIndex);

#endif // HEADER_INPUT
