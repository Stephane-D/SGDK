#include "config.h"
#include "task_cst.h"

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
        dc.l    __stack                 /* Stack address */
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
        dc.l    _Error_Exception
        dc.l    _INT
        dc.l    _EXTINT
        dc.l    _INT
        dc.l    hintCaller
        dc.l    _INT
        dc.l    _VINT
        dc.l    _INT
        dc.l    _trap_0                 /* Resume supervisor task */
        dc.l    _INT,_INT,_INT,_INT,_INT,_INT,_INT
        dc.l    _INT,_INT,_INT,_INT,_INT,_INT,_INT,_INT
        dc.l    _INT,_INT,_INT,_INT,_INT,_INT,_INT,_INT
        dc.l    _INT,_INT,_INT,_INT,_INT,_INT,_INT,_INT

rom_header:
        .incbin "out/rom_head.bin", 0, 0x100

_Entry_Point:
* disable interrupts
        move    #0x2700,%sr

* Configure a USER_STACK_LENGTH bytes user stack at bottom, and system stack on top of it
        move    %sp, %usp
        sub     #USER_STACK_LENGTH, %sp

* Halt Z80 (need to be done as soon as possible on reset)
        move.l  #0xA11100,%a0       /* Z80_HALT_PORT */
        move.w  #0x0100,%d0
        move.w  %d0,(%a0)           /* HALT Z80 */
        move.w  %d0,0x0100(%a0)     /* END RESET Z80 */

        tst.l   0xa10008
        bne.s   SkipInit

        tst.w   0xa1000c
        bne.s   SkipInit

* Check Version Number
        move.b  -0x10ff(%a0),%d0
        andi.b  #0x0f,%d0
        beq.s   NoTMSS

* Sega Security Code (SEGA)
        move.l  #0x53454741,0x2f00(%a0)

NoTMSS:
        jmp     _start_entry

SkipInit:
        jmp     _reset_entry


*------------------------------------------------
*
*       interrupt functions
*
*------------------------------------------------

#if     LEGACY_ERROR_HANDLER

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

#else

* ===============================================================
* ---------------------------------------------------------------
* MD Debugger and Error Handler v.2.6
*
* (c) 2016-2024, Vladikcomper
* ---------------------------------------------------------------
* Error handler blob (GNU AS version)
* ---------------------------------------------------------------

