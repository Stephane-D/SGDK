#include "input.h"
#include "global.h"
#include "menu_man.h"
#include "figure.h"


#define INPUT_CHARGE_FRAMES                     9
#define BUTTON_DOUBLE_TAP_FRAMES_AWAIT          10

// Button mappings to SGDK constants
#define BTN_L   BUTTON_LEFT
#define BTN_R   BUTTON_RIGHT
#define BTN_U   BUTTON_UP
#define BTN_D   BUTTON_DOWN
#define BTN_M   BUTTON_MODE
#define BTN_S   BUTTON_START
#define BTN_A   BUTTON_A
#define BTN_B   BUTTON_B
#define BTN_C   BUTTON_C
#define BTN_X   BUTTON_X
#define BTN_Y   BUTTON_Y
#define BTN_Z   BUTTON_Z

// Macro to update the state of a specific button in the Joypad struct
// Uses token concatenation to access struct members (e.g., joy->pressed.A)
#define UPDATE_BUTTON_STATE(BTN_NAME, SGDK_BTN_MASK)                        \
    do {                                                                    \
        if (state & changed & (SGDK_BTN_MASK)) /* Pressed */                  \
        {                                                                   \
            joy->pressed.BTN_NAME = true;                                   \
            joy->released.BTN_NAME = false;                                 \
            joy->holding.BTN_NAME = true;                                   \
            joy->charged.BTN_NAME = false;                                  \
            joy->ticksOfCharge_.BTN_NAME = INPUT_CHARGE_FRAMES;             \
            if (joy->ticksOfRelease_.BTN_NAME)                              \
                joy->doubleTaped.BTN_NAME = true;                           \
        }                                                                   \
        else if (~state & changed & (SGDK_BTN_MASK)) /* Released */           \
        {                                                                   \
            joy->pressed.BTN_NAME = false;                                  \
            joy->released.BTN_NAME = true;                                  \
            joy->holding.BTN_NAME = false;                                  \
            joy->doubleTaped.BTN_NAME = false;                              \
            joy->ticksOfRelease_.BTN_NAME = BUTTON_DOUBLE_TAP_FRAMES_AWAIT; \
        }                                                                   \
    } while(0)

// Macro to handle charging logic (holding a button down)
#define UPDATE_BUTTON_CHARGE(BTN_NAME)                                      \
    do {                                                                    \
        if (joy->holding.BTN_NAME && joy->ticksOfCharge_.BTN_NAME > 0)      \
        {                                                                   \
            joy->ticksOfCharge_.BTN_NAME--;                                 \
            if (joy->ticksOfCharge_.BTN_NAME == 0)                          \
            {                                                               \
                joy->charged.BTN_NAME = true;                               \
                /* Note: Resetting global state here might be dangerous */    \
                Joy_UpdateState(0, 0, 0);                                   \
                Input_OnTriggersUpdate();                                    \
            }                                                               \
        }                                                                   \
    } while(0)

// Macro to handle double tap timer decrement
#define UPDATE_DOUBLE_TAP_TIMER(BTN_NAME)                                   \
    do {                                                                    \
        if (!joy->holding.BTN_NAME && joy->ticksOfRelease_.BTN_NAME)        \
        {                                                                   \
            joy->ticksOfRelease_.BTN_NAME--;                                \
        }                                                                   \
    } while(0)

// Macro to reset charging
#define RESET_BUTTON_CHARGE(BTN_NAME, DELAY)                                 \
    do {                                                                     \
        joy->charged.BTN_NAME = false;                                       \
        joy->ticksOfCharge_.BTN_NAME = DELAY;                                \
    } while(0)


void Input_OnTriggersUpdate();
void UpdateHoldingInput(Joypad* joy);
void Joy_UpdateState(u16 joyIndex, u16 changed, u16 state);
void HandleTriggersInGame(Joypad* joy);
void Joy_ResetButtonsStates(u8 joyIndex);


// Input callback for in-game controller events - update input state and handle button triggers
void OnInputCallback_InGame(u16 joyIndex, u16 changed, u16 state)
{
    Joy_UpdateState(joyIndex, changed, state);
    Input_OnTriggersUpdate();
}

// GameplayState_OnUpdate button states based on hardware input changes (pressed, released, holding)
void Joy_UpdateState(u16 joyIndex, u16 changed, u16 state)
{
    // We only care about Controller 1 for now
    if (joyIndex != JOY_1) return;

    u16 idx = JOY_1_INDEX;
    Joypad* joy = &g_joypad[idx];

    // Reset transient states (Pressed/Released are valid for 1 frame only)
    Joy_ResetButtonsStates(idx);

    // D-Pad
    UPDATE_BUTTON_STATE(Right, BTN_R);
    UPDATE_BUTTON_STATE(Left, BTN_L);
    UPDATE_BUTTON_STATE(Up, BTN_U);
    UPDATE_BUTTON_STATE(Down, BTN_D);

    // Action Buttons
    UPDATE_BUTTON_STATE(A, BTN_A);
    UPDATE_BUTTON_STATE(B, BTN_B);
    UPDATE_BUTTON_STATE(C, BTN_C);
    UPDATE_BUTTON_STATE(X, BTN_X);
    UPDATE_BUTTON_STATE(Y, BTN_Y);
    UPDATE_BUTTON_STATE(Z, BTN_Z);

    // System Buttons
    UPDATE_BUTTON_STATE(Mode, BTN_M);
    UPDATE_BUTTON_STATE(Start, BTN_S);
}

