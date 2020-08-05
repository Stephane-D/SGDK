
#include "sgdk_asm_macros.h"

func SYS_assertReset

    reset
    rts


func SYS_reset

    move   #0x2700,%sr
    move.l (0),%a7
    jmp     _reset_entry


func SYS_hardReset

    move   #0x2700,%sr
    move.l (0),%a7
    jmp    _start_entry


func SYS_getInterruptMaskLevel

    move.w  %sr,%d0
    andi.w  #0x0700,%d0
    lsr.w   #8,%d0
    rts


func SYS_setInterruptMaskLevel

    move.w  6(%sp),%d0                      | d0 = value
    andi.w  #0x07,%d0
    move.w  %d0,intLevelSave                | overwrite intLevelSave so we do not lost new interrupt mask
    ori.w   #0x20,%d0
    lsl.w   #8,%d0
    move.w  %d0,%sr
    rts


func SYS_getAndSetInterruptMaskLevel

    move.w  6(%sp),%d1                      | d1 = value
    andi.w  #0x07,%d1
    ori.w   #0x20,%d1
    lsl.w   #8,%d1

    move.w  %sr,%d0                         | d0 = previous SR value
    move.w  %d1,%sr                         | SR = d1 (these 2 instructions should be canonical)

    lsr.w   #8,%d0
    andi.w  #0x07,%d0                       | d0 = previous interrupt mask
    rts