__ErrorHandler:
	dc.l	0x46FC2700, 0x4FEFFFF0, 0x48E7FFFE, 0x4EBA0242, 0x49EF004C, 0x4E682F08, 0x47EF0040, 0x4EBA0124
	dc.l	0x41FA02C4, 0x4EBA0B70, 0x225C45D4, 0x4EBA0C2E, 0x4EBA0AF6, 0x49D21C19, 0x6A025249, 0x47D10806
	dc.l	0x0000670E, 0x41FA02A7, 0x222C0002, 0x4EBA016A, 0x504C41FA, 0x02A4222C, 0x00024EBA, 0x015C0804
	dc.l	0x00026614, 0x22780000, 0x45EC0006, 0x4EBA01BC, 0x41FA0290, 0x4EBA0142, 0x4EBA0AAE, 0x08060006
	dc.l	0x660000AA, 0x45EF0004, 0x4EBA0A78, 0x3F017003, 0x4EBA0A3C, 0x303C6430, 0x7A074EBA, 0x0132321F
	dc.l	0x70114EBA, 0x0A2A303C, 0x61307A06, 0x4EBA0120, 0x303C7370, 0x7A002F0C, 0x45D74EBA, 0x0112584F
	dc.l	0x08060001, 0x671443FA, 0x025545D7, 0x4EBA0B92, 0x43FA0256, 0x45D44EBA, 0x0B84584F, 0x4EBA0A24
	dc.l	0x52417001, 0x4EBA09E8, 0x20380078, 0x41FA0244, 0x4EBA010A, 0x20380070, 0x41FA0240, 0x4EBA00FE
	dc.l	0x4EBA0A26, 0x22780000, 0x45D45389, 0x61404EBA, 0x09F27A19, 0x9A416B0A, 0x61484EBA, 0x005A51CD
	dc.l	0xFFFA0806, 0x0005660A, 0x4E7160FC, 0x72004EBA, 0x0A262ECB, 0x4CDF7FFF, 0x487AFFEE, 0x2F2FFFC4
	dc.l	0x4E7543FA, 0x015E45FA, 0x02084EFA, 0x08F2223C, 0x00FFFFFF, 0x2409C481, 0x2242240A, 0xC4812442
	dc.l	0x4E754FEF, 0xFFD041D7, 0x7EFF20FC, 0x28535029, 0x30FC3A20, 0x60184FEF, 0xFFD041D7, 0x7EFF30FC
	dc.l	0x202B320A, 0x924C4EBA, 0x05BE30FC, 0x3A207005, 0x72ECB5C9, 0x650272EE, 0x10C1321A, 0x4EBA05C6
	dc.l	0x10FC0020, 0x51C8FFEA, 0x421841D7, 0x72004EBA, 0x09E04FEF, 0x00304E75, 0x4EBA09DC, 0x2F012F01
	dc.l	0x45D743FA, 0x01484EBA, 0x0A94504F, 0x4E754FEF, 0xFFF07EFF, 0x41D730C0, 0x30FC3A20, 0x10FC00EC
	dc.l	0x221A4EBA, 0x05784218, 0x41D77200, 0x4EBA09A2, 0x524051CD, 0xFFE04FEF, 0x00104E75, 0x22004841
	dc.l	0x460166F6, 0x24400C5A, 0x4EF96604, 0x221260A8, 0x0C6A4EF8, 0xFFFE6606, 0x321248C1, 0x609A4EBA
	dc.l	0x097641FA, 0x011E4EFA, 0x096A5989, 0x4EBAFF20, 0xB3CA650C, 0x0C520040, 0x650A548A, 0xB3CA64F4
	dc.l	0x72004E75, 0x221267F2, 0x08010000, 0x66EC4E75, 0x4BF900C0, 0x00044DED, 0xFFFC44D5, 0x69FC41FA
	dc.l	0x00263018, 0x6A043A80, 0x60F87000, 0x2ABC4000, 0x00002C80, 0x2ABC4000, 0x00102C80, 0x2ABCC000
	dc.l	0x00003C80, 0x4E758004, 0x81348500, 0x87008B00, 0x8C818D00, 0x8F029011, 0x91009200, 0x82208404
	dc.l	0x00004400, 0x00000000, 0x00010010, 0x00110100, 0x01010110, 0x01111000, 0x10011010, 0x10111100
	dc.l	0x11011110, 0x1111FFFF, 0x0EEEFFF2, 0x00CEFFF2, 0x0EEAFFF2, 0x0E86FFF2, 0x40000002, 0x00280028
	dc.l	0x00000080, 0x00FFEAE0, 0xFA01F026, 0x00EA4164, 0x64726573, 0x733A2000, 0xEA4F6666, 0x7365743A
	dc.l	0x2000EA43, 0x616C6C65, 0x723A2000, 0xEC808120, 0xE8BFECC8, 0x00EC8320, 0xE8BFECC8, 0x00FA10E8
	dc.l	0x7573703A, 0x20EC8300, 0xFA03E873, 0x723A20EC, 0x8100EA56, 0x496E743A, 0x2000EA48, 0x496E743A
	dc.l	0x2000E83C, 0x756E6465, 0x66696E65, 0x643E0000, 0x02F70000, 0x00000000, 0x0000183C, 0x3C181800
	dc.l	0x18006C6C, 0x6C000000, 0x00006C6C, 0xFE6CFE6C, 0x6C00187E, 0xC07C06FC, 0x180000C6, 0x0C183060
	dc.l	0xC600386C, 0x3876CCCC, 0x76001818, 0x30000000, 0x00001830, 0x60606030, 0x18006030, 0x18181830
	dc.l	0x600000EE, 0x7CFE7CEE, 0x00000018, 0x187E1818, 0x00000000, 0x00001818, 0x30000000, 0x00FE0000
	dc.l	0x00000000, 0x00000038, 0x3800060C, 0x183060C0, 0x80007CC6, 0xCEDEF6E6, 0x7C001878, 0x18181818
	dc.l	0x7E007CC6, 0x0C183066, 0xFE007CC6, 0x063C06C6, 0x7C000C1C, 0x3C6CFE0C, 0x0C00FEC0, 0xFC0606C6
	dc.l	0x7C007CC6, 0xC0FCC6C6, 0x7C00FEC6, 0x060C1818, 0x18007CC6, 0xC67CC6C6, 0x7C007CC6, 0xC67E06C6
	dc.l	0x7C00001C, 0x1C00001C, 0x1C000018, 0x18000018, 0x18300C18, 0x30603018, 0x0C000000, 0xFE0000FE
	dc.l	0x00006030, 0x180C1830, 0x60007CC6, 0x060C1800, 0x18007CC6, 0xC6DEDCC0, 0x7E00386C, 0xC6C6FEC6
	dc.l	0xC600FC66, 0x667C6666, 0xFC003C66, 0xC0C0C066, 0x3C00F86C, 0x6666666C, 0xF800FEC2, 0xC0F8C0C2
	dc.l	0xFE00FE62, 0x607C6060, 0xF0007CC6, 0xC0C0DEC6, 0x7C00C6C6, 0xC6FEC6C6, 0xC6003C18, 0x18181818
	dc.l	0x3C003C18, 0x1818D8D8, 0x7000C6CC, 0xD8F0D8CC, 0xC600F060, 0x60606062, 0xFE00C6EE, 0xFED6D6C6
	dc.l	0xC600C6E6, 0xE6F6DECE, 0xC6007CC6, 0xC6C6C6C6, 0x7C00FC66, 0x667C6060, 0xF0007CC6, 0xC6C6C6D6
	dc.l	0x7C06FCC6, 0xC6FCD8CC, 0xC6007CC6, 0xC07C06C6, 0x7C007E5A, 0x18181818, 0x3C00C6C6, 0xC6C6C6C6
	dc.l	0x7C00C6C6, 0xC6C66C38, 0x1000C6C6, 0xD6D6FEEE, 0xC600C66C, 0x3838386C, 0xC6006666, 0x663C1818
	dc.l	0x3C00FE86, 0x0C183062, 0xFE007C60, 0x60606060, 0x7C00C060, 0x30180C06, 0x02007C0C, 0x0C0C0C0C
	dc.l	0x7C001038, 0x6CC60000, 0x00000000, 0x00000000, 0x00FF3030, 0x18000000, 0x00000000, 0x780C7CCC
	dc.l	0x7E00E060, 0x7C666666, 0xFC000000, 0x7CC6C0C6, 0x7C001C0C, 0x7CCCCCCC, 0x7E000000, 0x7CC6FEC0
	dc.l	0x7C001C36, 0x30FC3030, 0x78000000, 0x76CEC67E, 0x067CE060, 0x7C666666, 0xE6001800, 0x38181818
	dc.l	0x3C000C00, 0x1C0C0C0C, 0xCC78E060, 0x666C786C, 0xE6001818, 0x18181818, 0x1C000000, 0x6CFED6D6
	dc.l	0xC6000000, 0xDC666666, 0x66000000, 0x7CC6C6C6, 0x7C000000, 0xDC66667C, 0x60F00000, 0x76CCCC7C
	dc.l	0x0C1E0000, 0xDC666060, 0xF0000000, 0x7CC07C06, 0x7C003030, 0xFC303036, 0x1C000000, 0xCCCCCCCC
	dc.l	0x76000000, 0xC6C66C38, 0x10000000, 0xC6C6D6FE, 0x6C000000, 0xC66C386C, 0xC6000000, 0xC6C6CE76
	dc.l	0x067C0000, 0xFC983064, 0xFC000E18, 0x18701818, 0x0E001818, 0x18001818, 0x18007018, 0x180E1818
	dc.l	0x700076DC, 0x00000000, 0x00002279, MDDBG__SymbolDataPtr, 0x0C59DEB2, 0x667270FE, 0xD05974FC, 0x76004841
	dc.l	0x024100FF, 0xD241D241, 0xB240625C, 0x675E2031, 0x10006758, 0x47F10800, 0x48417000, 0x301BB253
	dc.l	0x654C43F3, 0x08FE45E9, 0xFFFCE248, 0xC042B273, 0x00006514, 0x6204D6C0, 0x601A47F3, 0x0004200A
	dc.l	0x908B6AE6, 0x594B600C, 0x45F300FC, 0x200A908B, 0x6AD847D2, 0x925B7400, 0x341BD3C2, 0x48414241
	dc.l	0x4841D283, 0x70004E75, 0x70FF4E75, 0x48417000, 0x3001D680, 0x5283323C, 0xFFFF4841, 0x59416A8E
	dc.l	0x70FF4E75
	dc.w	0x2679
	dc.l	MDDBG__SymbolDataPtr, 0x0C5BDEB2, 0x664AD6D3, 0x78007200, 0x740045D3, 0x51CC0006, 0x16197807, 0xD603D341
	dc.l	0x5242B252, 0x620A65EC, 0xB42A0002, 0x671265E4, 0x584AB252, 0x62FA65DC, 0xB42A0002, 0x65D666F0
	dc.l	0x10EA0003, 0x670A51CF, 0xFFC64E94, 0x64C04E75, 0x53484E75, 0x70004E75, 0x4EFA0024, 0x4EFA0018
	dc.l	0x760F3401, 0xE84AC443, 0x10FB205C, 0x51CF004A, 0x4E946444, 0x4E754841, 0x61046548, 0x4841E959
	dc.l	0x780FC841, 0x10FB4040, 0x51CF0006, 0x4E946534, 0xE959780F, 0xC84110FB, 0x402E51CF, 0x00064E94
	dc.l	0x6522E959, 0x780FC841, 0x10FB401C, 0x51CF0006, 0x4E946510, 0xE959760F, 0xC24310FB, 0x100A51CF
	dc.l	0x00044ED4, 0x4E753031, 0x32333435, 0x36373839, 0x41424344, 0x45464841, 0x67066106, 0x65E6609C
	dc.l	0x4841E959, 0x780FC841, 0x670E10FB, 0x40DA51CF, 0xFFA04E94, 0x649A4E75, 0xE959780F, 0xC841670E
	dc.l	0x10FB40C4, 0x51CFFF9C, 0x4E946496, 0x4E75E959, 0x780FC841, 0x679E10FB, 0x40AE51CF, 0xFF984E94
	dc.l	0x64924E75, 0x4EFA0026, 0x4EFA001A, 0x74077018, 0xD201D100, 0x10C051CF, 0x00064E94, 0x650451CA
	dc.l	0xFFEE4E75, 0x48416104, 0x65184841, 0x740F7018, 0xD241D100, 0x10C051CF, 0x00064E94, 0x650451CA
	dc.l	0xFFEE4E75, 0x4EFA0010, 0x4EFA0048, 0x47FA009A, 0x024100FF, 0x600447FA, 0x008C4200, 0x7609381B
	dc.l	0x34039244, 0x55CAFFFC, 0xD2449443, 0x44428002, 0x670E0602, 0x003010C2, 0x51CF0006, 0x4E946510
	dc.l	0x381B6ADC, 0x06010030, 0x10C151CF, 0x00044ED4, 0x4E7547FA, 0x002E4200, 0x7609281B, 0x34039284
	dc.l	0x55CAFFFC, 0xD2849443, 0x44428002, 0x670E0602, 0x003010C2, 0x51CF0006, 0x4E9465D4, 0x281B6ADC
	dc.l	0x609E3B9A, 0xCA0005F5, 0xE1000098, 0x9680000F, 0x42400001, 0x86A00000, 0x2710FFFF, 0x03E80064
	dc.l	0x000AFFFF, 0x271003E8, 0x0064000A, 0xFFFF48C1, 0x60084EFA, 0x00064881, 0x48C148E7, 0x50604EBA
	dc.l	0xFD446618, 0x2E814EBA, 0xFDD64CDF, 0x060A650A, 0x08030003, 0x66044EFA, 0x00B64E75, 0x4CDF060A
	dc.l	0x08030002, 0x670847FA, 0x000A4EFA, 0x00B470FF, 0x60DE3C75, 0x6E6B6E6F, 0x776E3E00, 0x10FC002B
	dc.l	0x51CF0006, 0x4E9465D2, 0x48414A41, 0x6700FE72, 0x6000FE68, 0x08030003, 0x66C04EFA, 0xFDFA48E7
	dc.l	0xF81010D9, 0x5FCFFFFC, 0x6E146718, 0x16207470, 0xC4034EBB, 0x201A64EA, 0x4CDF081F, 0x4E754E94
	dc.l	0x64E060F4, 0x53484E94, 0x4CDF081F, 0x4E7547FA, 0xFDA8B702, 0xD4024EFB, 0x205A4E71, 0x4E7147FA
	dc.l	0xFEA4B702, 0xD4024EFB, 0x204A4E71, 0x4E7147FA, 0xFE54B702, 0xD4024EFB, 0x203A5348, 0x4E7547FA
	dc.l	0xFF2E7403, 0xC403D442, 0x4EFB2028, 0x4E714A40, 0x6B084A81, 0x67164EFA, 0xFF644EFA, 0xFF78265A
	dc.l	0x10DB57CF, 0xFFFC67D2, 0x4E9464F4, 0x4E755248, 0x6032504B, 0x321A4ED3, 0x584B221A, 0x4ED35547
	dc.l	0x6028504B, 0x321A6004, 0x584B221A, 0x6A084481, 0x10FC002D, 0x600410FC, 0x002B51CF, 0x00064E94
	dc.l	0x65CA4ED3, 0x51CFFFC6, 0x4ED46506, 0x524810D9, 0x4E755447, 0x53494ED4, 0x4BF900C0, 0x00044DED
	dc.l	0xFFFC4A51, 0x6B102A99, 0x41D23818, 0x4EBA023C, 0x43E90020, 0x60EC5449, 0x2ABCC000, 0x00007000
	dc.l	0x76033C80, 0x34193C82, 0x34196AFA, 0x72004EBB, 0x204C51CB, 0xFFEE2A19, 0x200B4840, 0x024000FF
	dc.l	0x00405D00, 0x48402640, 0x4E6326C5, 0x26C526D9, 0x26D92A85, 0x70003219, 0x61122ABC, 0x40000000
	dc.l	0x72006108, 0x3ABC8174, 0x2A854E75, 0x2C802C80, 0x2C802C80, 0x2C802C80, 0x2C802C80, 0x51C9FFEE
	dc.l	0x4E754CAF, 0x00030004, 0x48E76010, 0x4E6B240B, 0x48424202, 0x0C425D00, 0x661C342B, 0x00040242
	dc.l	0xE000C2EB, 0x000ED441, 0xD440D440, 0x36823742, 0x0004504B, 0x36DB4CDF, 0x08064E75, 0x2F0B4E6B
	dc.l	0x200B4840, 0x42000C40, 0x5D006612, 0x72003213, 0x02411FFF, 0x82EB000E, 0x20014840, 0xE248265F
	dc.l	0x4E752F0B, 0x2F004E6B, 0x200B4840, 0x42000C40, 0x5D006616, 0x302B0004, 0xD06B000E, 0x02405FFF
	dc.l	0x36803740, 0x0004504B, 0x36DB201F, 0x265F4E75, 0x2F0B2F00, 0x4E6B200B, 0x48404200, 0x0C405D00
	dc.l	0x66043741, 0x000C201F, 0x265F4E75, 0x2F0B2F00, 0x4E6B200B, 0x48404200, 0x0C405D00, 0x6606504B
	dc.l	0x36C136C1, 0x201F265F, 0x4E7561C4, 0x487AFF94, 0x48E77F12, 0x4E6B240B, 0x48424202, 0x0C425D00
	dc.l	0x66282A1B, 0x2E1B4C93, 0x005C4846, 0x4DF900C0, 0x00002D45, 0x00044845, 0x72001218, 0x6E126B32
	dc.l	0x4893001C, 0x484548E3, 0x05004CDF, 0x48FE4E75, 0x51CB0012, 0xD642DE86, 0x0887001D, 0x2D470004
	dc.l	0x2A074845, 0xD2443C81, 0x54457200, 0x12186EE0, 0x67CE0241, 0x001E4EFB, 0x1002DE86, 0x721D0387
	dc.l	0x6020602A, 0x602E6036, 0x603E1418, 0x60141818, 0x60D8603A, 0x1218D241, 0x76804843, 0xCE834841
	dc.l	0x8E813602, 0x2D470004, 0x2A074845, 0x60BC0244, 0x9FFF60B6, 0x02449FFF, 0x00442000, 0x60AC0244
	dc.l	0x9FFF0044, 0x400060A2, 0x00446000, 0x609C3F04, 0x1E98381F, 0x6094487A, 0xFECA2F0C, 0x49FA0016
	dc.l	0x4FEFFFF0, 0x41D77E0E, 0x4EBAFCF4, 0x4FEF0010, 0x285F4E75, 0x42184447, 0x0647000F, 0x90C72F08
	dc.l	0x4EBAFF0E, 0x205F7E0E, 0x4E75741E, 0x10181200, 0xE609C242, 0x3CB11000, 0xD000C042, 0x3CB10000
	dc.l	0x51CCFFEA, 0x4E75487A, 0x00562F0C, 0x49FA0016, 0x4FEFFFF0, 0x41D77E0E, 0x4EBAFCA4, 0x4FEF0010
	dc.l	0x285F4E75, 0x42184447, 0x0647000F, 0x90C72F08, 0x2F0D4BF9, 0x00C00004, 0x3E3C9E00, 0x60023A87
	dc.l	0x1E186EFA, 0x67100407, 0x00E067F2, 0x0C070010, 0x6DEE5248, 0x60EA2A5F, 0x205F7E0E, 0x4E7533FC
	dc.l	0x9E0000C0, 0x00044E75, 0x487AFFF4, 0x3F072F0D, 0x4BF900C0, 0x00043E3C, 0x9E006002, 0x3A871E18
	dc.l	0x6EFA6710, 0x040700E0, 0x67F20C07, 0x00106DEE, 0x524860EA, 0x2A5F3E1F, 0x4E7546FC, 0x27004FEF
	dc.l	0xFFF048E7, 0xFFFE47EF, 0x003C4EBA, 0xF4FE4EBA, 0xF3EC4CDF, 0x7FFF487A, 0xF3CA2F2F, 0x00144E75
	dc.l	0x48E7C456, 0x4E6B200B, 0x48404200, 0x0C405D00, 0x66124BF9, 0x00C00004, 0x4DEDFFFC, 0x43FAF554
	dc.l	0x4EBAFCF4, 0x4CDF6A23, 0x4E7548E7, 0xC0D04E6B, 0x200B4840, 0x42000C40, 0x5D00660C, 0x3F3C0000
	dc.l	0x610C610A, 0x67FC544F, 0x4CDF0B03, 0x4E756174, 0x41EF0004, 0x43F900A1, 0x00036178, 0x70F0C02F
	dc.l	0x00054E75, 0x48E7FFFE, 0x3F3C0000, 0x61E04BF9, 0x00C00004, 0x4DEDFFFC, 0x61D467F2, 0x6B4041FA
	dc.l	0x00765888, 0xD00064FA, 0x20106F32, 0x20404FEF, 0xFFF043FA, 0xF4E247D7, 0x2A3C4000, 0x00034EBA
	dc.l	0xFC782ABC, 0x82308406, 0x2A85487A, 0x000C4850, 0x4CEF7FFF, 0x00164E75, 0x4FEF0010, 0x60B02ABA
	dc.l	0xF47660AA, 0x41F900C0, 0x000444D0, 0x6BFC44D0, 0x6AFC4E75, 0x12BC0000, 0x4E7172C0, 0x1011E508
	dc.l	0x12BC0040, 0x4E71C001, 0x12110201, 0x003F8001, 0x46001210, 0xB10110C0, 0xC20010C1
	dc.w	0x4E75
	dc.l	MDDBG__Debugger_AddressRegisters, 0x00000000, MDDBG__Debugger_Backtrace, 0x48E700FE, 0x41FA002A, 0x4EBAFD1C, 0x49D77C06, 0x3F3C2000
	dc.l	0x2F3CE861, 0x303A41D7, 0x221C4EBA, 0xF328522F, 0x000251CE, 0xFFF24FEF, 0x00224E75, 0xE0FA01F0
	dc.l	0x26EA4164, 0x64726573, 0x73205265, 0x67697374, 0x6572733A, 0xE0E00000, 0x41FA0088, 0x4EBAFCD4
	dc.l	0x22780000, 0x598945D7, 0x4EBAF280, 0xB3CA6570, 0x0C520040, 0x64642012, 0x67602040, 0x02400001
	dc.l	0x66581220, 0x10200C00, 0x00616604, 0x4A01663A, 0x0C00004E, 0x660A0201, 0x00F80C01, 0x0090672A
	dc.l	0x30200C40, 0x61006722, 0x12004200, 0x0C404E00, 0x66120C01, 0x00A8650C, 0x0C0100BB, 0x62060C01
	dc.l	0x00B96606, 0x0C604EB9, 0x66102F0A, 0x2F092208, 0x4EBAF286, 0x225F245F, 0x548A548A, 0xB3CA6490
	dc.l	0x4E75E0FA, 0x01F026EA, 0x4261636B, 0x74726163, 0x653AE0E0, 0x00004EBA, 0xF0A44255, 0x53204552
	dc.l	0x524F5200, 0xA7004EFA, 0xFE5E4EBA, 0xF0904144, 0x44524553, 0x53204552, 0x524F5200, 0xA7004EFA
	dc.l	0xFE464EBA, 0xF078494C, 0x4C454741, 0x4C20494E, 0x53545255, 0x4354494F, 0x4E00A600, 0x4EFAFE28
	dc.l	0x4EBAF05A, 0x5A45524F, 0x20444956, 0x49444500, 0xA6004EFA, 0xFE124EBA, 0xF0444348, 0x4B20494E
	dc.l	0x53545255, 0x4354494F, 0x4E00A600, 0x4EFAFDF8, 0x4EBAF02A, 0x54524150, 0x5620494E, 0x53545255
	dc.l	0x4354494F, 0x4E00A600, 0x4EFAFDDC, 0x4EBAF00E, 0x50524956, 0x494C4547, 0x45205649, 0x4F4C4154
	dc.l	0x494F4E00, 0xA6004EFA, 0xFDBE4EBA, 0xEFF05452, 0x41434500, 0xA6004EFA, 0xFDAE4EBA, 0xEFE04C49
	dc.l	0x4E452031, 0x30313020, 0x454D554C, 0x41544F52, 0x00264EFA, 0xFD924EBA, 0xEFC44C49, 0x4E452031
	dc.l	0x31313120, 0x454D554C, 0x41544F52, 0x00264EFA, 0xFD764EBA, 0xEFA84552, 0x524F5220, 0x45584345
	dc.l	0x5054494F, 0x4E00A600, 0x4EFAFD5C