// GameplayState_OnUpdate holding/charging state of buttons and handle repeated inputs (like DAS)
void Input_HoldingUpdate()
{
    Joypad* joy = &g_joypad[JOY_1_INDEX];

    // GameplayState_OnUpdate Charge Timers
    UPDATE_BUTTON_CHARGE(Right);
    UPDATE_BUTTON_CHARGE(Left);
    UPDATE_BUTTON_CHARGE(Up);
    UPDATE_BUTTON_CHARGE(Down);

    UPDATE_BUTTON_CHARGE(A);
    UPDATE_BUTTON_CHARGE(B);
    UPDATE_BUTTON_CHARGE(C);
    UPDATE_BUTTON_CHARGE(X);
    UPDATE_BUTTON_CHARGE(Y);
    UPDATE_BUTTON_CHARGE(Z);
    UPDATE_BUTTON_CHARGE(Mode);
    UPDATE_BUTTON_CHARGE(Start);

    // GameplayState_OnUpdate Double Tap Timers
    UPDATE_DOUBLE_TAP_TIMER(Left);
    UPDATE_DOUBLE_TAP_TIMER(Right);

    // GameplayState_OnUpdate Physics Direction
    UpdateHoldingInput(joy);
}

// Process button triggers and handle game actions (move, rotate, drop)
void Input_OnTriggersUpdate()
{
    Joypad* joy = &g_joypad[JOY_1_INDEX];
    HandleTriggersInGame(joy);
}

// Handle in-game button presses - movement, rotation, dropping
void HandleTriggersInGame(Joypad* joy)
{
    if (!Input_IsEnabled())
        return;

    if (joy->pressed.Start && (g_inGameMenu.desk.state == UI_HIDDEN))
        Menu_InGame_Show();

    // Handle left/right movement
    if (joy->pressed.Left)
        Figure_TryMoveTo(-1);

    if (joy->pressed.Right)
        Figure_TryMoveTo(1);

    // Soft drop on down button
    if (joy->pressed.Down)
        g_input.dropType = SOFT_DROP;
    else if (joy->released.Down)
        g_input.dropType = NO_DROP;

    // Hard drop on up button
    if (joy->pressed.Up)
        Figure_DropHard();

    // Rotate clockwise (A button)
    if (joy->pressed.A)
    {
        Figure tempFig = g_activeFig;
        g_activeFig.rot = (g_activeFig.rot + 1) & 3;

        FigCollision figCol = Figure_GetCollision(&g_activeFig);

        // Adjust position if rotation caused wall collision (wall kick)
        if (figCol.dir == DIR_RIGHT)
            g_activeFig.x--;
        if (figCol.dir == DIR_LEFT)
            g_activeFig.x++;

        // Revert rotation if collision still exists
        if (Figure_IsCollided(&g_activeFig))
            g_activeFig = tempFig;
    }

    // Rotate counter-clockwise (B button)
    if (joy->pressed.B)
    {
        Figure tempFig = g_activeFig;
        g_activeFig.rot = (g_activeFig.rot + 3) & 3;

        FigCollision figCol = Figure_GetCollision(&g_activeFig);

        // Adjust position if rotation caused wall collision (wall kick)
        if (figCol.dir == DIR_RIGHT)
            g_activeFig.x--;
        if (figCol.dir == DIR_LEFT)
            g_activeFig.x++;

        // Revert rotation if collision still exists
        if (Figure_IsCollided(&g_activeFig))
            g_activeFig = tempFig;
    }
}

// Handle continuous input - repeated movement while holding button (DAS - Delayed Auto Shift)
void UpdateHoldingInput(Joypad* joy)
{
    if (!Input_IsEnabled())
        return;

    if (joy->charged.Left)
    {
        Figure_TryMoveTo(-1);
        RESET_BUTTON_CHARGE(Left, 3);
    }

    if (joy->charged.Right)
    {
        Figure_TryMoveTo(1);
        RESET_BUTTON_CHARGE(Right, 3);
    }
}

// Clear single-frame transient button states (pressed and released)
void Joy_ResetButtonsStates(u8 joyIndex)
{
    Joypad* joy = &g_joypad[joyIndex];
    memset(&joy->pressed, 0, sizeof(joy->pressed));
    memset(&joy->released, 0, sizeof(joy->released));
}

// Reset transient button states
void Joy_ResetStates(u8 joyIndex)
{
    Joy_ResetButtonsStates(joyIndex);
}

// Reset all button states including holding states
void Input_ResetAllJoyStates(u8 joyIndex)
{
    Joypad* joy = &g_joypad[joyIndex];
    
    Joy_ResetStates(joyIndex);
    memset(&g_joypad[joyIndex].holding, 0, sizeof(g_joypad[joyIndex].holding));
    
    RESET_BUTTON_CHARGE(Left, INPUT_CHARGE_FRAMES);
    RESET_BUTTON_CHARGE(Right, INPUT_CHARGE_FRAMES);
}

// Enable game input processing
void Input_Enable()
{
    g_input.enabled = true;
}

// Disable game input processing (used during menus and transitions)
void Input_Disable()
{
    g_input.enabled = false;
}

// Check if game input is currently enabled
bool Input_IsEnabled()
{
    return g_input.enabled;
}

// Input callback for any menu
void OnMenuInputCallback(u16 joy, u16 changed, u16 state)
{
    if (joy != JOY_1) return;

    if (Input_IsEnabled())
        Menu_OnInputCallback(g_currentMenu, joy, changed, state);
    
    // Toggle menu off if in-game menu is active
    if (g_currentMenu == &g_inGameMenu)
    {
        if (changed & state & BUTTON_START)
            Menu_Hide(g_currentMenu);
    }
    
    // If menu is active, consume all input (don't pass to player)
    if (g_currentMenu->desk.state == UI_SHOWED)
        Input_ResetAllJoyStates(JOY_1_INDEX);
}

