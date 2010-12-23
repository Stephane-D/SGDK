 OOO     O   O    OOO  O   O
O   O       O O  O   O OO OO
O       OO O   O O     O O O
 OOO     O O   O  OOO  O O O
    O    O OOOOO     O O   O
O   O    O O   O O   O O   O
 OOO  O  O O   O  OOO  O   O
       OO

SjASM Z80 Assembler v0.39g6
Copyright 2006 Sjoerd Mastijn

www.xl2s.tk - sjasm@xl2s.tk

Introduction
============
SjASM version 0.3 is just yet another Z80 cross assembler.
Thanks to Eli-Jean Leyssens there is also a Linux version of SjASM.

DISCLAIMER
----------
If SjASM breaks anything - I didn't do it.

Changes from 0.2
----------------
- SjASM v0.3x assumes Z80-mode as there are no other cpus supported.
- All calculations are 32 bits.
 
Changes from 0.30
-----------------
- '#' Can be used now to indicate a hexadecimal value.
- Local 'number' labels should really work now.
- Multiple error messages per source line possible.
- Things like ld a,(4)+1 work again (=> ld a,5). Somehow I broke this in v0.3.
- Filenames don't need surrounding quotes anymore.
- Macro's can be nested once again.
- Better define-handling.
- 'Textarea' detects forward references.
- Include within include searches relative to that include.
- 'Unlocked' some directives (assert, output).
- '#' Can be used in labels.
- No space needed between label and '=' in statements like 'label=value'.
- The 'export' directive now exports 'label: EQU value' instead of 'label = value'.
- Labelnames starting with a register name (like HL_kip) shouldn't confuse SjASM anymore.
- RLC, RRC, RL, RR, SLA, SRA, SLL (SLI), RES and SET undocumented instructions added.
- "ex af,af" and "ex af" are possible now.
- Added defb, defw and defs.
- Locallabels now also work correctly when used in macros.
- Added // and limited /* */ comments.
- SjASM now checks the label values between passes.
- '>>>' Operator added.
- Sources included.
- '>>>' And '>>' operators now work the way they should (I think).
- Removed the 'data/text/pool'-bug. (Together with the data/text/pool directives. ~8)
- Added endmodule and endmod to end a module.
- STRUCT directive.
- REPT, DWORD, DD, DEFD directives.
- More freedom with character constants; in some cases it's possible to use double quotes...
- It's now possible to specify include paths at the command line.
- Angle brackets are used now to include commas in macroarguments. So check your macro's.
- Fixed a structure initialization bug.
- It's not possible to use CALLs or jumps without arguments anymore.
- DZ and ABYTEZ directives to define null terminated strings.
- SjASM now checks for lines that are too long.
- Added '16 bit' LD, LDI and LDD 'instructions'. (See end of this text:)
- PUSH and POP accept a list of registers.
- Added '16 bit SUB' instruction.
- Unreferenced labels are indicated with an 'X' in the label listing.
- Unknown escapecodes in strings result in just one error (instead of more).
- Labelnameparts now have a maximum of 70 characters.
- Improved IX and IY offset checking.
- Maximum, minimum, and, or, mod, xor, not, shl, shr, low and high operators added.
- Logical operations result in -1 or 0 instead of 1 or 0. Of course, -1 for true and 0 for false ;)
- Fixed the 'ifdef <illegal identifier>' bug. (:

New in 0.39g:
--------------
- ENDMAP directive.
- DEFM and DM synonyms for BYTE.
- Some bug fixes:
 - file size is reset when a new output file is opened.
 - 'bytes lost' warning fixed.
And thanks to Konami Man:
- PHASE and DEPHASE directives as synonyms of TEXTAREA and ENDT.
- FPOS directive.
- Expanded OUTPUT directive.
- The possibility to generate a symbol file.

New in 0.39g1:
- SjASM now allows spaces in filenames.

New in 0.39g6:
- : Operator.
- MULUB and MULUW work.
- Labels starting with an underscore are listed.
- Can't remember the other changes.

Known bugs:
-----------
- The listfile doesn't always look that good.
- Commas in strings in macroarguments are not ignored.
- Sourcelines are not further processed after an ELSE, ENDwhatever and the like.
- Using INCLUDE instead of INCBIN isn't detected, and will crash SjASM quite nicely.
- The combination of comments, strings and the af' register can be very confusing for SjASM :)

Files
-----
The SjASM package consists of the following files:
sjasm.exe     - the Win32 executable.
sjasm         - the Debian Linux executable.
sjasm.txt     - this file.
sjasmsrc039g6 - the sources (directory).

RUN!
====
To use SjASM:

sjasm [options] inputfile [outputfile [listfile [exportfile]]]

Options:
-L  output a list of all defined labels in the listing
-Q  do not list the source and resulting objectcode in listfile
-I  specify a directory to search for includefiles
-S  output a .SYM symbolfile which contains all labels.

Options may be grouped, and it's permitted to place them anywhere. Examples:

sjasm -l prog.asm 
     assemble prog.asm to prog.out with prog.lst as listfile, include labels and code in listfile
sjasm -ql prog.asm
     assemble prog.asm to prog.out with prog.lst as listfile, include only the label listing in listfile
sjasm prog.asm -i/devel/inc
     assemble prog.asm to prog.out with prog.lst as listfile, include only code in listfile and search in /devel/inc for includefiles if not found in the current directory
sjasm -lidevel -izooi prog.asm prog.com
     assemble prog.asm to prog.com with prog.lst as listfile, include code and label listing in listfile and search in zooi and devel for includefiles if not found in the current directory

When the output-, list- or exportfile are omitted the following filenames are constructed:
Outputfile: inputfile-without-extension.out
Listfile: inputfile-without-extension.lst
Exportfile: inputfile-without-extension.exp

The symbolfile filenaam will always constructed as follows: exportfile-without-extension.sym

Source file format
==================
Lines in the source file should have the following form:

Label Operator Operand Comment

All fields are optional. Lines without label should start with whitespace. Comments should start with ';' or '//'. Comment blocks start with '/*' and end with '*/'.
; comment
// comment
 ld /* comment */ a,80
/*
 comment
*/
 ld /*
 but this won't be ld a,3!
 */ a,3

Labels
======
Labels are case-sensitive and may be of any reasonable length, that is: up to about 70 characters. Label definitions should start on the beginning of a line, but don't have to be followed by a colon ':'. Generally labels should start with a letter or a underscore ('_'), the following characters may be chosen from letters, numbers and the following special symbols: '_', '.', '!', '?', '#' and '@'. Note that the '.' has special meaning, as it is used between module names, labels and local labels. The following are all legal and distinct labels:
Kip
KIP
Kip@@
MAIN.loop?

It is possible to use mnemonics, pseudo-ops and register names as labels but it is not advised to do so. Also note that the identifiers defined with the DEFINE pseudo-op use another name space.

Local labels
------------
When there is a module definition (see module pseudo-op) all labels (except those starting with a '@') are local to that module. To use a label from outside the module use modulename.labelname, in this example: 'vdp.Cls'
Labels starting with a '.' are also local to the previous non-local label.

  module main
Main:                     ; main.Main
  CALL SetScreen          ; SetScreen
  CALL vdp.Cls            ; vdp.Cls
.loop:                    ; main.Main.loop
  LD A,(.event)           ; main.Main.event
  CALL ProcesEvent        ; label not found: main.ProcesEvent
  DJNZ .loop              ; main.Main.loop

  module vdp
@SetScreen:               ; SetScreen
.loop                     ; vdp.SetScreen.loop
  RET
Cls:                      ; vdp.Cls
.loop                     ; vdp.Cls.loop
  DJNZ .loop              ; vdp.Cls.loop
  RET

  endmodule
Main.event                ; main.Main.event
  byte 0

@ Labels
--------
Labels starting with a '@' are not touched by the label processing and used 'as-is'. See 'SetScreen' in the previous example code.

Another example:
  MODULE xxx
Label      ; xxx.Label
.Local     ; xxx.Label.Local
@Label     ; Label
.Local     ; xxx.Label.Local => duplicate label error
@Label2    ; Label2
.Local     ; xxx.Label2.Local
@yyy.Local ; yyy.Local
yyy.Local  ; xxx.yyy.Local

Temporary labels
----------------
To keep the number of used labels reasonable it is possible to use numbers as labels. These labels can only be used as labels to jump to. To jump to these labels, use the number followed by an 'F' for forward branches or a 'B' for backward branches. Temporary labels should not be used within macro's.
Example:
  ADD A,E
  JR NC,1F
  INC D
1 LD E,A
2 LD B,4
  LD A,(DE)
  OUT (152),A
  DJNZ 2B

Constants
=========

Numeric constants
-----------------
Numeric constants should always start with a digit or $, # or %. The following formats are supported:
12     decimal
12d    decimal
0ch    hexadecimal
0xc    hexadecimal
$c     hexadecimal
#c     hexadecimal
1100b  binary
%1100  binary
14q    octal
14o    octal

Character and string constants
------------------------------
Character constants are characters surrounded by single quotes. It is possible to use double quotes in some cases, but in general it is better to use single quotes. String constants are characters surrounded by double quotes. The following escape sequences are recognized:
\\ 92
\? 63
\' 39
\" 34
\A 7
\B 8
\D 127
\E 27
\F 12
\N 10
\R 13
\T 9
\V 11

Examples:
  BYTE "stringconstant"  ; single quote string constants can't be used with BYTE
  LD HL,'hl'
  LD HL,"hl" ; :(
  LD A,"7"   ; :(
  LD A,'8'   ; :)
  LD A,'\E'
  LD A,'"'
  LD A,"'"

Expressions
===========
Expressions are evaluated in 32 bits in this version of SjASM. '$' Represents the current program counter. It is possible to use parenthesis '(' and ')' to override the precedence of the operators. The following operators may be used in expressions:

!     !x       logical not
~     ~x       complement
+     +x       does absolutely nothing :)
-     -x       minus
low   low x    low 8 bits of 16 bit value
high  high x   high 8 bits of 16 bit value
not   not x    logical not

:     x:y      x multiplied by 256, added to y

*     x*y      multiplication
/     x/y      division
%     x%y      modulo
mod   x mod y  modulo

+     x+y      addition
-     x-y      subtraction

<<    x<<y     shift left
>>    x>>y     shift right signed
>>>   x>>>y    shift right unsigned
shl   x shl y  shift left
shr   x shr y  shift right signed

<?    x<?y     minimum
>?    x>?y     maximum

<     x<y      less than
>     x>y      greater than
<=    x<=y     equal or less than
>=    x>=y     equal or greater than

=     x=y      equal
==    x==y     equal
!=    x!=y     not equal

&     x&y      bitwise and
and   x and y  bitwise and

^     x^y      bitwise xor
xor   x xor y  bitwise xor

|     x|y      bitwise or
or    x or y   bitwise or

&&    x&&y     logical and

||    x||y     logical or


Assembly language
=================
This version only accepts Z80 mnemonics. There are some additions to what I think is standard Z80:
- '[' and ']' can be used in stead of '(' and ')' for indirections. So LD A,[HL] is the same as LD A,(HL).
- IN F,(C) and OUT (C),0 and SLL/SLI can be used.
- IXL, IYL, IXH and IYH registers are supported.
- JP HL, JP IX and JP IY may be used instead of JP (HL), etc.
- EX AF,AF or EX AF may be used instead of EX AF,AF'.
- MULUB and MULUW are recognised (but won't work on Z80, of course:)
- RLC, RRC, RL, RR, SLA, SRA, SLL (SLI), RES, SET undocumented instructions added.
  examples:
    SET 4,(IX+4),C (aka LD C,SET 4,(IX+4)) is LD C,(IX+4) / SET 4,C / LD (IX+4),C
    RRC (IY),A     (aka LD A,RRC (IY+0))   is LD A,(IY)   / RRC A   / LD (IY),A
- PUSH and POP can take register lists:
    PUSH AF,BC  ; push af / push bc
    POP  AF,BC  ; pop  bc / pop  af <- reversed order

Pseudo-ops
==========
Pseudo-ops don't have to start with a '.' anymore. However, it is still permitted.

ABYTE <offset> <bytes>
-----
Defines a byte or a string of bytes. The offset is added to each of the following bytes.

  ABYTE 2 4,9    ; Same as BYTE 6,11
  ABYTE 3 "ABC"  ; Same as BYTE "DEF"

ABYTEC <offset> <bytes>
------
Defines a byte or a string of bytes, where the last byte of the string will have bit 7 set. The offset is added to each of the following bytes.

  ABYTEC 0 "KIP"        ; Same as BYTE "KI",'P'|128
  ABYTEC 1 "ABC",0,"DE" ; Same as BYTE "BC",'D'|128,1,'E','F'|128

ABYTEZ <offset> <bytes>
------
Defines a byte or a string of bytes, followed by a zero. The offset is added to each of the following bytes.

  ABYTEZ 0 "KIP"        ; Same as BYTE "KIP",0

ALIGN <2,4,8,16,32,64,128 or 256>
-----
Align fills zero or more byte with zeros until the new address modulo <expression> equals zero.

  ALIGN         ; => ALIGN 4
  ALIGN 2       ; 

ASSERT <expression>
------
An 'assertion failed' error is issued if the expression evaluates to zero.

STACKPOINTER=0D500H
  ASSERT END_OF_PROGRAM < STACKPOINTER
END_OF_PROGRAM
  END

BLOCK <length>[,<fill byte>]
-----
Defines space. Has to be followed by the number of byte to reserve, optionally followed by the value to fill these bytes with.

  BLOCK 500     ; define a block of 500 bytes of zero
  BLOCK 500,0   ; define a block of 500 bytes of zero
  BLOCK 400,-1  ; define a block of 400 bytes of 255

BYTE <bytes>
----
Defines a byte or a string of bytes. Each value should be between -129 and 256.
  BYTE 0x56
  BYTE 1,-78,'@'
  BYTE "Format C:? ",0h

DB
--
See BYTE.

DC
--
Same as BYTE, but every last character of a string will have bit 7 set.

  DC "kip" ; same as BYTE "ki",'p'|128

DD
--
See DWORD.

DEFB
----
See BYTE.

DEFD
----
See DWORD.

DEFINE <id> <replacement>
------
The identifier <id> will be replaced with the <replacement>. The replacement could be omitted, in such case it is still possible to check if the identifier was defined with IFDEF or IFNDEF.

  DEFINE str_honderd "Honderd"
  BYTE str_honderd,0             ; BYTE "Honderd",0

DEFS
----
See BLOCK.

DEFW
----
See WORD.

DEPHASE
-------
See ENDT.

DS
--
See BLOCK.

DW
--
See WORD.

DWORD
-----
Defines a so called doubleword. Values should be between -2147483649 and 4294967296.

  DWORD 4000h,0d000h
  DWORD 4

DZ
--
Same as BYTE, but an extra zero will be added at the end.

  DZ 1      ; same as BYTE 1,0
  DZ "kip"  ; same as BYTE "kip",0

END
---
The assembler will stop at this point.

ENDMAP
------

Restores the mapcounter with the value it had before the last MAP directive.

  MAP 100
  MAP 200
kop  # 2    ; kop=200
  ENDMAP
kip  # 2    ; kip=100
kip2 # 2    ; kip2=102

ENDMOD
------
See ENDMODULE.

ENDMODULE
---------
To indicate the end of a module (see MODULE), and use the previous modulename.

  MODULE M1
A                 ; M1.A
  MODULE M2
A                 ; M2.A
  ENDMODULE
B                 ; M1.B

EQU
---
To give a label a value other than the current program counter. '=' can be used instead of 'EQU'. The label should not already exist.

Label EQU 3
Kip=3

EXPORT label
------
The named label will be written to the export-file, in the form 'label: EQU value'. This way the export-file can be included in other sources.

DRIE=3
  EXPORT DRIE

FIELD
-----
To give a label the value of the current map counter. Afterwards the map counter is increment by the given amount. '#' May be used instead of 'FIELD'. With map and field it is possible to create structure-like data structures. With '##' it is possible to align the map counter.

  MAP 8
Label # 2     ; Label=8
Kip   # 3     ; Kip=10
Kop   #       ; Kop=13
Kop2  # 1     ; Kop2=13
     ##       ; align map address (align 4 is default)
Kop3  # 6     ; Kop3=16

FPOS <position>
----
The FPOS directive makes it possible to set the file position to anywhere in the output file.

This example will result in a file with a length of one byte:
  BYTE 0
  FPOS 0
  BYTE 1
  END

In combination with OUTPUT "<filename>",r it is possible to update existing files.

INCBIN <filename>[,offset[,length]]
------
To include a binary file into the outputfile. The offset and length are optional.

  INCBIN "gfx.scc",7        ; include gfx.scc, skip first 7 bytes
  INCBIN "rantab.com",3,256 ; include 256 bytes from offset 3
  INCBIN gfx.scc ,7         ; note the space between the filename and the ',7' here :)

INCLUDE <filename>
-------
To include another sourcefile into the current. Sourcefiles can be nested 20 levels deep. If the file cannot be found in the current directory (the current directory is the directory the current file comes from) the file will be searched for in the directories specified at the commandline. When angle brackets are used, the commandline directories are searched before the current directory.

  INCLUDE <VDP.I>
  INCLUDE MORE.I
  INCLUDE "MORE.I"

MAP <address>
---
Set the map counter to the specified value. See 'FIELD' for an example.
  MAP 5

MODULE <name>
------
Labels are to be unique only in the current module. Also note the use of '@' to suppress all this label-processing. (The '@' is NOT part of the label name though!)

  MODULE xxx
Kip                ; label xxx.Kip
  CALL Kip         ; call xxx.Kip
  CALL yyy.Kip     ; call yyy.Kip
  CALL Kop         ; call xxx.Kop
  CALL @Kop        ; call Kop
  Call @Kip        ; call Kip

  MODULE yyy
Kip                ; label yyy.Kip
@Kop               ; label Kop
@xxx.Kop           ; label xxx.Kop

  MODULE           ; no modulename
Kip                ; label Kip

ORG <address>
---
Set the program counter to a specific address.

  ORG 100h

OUTPUT "<filename>",mode
------
With OUTPUT it is possible to create multiple files from one source. All following instructions will be assembled to this file.
There are three possible output modes: truncate (overwrite existing files, this is the default), rewind (open and execute FPOS 0) and append (open and leave the file pointer at the end of the file).

OUTPUT "<filename>",t  ; truncate (default)
OUTPUT "<filename>",r  ; rewind
OUTPUT "<filename>",a  ; append

Example:

  OUTPUT loader.com
  ORG 100H
  INCLUDE loader.asm
  INCLUDE bios.asm

  OUTPUT bigfile.dat
  ORG 4000H
  INCLUDE main.asm
  ORG 8000H
  INCLUDE data.asm

  END

This will create two files: loader.com and bigfile.dat.
When SjASM is invoked without specifying an output file, there is still one created even when no bytes are output to it. So when the above file is called bigfile.asm, and assembled with the following line:
sjasm bigfile.asm
The following files are created:
Bigfile.out  ; file length is zero
Loader.com
Bigfile.dat

PHASE
-----
See TEXTAREA.

REPT <count>
----
REPT specifies the number of times to generate the statements inside the macro. REPT cannot be used in macro's.

  REPT 3
  NOP
  ENDM

this will expand to:
  NOP
  NOP
  NOP

SIZE <filesize in bytes>
----
If the resulting file is less than the given length, as many bytes are added as necessary.

  SIZE 32768       ; make sure file will be 32K

TEXTAREA <address>
--------
The program counter is set to the specified address, and restored with an 'ENDT' directive.

  MODULE Main
  LD HL,start
  LD DE,8000h
  LD BC,len
  LDIR
  CALL DoThings
  RET

start
  MODULE DoUsefulThings
  TEXTAREA 8000h
@DoThings
  CALL DoALot
  CALL DoEvenMore
  RET
  ENDT
  MODULE Main
Len:=$-start
 
WORD <words>
----
Defines a word. Values should be between -32787 and 65536.

  WORD 4000h,0d000h
  WORD 4,"HA"


Conditional assembly
====================

It may be useful to assemble a part or not based on a certain condition.

IF <expression>
--
If <expression> is non-zero the following lines are assembled until an ELSE or ENDIF.

IFDEF <id>
-----
The condition is true if there is an id defined. These are NOT labels.

  IFDEF MSX_LEAN_AND_MEAN
  CALL InitOwnMM
  ELSE
  CALL InitDos2MemMan
  ENDIF

IFNDEF <id>
------
The condition is true if there isn't an id defined. These are NOT labels.

1 IN A,(0C4H)
  AND 2
  IFNDEF DEBUG
  JR NC,1B
  ENDIF

ELSE
----
See IF. If the condition is not true, the else-part is assembled.

ENDIF
-----
Every IF should be followed by an ENDIF.


Macro's
=======
The MACRO pseudo-op defines a macro. It should be followed by the name of the macro, optionally followed by the parameters. The following lines will be stored as the macro-body until an ENDM pseudo-op is encountered. Macro's have to be defined before their use.

Macro without parameters:
  MACRO ADD_HL_A
  ADD A,L
  JR NC,.hup
  INC H
.hup
  LD L,A
  ENDM

Labels in a macro starting with a dot are local to each macro expansion.

A macro with parameters:
  MACRO WAVEOUT reg, data
  LD A,reg
  OUT (7EH),A
  LD A,data
  OUT (7FH),A
  ENDM
This macro will make
  WAVEOUT 2,17
Expand to:
  LD A,2
  OUT (7EH),A
  LD A,17
  OUT (7FH),A

Another example:
  MACRO LOOP
    IF $-.lus<127
      DJNZ .lus
    ELSE
      DEC B
      JP NZ,.lus
    ENDIF
  ENDM

Main
.lus
  CALL DoALot
  LOOP
This will expand to:
Main
.lus                ; Main.lus
  call DoALot
  DJNZ .lus         ; Main.lus

Angle brackets can be used when the arguments contain commas:
  MACRO UseLess data
    DB data
  ENDM

  UseLess <10,12,13,0>
Expands to:
  DB 10,12,13,0

Use '!' to include '!' and '>' in those strings:

  UseLess <5, 6 !> 3>
Expands to:
  DB 5, 6 > 3

  UseLess <"Kip!!",3>
Expands to:
  DB "Kip!",3

Structures
==========
Structures can be used to define data structures in memory more easily. The name of the structure is set to the total size of the structure.

A structure definition starts with: 'STRUCT <name>,[<initial offset>]' and ends with 'ENDS'. Structure definitions are local to the current module, but, as with labels, '@' can be used to override this.
Lines between STRUCT and ENDS should have the following format:

membername pseudo-operation operands

All fields are optional. Lines without label should start with whitespace.
Between the STRUCT and ENDS pseudo-instructions the following instructions can be used:

BYTE [<defaultvalue>]
----
To define a one byte member. The defaultvalue is used when no initialisation value is given when the structure is declared. (DB and DEFB may be used instead of BYTE).

WORD [<defaultvalue>]
----
To define a two byte member. The defaultvalue is used when no initialisation value is given when the structure is declared. (DW and DEFW may be used instead of WORD).

D24 [<defaultvalue>]
---
To define a three byte member. The defaultvalue is used when no initialisation value is given when the structure is declared.

DWORD [<defaultvalue>]
----
To define a four byte member. The defaultvalue is used when no initialisation value is given when the structure is declared. (DD and DEFD may be used instead of WORD).

BLOCK <length>[,<fillbyte>]]
-----
To define an member of the specified number of bytes. ('#', DS and DEFS may be used instead of WORD).

