.section .text.keepboot

*-------------------------------------------------------
*
*       Sega startup code for the GNU Assembler
*       Translated from:
*       Sega startup code for the Sozobon C compiler
*       Written by Paul W. Lee
*       Modified by Charles Coty
*       Modified by Stephane Dallongeville
*
*-------------------------------------------------------

    .globl  rom_header

    .org    0x00000000

_Start_Of_Rom:
_Vecteurs_68K:
        dc.l    0x00000000              /* Stack address */
        dc.l    _Entry_Point            /* Program start address */
        dc.l    _Bus_Error
        dc.l    _Address_Error
        dc.l    _Illegal_Instruction
        dc.l    _Zero_Divide
        dc.l    _Chk_Instruction
        dc.l    _Trapv_Instruction
        dc.l    _Privilege_Violation
        dc.l    _Trace
        dc.l    _Line_1010_Emulation
        dc.l    _Line_1111_Emulation
        dc.l     _Error_Exception, _Error_Exception, _Error_Exception, _Error_Exception
        dc.l     _Error_Exception, _Error_Exception, _Error_Exception, _Error_Exception
        dc.l     _Error_Exception, _Error_Exception, _Error_Exception, _Error_Exception
        dc.l    _Error_Exception, _INT, _EXTINT, _INT
        dc.l    _HINT
        dc.l    _INT
        dc.l    _VINT
        dc.l    _INT
        dc.l    _INT,_INT,_INT,_INT,_INT,_INT,_INT,_INT
        dc.l    _INT,_INT,_INT,_INT,_INT,_INT,_INT,_INT
        dc.l    _INT,_INT,_INT,_INT,_INT,_INT,_INT,_INT
        dc.l    _INT,_INT,_INT,_INT,_INT,_INT,_INT,_INT

rom_header:
        .incbin "out/rom_head.bin", 0x10, 0x100

_Entry_Point:
        move    #0x2700,%sr
        tst.l   0xa10008
        bne.s   SkipJoyDetect

        tst.w   0xa1000c

SkipJoyDetect:
        bne.s   SkipSetup

        lea     Table,%a5
        movem.w (%a5)+,%d5-%d7
        movem.l (%a5)+,%a0-%a4
* Check Version Number
        move.b  -0x10ff(%a1),%d0
        andi.b  #0x0f,%d0
        beq.s   WrongVersion

* Sega Security Code (SEGA)
        move.l  #0x53454741,0x2f00(%a1)
WrongVersion:
        move.w  (%a4),%d0
        moveq   #0x00,%d0
        movea.l %d0,%a6
        move    %a6,%usp
        move.w  %d7,(%a1)
        move.w  %d7,(%a2)

* Jump to initialisation process now...

        jmp     _start_entry

SkipSetup:
        jmp     _reset_entry


Table:
        dc.w    0x8000,0x3fff,0x0100
        dc.l    0xA00000,0xA11100,0xA11200,0xC00000,0xC00004


*------------------------------------------------
*
*       interrupt functions
*
*------------------------------------------------

registersDump:
        move.l %d0,registerState+0
        move.l %d1,registerState+4
        move.l %d2,registerState+8
        move.l %d3,registerState+12
        move.l %d4,registerState+16
        move.l %d5,registerState+20
        move.l %d6,registerState+24
        move.l %d7,registerState+28
        move.l %a0,registerState+32
        move.l %a1,registerState+36
        move.l %a2,registerState+40
        move.l %a3,registerState+44
        move.l %a4,registerState+48
        move.l %a5,registerState+52
        move.l %a6,registerState+56
        move.l %a7,registerState+60
        rts

busAddressErrorDump:
        move.w 4(%sp),ext1State
        move.l 6(%sp),addrState
        move.w 10(%sp),ext2State
        move.w 12(%sp),srState
        move.l 14(%sp),pcState
        jmp registersDump

exception4WDump:
        move.w 4(%sp),srState
        move.l 6(%sp),pcState
        move.w 10(%sp),ext1State
        jmp registersDump

exceptionDump:
        move.w 4(%sp),srState
        move.l 6(%sp),pcState
        jmp registersDump


_Bus_Error:
        jsr busAddressErrorDump
        movem.l %d0-%d1/%a0-%a1,-(%sp)
        move.l  busErrorCB, %a0
        jsr    (%a0)
        movem.l (%sp)+,%d0-%d1/%a0-%a1
        rte

_Address_Error:
        jsr busAddressErrorDump
        movem.l %d0-%d1/%a0-%a1,-(%sp)
        move.l  addressErrorCB, %a0
        jsr    (%a0)
        movem.l (%sp)+,%d0-%d1/%a0-%a1
        rte

_Illegal_Instruction:
        jsr exception4WDump
        movem.l %d0-%d1/%a0-%a1,-(%sp)
        move.l  illegalInstCB, %a0
        jsr    (%a0)
        movem.l (%sp)+,%d0-%d1/%a0-%a1
        rte