* ---------------------------------------------------------------
* Exported error vectors and symbols
* ---------------------------------------------------------------

	.equ MDDBG__Debugger_AddressRegisters, __ErrorHandler+3696
	.equ MDDBG__Debugger_Backtrace, __ErrorHandler+3772

	.equ _Bus_Error, __ErrorHandler+3930
	.equ _Address_Error, __ErrorHandler+3950
	.equ _Illegal_Instruction, __ErrorHandler+3974
	.equ _Zero_Divide, __ErrorHandler+4004
	.equ _Chk_Instruction, __ErrorHandler+4026
	.equ _Trapv_Instruction, __ErrorHandler+4052
	.equ _Privilege_Violation, __ErrorHandler+4080
	.equ _Trace, __ErrorHandler+4110
	.equ _Line_1010_Emulation, __ErrorHandler+4126
	.equ _Line_1111_Emulation, __ErrorHandler+4154
	.equ _Error_Exception, __ErrorHandler+4182

#endif

* Symbol table data pointer for the MD Debugger (32-bit absolute ROM pointer)
*
* This defaults to 0 (no table), it's injected by "convsym" tool if "debug" build profile is used.
* 
* NOTE: This is present even if MD Debugger is not used (LEGACY_ERROR_HANDLER=1) to ensure
* build process won't break if symbol table injection is performed by the Makefile either way.