ALIGN [<expression>]
-----
To align the offset. If the expression is omitted, 4 is assumed. ('##' May be used instead of ALIGN).

<structure name> [<init values>]
----------------
It is possible to nest structures, and give new defaults for the BYTE and WORD members.


Examples
--------
  STRUCT SCOLOR
RED   BYTE 4
GREEN BYTE 5
BLUE  BYTE 6
  ENDS

This is identical to:
SCOLOR       EQU 3 ; lenght
SCOLOR.RED   EQU 0 ; offset
SCOLOR.GREEN EQU 1 ; offset
SCOLOR.BLUE  EQU 2 ; offset

  STRUCT SDOT
X BYTE
Y BYTE
C SCOLOR 0,0,0   ; use new default values
  ENDS

This is identical to:
SDOT         EQU 5 ; length
SDOT.X       EQU 0 ; offset
SDOT.Y       EQU 1 ; offset
SDOT.C       EQU 2 ; offset
SDOT.C.RED   EQU 2 ; offset
SDOT.C.GREEN EQU 3 ; offset
SDOT.C.BLUE  EQU 4 ; offset

  STRUCT SPOS,4
X WORD
Y BYTE
  ALIGN 2
AD WORD
  ENDS

This is identical to:
SPOS    EQU 10 ; length
SPOS.X  EQU  4 ; offset
SPOS.Y  EQU  6 ; offset
SPOS.AD EQU  8 ; offset


