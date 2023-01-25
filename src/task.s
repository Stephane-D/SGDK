#include "task_cst.h"

#include "asm_mac.i"

// required by the V-Int handler of sega.s boot file (probable need a better way of doing that)
    .globl  task_sr
    .globl  task_pc
    .globl  task_regs
    .globl  task_lock

.data

/****************************************************************************
 * Variables needed for the task context switches
 ****************************************************************************/

// User task status register. Initial value of second nibble must be 5 or
// lower in order for VINT interrupts to fire. First nibble must be 0 in
// order for the user task to run.
task_sr: .word 0x0400

// User task program counter
task_pc: .long 0x00000000

    .align 2

// User task registers saved on context switch
task_regs: .fill UTSK_REGS_LEN, 1, 0

// Supervisor task lock
task_lock: .word 0


    .section .text

/**
 * Initialize the task sub system (reset internal variables)
 */
func TSK_init
        move.w  #0x0400, task_sr
        move.l  #0x00000000, task_pc
        move.w  #0x0000, task_lock

        move.w  #(UTSK_REGS_LEN - 1), %d0
        moveq   #0, %d1
        lea     task_regs,%a0

.loop:
        move.b  %d1, (%a0)+
        dbra    %d0, .loop

        rts

/**
 * Configure the task used as user task. Must be invoked once before calling
 * TSK_userYield().
 *
 * Receives a parameter with the pointer to the user task.
 */
func TSK_userSet
        move.l  4(%sp), task_pc
        rts

/**
 * Stop the user task.
 *  This has the same effect than using TSK_setUser(NULL).
 */
func TSK_stop
        move.l  #0, task_pc
        rts

/**
 * Resume a blocked supervisor task. Must be called from user task.
 */
func TSK_superPost
        tst.l task_pc
        beq.s no_task

        clr   task_lock
        tst.b 7(%sp)
        beq.s no_task

        /* Call supervisor for the context switch */
        trap  #0

no_task:
        rts

/**
 * Block supervisor task and resume user task. Supervisor task will not
 * resume execution until TSK_superPost() is called from user task.
 */
func TSK_superPend
        tst.l  task_pc
        beq.s  no_task

        /* Set lock and fallthrough to TSK_user_yield to switch to user task */
        move.w 6(%sp), task_lock
        bra.s  userYield

/**
 * TSK_userYield: yield from supervisor task to user task
 */
func TSK_userYield
        tst.l  task_pc
        beq.s  no_task

userYield:
        /* Push sr onto the stack. We already have there the pc, so we can */
        /* use rte in the _VINT code to go back to the supervisor task.    */
        move.w  %sr, %d0
        move.w  #0x2700, %sr
        move.w  %d0, -(%sp)

        /* Push non clobberable registers. Since user task can modify them,   */
        /* they need to be restored before resuming the supervisor task.      */
        /* a7 (the sp) should not need to be saved because when this function */
        /* exits, the usp will be used instead, and if any interrupt fires    */
        /* (so sp is used again), on return it should leave it where it was.  */
        movem.l %d2-%d7/%a2-%a6, -(%sp)

        /* Restore bg task registers. a7 does not need to be restored,        */
        /* since it is the usp and is restored when we go to user mode.       */
        lea     (task_regs + 4), %a0
        movem.l (%a0)+, %d0-%d7/%a1-%a6
        move.l  task_regs, %a0

        /* Enter the user task by pushing its pc and sr, and executing an rte.*/
        /* This will change context to user mode.                             */
        move.l  task_pc, -(%sp)
        move.w  task_sr, -(%sp)
        rte

/**
 * Trap #0. Unlocks supervisor task immediately (without waiting for VBLANK interrupt to occur).
 */
    .globl  _trap_0
_trap_0:
        move.l  %a0, task_regs
        lea     (task_regs + UTSK_REGS_LEN), %a0
        movem.l %d0-%d7/%a1-%a6, -(%a0)

        move.w  (%sp)+, task_sr
        move.l  (%sp)+, task_pc

        movem.l (%sp)+, %d2-%d7/%a2-%a6

        // For the pending task to return 0
        moveq   #0, %d0
        rte