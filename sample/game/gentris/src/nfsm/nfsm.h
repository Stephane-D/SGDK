#ifndef HEADER_NFSM_NFSM
#define HEADER_NFSM_NFSM

typedef struct NFSM_State NFSM_State;
// Function pointer type for callbacks with no parameters and no return value.
typedef void VoidCallBack();

// Structure describing a finite state machine state.
// Contains pointers to callback functions for state events and transition parameters.
typedef struct NFSM_State
{
    VoidCallBack* OnEnter;            // Called when entering the state.
    VoidCallBack* OnUpdate;           // Called when updating the state.
    VoidCallBack* OnExit;             // Called when exiting the state.
    VoidCallBack* OnVerticalBlank;    // Called on vertical blank (VBlank).
    VoidCallBack* OnHorizontalBlank;  // Called on horizontal blank (HBlank).
    int TransitionIn;                 // Transition-in parameter.
    int TransitionOut;                // Transition-out parameter.
    NFSM_State* nextState;
} NFSM_State;

// Structure representing the finite state machine.
// Stores information about the current, previous, and next states,
// as well as transition frame parameters.
typedef struct
{
    int TransitionOutFrames;            // Number of frames for state exit.
    int TransitionInFrames;             // Number of frames for state entry.
    const NFSM_State* state;            // Current state.
    const NFSM_State* prevState;        // Previous state.
    // const NFSM_State *nextState;      // Next state.
} NFSM;

// Enter a state without calling OnExit of the previous state.
void NFSM_EnterStateWOExit(NFSM* fsm, const NFSM_State* state);

// Set the next state for the FSM.
void NFSM_TransitToState(NFSM* fsm, const NFSM_State* state);

void NFSM_TransitToNextState(NFSM* fsm);

void NFSM_ExitAndStopFSM(NFSM* fsm);

// Get the current state for the FSM.
const NFSM_State* NFSM_GetState(NFSM* fsm);

// Update the current state of the FSM.
void NFSM_Update(NFSM* fsm);

// Call the vertical blank handler of the current state.
void NFSM_OnVerticalBlank(NFSM* fsm);

// Call the horizontal blank handler of the current state.
void NFSM_OnHorizontalBlank(NFSM* fsm);

#endif // HEADER_NFSM_SFSM