When a structure is defined it is possible to declare labels with it:

COLOR SCOLOR

This is identical to:
COLOR
COLOR.RED   BYTE 4
COLOR.GREEN BYTE 5
COLOR.BLUE  BYTE 6
Note the default values.

Or without label:
COLORTABLE
  SCOLOR 0,0,0
  SCOLOR 1,2,3
  SCOLOR ,2
  ; etc.

This is identical to:
COLORTABLE
  BYTE 0,0,0
  BYTE 1,2,3
  BYTE 4,2,6
  ; etc.

DOT1 SDOT 0,0, 0,0,0     ; or 0,0,0,0,0 or {0,0,{0,0,0}}

Only BYTE and WORD members can be initialised.

The resulting labels can be used as any other label:
  ld b,(ix+SCOLOR.RED)
  ld a,(COLOR.GREEN)
  ld de,COLOR
  ; etc.

BUT!

Do not use the offset labels in indirections like:
  LD A,(SDOT.X)
This will conflict with futher 'improvements' ;-)

If this is absolutely necessary (why?) use something like this:
  LD A,(+SDOT.X)


"Extended instructions"
=======================

Of course the Z80 is only an 8 bit cpu, but sometimes ld hl,de would be nice. SjASM now 'fakes' some instructions like that. This improves the readability of the source, but it might not be the fastest way to get the result. Also possibly some 'new' load instructions do affect the flags in ways you wouldn't expect.
Anyway, here's the list:

  rl bc
  rl de
  rl hl
  rr bc
  rr de
  rr hl
  sla bc
  sla de
  sla hl
  sll bc
  sll de
  sll hl
  sli bc
  sli de
  sli hl
  sra bc
  sra de
  sra hl
  srl bc
  srl de
  srl hl

  ld bc,bc
  ld bc,de
  ld bc,hl
  ld bc,ix
  ld bc,iy
  ld bc,(hl)
  ld bc,(ix+nn)
  ld bc,(iy+nn)

  ld de,bc
  ld de,de
  ld de,hl
  ld de,ix
  ld de,iy
  ld de,(hl)
  ld de,(ix+nn)
  ld de,(iy+nn)

  ld hl,bc
  ld hl,de
  ld hl,hl
  ld hl,ix
  ld hl,iy
  ld hl,(ix+nn)
  ld hl,(iy+nn)

  ld ix,bc
  ld ix,de
  ld ix,hl
  ld ix,ix
  ld ix,iy

  ld iy,bc
  ld iy,de
  ld iy,hl
  ld iy,ix
  ld iy,iy

  ld (hl),bc
  ld (hl),de

  ld (ix+nn),bc
  ld (ix+nn),de
  ld (ix+nn),hl

  ld (iy+nn),bc
  ld (iy+nn),de
  ld (iy+nn),hl

  ldi bc,(hl)
  ldi bc,(ix+nn)
  ldi bc,(iy+nn)

  ldi de,(hl)
  ldi de,(ix+nn)
  ldi de,(iy+nn)

  ldi hl,(ix+nn)
  ldi hl,(iy+nn)

  ldi (hl),bc
  ldi (hl),de

  ldi (ix+nn),bc
  ldi (ix+nn),de
  ldi (ix+nn),hl

  ldi (iy+nn),bc
  ldi (iy+nn),de
  ldi (iy+nn),hl

  ldi a,(bc)
  ldi a,(de)
  ldi a,(hl)
  ldi b,(hl)
  ldi c,(hl)
  ldi d,(hl)
  ldi e,(hl)
  ldi h,(hl)
  ldi l,(hl)
  ldi a,(ix+nn)
  ldi b,(ix+nn)
  ldi c,(ix+nn)
  ldi d,(ix+nn)
  ldi e,(ix+nn)
  ldi h,(ix+nn)
  ldi l,(ix+nn)
  ldi a,(iy+nn)
  ldi b,(iy+nn)
  ldi c,(iy+nn)
  ldi d,(iy+nn)
  ldi e,(iy+nn)
  ldi h,(iy+nn)
  ldi l,(iy+nn)

  ldd a,(bc)
  ldd a,(de)
  ldd a,(hl)
  ldd b,(hl)
  ldd c,(hl)
  ldd d,(hl)
  ldd e,(hl)
  ldd h,(hl)
  ldd l,(hl)
  ldd a,(ix+nn)
  ldd b,(ix+nn)
  ldd c,(ix+nn)
  ldd d,(ix+nn)
  ldd e,(ix+nn)
  ldd h,(ix+nn)
  ldd l,(ix+nn)
  ldd a,(iy+nn)
  ldd b,(iy+nn)
  ldd c,(iy+nn)
  ldd d,(iy+nn)
  ldd e,(iy+nn)
  ldd h,(iy+nn)
  ldd l,(iy+nn)

  ldi (bc),a
  ldi (de),a
  ldi (hl),a
  ldi (hl),b
  ldi (hl),c
  ldi (hl),d
  ldi (hl),e
  ldi (hl),h
  ldi (hl),l
  ldi (ix+nn),a
  ldi (ix+nn),b
  ldi (ix+nn),c
  ldi (ix+nn),d
  ldi (ix+nn),e
  ldi (ix+nn),h
  ldi (ix+nn),l
  ldi (iy+nn),a
  ldi (iy+nn),b
  ldi (iy+nn),c
  ldi (iy+nn),d
  ldi (iy+nn),e
  ldi (iy+nn),h
  ldi (iy+nn),l
   
  ldd (bc),a
  ldd (de),a
  ldd (hl),a
  ldd (hl),b
  ldd (hl),c
  ldd (hl),d
  ldd (hl),e
  ldd (hl),h
  ldd (hl),l
  ldd (ix+nn),a
  ldd (ix+nn),b
  ldd (ix+nn),c
  ldd (ix+nn),d
  ldd (ix+nn),e
  ldd (ix+nn),h
  ldd (ix+nn),l
  ldd (iy+nn),a
  ldd (iy+nn),b
  ldd (iy+nn),c
  ldd (iy+nn),d
  ldd (iy+nn),e
  ldd (iy+nn),h
  ldd (iy+nn),l

  ldi (hl),nn
  ldi (ix+nn),nn
  ldi (iy+nn),nn

  ldd (hl),nn
  ldd (ix+nn),nn
  ldd (iy+nn),nn

  sub hl,bc
  sub hl,de
  sub hl,hl
  sub hl,sp

ldi increases the data pointer after the data access, so LDI A,(HL) is the same as LD A,(HL)\ INC HL. likewise, LDD A,(DE) is LD A,(DE)\ DEC DE.


=====
<EOF> SjASM v0.3
