****************************************************************
*
* Gens KMod debugging stuff
*
* history
*        041206: initial release
*        061029: added AlertNumber
*
****************************************************************

* Important note :
* m68k calling convention says that, between function calls,
* d0,d1,a0,a1 can be used as scratch registers, however,
* others should be preserved and
* return value -if there is- is placed in d0

#include "asm_mac.i"

**************
* Pause Gens *
**************
func KDebug_Halt
    move.w  #0x9D00, 0xC00004
    rts


**************************************
* Show a text on Gens Message window *
**************************************
func KDebug_Alert
* %a0 = uchar *string
    movea.l 4(%sp), %a0             | get the first parameter (note that (%sp) is the return address)
    move.w  #0x9e00, %d0
    move.b  (%a0)+, %d0             | write null-termiated string byte by byte to port #0x1e
    beq.s   1f                      | zero length string? no, thanks.
    movea.l #0xC00004, %a1

0:
    move.w  %d0, (%a1)
    move.b  (%a0)+, %d0
    bne.s   0b

    move.w  %d0, (%a1)              | session ends with a 0 write to port #0x1e
1:
    rts




****************************************
* Show a number on Gens Message window *
****************************************
func KDebug_AlertNumber
* %a0 = ulong number
    move.l  4(%sp), %a0
    move.w  #7, %d1                                | DBRA only deal with signed word

numLoop:
    move.l  %a0, %d0
    lsr.l   #8, %d0
    lsr.l   #8, %d0
    lsr.l   #8, %d0
    lsr.l   #4, %d0                                | lsr.l #28,%d0 => hint to avoid %d2 use, needed for +8 shift
    and.l   #0xF, %d0

* number or hexa-letter needed ?
    cmp.l   #9, %d0
    bhi.s   letter

number:
    addi.l  #48, %d0
    bra.s   ascii

letter:
    addi.l  #55, %d0

* write it on special debug register
ascii:
    add.w   #0x9e00, %d0
    move.w  %d0, (0xC00004)

*
    move.l  %a0, %d0
    lsl.l   #4, %d0
    move.l  %d0, %a0


    dbra    %d1, numLoop

    move.w  #0x9e00, %d0
    move.w  %d0, (0xC00004)
1:
    rts


***************
* Start Timer *
***************
func KDebug_StartTimer
    move.w  #0x9F80, 0xC00004

    rts

**************
* Stop Timer *
**************
func KDebug_StopTimer
    move.w  #0x9F40, 0xC00004
    rts