_Zero_Divide:
        jsr exceptionDump
        movem.l %d0-%d1/%a0-%a1,-(%sp)
        move.l  zeroDivideCB, %a0
        jsr    (%a0)
        movem.l (%sp)+,%d0-%d1/%a0-%a1
        rte

_Chk_Instruction:
        jsr exception4WDump
        movem.l %d0-%d1/%a0-%a1,-(%sp)
        move.l  chkInstCB, %a0
        jsr    (%a0)
        movem.l (%sp)+,%d0-%d1/%a0-%a1
        rte

_Trapv_Instruction:
        jsr exception4WDump
        movem.l %d0-%d1/%a0-%a1,-(%sp)
        move.l  trapvInstCB, %a0
        jsr    (%a0)
        movem.l (%sp)+,%d0-%d1/%a0-%a1
        rte

_Privilege_Violation:
        jsr exceptionDump
        movem.l %d0-%d1/%a0-%a1,-(%sp)
        move.l  privilegeViolationCB, %a0
        jsr    (%a0)
        movem.l (%sp)+,%d0-%d1/%a0-%a1
        rte

_Trace:
        jsr exceptionDump
        movem.l %d0-%d1/%a0-%a1,-(%sp)
        move.l  traceCB, %a0
        jsr    (%a0)
        movem.l (%sp)+,%d0-%d1/%a0-%a1
        rte

_Line_1010_Emulation:
_Line_1111_Emulation:
        jsr exceptionDump
        movem.l %d0-%d1/%a0-%a1,-(%sp)
        move.l  line1x1xCB, %a0
        jsr    (%a0)
        movem.l (%sp)+,%d0-%d1/%a0-%a1
        rte

_Error_Exception:
        jsr exceptionDump
        movem.l %d0-%d1/%a0-%a1,-(%sp)
        move.l  errorExceptionCB, %a0
        jsr    (%a0)
        movem.l (%sp)+,%d0-%d1/%a0-%a1
        rte

_INT:
        movem.l %d0-%d1/%a0-%a1,-(%sp)
        move.l  intCB, %a0
        jsr    (%a0)
        movem.l (%sp)+,%d0-%d1/%a0-%a1
        rte

_EXTINT:
        movem.l %d0-%d1/%a0-%a1,-(%sp)
        move.l  eintCB, %a0
        jsr    (%a0)
        movem.l (%sp)+,%d0-%d1/%a0-%a1
        rte

_HINT:
        movem.l %d0-%d1/%a0-%a1,-(%sp)
        move.l  hintCB, %a0
        jsr    (%a0)
        movem.l (%sp)+,%d0-%d1/%a0-%a1
        rte

_VINT:
        movem.l %d0-%d1/%a0-%a1,-(%sp)
        ori.w   #0x0001, intTrace           /* in V-Int */
        addq.l  #1, vtimer                  /* increment frame counter (more a vint counter) */
        btst    #3, VBlankProcess+1         /* PROCESS_XGM_TASK ? (use VBlankProcess+1 as btst is a byte operation) */
        beq.s   _no_xgm_task

        jsr     XGM_doVBlankProcess         /* do XGM vblank task */

_no_xgm_task:
        btst    #1, VBlankProcess+1         /* PROCESS_BITMAP_TASK ? (use VBlankProcess+1 as btst is a byte operation) */
        beq.s   _no_bmp_task

        jsr     BMP_doVBlankProcess         /* do BMP vblank task */

_no_bmp_task:
        move.l  vintCB, %a0                 /* load user callback */
        jsr    (%a0)                        /* call user callback */
        andi.w  #0xFFFE, intTrace           /* out V-Int */
        movem.l (%sp)+,%d0-%d1/%a0-%a1
        rte


*------------------------------------------------
*
* Copyright (c) 1988 by Sozobon, Limited.  Author: Johann Ruegg
*
* Permission is granted to anyone to use this software for any purpose
* on any computer system, and to redistribute it freely, with the
* following restrictions:
* 1) No charge may be made other than reasonable charges for reproduction.
* 2) Modified versions must be clearly marked as such.
* 3) The authors are not responsible for any harmful consequences
*    of using this software, even if they result from defects in it.
*
*------------------------------------------------

ldiv:
        move.l  4(%a7),%d0
        bpl     ld1
        neg.l   %d0
ld1:
        move.l  8(%a7),%d1
        bpl     ld2
        neg.l   %d1
        eor.b   #0x80,4(%a7)
ld2:
        bsr     i_ldiv          /* d0 = d0/d1 */
        tst.b   4(%a7)
        bpl     ld3
        neg.l   %d0
ld3:
        rts

lmul:
        move.l  4(%a7),%d0
        bpl     lm1
        neg.l   %d0
lm1:
        move.l  8(%a7),%d1
        bpl     lm2
        neg.l   %d1
        eor.b   #0x80,4(%a7)