MDDBG__SymbolDataPtr:   dc.l    0


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

_VINT:
        btst    #5, (%sp)       /* Skip context switch if not in user task */
        bne.s   no_user_task

        tst.w   task_lock
        bne.s   1f
        move.w  #0, -(%sp)      /* TSK_superPend() will return 0 */
        bra.s   unlock          /* If lock == 0, supervisor task is not locked */

1:
        bcs.s   no_user_task    /* If lock < 0, super is locked with infinite wait */
        subq.w  #1, task_lock   /* Locked with wait, subtract 1 to the frame count */
        bne.s   no_user_task    /* And do not unlock if we did not reach 0 */
        move.w  #1, -(%sp)      /* TSK_superPend() will return 1 */

unlock:
        /* Save bg task registers (excepting a7, that is stored in usp) */
        move.l  %a0, task_regs
        lea     (task_regs + UTSK_REGS_LEN), %a0
        movem.l %d0-%d7/%a1-%a6, -(%a0)

        move.w  (%sp)+, %d0     /* Load return value previously pushed to stack */

        move.w  (%sp)+, task_sr /* Pop user task sr and pc, and save them, */
        move.l  (%sp)+, task_pc /* so they can be restored later.          */
        movem.l (%sp)+, %d2-%d7/%a2-%a6 /* Restore non clobberable registers */

no_user_task:
        /* At this point, we always have in the stack the SR and PC of the task */
        /* we want to jump after processing the interrupt, that might be the    */
        /* point where we came from (if there is no context switch) or the      */
        /* supervisor task (if we unlocked it).                                 */

        movem.l %d0-%d1/%a0-%a1,-(%sp)
        ori.w   #0x0001, intTrace           /* in V-Int */
        addq.l  #1, vtimer                  /* increment frame counter (more a vint counter) */
        btst    #3, VBlankProcess+1         /* PROCESS_XGM_TASK ? (use VBlankProcess+1 as btst is a byte operation) */
        beq.s   no_xgm_task

        jsr     XGM_doVBlankProcess         /* do XGM vblank task */

no_xgm_task:
        btst    #1, VBlankProcess+1         /* PROCESS_BITMAP_TASK ? (use VBlankProcess+1 as btst is a byte operation) */
        beq.s   no_bmp_task

        jsr     BMP_doVBlankProcess         /* do BMP vblank task */

no_bmp_task:
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
