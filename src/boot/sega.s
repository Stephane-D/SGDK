.text

*-------------------------------------------------------
*
*       Sega startup code for the GNU Assembler
*       Translated from:
*       Sega startup code for the Sozobon C compiler
*       Written by Paul W. Lee
*       Modified from Charles Coty's code
*       Modified from Stephane Dallongeville's code
*
*-------------------------------------------------------

        .org    0x00000000

_Start_Of_Rom:
_Vecteurs_68K:
        dc.l    0x00FFFE00              /* Stack address */
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
        dc.l    _Error_Exception, _INT, _INT, _INT
        dc.l    _HBL
        dc.l    _INT
        dc.l    _VBL
        dc.l    _INT
        dc.l    _INT,_INT,_INT,_INT,_INT,_INT,_INT,_INT
        dc.l    _INT,_INT,_INT,_INT,_INT,_INT,_INT,_INT
        dc.l    _INT,_INT,_INT,_INT,_INT,_INT,_INT,_INT
        dc.l    _INT,_INT,_INT,_INT,_INT,_INT,_INT,_INT

_Console:
        .ascii  "SEGA MEGA DRIVE "                                  /* Console Name (16) */
_Copyright:
        .ascii  "(C)FLEMTEAM 2010"                                  /* Copyright Information (16) */
_Title_Local:
        .ascii  "SAMPLE PROGRAM                                  "  /* Domestic Name (48) */
_Title_Int:
        .ascii  "SAMPLE PROGRAM                                  "  /* Overseas Name (48) */
_Serial:
        .ascii  "GM 00000000-00"                                    /* Serial Number (2, 14) */
_Checksum:
        dc.w    0x0000                                              /* Checksum (2) */
        .ascii  "JD              "                                  /* I/O Support (16) */
_Rom_Start_Loc:
        dc.l    _Start_Of_Rom                                       /* ROM Start Address (4) */
_Rom_End_Loc:
        dc.l    0x00020000                                          /* ROM End Address (4) */
_Ram_Start_Loc:
        dc.l    0x00FF0000                                          /* Start of Backup RAM (4) */
_Ram_End_Loc:
        dc.l    0x00FFFFFF                                          /* End of Backup RAM (4) */
        .ascii  "                        "                          /* Modem Support (12) */
_Notes:
        .ascii  "DEMONSTRATION PROGRAM                   "          /* Memo (40) */
_Region:
        .ascii  "JUE             "                                  /* Country Support (16) */

_Entry_Point:
        tst.l   0xa10008
        bne.s   SkipJoyDetect
        tst.w   0xa1000c
SkipJoyDetect:
        bne.s   SkipSetup

        move    #0x2700,%sr
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
        jmp     Continue

Table:
        dc.w    0x8000,0x3fff,0x0100
        dc.l    0xA00000,0xA11100,0xA11200,0xC00000,0xC00004

SkipSetup:
        move.w  #0,%a7
        move.w  #0x2300,%sr
        jmp     _reset_entry

Continue:

* clear Genesis RAM
        lea     0xff0000,%a0
        moveq   #0,%d0
        move.w  #0x3FFF,%d1
clrram:
        move.l  %d0,(%a0)+
        dbra    %d1,clrram

        move.w  #0,%a7
        move.w  #0x2300,%sr

* Jump to initialisation process...

        jmp     _start_entry


*------------------------------------------------
*
*       interrupt functions
*
*------------------------------------------------

.text


_Bus_Error:
        movem.l %d0-%d1/%a0-%a1,-(%sp)
        jsr     _buserror_callback
        movem.l (%sp)+,%d0-%d1/%a0-%a1
        rte

_Address_Error:
        movem.l %d0-%d1/%a0-%a1,-(%sp)
        jsr     _addresserror_callback
        movem.l (%sp)+,%d0-%d1/%a0-%a1
        rte

_Illegal_Instruction:
        movem.l %d0-%d1/%a0-%a1,-(%sp)
        jsr     _illegalinst_callback
        movem.l (%sp)+,%d0-%d1/%a0-%a1
        rte

_Zero_Divide:
        movem.l %d0-%d1/%a0-%a1,-(%sp)
        jsr     _zerodivide_callback
        movem.l (%sp)+,%d0-%d1/%a0-%a1
        rte

_Chk_Instruction:
        movem.l %d0-%d1/%a0-%a1,-(%sp)
        jsr     _chkinst_callback
        movem.l (%sp)+,%d0-%d1/%a0-%a1
        rte

_Trapv_Instruction:
        movem.l %d0-%d1/%a0-%a1,-(%sp)
        jsr     _trapvinst_callback
        movem.l (%sp)+,%d0-%d1/%a0-%a1
        rte

_Privilege_Violation:
        movem.l %d0-%d1/%a0-%a1,-(%sp)
        jsr     _privilegeviolation_callback
        movem.l (%sp)+,%d0-%d1/%a0-%a1
        rte

_Trace:
        movem.l %d0-%d1/%a0-%a1,-(%sp)
        jsr     _trace_callback
        movem.l (%sp)+,%d0-%d1/%a0-%a1
        rte

_Line_1010_Emulation:
_Line_1111_Emulation:
        movem.l %d0-%d1/%a0-%a1,-(%sp)
        jsr     _line1x1x_callback
        movem.l (%sp)+,%d0-%d1/%a0-%a1
        rte

_Error_Exception:
        movem.l %d0-%d1/%a0-%a1,-(%sp)
        jsr    _errorexception_callback
        movem.l (%sp)+,%d0-%d1/%a0-%a1
        rte

_INT:
        movem.l %d0-%d1/%a0-%a1,-(%sp)
        jsr    _int_callback
        movem.l (%sp)+,%d0-%d1/%a0-%a1
        rte
_HBL:
        movem.l %d0-%d1/%a0-%a1,-(%sp)
        jsr    _hblank_callback
        movem.l (%sp)+,%d0-%d1/%a0-%a1
        rte

_VBL:
        movem.l %d0-%d1/%a0-%a1,-(%sp)
        jsr    _vblank_callback
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