lm2:
        bsr     i_lmul          /* d0 = d0*d1 */
        tst.b   4(%a7)
        bpl     lm3
        neg.l   %d0
lm3:
        rts

lrem:
        move.l  4(%a7),%d0
        bpl     lr1
        neg.l   %d0
lr1:
        move.l  8(%a7),%d1
        bpl     lr2
        neg.l   %d1
lr2:
        bsr     i_ldiv          /* d1 = d0%d1 */
        move.l  %d1,%d0
        tst.b   4(%a7)
        bpl     lr3
        neg.l   %d0
lr3:
        rts

ldivu:
        move.l  4(%a7),%d0
        move.l  8(%a7),%d1
        bsr     i_ldiv
        rts

lmulu:
        move.l  4(%a7),%d0
        move.l  8(%a7),%d1
        bsr     i_lmul
        rts

lremu:
        move.l  4(%a7),%d0
        move.l  8(%a7),%d1
        bsr     i_ldiv
        move.l  %d1,%d0
        rts
*
* A in d0, B in d1, return A*B in d0
*
i_lmul:
        move.l  %d3,%a2           /* save d3 */
        move.w  %d1,%d2
        mulu    %d0,%d2           /* d2 = Al * Bl */

        move.l  %d1,%d3
        swap    %d3
        mulu    %d0,%d3           /* d3 = Al * Bh */

        swap    %d0
        mulu    %d1,%d0           /* d0 = Ah * Bl */

        add.l   %d3,%d0           /* d0 = (Ah*Bl + Al*Bh) */
        swap    %d0
        clr.w   %d0               /* d0 = (Ah*Bl + Al*Bh) << 16 */

        add.l   %d2,%d0           /* d0 = A*B */
        move.l  %a2,%d3           /* restore d3 */
        rts
*
*A in d0, B in d1, return A/B in d0, A%B in d1
*
i_ldiv:
        tst.l   %d1
        bne     nz1

*       divide by zero
*       divu    #0,%d0            /* cause trap */
        move.l  #0x80000000,%d0
        move.l  %d0,%d1
        rts
nz1:
        move.l  %d3,%a2           /* save d3 */
        cmp.l   %d1,%d0
        bhi     norm
        beq     is1
*       A<B, so ret 0, rem A
        move.l  %d0,%d1
        clr.l   %d0
        move.l  %a2,%d3           /* restore d3 */
        rts
*       A==B, so ret 1, rem 0
is1:
        moveq.l #1,%d0
        clr.l   %d1
        move.l  %a2,%d3           /* restore d3 */
        rts
*       A>B and B is not 0
norm:
        cmp.l   #1,%d1
        bne     not1
*       B==1, so ret A, rem 0
        clr.l   %d1
        move.l  %a2,%d3           /* restore d3 */
        rts
*  check for A short (implies B short also)
not1:
        cmp.l   #0xffff,%d0
        bhi     slow
*  A short and B short -- use 'divu'
        divu    %d1,%d0           /* d0 = REM:ANS */
        swap    %d0               /* d0 = ANS:REM */
        clr.l   %d1
        move.w  %d0,%d1           /* d1 = REM */
        clr.w   %d0
        swap    %d0
        move.l  %a2,%d3           /* restore d3 */
        rts
* check for B short
slow:
        cmp.l   #0xffff,%d1
        bhi     slower
* A long and B short -- use special stuff from gnu
        move.l  %d0,%d2
        clr.w   %d2
        swap    %d2
        divu    %d1,%d2           /* d2 = REM:ANS of Ahi/B */
        clr.l   %d3
        move.w  %d2,%d3           /* d3 = Ahi/B */
        swap    %d3

        move.w  %d0,%d2           /* d2 = REM << 16 + Alo */
        divu    %d1,%d2           /* d2 = REM:ANS of stuff/B */

        move.l  %d2,%d1
        clr.w   %d1
        swap    %d1               /* d1 = REM */

        clr.l   %d0
        move.w  %d2,%d0
        add.l   %d3,%d0           /* d0 = ANS */
        move.l  %a2,%d3           /* restore d3 */
        rts
*       A>B, B > 1
slower:
        move.l  #1,%d2
        clr.l   %d3
moreadj:
        cmp.l   %d0,%d1
        bhs     adj
        add.l   %d2,%d2
        add.l   %d1,%d1
        bpl     moreadj
* we shifted B until its >A or sign bit set
* we shifted #1 (d2) along with it
adj:
        cmp.l   %d0,%d1
        bhi     ltuns
        or.l    %d2,%d3
        sub.l   %d1,%d0
ltuns:
        lsr.l   #1,%d1
        lsr.l   #1,%d2
        bne     adj
* d3=answer, d0=rem
        move.l  %d0,%d1
        move.l  %d3,%d0
        move.l  %a2,%d3           /* restore d3 */
        rts
