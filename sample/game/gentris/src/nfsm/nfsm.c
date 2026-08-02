#include <types.h>
#include "nfsm.h"


// Enter a new state without calling OnExit of the previous state.
void NFSM_EnterStateWOExit(NFSM* fsm, const NFSM_State* state)
{
    if (state == NULL || fsm == NULL)
        return;
    
    fsm->prevState = fsm->state;
    fsm->state = state;
    
    if (state->OnEnter)
        state->OnEnter();
    
    if (!state->OnUpdate)
        NFSM_TransitToNextState(fsm);
}

// Set the next state for the FSM.
void NFSM_TransitToState(NFSM* fsm, const NFSM_State* state)
{
    if (state == NULL || fsm == NULL)
        return;
    
    if (fsm->state)
    {
        if (fsm->state->OnExit)
            fsm->state->OnExit();
    }
    
    NFSM_EnterStateWOExit(fsm, state);
}


void NFSM_TransitToNextState(NFSM* fsm)
{
    if (fsm == NULL)
        return;
    
    if (fsm->state)
    {
        if (fsm->state->nextState)
            NFSM_TransitToState(fsm, fsm->state->nextState);
        else
            NFSM_ExitAndStopFSM(fsm);
    }
}

void NFSM_ExitAndStopFSM(NFSM* fsm)
{
    if (fsm == NULL)
        return;
    
    if (fsm->state)
    {
        if (fsm->state->OnExit)
            fsm->state->OnExit();
    }
    
    fsm->prevState = NULL;
    fsm->state = NULL;
    fsm = NULL;

}

// Get the current state for the FSM.
const NFSM_State* NFSM_GetState(NFSM* fsm)
{

    return fsm->state;
}


// GameplayState_OnUpdate the current state and handle transitions.
void NFSM_Update(NFSM* fsm)
{

    if (!fsm || !fsm->state)
        return;
    
    if (fsm->state->OnUpdate)
        fsm->state->OnUpdate();
}

// Call the vertical blank handler of the current state.
void NFSM_OnVerticalBlank(NFSM* fsm)
{

    if (fsm->state && fsm->state->OnVerticalBlank)
        fsm->state->OnVerticalBlank();
}

// Call the horizontal blank handler of the current state.
void NFSM_OnHorizontalBlank(NFSM* fsm)
{

    if (fsm->state && fsm->state->OnHorizontalBlank)
        fsm->state->OnHorizontalBlank();
}
