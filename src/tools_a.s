# ---------------------------------------------------------------
# Quicksort for MC68000
# by Stephane Dallongeville @2016
# original implementation by (c) 2014 Dale Whinham
#
# void qsort(void** src, u16 size, _comparatorCallback* cb);
# ---------------------------------------------------------------

#include "sgdk_asm_macros.h"

func qsort
    movem.l %a2-%a6,-(%sp)

    move.l  24(%sp),%a2             | a2 = src
    move.l  28(%sp),%d0             | d0 = size (limited to 0x3FFF)
    move.l  32(%sp),%a6             | a6 = comparator

    add.w   %d0,%d0
    add.w   %d0,%d0
    lea     -4(%a2,%d0.w),%a3       | a3 = &src[size - 1]

    bsr quicksort                   | start sort !

    movem.l (%sp)+,%a2-%a6
    rts

# ---------------------------------------------------------------
# Quicksort
# a2 - left pointer
# a3 - right pointer
# ---------------------------------------------------------------

quicksort:
    cmp.l   %a2,%a3                 | L >= R ?
    ble     .endqsort               | done !

    bsr     partition               | index of partition is in a1

    movem.l %a1-%a3,-(%sp)          | save P,L,R

    lea     -4(%a1),%a3             | R = P-1

    bsr     quicksort               | quicksort(L, P-1)

    movem.l (%sp),%a1-%a3           | restore P,L,R
    lea     4(%a1),%a2              | L = P+1
    move.l  %a2,4(%sp)              | save changed L

    bsr     quicksort               | quicksort(P+1, R)

    movem.l (%sp)+,%a1-%a3          | restore P,L,R

.endqsort:
    rts

# ---------------------------------------------------------------
# Partition
# a2 - left pointer
# a3 - right pointer
# return pivot in a1
# ---------------------------------------------------------------
partition:
    move.l  %a2,%a4                 | a4 = L
    move.l  %a3,%a5                 | a5 = R = P

|    move.l  %a3,%d0
|    add.l   %a2,%d0
|    asr.l   %d0                     | d0 = P = (L+R)/2
|    and.w   0xFFFC,%d0              | clear bit 0 & 1
|    move.l  %d0,%a0                 | a0 = P

    move.l  (%a5),-(%sp)            | reserve space for comparator arguments, put (*L,*P) by default
    move.l  (%a4),-(%sp)

    lea     -4(%a5),%a0             | tmp = next R
    cmp.l   %a4,%a0                 | L >= R ?
    ble     .finish                 | done !

.loop:
.findleft:
    move.l  (%a4)+,(%sp)            | first argument comparator = *L
    jsr     (%a6)                   | compare

    tst.w   %d0                     | *L < *P
    blt     .findleft

    lea     -4(%a4),%a4             | L fix (put on value to swap)

.findright:
    move.l  -(%a5),(%sp)            | first argument comparator = *R
    jsr     (%a6)                   | compare

    tst.w   %d0                     | *R > *P
    bgt     .findright

    cmp.l   %a4,%a5                 | L >= R ?
    ble     .finish                 | done !

    move.l  (%a4),%d0               | swap *R / *L
    move.l  (%a5),(%a4)+            | L++
    move.l  %d0,(%a5)

    lea     -4(%a5),%a0             | tmp = next R
    cmp.l   %a4,%a0                 | R > L ?
    bhi     .loop                   | continue !

.finish:
    move.l  (%a4),%d0               | swap *L / *P
    move.l  (%a3),(%a4)
    move.l  %d0,(%a3)

    move.l  %a4,%a1                 | a1 = L = new pivot

    lea     8(%sp),%sp              | release space for comparator arguments
    rts


# -------------------------------------------------------------------------------------------------
# Aplib decruncher for MC68000 "gcc version"
# by MML 2010
# Size optimized (164 bytes) by Franck "hitchhikr" Charlet.
# More optimizations by r57shell.
#
# aplib_decrunch: A0 = Source / A1 = Destination / Returns unpacked size
# u32 aplib_unpack(u8 *src, u8 *dest); /* c prototype */
#
# -------------------------------------------------------------------------------------------------

func aplib_unpack
    movem.l 4(%a7),%a0-%a1

aplib_decrunch:
    movem.l %a2-%a6/%d2-%d5,-(%a7)

    lea     32000.w,%a3
    lea     1280.w,%a4
    lea     128.w,%a5
    move.l  %a1,%a6
    moveq   #-128,%d3

.copy_byte:
    move.b  (%a0)+,(%a1)+

.next_sequence_init:
    moveq   #2,%d1                  | Initialize LWM

.next_sequence:
    bsr.b   .get_bit
    bcc.b   .copy_byte              | if bit sequence is %0..., then copy next byte

    bsr.b   .get_bit
    bcc.b   .code_pair              | if bit sequence is %10..., then is a code pair

    moveq   #0,%d0                  | offset = 0 (eor.l %d0,%d0)
    bsr.b   .get_bit
    bcc.b   .short_match            | if bit sequence is %110..., then is a short match

    moveq   #4-1,%d5                | The sequence is %111..., the next 4 bits are the offset (0-15)
.get_3_bits:
    bsr.b   .get_bit
    roxl.l  #1,%d0                  | addx.l  %d0,%d0 <- my bug, Z flag only cleared, not SET
    dbf     %d5,.get_3_bits         | (dbcc doesn't modify flags)
    beq.b   .write_byte             | if offset == 0, then write 0x00

                                    | If offset != 0, then write the byte on destination - offset
    move.l  %a1,%a2
    suba.l  %d0,%a2
    move.b  (%a2),%d0

.write_byte:
    move.b  %d0,(%a1)+
    bra.b   .next_sequence_init

.short_match:                       | Short match %110...
    moveq   #3,%d2                  | length = 3
    move.b  (%a0)+,%d0              | Get offset (offset is 7 bits + 1 bit to mark if copy 2 or 3 bytes)
    lsr.b   #1,%d0
    beq.b   .end_decrunch           | if offset == 0, end of decrunching
    bcs.b   .domatch_new_lastpos
    moveq   #2,%d2                  | length = 2
    bra.b   .domatch_new_lastpos

.code_pair:                         | Code pair %10...
    bsr.b   .decode_gamma
    sub.l   %d1,%d2                 | offset -= LWM
    bne.b   .normal_code_pair
    move.l  %d4,%d0                 | offset = old_offset
    bsr.b   .decode_gamma
    bra.b   .copy_code_pair

.normal_code_pair:
    subq.l  #1,%d2                  | offset -= 1
    lsl.l   #8,%d2                  | offset << 8
    move.b  (%a0)+,%d2              | get the least significant byte of the offset (16 bits)
    move.l  %d2,%d0
    bsr.b   .decode_gamma
    cmp.l   %a3,%d0                 | >=32000
    bge.b   .domatch_with_2inc

.compare_1280:
    cmp.l   %a4,%d0                 | >=1280 <32000
    bge.b   .domatch_with_inc

.compare_128:
    cmp.l   %a5,%d0                 | >=128 <1280
    bge.b   .domatch_new_lastpos

.domatch_with_2inc:
    addq.l  #1,%d2
.domatch_with_inc:
    addq.l  #1,%d2
.domatch_new_lastpos:
    move.l  %d0,%d4                 | old_offset = offset
.copy_code_pair:
    subq.l  #1,%d2                  | length--
    move.l  %a1,%a2
    suba.l  %d0,%a2

.loop_do_copy:
    move.b  (%a2)+,(%a1)+
    dbf     %d2,.loop_do_copy
    moveq   #1,%d1                  | LWM = 1
    bra.b   .next_sequence          | Process next sequence

.get_bit:                           | Get bits from the crunched data (D3) and insert the most significant bit in the carry flag.
    add.b   %d3,%d3
    bne.b   .still_bits_left
    move.b  (%a0)+,%d3              | Read next crunched byte
    addx.b  %d3,%d3

.still_bits_left:
    rts

.decode_gamma:                      | Decode values from the crunched data using gamma code
    moveq   #1,%d2

.get_more_gamma:
    bsr.b   .get_bit
    addx.l  %d2,%d2
    bsr.b   .get_bit
    bcs.b   .get_more_gamma
    rts

.end_decrunch:
    move.l %a1,%d0
    sub.l %a6,%d0                   | d0 = unpacked size

    movem.l (%a7)+,%a2-%a6/%d2-%d5
    rts


# ---------------------------------------------------------------------------
# LZ4W unpacker for MC68000
# by Stephane Dallongeville @2017
#
# lz4w_unpack_a: A0 = Source / A1 = Destination / Returns unpacked size
# u16 lz4w_unpack(const u8 *src, u8 *dest);  /* c prototype */
# ---------------------------------------------------------------------------

func lz4w_unpack
    move.l  4(%sp),%a0              | a0 = src
    move.l  8(%sp),%a1              | a1 = dst

lz4w_unpack_a:
    movem.l %a2-%a4,-(%sp)

    lea     .next,%a3               | used for fast jump
    moveq   #0,%d1

.next:
    moveq   #0,%d0
    move.b  (%a0)+,%d0              | d0 = literal & match length
    move.b  (%a0)+,%d1              | d1 = match offset

    add.w   %d0,%d0
    add.w   %d0,%d0

.jump_base:
    move.l  (.jump_table-.jump_base)-2(%pc,%d0.w),%a4
    jmp     (%a4)

    .align 2
.jump_table:

    .long .lit0_mat0
    .long .lit0_mat1
    .long .lit0_mat2
    .long .lit0_mat3
    .long .lit0_mat4
    .long .lit0_mat5
    .long .lit0_mat6
    .long .lit0_mat7
    .long .lit0_mat8
    .long .lit0_mat9
    .long .lit0_matA
    .long .lit0_matB
    .long .lit0_matC
    .long .lit0_matD
    .long .lit0_matE
    .long .lit0_matF
    .long .lit1_mat0
    .long .lit1_mat1
    .long .lit1_mat2
    .long .lit1_mat3
    .long .lit1_mat4
    .long .lit1_mat5
    .long .lit1_mat6
    .long .lit1_mat7
    .long .lit1_mat8
    .long .lit1_mat9
    .long .lit1_matA
    .long .lit1_matB
    .long .lit1_matC
    .long .lit1_matD
    .long .lit1_matE
    .long .lit1_matF
    .long .lit2_mat0
    .long .lit2_mat1
    .long .lit2_mat2
    .long .lit2_mat3
    .long .lit2_mat4
    .long .lit2_mat5
    .long .lit2_mat6
    .long .lit2_mat7
    .long .lit2_mat8
    .long .lit2_mat9
    .long .lit2_matA
    .long .lit2_matB
    .long .lit2_matC
    .long .lit2_matD
    .long .lit2_matE
    .long .lit2_matF
    .long .lit3_mat0
    .long .lit3_mat1
    .long .lit3_mat2
    .long .lit3_mat3
    .long .lit3_mat4
    .long .lit3_mat5
    .long .lit3_mat6
    .long .lit3_mat7
    .long .lit3_mat8
    .long .lit3_mat9
    .long .lit3_matA
    .long .lit3_matB
    .long .lit3_matC
    .long .lit3_matD
    .long .lit3_matE
    .long .lit3_matF
    .long .lit4_mat0
    .long .lit4_mat1
    .long .lit4_mat2
    .long .lit4_mat3
    .long .lit4_mat4
    .long .lit4_mat5
    .long .lit4_mat6
    .long .lit4_mat7
    .long .lit4_mat8
    .long .lit4_mat9
    .long .lit4_matA
    .long .lit4_matB
    .long .lit4_matC
    .long .lit4_matD
    .long .lit4_matE
    .long .lit4_matF
    .long .lit5_mat0
    .long .lit5_mat1
    .long .lit5_mat2
    .long .lit5_mat3
    .long .lit5_mat4
    .long .lit5_mat5
    .long .lit5_mat6
    .long .lit5_mat7
    .long .lit5_mat8
    .long .lit5_mat9
    .long .lit5_matA
    .long .lit5_matB
    .long .lit5_matC
    .long .lit5_matD
    .long .lit5_matE
    .long .lit5_matF
    .long .lit6_mat0
    .long .lit6_mat1
    .long .lit6_mat2
    .long .lit6_mat3
    .long .lit6_mat4
    .long .lit6_mat5
    .long .lit6_mat6
    .long .lit6_mat7
    .long .lit6_mat8
    .long .lit6_mat9
    .long .lit6_matA
    .long .lit6_matB
    .long .lit6_matC
    .long .lit6_matD
    .long .lit6_matE
    .long .lit6_matF
    .long .lit7_mat0
    .long .lit7_mat1
    .long .lit7_mat2
    .long .lit7_mat3
    .long .lit7_mat4
    .long .lit7_mat5
    .long .lit7_mat6
    .long .lit7_mat7
    .long .lit7_mat8
    .long .lit7_mat9
    .long .lit7_matA
    .long .lit7_matB
    .long .lit7_matC
    .long .lit7_matD
    .long .lit7_matE
    .long .lit7_matF
    .long .lit8_mat0
    .long .lit8_mat1
    .long .lit8_mat2
    .long .lit8_mat3
    .long .lit8_mat4
    .long .lit8_mat5
    .long .lit8_mat6
    .long .lit8_mat7
    .long .lit8_mat8
    .long .lit8_mat9
    .long .lit8_matA
    .long .lit8_matB
    .long .lit8_matC
    .long .lit8_matD
    .long .lit8_matE
    .long .lit8_matF
    .long .lit9_mat0
    .long .lit9_mat1
    .long .lit9_mat2
    .long .lit9_mat3
    .long .lit9_mat4
    .long .lit9_mat5
    .long .lit9_mat6
    .long .lit9_mat7
    .long .lit9_mat8
    .long .lit9_mat9
    .long .lit9_matA
    .long .lit9_matB
    .long .lit9_matC
    .long .lit9_matD
    .long .lit9_matE
    .long .lit9_matF
    .long .litA_mat0
    .long .litA_mat1
    .long .litA_mat2
    .long .litA_mat3
    .long .litA_mat4
    .long .litA_mat5
    .long .litA_mat6
    .long .litA_mat7
    .long .litA_mat8
    .long .litA_mat9
    .long .litA_matA
    .long .litA_matB
    .long .litA_matC
    .long .litA_matD
    .long .litA_matE
    .long .litA_matF
    .long .litB_mat0
    .long .litB_mat1
    .long .litB_mat2
    .long .litB_mat3
    .long .litB_mat4
    .long .litB_mat5
    .long .litB_mat6
    .long .litB_mat7
    .long .litB_mat8
    .long .litB_mat9
    .long .litB_matA
    .long .litB_matB
    .long .litB_matC
    .long .litB_matD
    .long .litB_matE
    .long .litB_matF
    .long .litC_mat0
    .long .litC_mat1
    .long .litC_mat2
    .long .litC_mat3
    .long .litC_mat4
    .long .litC_mat5
    .long .litC_mat6
    .long .litC_mat7
    .long .litC_mat8
    .long .litC_mat9
    .long .litC_matA
    .long .litC_matB
    .long .litC_matC
    .long .litC_matD
    .long .litC_matE
    .long .litC_matF
    .long .litD_mat0
    .long .litD_mat1
    .long .litD_mat2
    .long .litD_mat3
    .long .litD_mat4
    .long .litD_mat5
    .long .litD_mat6
    .long .litD_mat7
    .long .litD_mat8
    .long .litD_mat9
    .long .litD_matA
    .long .litD_matB
    .long .litD_matC
    .long .litD_matD
    .long .litD_matE
    .long .litD_matF
    .long .litE_mat0
    .long .litE_mat1
    .long .litE_mat2
    .long .litE_mat3
    .long .litE_mat4
    .long .litE_mat5
    .long .litE_mat6
    .long .litE_mat7
    .long .litE_mat8
    .long .litE_mat9
    .long .litE_matA
    .long .litE_matB
    .long .litE_matC
    .long .litE_matD
    .long .litE_matE
    .long .litE_matF
    .long .litF_mat0
    .long .litF_mat1
    .long .litF_mat2
    .long .litF_mat3
    .long .litF_mat4
    .long .litF_mat5
    .long .litF_mat6
    .long .litF_mat7
    .long .litF_mat8
    .long .litF_mat9
    .long .litF_matA
    .long .litF_matB
    .long .litF_matC
    .long .litF_matD
    .long .litF_matE
    .long .litF_matF

.lm_len_FF:
	move.w  (%a2)+,(%a1)+
.lm_len_FE:
	move.w  (%a2)+,(%a1)+
.lm_len_FD:
	move.w  (%a2)+,(%a1)+
.lm_len_FC:
	move.w  (%a2)+,(%a1)+
.lm_len_FB:
	move.w  (%a2)+,(%a1)+
.lm_len_FA:
	move.w  (%a2)+,(%a1)+
.lm_len_F9:
	move.w  (%a2)+,(%a1)+
.lm_len_F8:
	move.w  (%a2)+,(%a1)+
.lm_len_F7:
	move.w  (%a2)+,(%a1)+
.lm_len_F6:
	move.w  (%a2)+,(%a1)+
.lm_len_F5:
	move.w  (%a2)+,(%a1)+
.lm_len_F4:
	move.w  (%a2)+,(%a1)+
.lm_len_F3:
	move.w  (%a2)+,(%a1)+
.lm_len_F2:
	move.w  (%a2)+,(%a1)+
.lm_len_F1:
	move.w  (%a2)+,(%a1)+
.lm_len_F0:
	move.w  (%a2)+,(%a1)+
.lm_len_EF:
	move.w  (%a2)+,(%a1)+
.lm_len_EE:
	move.w  (%a2)+,(%a1)+
.lm_len_ED:
	move.w  (%a2)+,(%a1)+
.lm_len_EC:
	move.w  (%a2)+,(%a1)+
.lm_len_EB:
	move.w  (%a2)+,(%a1)+
.lm_len_EA:
	move.w  (%a2)+,(%a1)+
.lm_len_E9:
	move.w  (%a2)+,(%a1)+
.lm_len_E8:
	move.w  (%a2)+,(%a1)+
.lm_len_E7:
	move.w  (%a2)+,(%a1)+
.lm_len_E6:
	move.w  (%a2)+,(%a1)+
.lm_len_E5:
	move.w  (%a2)+,(%a1)+
.lm_len_E4:
	move.w  (%a2)+,(%a1)+
.lm_len_E3:
	move.w  (%a2)+,(%a1)+
.lm_len_E2:
	move.w  (%a2)+,(%a1)+
.lm_len_E1:
	move.w  (%a2)+,(%a1)+
.lm_len_E0:
	move.w  (%a2)+,(%a1)+
.lm_len_DF:
	move.w  (%a2)+,(%a1)+
.lm_len_DE:
	move.w  (%a2)+,(%a1)+
.lm_len_DD:
	move.w  (%a2)+,(%a1)+
.lm_len_DC:
	move.w  (%a2)+,(%a1)+
.lm_len_DB:
	move.w  (%a2)+,(%a1)+
.lm_len_DA:
	move.w  (%a2)+,(%a1)+
.lm_len_D9:
	move.w  (%a2)+,(%a1)+
.lm_len_D8:
	move.w  (%a2)+,(%a1)+
.lm_len_D7:
	move.w  (%a2)+,(%a1)+
.lm_len_D6:
	move.w  (%a2)+,(%a1)+
.lm_len_D5:
	move.w  (%a2)+,(%a1)+
.lm_len_D4:
	move.w  (%a2)+,(%a1)+
.lm_len_D3:
	move.w  (%a2)+,(%a1)+
.lm_len_D2:
	move.w  (%a2)+,(%a1)+
.lm_len_D1:
	move.w  (%a2)+,(%a1)+
.lm_len_D0:
	move.w  (%a2)+,(%a1)+
.lm_len_CF:
	move.w  (%a2)+,(%a1)+
.lm_len_CE:
	move.w  (%a2)+,(%a1)+
.lm_len_CD:
	move.w  (%a2)+,(%a1)+
.lm_len_CC:
	move.w  (%a2)+,(%a1)+
.lm_len_CB:
	move.w  (%a2)+,(%a1)+
.lm_len_CA:
	move.w  (%a2)+,(%a1)+
.lm_len_C9:
	move.w  (%a2)+,(%a1)+
.lm_len_C8:
	move.w  (%a2)+,(%a1)+
.lm_len_C7:
	move.w  (%a2)+,(%a1)+
.lm_len_C6:
	move.w  (%a2)+,(%a1)+
.lm_len_C5:
	move.w  (%a2)+,(%a1)+
.lm_len_C4:
	move.w  (%a2)+,(%a1)+
.lm_len_C3:
	move.w  (%a2)+,(%a1)+
.lm_len_C2:
	move.w  (%a2)+,(%a1)+
.lm_len_C1:
	move.w  (%a2)+,(%a1)+
.lm_len_C0:
	move.w  (%a2)+,(%a1)+
.lm_len_BF:
	move.w  (%a2)+,(%a1)+
.lm_len_BE:
	move.w  (%a2)+,(%a1)+
.lm_len_BD:
	move.w  (%a2)+,(%a1)+
.lm_len_BC:
	move.w  (%a2)+,(%a1)+
.lm_len_BB:
	move.w  (%a2)+,(%a1)+
.lm_len_BA:
	move.w  (%a2)+,(%a1)+
.lm_len_B9:
	move.w  (%a2)+,(%a1)+
.lm_len_B8:
	move.w  (%a2)+,(%a1)+
.lm_len_B7:
	move.w  (%a2)+,(%a1)+
.lm_len_B6:
	move.w  (%a2)+,(%a1)+
.lm_len_B5:
	move.w  (%a2)+,(%a1)+
.lm_len_B4:
	move.w  (%a2)+,(%a1)+
.lm_len_B3:
	move.w  (%a2)+,(%a1)+
.lm_len_B2:
	move.w  (%a2)+,(%a1)+
.lm_len_B1:
	move.w  (%a2)+,(%a1)+
.lm_len_B0:
	move.w  (%a2)+,(%a1)+
.lm_len_AF:
	move.w  (%a2)+,(%a1)+
.lm_len_AE:
	move.w  (%a2)+,(%a1)+
.lm_len_AD:
	move.w  (%a2)+,(%a1)+
.lm_len_AC:
	move.w  (%a2)+,(%a1)+
.lm_len_AB:
	move.w  (%a2)+,(%a1)+
.lm_len_AA:
	move.w  (%a2)+,(%a1)+
.lm_len_A9:
	move.w  (%a2)+,(%a1)+
.lm_len_A8:
	move.w  (%a2)+,(%a1)+
.lm_len_A7:
	move.w  (%a2)+,(%a1)+
.lm_len_A6:
	move.w  (%a2)+,(%a1)+
.lm_len_A5:
	move.w  (%a2)+,(%a1)+
.lm_len_A4:
	move.w  (%a2)+,(%a1)+
.lm_len_A3:
	move.w  (%a2)+,(%a1)+
.lm_len_A2:
	move.w  (%a2)+,(%a1)+
.lm_len_A1:
	move.w  (%a2)+,(%a1)+
.lm_len_A0:
	move.w  (%a2)+,(%a1)+
.lm_len_9F:
	move.w  (%a2)+,(%a1)+
.lm_len_9E:
	move.w  (%a2)+,(%a1)+
.lm_len_9D:
	move.w  (%a2)+,(%a1)+
.lm_len_9C:
	move.w  (%a2)+,(%a1)+
.lm_len_9B:
	move.w  (%a2)+,(%a1)+
.lm_len_9A:
	move.w  (%a2)+,(%a1)+
.lm_len_99:
	move.w  (%a2)+,(%a1)+
.lm_len_98:
	move.w  (%a2)+,(%a1)+
.lm_len_97:
	move.w  (%a2)+,(%a1)+
.lm_len_96:
	move.w  (%a2)+,(%a1)+
.lm_len_95:
	move.w  (%a2)+,(%a1)+
.lm_len_94:
	move.w  (%a2)+,(%a1)+
.lm_len_93:
	move.w  (%a2)+,(%a1)+
.lm_len_92:
	move.w  (%a2)+,(%a1)+
.lm_len_91:
	move.w  (%a2)+,(%a1)+
.lm_len_90:
	move.w  (%a2)+,(%a1)+
.lm_len_8F:
	move.w  (%a2)+,(%a1)+
.lm_len_8E:
	move.w  (%a2)+,(%a1)+
.lm_len_8D:
	move.w  (%a2)+,(%a1)+
.lm_len_8C:
	move.w  (%a2)+,(%a1)+
.lm_len_8B:
	move.w  (%a2)+,(%a1)+
.lm_len_8A:
	move.w  (%a2)+,(%a1)+
.lm_len_89:
	move.w  (%a2)+,(%a1)+
.lm_len_88:
	move.w  (%a2)+,(%a1)+
.lm_len_87:
	move.w  (%a2)+,(%a1)+
.lm_len_86:
	move.w  (%a2)+,(%a1)+
.lm_len_85:
	move.w  (%a2)+,(%a1)+
.lm_len_84:
	move.w  (%a2)+,(%a1)+
.lm_len_83:
	move.w  (%a2)+,(%a1)+
.lm_len_82:
	move.w  (%a2)+,(%a1)+
.lm_len_81:
	move.w  (%a2)+,(%a1)+
.lm_len_80:
	move.w  (%a2)+,(%a1)+
.lm_len_7F:
	move.w  (%a2)+,(%a1)+
.lm_len_7E:
	move.w  (%a2)+,(%a1)+
.lm_len_7D:
	move.w  (%a2)+,(%a1)+
.lm_len_7C:
	move.w  (%a2)+,(%a1)+
.lm_len_7B:
	move.w  (%a2)+,(%a1)+
.lm_len_7A:
	move.w  (%a2)+,(%a1)+
.lm_len_79:
	move.w  (%a2)+,(%a1)+
.lm_len_78:
	move.w  (%a2)+,(%a1)+
.lm_len_77:
	move.w  (%a2)+,(%a1)+
.lm_len_76:
	move.w  (%a2)+,(%a1)+
.lm_len_75:
	move.w  (%a2)+,(%a1)+
.lm_len_74:
	move.w  (%a2)+,(%a1)+
.lm_len_73:
	move.w  (%a2)+,(%a1)+
.lm_len_72:
	move.w  (%a2)+,(%a1)+
.lm_len_71:
	move.w  (%a2)+,(%a1)+
.lm_len_70:
	move.w  (%a2)+,(%a1)+
.lm_len_6F:
	move.w  (%a2)+,(%a1)+
.lm_len_6E:
	move.w  (%a2)+,(%a1)+
.lm_len_6D:
	move.w  (%a2)+,(%a1)+
.lm_len_6C:
	move.w  (%a2)+,(%a1)+
.lm_len_6B:
	move.w  (%a2)+,(%a1)+
.lm_len_6A:
	move.w  (%a2)+,(%a1)+
.lm_len_69:
	move.w  (%a2)+,(%a1)+
.lm_len_68:
	move.w  (%a2)+,(%a1)+
.lm_len_67:
	move.w  (%a2)+,(%a1)+
.lm_len_66:
	move.w  (%a2)+,(%a1)+
.lm_len_65:
	move.w  (%a2)+,(%a1)+
.lm_len_64:
	move.w  (%a2)+,(%a1)+
.lm_len_63:
	move.w  (%a2)+,(%a1)+
.lm_len_62:
	move.w  (%a2)+,(%a1)+
.lm_len_61:
	move.w  (%a2)+,(%a1)+
.lm_len_60:
	move.w  (%a2)+,(%a1)+
.lm_len_5F:
	move.w  (%a2)+,(%a1)+
.lm_len_5E:
	move.w  (%a2)+,(%a1)+
.lm_len_5D:
	move.w  (%a2)+,(%a1)+
.lm_len_5C:
	move.w  (%a2)+,(%a1)+
.lm_len_5B:
	move.w  (%a2)+,(%a1)+
.lm_len_5A:
	move.w  (%a2)+,(%a1)+
.lm_len_59:
	move.w  (%a2)+,(%a1)+
.lm_len_58:
	move.w  (%a2)+,(%a1)+
.lm_len_57:
	move.w  (%a2)+,(%a1)+
.lm_len_56:
	move.w  (%a2)+,(%a1)+
.lm_len_55:
	move.w  (%a2)+,(%a1)+
.lm_len_54:
	move.w  (%a2)+,(%a1)+
.lm_len_53:
	move.w  (%a2)+,(%a1)+
.lm_len_52:
	move.w  (%a2)+,(%a1)+
.lm_len_51:
	move.w  (%a2)+,(%a1)+
.lm_len_50:
	move.w  (%a2)+,(%a1)+
.lm_len_4F:
	move.w  (%a2)+,(%a1)+
.lm_len_4E:
	move.w  (%a2)+,(%a1)+
.lm_len_4D:
	move.w  (%a2)+,(%a1)+
.lm_len_4C:
	move.w  (%a2)+,(%a1)+
.lm_len_4B:
	move.w  (%a2)+,(%a1)+
.lm_len_4A:
	move.w  (%a2)+,(%a1)+
.lm_len_49:
	move.w  (%a2)+,(%a1)+
.lm_len_48:
	move.w  (%a2)+,(%a1)+
.lm_len_47:
	move.w  (%a2)+,(%a1)+
.lm_len_46:
	move.w  (%a2)+,(%a1)+
.lm_len_45:
	move.w  (%a2)+,(%a1)+
.lm_len_44:
	move.w  (%a2)+,(%a1)+
.lm_len_43:
	move.w  (%a2)+,(%a1)+
.lm_len_42:
	move.w  (%a2)+,(%a1)+
.lm_len_41:
	move.w  (%a2)+,(%a1)+
.lm_len_40:
	move.w  (%a2)+,(%a1)+
.lm_len_3F:
	move.w  (%a2)+,(%a1)+
.lm_len_3E:
	move.w  (%a2)+,(%a1)+
.lm_len_3D:
	move.w  (%a2)+,(%a1)+
.lm_len_3C:
	move.w  (%a2)+,(%a1)+
.lm_len_3B:
	move.w  (%a2)+,(%a1)+
.lm_len_3A:
	move.w  (%a2)+,(%a1)+
.lm_len_39:
	move.w  (%a2)+,(%a1)+
.lm_len_38:
	move.w  (%a2)+,(%a1)+
.lm_len_37:
	move.w  (%a2)+,(%a1)+
.lm_len_36:
	move.w  (%a2)+,(%a1)+
.lm_len_35:
	move.w  (%a2)+,(%a1)+
.lm_len_34:
	move.w  (%a2)+,(%a1)+
.lm_len_33:
	move.w  (%a2)+,(%a1)+
.lm_len_32:
	move.w  (%a2)+,(%a1)+
.lm_len_31:
	move.w  (%a2)+,(%a1)+
.lm_len_30:
	move.w  (%a2)+,(%a1)+
.lm_len_2F:
	move.w  (%a2)+,(%a1)+
.lm_len_2E:
	move.w  (%a2)+,(%a1)+
.lm_len_2D:
	move.w  (%a2)+,(%a1)+
.lm_len_2C:
	move.w  (%a2)+,(%a1)+
.lm_len_2B:
	move.w  (%a2)+,(%a1)+
.lm_len_2A:
	move.w  (%a2)+,(%a1)+
.lm_len_29:
	move.w  (%a2)+,(%a1)+
.lm_len_28:
	move.w  (%a2)+,(%a1)+
.lm_len_27:
	move.w  (%a2)+,(%a1)+
.lm_len_26:
	move.w  (%a2)+,(%a1)+
.lm_len_25:
	move.w  (%a2)+,(%a1)+
.lm_len_24:
	move.w  (%a2)+,(%a1)+
.lm_len_23:
	move.w  (%a2)+,(%a1)+
.lm_len_22:
	move.w  (%a2)+,(%a1)+
.lm_len_21:
	move.w  (%a2)+,(%a1)+
.lm_len_20:
	move.w  (%a2)+,(%a1)+
.lm_len_1F:
	move.w  (%a2)+,(%a1)+
.lm_len_1E:
	move.w  (%a2)+,(%a1)+
.lm_len_1D:
	move.w  (%a2)+,(%a1)+
.lm_len_1C:
	move.w  (%a2)+,(%a1)+
.lm_len_1B:
	move.w  (%a2)+,(%a1)+
.lm_len_1A:
	move.w  (%a2)+,(%a1)+
.lm_len_19:
	move.w  (%a2)+,(%a1)+
.lm_len_18:
	move.w  (%a2)+,(%a1)+
.lm_len_17:
	move.w  (%a2)+,(%a1)+
.lm_len_16:
	move.w  (%a2)+,(%a1)+
.lm_len_15:
	move.w  (%a2)+,(%a1)+
.lm_len_14:
	move.w  (%a2)+,(%a1)+
.lm_len_13:
	move.w  (%a2)+,(%a1)+
.lm_len_12:
	move.w  (%a2)+,(%a1)+
.lm_len_11:
	move.w  (%a2)+,(%a1)+
.lm_len_10:
	move.w  (%a2)+,(%a1)+
.lm_len_0F:
	move.w  (%a2)+,(%a1)+
.lm_len_0E:
	move.w  (%a2)+,(%a1)+
.lm_len_0D:
	move.w  (%a2)+,(%a1)+
.lm_len_0C:
	move.w  (%a2)+,(%a1)+
.lm_len_0B:
	move.w  (%a2)+,(%a1)+
.lm_len_0A:
	move.w  (%a2)+,(%a1)+
.lm_len_09:
	move.w  (%a2)+,(%a1)+
.lm_len_08:
	move.w  (%a2)+,(%a1)+
.lm_len_07:
	move.w  (%a2)+,(%a1)+
.lm_len_06:
	move.w  (%a2)+,(%a1)+
.lm_len_05:
	move.w  (%a2)+,(%a1)+
.lm_len_04:
	move.w  (%a2)+,(%a1)+
.lm_len_03:
	move.w  (%a2)+,(%a1)+
.lm_len_02:
	move.w  (%a2)+,(%a1)+
.lm_len_01:
	move.w  (%a2)+,(%a1)+
.lm_len_00:
	move.w  (%a2)+,(%a1)+
	move.w  (%a2)+,(%a1)+

    moveq   #0,%d0
    moveq   #0,%d1
    jmp     (%a3)

.lmr_len_FF:
	move.l  (%a2)+,(%a1)+
.lmr_len_FD:
	move.l  (%a2)+,(%a1)+
.lmr_len_FB:
	move.l  (%a2)+,(%a1)+
.lmr_len_F9:
	move.l  (%a2)+,(%a1)+
.lmr_len_F7:
	move.l  (%a2)+,(%a1)+
.lmr_len_F5:
	move.l  (%a2)+,(%a1)+
.lmr_len_F3:
	move.l  (%a2)+,(%a1)+
.lmr_len_F1:
	move.l  (%a2)+,(%a1)+
.lmr_len_EF:
	move.l  (%a2)+,(%a1)+
.lmr_len_ED:
	move.l  (%a2)+,(%a1)+
.lmr_len_EB:
	move.l  (%a2)+,(%a1)+
.lmr_len_E9:
	move.l  (%a2)+,(%a1)+
.lmr_len_E7:
	move.l  (%a2)+,(%a1)+
.lmr_len_E5:
	move.l  (%a2)+,(%a1)+
.lmr_len_E3:
	move.l  (%a2)+,(%a1)+
.lmr_len_E1:
	move.l  (%a2)+,(%a1)+
.lmr_len_DF:
	move.l  (%a2)+,(%a1)+
.lmr_len_DD:
	move.l  (%a2)+,(%a1)+
.lmr_len_DB:
	move.l  (%a2)+,(%a1)+
.lmr_len_D9:
	move.l  (%a2)+,(%a1)+
.lmr_len_D7:
	move.l  (%a2)+,(%a1)+
.lmr_len_D5:
	move.l  (%a2)+,(%a1)+
.lmr_len_D3:
	move.l  (%a2)+,(%a1)+
.lmr_len_D1:
	move.l  (%a2)+,(%a1)+
.lmr_len_CF:
	move.l  (%a2)+,(%a1)+
.lmr_len_CD:
	move.l  (%a2)+,(%a1)+
.lmr_len_CB:
	move.l  (%a2)+,(%a1)+
.lmr_len_C9:
	move.l  (%a2)+,(%a1)+
.lmr_len_C7:
	move.l  (%a2)+,(%a1)+
.lmr_len_C5:
	move.l  (%a2)+,(%a1)+
.lmr_len_C3:
	move.l  (%a2)+,(%a1)+
.lmr_len_C1:
	move.l  (%a2)+,(%a1)+
.lmr_len_BF:
	move.l  (%a2)+,(%a1)+
.lmr_len_BD:
	move.l  (%a2)+,(%a1)+
.lmr_len_BB:
	move.l  (%a2)+,(%a1)+
.lmr_len_B9:
	move.l  (%a2)+,(%a1)+
.lmr_len_B7:
	move.l  (%a2)+,(%a1)+
.lmr_len_B5:
	move.l  (%a2)+,(%a1)+
.lmr_len_B3:
	move.l  (%a2)+,(%a1)+
.lmr_len_B1:
	move.l  (%a2)+,(%a1)+
.lmr_len_AF:
	move.l  (%a2)+,(%a1)+
.lmr_len_AD:
	move.l  (%a2)+,(%a1)+
.lmr_len_AB:
	move.l  (%a2)+,(%a1)+
.lmr_len_A9:
	move.l  (%a2)+,(%a1)+
.lmr_len_A7:
	move.l  (%a2)+,(%a1)+
.lmr_len_A5:
	move.l  (%a2)+,(%a1)+
.lmr_len_A3:
	move.l  (%a2)+,(%a1)+
.lmr_len_A1:
	move.l  (%a2)+,(%a1)+
.lmr_len_9F:
	move.l  (%a2)+,(%a1)+
.lmr_len_9D:
	move.l  (%a2)+,(%a1)+
.lmr_len_9B:
	move.l  (%a2)+,(%a1)+
.lmr_len_99:
	move.l  (%a2)+,(%a1)+
.lmr_len_97:
	move.l  (%a2)+,(%a1)+
.lmr_len_95:
	move.l  (%a2)+,(%a1)+
.lmr_len_93:
	move.l  (%a2)+,(%a1)+
.lmr_len_91:
	move.l  (%a2)+,(%a1)+
.lmr_len_8F:
	move.l  (%a2)+,(%a1)+
.lmr_len_8D:
	move.l  (%a2)+,(%a1)+
.lmr_len_8B:
	move.l  (%a2)+,(%a1)+
.lmr_len_89:
	move.l  (%a2)+,(%a1)+
.lmr_len_87:
	move.l  (%a2)+,(%a1)+
.lmr_len_85:
	move.l  (%a2)+,(%a1)+
.lmr_len_83:
	move.l  (%a2)+,(%a1)+
.lmr_len_81:
	move.l  (%a2)+,(%a1)+
.lmr_len_7F:
	move.l  (%a2)+,(%a1)+
.lmr_len_7D:
	move.l  (%a2)+,(%a1)+
.lmr_len_7B:
	move.l  (%a2)+,(%a1)+
.lmr_len_79:
	move.l  (%a2)+,(%a1)+
.lmr_len_77:
	move.l  (%a2)+,(%a1)+
.lmr_len_75:
	move.l  (%a2)+,(%a1)+
.lmr_len_73:
	move.l  (%a2)+,(%a1)+
.lmr_len_71:
	move.l  (%a2)+,(%a1)+
.lmr_len_6F:
	move.l  (%a2)+,(%a1)+
.lmr_len_6D:
	move.l  (%a2)+,(%a1)+
.lmr_len_6B:
	move.l  (%a2)+,(%a1)+
.lmr_len_69:
	move.l  (%a2)+,(%a1)+
.lmr_len_67:
	move.l  (%a2)+,(%a1)+
.lmr_len_65:
	move.l  (%a2)+,(%a1)+
.lmr_len_63:
	move.l  (%a2)+,(%a1)+
.lmr_len_61:
	move.l  (%a2)+,(%a1)+
.lmr_len_5F:
	move.l  (%a2)+,(%a1)+
.lmr_len_5D:
	move.l  (%a2)+,(%a1)+
.lmr_len_5B:
	move.l  (%a2)+,(%a1)+
.lmr_len_59:
	move.l  (%a2)+,(%a1)+
.lmr_len_57:
	move.l  (%a2)+,(%a1)+
.lmr_len_55:
	move.l  (%a2)+,(%a1)+
.lmr_len_53:
	move.l  (%a2)+,(%a1)+
.lmr_len_51:
	move.l  (%a2)+,(%a1)+
.lmr_len_4F:
	move.l  (%a2)+,(%a1)+
.lmr_len_4D:
	move.l  (%a2)+,(%a1)+
.lmr_len_4B:
	move.l  (%a2)+,(%a1)+
.lmr_len_49:
	move.l  (%a2)+,(%a1)+
.lmr_len_47:
	move.l  (%a2)+,(%a1)+
.lmr_len_45:
	move.l  (%a2)+,(%a1)+
.lmr_len_43:
	move.l  (%a2)+,(%a1)+
.lmr_len_41:
	move.l  (%a2)+,(%a1)+
.lmr_len_3F:
	move.l  (%a2)+,(%a1)+
.lmr_len_3D:
	move.l  (%a2)+,(%a1)+
.lmr_len_3B:
	move.l  (%a2)+,(%a1)+
.lmr_len_39:
	move.l  (%a2)+,(%a1)+
.lmr_len_37:
	move.l  (%a2)+,(%a1)+
.lmr_len_35:
	move.l  (%a2)+,(%a1)+
.lmr_len_33:
	move.l  (%a2)+,(%a1)+
.lmr_len_31:
	move.l  (%a2)+,(%a1)+
.lmr_len_2F:
	move.l  (%a2)+,(%a1)+
.lmr_len_2D:
	move.l  (%a2)+,(%a1)+
.lmr_len_2B:
	move.l  (%a2)+,(%a1)+
.lmr_len_29:
	move.l  (%a2)+,(%a1)+
.lmr_len_27:
	move.l  (%a2)+,(%a1)+
.lmr_len_25:
	move.l  (%a2)+,(%a1)+
.lmr_len_23:
	move.l  (%a2)+,(%a1)+
.lmr_len_21:
	move.l  (%a2)+,(%a1)+
.lmr_len_1F:
	move.l  (%a2)+,(%a1)+
.lmr_len_1D:
	move.l  (%a2)+,(%a1)+
.lmr_len_1B:
	move.l  (%a2)+,(%a1)+
.lmr_len_19:
	move.l  (%a2)+,(%a1)+
.lmr_len_17:
	move.l  (%a2)+,(%a1)+
.lmr_len_15:
	move.l  (%a2)+,(%a1)+
.lmr_len_13:
	move.l  (%a2)+,(%a1)+
.lmr_len_11:
	move.l  (%a2)+,(%a1)+
.lmr_len_0F:
	move.l  (%a2)+,(%a1)+
.lmr_len_0D:
	move.l  (%a2)+,(%a1)+
.lmr_len_0B:
	move.l  (%a2)+,(%a1)+
.lmr_len_09:
	move.l  (%a2)+,(%a1)+
.lmr_len_07:
	move.l  (%a2)+,(%a1)+
.lmr_len_05:
	move.l  (%a2)+,(%a1)+
.lmr_len_03:
	move.l  (%a2)+,(%a1)+
.lmr_len_01:
	move.l  (%a2)+,(%a1)+
	move.w  (%a2)+,(%a1)+

    moveq   #0,%d0
    moveq   #0,%d1
    jmp     (%a3)

.lmr_len_FE:
	move.l  (%a2)+,(%a1)+
.lmr_len_FC:
	move.l  (%a2)+,(%a1)+
.lmr_len_FA:
	move.l  (%a2)+,(%a1)+
.lmr_len_F8:
	move.l  (%a2)+,(%a1)+
.lmr_len_F6:
	move.l  (%a2)+,(%a1)+
.lmr_len_F4:
	move.l  (%a2)+,(%a1)+
.lmr_len_F2:
	move.l  (%a2)+,(%a1)+
.lmr_len_F0:
	move.l  (%a2)+,(%a1)+
.lmr_len_EE:
	move.l  (%a2)+,(%a1)+
.lmr_len_EC:
	move.l  (%a2)+,(%a1)+
.lmr_len_EA:
	move.l  (%a2)+,(%a1)+
.lmr_len_E8:
	move.l  (%a2)+,(%a1)+
.lmr_len_E6:
	move.l  (%a2)+,(%a1)+
.lmr_len_E4:
	move.l  (%a2)+,(%a1)+
.lmr_len_E2:
	move.l  (%a2)+,(%a1)+
.lmr_len_E0:
	move.l  (%a2)+,(%a1)+
.lmr_len_DE:
	move.l  (%a2)+,(%a1)+
.lmr_len_DC:
	move.l  (%a2)+,(%a1)+
.lmr_len_DA:
	move.l  (%a2)+,(%a1)+
.lmr_len_D8:
	move.l  (%a2)+,(%a1)+
.lmr_len_D6:
	move.l  (%a2)+,(%a1)+
.lmr_len_D4:
	move.l  (%a2)+,(%a1)+
.lmr_len_D2:
	move.l  (%a2)+,(%a1)+
.lmr_len_D0:
	move.l  (%a2)+,(%a1)+
.lmr_len_CE:
	move.l  (%a2)+,(%a1)+
.lmr_len_CC:
	move.l  (%a2)+,(%a1)+
.lmr_len_CA:
	move.l  (%a2)+,(%a1)+
.lmr_len_C8:
	move.l  (%a2)+,(%a1)+
.lmr_len_C6:
	move.l  (%a2)+,(%a1)+
.lmr_len_C4:
	move.l  (%a2)+,(%a1)+
.lmr_len_C2:
	move.l  (%a2)+,(%a1)+
.lmr_len_C0:
	move.l  (%a2)+,(%a1)+
.lmr_len_BE:
	move.l  (%a2)+,(%a1)+
.lmr_len_BC:
	move.l  (%a2)+,(%a1)+
.lmr_len_BA:
	move.l  (%a2)+,(%a1)+
.lmr_len_B8:
	move.l  (%a2)+,(%a1)+
.lmr_len_B6:
	move.l  (%a2)+,(%a1)+
.lmr_len_B4:
	move.l  (%a2)+,(%a1)+
.lmr_len_B2:
	move.l  (%a2)+,(%a1)+
.lmr_len_B0:
	move.l  (%a2)+,(%a1)+
.lmr_len_AE:
	move.l  (%a2)+,(%a1)+
.lmr_len_AC:
	move.l  (%a2)+,(%a1)+
.lmr_len_AA:
	move.l  (%a2)+,(%a1)+
.lmr_len_A8:
	move.l  (%a2)+,(%a1)+
.lmr_len_A6:
	move.l  (%a2)+,(%a1)+
.lmr_len_A4:
	move.l  (%a2)+,(%a1)+
.lmr_len_A2:
	move.l  (%a2)+,(%a1)+
.lmr_len_A0:
	move.l  (%a2)+,(%a1)+
.lmr_len_9E:
	move.l  (%a2)+,(%a1)+
.lmr_len_9C:
	move.l  (%a2)+,(%a1)+
.lmr_len_9A:
	move.l  (%a2)+,(%a1)+
.lmr_len_98:
	move.l  (%a2)+,(%a1)+
.lmr_len_96:
	move.l  (%a2)+,(%a1)+
.lmr_len_94:
	move.l  (%a2)+,(%a1)+
.lmr_len_92:
	move.l  (%a2)+,(%a1)+
.lmr_len_90:
	move.l  (%a2)+,(%a1)+
.lmr_len_8E:
	move.l  (%a2)+,(%a1)+
.lmr_len_8C:
	move.l  (%a2)+,(%a1)+
.lmr_len_8A:
	move.l  (%a2)+,(%a1)+
.lmr_len_88:
	move.l  (%a2)+,(%a1)+
.lmr_len_86:
	move.l  (%a2)+,(%a1)+
.lmr_len_84:
	move.l  (%a2)+,(%a1)+
.lmr_len_82:
	move.l  (%a2)+,(%a1)+
.lmr_len_80:
	move.l  (%a2)+,(%a1)+
.lmr_len_7E:
	move.l  (%a2)+,(%a1)+
.lmr_len_7C:
	move.l  (%a2)+,(%a1)+
.lmr_len_7A:
	move.l  (%a2)+,(%a1)+
.lmr_len_78:
	move.l  (%a2)+,(%a1)+
.lmr_len_76:
	move.l  (%a2)+,(%a1)+
.lmr_len_74:
	move.l  (%a2)+,(%a1)+
.lmr_len_72:
	move.l  (%a2)+,(%a1)+
.lmr_len_70:
	move.l  (%a2)+,(%a1)+
.lmr_len_6E:
	move.l  (%a2)+,(%a1)+
.lmr_len_6C:
	move.l  (%a2)+,(%a1)+
.lmr_len_6A:
	move.l  (%a2)+,(%a1)+
.lmr_len_68:
	move.l  (%a2)+,(%a1)+
.lmr_len_66:
	move.l  (%a2)+,(%a1)+
.lmr_len_64:
	move.l  (%a2)+,(%a1)+
.lmr_len_62:
	move.l  (%a2)+,(%a1)+
.lmr_len_60:
	move.l  (%a2)+,(%a1)+
.lmr_len_5E:
	move.l  (%a2)+,(%a1)+
.lmr_len_5C:
	move.l  (%a2)+,(%a1)+
.lmr_len_5A:
	move.l  (%a2)+,(%a1)+
.lmr_len_58:
	move.l  (%a2)+,(%a1)+
.lmr_len_56:
	move.l  (%a2)+,(%a1)+
.lmr_len_54:
	move.l  (%a2)+,(%a1)+
.lmr_len_52:
	move.l  (%a2)+,(%a1)+
.lmr_len_50:
	move.l  (%a2)+,(%a1)+
.lmr_len_4E:
	move.l  (%a2)+,(%a1)+
.lmr_len_4C:
	move.l  (%a2)+,(%a1)+
.lmr_len_4A:
	move.l  (%a2)+,(%a1)+
.lmr_len_48:
	move.l  (%a2)+,(%a1)+
.lmr_len_46:
	move.l  (%a2)+,(%a1)+
.lmr_len_44:
	move.l  (%a2)+,(%a1)+
.lmr_len_42:
	move.l  (%a2)+,(%a1)+
.lmr_len_40:
	move.l  (%a2)+,(%a1)+
.lmr_len_3E:
	move.l  (%a2)+,(%a1)+
.lmr_len_3C:
	move.l  (%a2)+,(%a1)+
.lmr_len_3A:
	move.l  (%a2)+,(%a1)+
.lmr_len_38:
	move.l  (%a2)+,(%a1)+
.lmr_len_36:
	move.l  (%a2)+,(%a1)+
.lmr_len_34:
	move.l  (%a2)+,(%a1)+
.lmr_len_32:
	move.l  (%a2)+,(%a1)+
.lmr_len_30:
	move.l  (%a2)+,(%a1)+
.lmr_len_2E:
	move.l  (%a2)+,(%a1)+
.lmr_len_2C:
	move.l  (%a2)+,(%a1)+
.lmr_len_2A:
	move.l  (%a2)+,(%a1)+
.lmr_len_28:
	move.l  (%a2)+,(%a1)+
.lmr_len_26:
	move.l  (%a2)+,(%a1)+
.lmr_len_24:
	move.l  (%a2)+,(%a1)+
.lmr_len_22:
	move.l  (%a2)+,(%a1)+
.lmr_len_20:
	move.l  (%a2)+,(%a1)+
.lmr_len_1E:
	move.l  (%a2)+,(%a1)+
.lmr_len_1C:
	move.l  (%a2)+,(%a1)+
.lmr_len_1A:
	move.l  (%a2)+,(%a1)+
.lmr_len_18:
	move.l  (%a2)+,(%a1)+
.lmr_len_16:
	move.l  (%a2)+,(%a1)+
.lmr_len_14:
	move.l  (%a2)+,(%a1)+
.lmr_len_12:
	move.l  (%a2)+,(%a1)+
.lmr_len_10:
	move.l  (%a2)+,(%a1)+
.lmr_len_0E:
	move.l  (%a2)+,(%a1)+
.lmr_len_0C:
	move.l  (%a2)+,(%a1)+
.lmr_len_0A:
	move.l  (%a2)+,(%a1)+
.lmr_len_08:
	move.l  (%a2)+,(%a1)+
.lmr_len_06:
	move.l  (%a2)+,(%a1)+
.lmr_len_04:
	move.l  (%a2)+,(%a1)+
.lmr_len_02:
	move.l  (%a2)+,(%a1)+
.lmr_len_00:
	move.l  (%a2)+,(%a1)+

    moveq   #0,%d0
    moveq   #0,%d1
    jmp     (%a3)


.litE_mat0:
    move.l  (%a0)+,(%a1)+
.litC_mat0:
    move.l  (%a0)+,(%a1)+
.litA_mat0:
    move.l  (%a0)+,(%a1)+
.lit8_mat0:
    move.l  (%a0)+,(%a1)+
.lit6_mat0:
    move.l  (%a0)+,(%a1)+
.lit4_mat0:
    move.l  (%a0)+,(%a1)+
.lit2_mat0:
    move.l  (%a0)+,(%a1)+

    tst.b   %d1                     | match offset null ?
    jeq     .next                   | not a long match

.long_match_1:
    move.w  (%a0)+,%d0              | get long offset (already negated)

    add.w   %d1,%d1                 |
    add.w   %d1,%d1                 | len = len * 4 (for jump table)

    add.w   %d0,%d0                 | bit 15 contains ROM source info
    jcs     .lm_rom

    lea     -2(%a1,%d0.w),%a2       | a2 = dst - (match offset + 2)

.lm1_jump_base:
    move.l  (.lm_jump_table-.lm1_jump_base)-2(%pc,%d1.w),%a4
    jmp     (%a4)

.litF_mat0:
    move.l  (%a0)+,(%a1)+
.litD_mat0:
    move.l  (%a0)+,(%a1)+
.litB_mat0:
    move.l  (%a0)+,(%a1)+
.lit9_mat0:
    move.l  (%a0)+,(%a1)+
.lit7_mat0:
    move.l  (%a0)+,(%a1)+
.lit5_mat0:
    move.l  (%a0)+,(%a1)+
.lit3_mat0:
    move.l  (%a0)+,(%a1)+
.lit1_mat0:
    move.w  (%a0)+,(%a1)+

    tst.b   %d1                     | match offset null ?
    jeq     .next                   | not a long match

.long_match_2:
    move.w  (%a0)+,%d0              | get long offset (already negated)

    add.w   %d1,%d1                 |
    add.w   %d1,%d1                 | len = len * 4 (for jump table)

    add.w   %d0,%d0                 | bit 15 contains ROM source info
    jcs     .lm_rom

    lea     -2(%a1,%d0.w),%a2       | a2 = dst - (match offset + 2)

.lm2_jump_base:
    move.l  (.lm_jump_table-.lm2_jump_base)-2(%pc,%d1.w),%a4
    jmp     (%a4)


.lit0_mat0:                         | special case of lit=0 and mat=0
    tst.b   %d1                     | match offset null ?
    jeq    .done                    | not a long match --> done

.long_match_3:
    move.w  (%a0)+,%d0              | get long offset (already negated)

    add.w   %d1,%d1                 |
    add.w   %d1,%d1                 | len = len * 4 (for jump table)

    add.w   %d0,%d0                 | bit 15 contains ROM source info
    jcs     .lm_rom

    lea     -2(%a1,%d0.w),%a2       | a2 = dst - (match offset + 2)

.lm3_jump_base:
    move.l  (.lm_jump_table-.lm3_jump_base)-2(%pc,%d1.w),%a4
    jmp     (%a4)

    .align 2
.lm_jump_table:

    .long .lm_len_00
    .long .lm_len_01
    .long .lm_len_02
    .long .lm_len_03
    .long .lm_len_04
    .long .lm_len_05
    .long .lm_len_06
    .long .lm_len_07
    .long .lm_len_08
    .long .lm_len_09
    .long .lm_len_0A
    .long .lm_len_0B
    .long .lm_len_0C
    .long .lm_len_0D
    .long .lm_len_0E
    .long .lm_len_0F
    .long .lm_len_10
    .long .lm_len_11
    .long .lm_len_12
    .long .lm_len_13
    .long .lm_len_14
    .long .lm_len_15
    .long .lm_len_16
    .long .lm_len_17
    .long .lm_len_18
    .long .lm_len_19
    .long .lm_len_1A
    .long .lm_len_1B
    .long .lm_len_1C
    .long .lm_len_1D
    .long .lm_len_1E
    .long .lm_len_1F
    .long .lm_len_20
    .long .lm_len_21
    .long .lm_len_22
    .long .lm_len_23
    .long .lm_len_24
    .long .lm_len_25
    .long .lm_len_26
    .long .lm_len_27
    .long .lm_len_28
    .long .lm_len_29
    .long .lm_len_2A
    .long .lm_len_2B
    .long .lm_len_2C
    .long .lm_len_2D
    .long .lm_len_2E
    .long .lm_len_2F
    .long .lm_len_30
    .long .lm_len_31
    .long .lm_len_32
    .long .lm_len_33
    .long .lm_len_34
    .long .lm_len_35
    .long .lm_len_36
    .long .lm_len_37
    .long .lm_len_38
    .long .lm_len_39
    .long .lm_len_3A
    .long .lm_len_3B
    .long .lm_len_3C
    .long .lm_len_3D
    .long .lm_len_3E
    .long .lm_len_3F
    .long .lm_len_40
    .long .lm_len_41
    .long .lm_len_42
    .long .lm_len_43
    .long .lm_len_44
    .long .lm_len_45
    .long .lm_len_46
    .long .lm_len_47
    .long .lm_len_48
    .long .lm_len_49
    .long .lm_len_4A
    .long .lm_len_4B
    .long .lm_len_4C
    .long .lm_len_4D
    .long .lm_len_4E
    .long .lm_len_4F
    .long .lm_len_50
    .long .lm_len_51
    .long .lm_len_52
    .long .lm_len_53
    .long .lm_len_54
    .long .lm_len_55
    .long .lm_len_56
    .long .lm_len_57
    .long .lm_len_58
    .long .lm_len_59
    .long .lm_len_5A
    .long .lm_len_5B
    .long .lm_len_5C
    .long .lm_len_5D
    .long .lm_len_5E
    .long .lm_len_5F
    .long .lm_len_60
    .long .lm_len_61
    .long .lm_len_62
    .long .lm_len_63
    .long .lm_len_64
    .long .lm_len_65
    .long .lm_len_66
    .long .lm_len_67
    .long .lm_len_68
    .long .lm_len_69
    .long .lm_len_6A
    .long .lm_len_6B
    .long .lm_len_6C
    .long .lm_len_6D
    .long .lm_len_6E
    .long .lm_len_6F
    .long .lm_len_70
    .long .lm_len_71
    .long .lm_len_72
    .long .lm_len_73
    .long .lm_len_74
    .long .lm_len_75
    .long .lm_len_76
    .long .lm_len_77
    .long .lm_len_78
    .long .lm_len_79
    .long .lm_len_7A
    .long .lm_len_7B
    .long .lm_len_7C
    .long .lm_len_7D
    .long .lm_len_7E
    .long .lm_len_7F
    .long .lm_len_80
    .long .lm_len_81
    .long .lm_len_82
    .long .lm_len_83
    .long .lm_len_84
    .long .lm_len_85
    .long .lm_len_86
    .long .lm_len_87
    .long .lm_len_88
    .long .lm_len_89
    .long .lm_len_8A
    .long .lm_len_8B
    .long .lm_len_8C
    .long .lm_len_8D
    .long .lm_len_8E
    .long .lm_len_8F
    .long .lm_len_90
    .long .lm_len_91
    .long .lm_len_92
    .long .lm_len_93
    .long .lm_len_94
    .long .lm_len_95
    .long .lm_len_96
    .long .lm_len_97
    .long .lm_len_98
    .long .lm_len_99
    .long .lm_len_9A
    .long .lm_len_9B
    .long .lm_len_9C
    .long .lm_len_9D
    .long .lm_len_9E
    .long .lm_len_9F
    .long .lm_len_A0
    .long .lm_len_A1
    .long .lm_len_A2
    .long .lm_len_A3
    .long .lm_len_A4
    .long .lm_len_A5
    .long .lm_len_A6
    .long .lm_len_A7
    .long .lm_len_A8
    .long .lm_len_A9
    .long .lm_len_AA
    .long .lm_len_AB
    .long .lm_len_AC
    .long .lm_len_AD
    .long .lm_len_AE
    .long .lm_len_AF
    .long .lm_len_B0
    .long .lm_len_B1
    .long .lm_len_B2
    .long .lm_len_B3
    .long .lm_len_B4
    .long .lm_len_B5
    .long .lm_len_B6
    .long .lm_len_B7
    .long .lm_len_B8
    .long .lm_len_B9
    .long .lm_len_BA
    .long .lm_len_BB
    .long .lm_len_BC
    .long .lm_len_BD
    .long .lm_len_BE
    .long .lm_len_BF
    .long .lm_len_C0
    .long .lm_len_C1
    .long .lm_len_C2
    .long .lm_len_C3
    .long .lm_len_C4
    .long .lm_len_C5
    .long .lm_len_C6
    .long .lm_len_C7
    .long .lm_len_C8
    .long .lm_len_C9
    .long .lm_len_CA
    .long .lm_len_CB
    .long .lm_len_CC
    .long .lm_len_CD
    .long .lm_len_CE
    .long .lm_len_CF
    .long .lm_len_D0
    .long .lm_len_D1
    .long .lm_len_D2
    .long .lm_len_D3
    .long .lm_len_D4
    .long .lm_len_D5
    .long .lm_len_D6
    .long .lm_len_D7
    .long .lm_len_D8
    .long .lm_len_D9
    .long .lm_len_DA
    .long .lm_len_DB
    .long .lm_len_DC
    .long .lm_len_DD
    .long .lm_len_DE
    .long .lm_len_DF
    .long .lm_len_E0
    .long .lm_len_E1
    .long .lm_len_E2
    .long .lm_len_E3
    .long .lm_len_E4
    .long .lm_len_E5
    .long .lm_len_E6
    .long .lm_len_E7
    .long .lm_len_E8
    .long .lm_len_E9
    .long .lm_len_EA
    .long .lm_len_EB
    .long .lm_len_EC
    .long .lm_len_ED
    .long .lm_len_EE
    .long .lm_len_EF
    .long .lm_len_F0
    .long .lm_len_F1
    .long .lm_len_F2
    .long .lm_len_F3
    .long .lm_len_F4
    .long .lm_len_F5
    .long .lm_len_F6
    .long .lm_len_F7
    .long .lm_len_F8
    .long .lm_len_F9
    .long .lm_len_FA
    .long .lm_len_FB
    .long .lm_len_FC
    .long .lm_len_FD
    .long .lm_len_FE
    .long .lm_len_FF

.lm_rom:
    lea     -2(%a0,%d0.w),%a2       | a2 = src - (match offset + 2)

.lmr_jump_base:
    move.l  (.lmr_jump_table-.lmr_jump_base)-2(%pc,%d1.w),%a4
    jmp     (%a4)

    .align 2
.lmr_jump_table:

    .long .lmr_len_00
    .long .lmr_len_01
    .long .lmr_len_02
    .long .lmr_len_03
    .long .lmr_len_04
    .long .lmr_len_05
    .long .lmr_len_06
    .long .lmr_len_07
    .long .lmr_len_08
    .long .lmr_len_09
    .long .lmr_len_0A
    .long .lmr_len_0B
    .long .lmr_len_0C
    .long .lmr_len_0D
    .long .lmr_len_0E
    .long .lmr_len_0F
    .long .lmr_len_10
    .long .lmr_len_11
    .long .lmr_len_12
    .long .lmr_len_13
    .long .lmr_len_14
    .long .lmr_len_15
    .long .lmr_len_16
    .long .lmr_len_17
    .long .lmr_len_18
    .long .lmr_len_19
    .long .lmr_len_1A
    .long .lmr_len_1B
    .long .lmr_len_1C
    .long .lmr_len_1D
    .long .lmr_len_1E
    .long .lmr_len_1F
    .long .lmr_len_20
    .long .lmr_len_21
    .long .lmr_len_22
    .long .lmr_len_23
    .long .lmr_len_24
    .long .lmr_len_25
    .long .lmr_len_26
    .long .lmr_len_27
    .long .lmr_len_28
    .long .lmr_len_29
    .long .lmr_len_2A
    .long .lmr_len_2B
    .long .lmr_len_2C
    .long .lmr_len_2D
    .long .lmr_len_2E
    .long .lmr_len_2F
    .long .lmr_len_30
    .long .lmr_len_31
    .long .lmr_len_32
    .long .lmr_len_33
    .long .lmr_len_34
    .long .lmr_len_35
    .long .lmr_len_36
    .long .lmr_len_37
    .long .lmr_len_38
    .long .lmr_len_39
    .long .lmr_len_3A
    .long .lmr_len_3B
    .long .lmr_len_3C
    .long .lmr_len_3D
    .long .lmr_len_3E
    .long .lmr_len_3F
    .long .lmr_len_40
    .long .lmr_len_41
    .long .lmr_len_42
    .long .lmr_len_43
    .long .lmr_len_44
    .long .lmr_len_45
    .long .lmr_len_46
    .long .lmr_len_47
    .long .lmr_len_48
    .long .lmr_len_49
    .long .lmr_len_4A
    .long .lmr_len_4B
    .long .lmr_len_4C
    .long .lmr_len_4D
    .long .lmr_len_4E
    .long .lmr_len_4F
    .long .lmr_len_50
    .long .lmr_len_51
    .long .lmr_len_52
    .long .lmr_len_53
    .long .lmr_len_54
    .long .lmr_len_55
    .long .lmr_len_56
    .long .lmr_len_57
    .long .lmr_len_58
    .long .lmr_len_59
    .long .lmr_len_5A
    .long .lmr_len_5B
    .long .lmr_len_5C
    .long .lmr_len_5D
    .long .lmr_len_5E
    .long .lmr_len_5F
    .long .lmr_len_60
    .long .lmr_len_61
    .long .lmr_len_62
    .long .lmr_len_63
    .long .lmr_len_64
    .long .lmr_len_65
    .long .lmr_len_66
    .long .lmr_len_67
    .long .lmr_len_68
    .long .lmr_len_69
    .long .lmr_len_6A
    .long .lmr_len_6B
    .long .lmr_len_6C
    .long .lmr_len_6D
    .long .lmr_len_6E
    .long .lmr_len_6F
    .long .lmr_len_70
    .long .lmr_len_71
    .long .lmr_len_72
    .long .lmr_len_73
    .long .lmr_len_74
    .long .lmr_len_75
    .long .lmr_len_76
    .long .lmr_len_77
    .long .lmr_len_78
    .long .lmr_len_79
    .long .lmr_len_7A
    .long .lmr_len_7B
    .long .lmr_len_7C
    .long .lmr_len_7D
    .long .lmr_len_7E
    .long .lmr_len_7F
    .long .lmr_len_80
    .long .lmr_len_81
    .long .lmr_len_82
    .long .lmr_len_83
    .long .lmr_len_84
    .long .lmr_len_85
    .long .lmr_len_86
    .long .lmr_len_87
    .long .lmr_len_88
    .long .lmr_len_89
    .long .lmr_len_8A
    .long .lmr_len_8B
    .long .lmr_len_8C
    .long .lmr_len_8D
    .long .lmr_len_8E
    .long .lmr_len_8F
    .long .lmr_len_90
    .long .lmr_len_91
    .long .lmr_len_92
    .long .lmr_len_93
    .long .lmr_len_94
    .long .lmr_len_95
    .long .lmr_len_96
    .long .lmr_len_97
    .long .lmr_len_98
    .long .lmr_len_99
    .long .lmr_len_9A
    .long .lmr_len_9B
    .long .lmr_len_9C
    .long .lmr_len_9D
    .long .lmr_len_9E
    .long .lmr_len_9F
    .long .lmr_len_A0
    .long .lmr_len_A1
    .long .lmr_len_A2
    .long .lmr_len_A3
    .long .lmr_len_A4
    .long .lmr_len_A5
    .long .lmr_len_A6
    .long .lmr_len_A7
    .long .lmr_len_A8
    .long .lmr_len_A9
    .long .lmr_len_AA
    .long .lmr_len_AB
    .long .lmr_len_AC
    .long .lmr_len_AD
    .long .lmr_len_AE
    .long .lmr_len_AF
    .long .lmr_len_B0
    .long .lmr_len_B1
    .long .lmr_len_B2
    .long .lmr_len_B3
    .long .lmr_len_B4
    .long .lmr_len_B5
    .long .lmr_len_B6
    .long .lmr_len_B7
    .long .lmr_len_B8
    .long .lmr_len_B9
    .long .lmr_len_BA
    .long .lmr_len_BB
    .long .lmr_len_BC
    .long .lmr_len_BD
    .long .lmr_len_BE
    .long .lmr_len_BF
    .long .lmr_len_C0
    .long .lmr_len_C1
    .long .lmr_len_C2
    .long .lmr_len_C3
    .long .lmr_len_C4
    .long .lmr_len_C5
    .long .lmr_len_C6
    .long .lmr_len_C7
    .long .lmr_len_C8
    .long .lmr_len_C9
    .long .lmr_len_CA
    .long .lmr_len_CB
    .long .lmr_len_CC
    .long .lmr_len_CD
    .long .lmr_len_CE
    .long .lmr_len_CF
    .long .lmr_len_D0
    .long .lmr_len_D1
    .long .lmr_len_D2
    .long .lmr_len_D3
    .long .lmr_len_D4
    .long .lmr_len_D5
    .long .lmr_len_D6
    .long .lmr_len_D7
    .long .lmr_len_D8
    .long .lmr_len_D9
    .long .lmr_len_DA
    .long .lmr_len_DB
    .long .lmr_len_DC
    .long .lmr_len_DD
    .long .lmr_len_DE
    .long .lmr_len_DF
    .long .lmr_len_E0
    .long .lmr_len_E1
    .long .lmr_len_E2
    .long .lmr_len_E3
    .long .lmr_len_E4
    .long .lmr_len_E5
    .long .lmr_len_E6
    .long .lmr_len_E7
    .long .lmr_len_E8
    .long .lmr_len_E9
    .long .lmr_len_EA
    .long .lmr_len_EB
    .long .lmr_len_EC
    .long .lmr_len_ED
    .long .lmr_len_EE
    .long .lmr_len_EF
    .long .lmr_len_F0
    .long .lmr_len_F1
    .long .lmr_len_F2
    .long .lmr_len_F3
    .long .lmr_len_F4
    .long .lmr_len_F5
    .long .lmr_len_F6
    .long .lmr_len_F7
    .long .lmr_len_F8
    .long .lmr_len_F9
    .long .lmr_len_FA
    .long .lmr_len_FB
    .long .lmr_len_FC
    .long .lmr_len_FD
    .long .lmr_len_FE
    .long .lmr_len_FF


.litE_mat1:
    move.l  (%a0)+,(%a1)+
.litC_mat1:
    move.l  (%a0)+,(%a1)+
.litA_mat1:
    move.l  (%a0)+,(%a1)+
.lit8_mat1:
    move.l  (%a0)+,(%a1)+
.lit6_mat1:
    move.l  (%a0)+,(%a1)+
.lit4_mat1:
    move.l  (%a0)+,(%a1)+
.lit2_mat1:
    move.l  (%a0)+,(%a1)+
.lit0_mat1:
    add.w   %d1,%d1
    neg.w   %d1
    lea     -2(%a1,%d1.w),%a2       | a2 = dst - ((match offset + 1) * 2)

    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    moveq   #0,%d1
    jmp     (%a3)

.litF_mat1:
    move.l  (%a0)+,(%a1)+
.litD_mat1:
    move.l  (%a0)+,(%a1)+
.litB_mat1:
    move.l  (%a0)+,(%a1)+
.lit9_mat1:
    move.l  (%a0)+,(%a1)+
.lit7_mat1:
    move.l  (%a0)+,(%a1)+
.lit5_mat1:
    move.l  (%a0)+,(%a1)+
.lit3_mat1:
    move.l  (%a0)+,(%a1)+
.lit1_mat1:
    move.w  (%a0)+,(%a1)+

    add.w   %d1,%d1
    neg.w   %d1
    lea     -2(%a1,%d1.w),%a2       | a2 = dst - ((match offset + 1) * 2)

    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    moveq   #0,%d1
    jmp     (%a3)

.litE_mat2:
    move.l  (%a0)+,(%a1)+
.litC_mat2:
    move.l  (%a0)+,(%a1)+
.litA_mat2:
    move.l  (%a0)+,(%a1)+
.lit8_mat2:
    move.l  (%a0)+,(%a1)+
.lit6_mat2:
    move.l  (%a0)+,(%a1)+
.lit4_mat2:
    move.l  (%a0)+,(%a1)+
.lit2_mat2:
    move.l  (%a0)+,(%a1)+
.lit0_mat2:
    add.w   %d1,%d1
    neg.w   %d1
    lea     -2(%a1,%d1.w),%a2       | a2 = dst - ((match offset + 1) * 2)

    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    moveq   #0,%d1
    jmp     (%a3)

.litF_mat2:
    move.l  (%a0)+,(%a1)+
.litD_mat2:
    move.l  (%a0)+,(%a1)+
.litB_mat2:
    move.l  (%a0)+,(%a1)+
.lit9_mat2:
    move.l  (%a0)+,(%a1)+
.lit7_mat2:
    move.l  (%a0)+,(%a1)+
.lit5_mat2:
    move.l  (%a0)+,(%a1)+
.lit3_mat2:
    move.l  (%a0)+,(%a1)+
.lit1_mat2:
    move.w  (%a0)+,(%a1)+

    add.w   %d1,%d1
    neg.w   %d1
    lea     -2(%a1,%d1.w),%a2       | a2 = dst - ((match offset + 1) * 2)

    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    moveq   #0,%d1
    jmp     (%a3)

.litE_mat3:
    move.l  (%a0)+,(%a1)+
.litC_mat3:
    move.l  (%a0)+,(%a1)+
.litA_mat3:
    move.l  (%a0)+,(%a1)+
.lit8_mat3:
    move.l  (%a0)+,(%a1)+
.lit6_mat3:
    move.l  (%a0)+,(%a1)+
.lit4_mat3:
    move.l  (%a0)+,(%a1)+
.lit2_mat3:
    move.l  (%a0)+,(%a1)+
.lit0_mat3:
    add.w   %d1,%d1
    neg.w   %d1
    lea     -2(%a1,%d1.w),%a2       | a2 = dst - ((match offset + 1) * 2)

    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    moveq   #0,%d1
    jmp     (%a3)

.litF_mat3:
    move.l  (%a0)+,(%a1)+
.litD_mat3:
    move.l  (%a0)+,(%a1)+
.litB_mat3:
    move.l  (%a0)+,(%a1)+
.lit9_mat3:
    move.l  (%a0)+,(%a1)+
.lit7_mat3:
    move.l  (%a0)+,(%a1)+
.lit5_mat3:
    move.l  (%a0)+,(%a1)+
.lit3_mat3:
    move.l  (%a0)+,(%a1)+
.lit1_mat3:
    move.w  (%a0)+,(%a1)+

    add.w   %d1,%d1
    neg.w   %d1
    lea     -2(%a1,%d1.w),%a2       | a2 = dst - ((match offset + 1) * 2)

    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    moveq   #0,%d1
    jmp     (%a3)

.litE_mat4:
    move.l  (%a0)+,(%a1)+
.litC_mat4:
    move.l  (%a0)+,(%a1)+
.litA_mat4:
    move.l  (%a0)+,(%a1)+
.lit8_mat4:
    move.l  (%a0)+,(%a1)+
.lit6_mat4:
    move.l  (%a0)+,(%a1)+
.lit4_mat4:
    move.l  (%a0)+,(%a1)+
.lit2_mat4:
    move.l  (%a0)+,(%a1)+
.lit0_mat4:
    add.w   %d1,%d1
    neg.w   %d1
    lea     -2(%a1,%d1.w),%a2       | a2 = dst - ((match offset + 1) * 2)

    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    moveq   #0,%d1
    jmp     (%a3)

.litF_mat4:
    move.l  (%a0)+,(%a1)+
.litD_mat4:
    move.l  (%a0)+,(%a1)+
.litB_mat4:
    move.l  (%a0)+,(%a1)+
.lit9_mat4:
    move.l  (%a0)+,(%a1)+
.lit7_mat4:
    move.l  (%a0)+,(%a1)+
.lit5_mat4:
    move.l  (%a0)+,(%a1)+
.lit3_mat4:
    move.l  (%a0)+,(%a1)+
.lit1_mat4:
    move.w  (%a0)+,(%a1)+

    add.w   %d1,%d1
    neg.w   %d1
    lea     -2(%a1,%d1.w),%a2       | a2 = dst - ((match offset + 1) * 2)

    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    moveq   #0,%d1
    jmp     (%a3)

.litE_mat5:
    move.l  (%a0)+,(%a1)+
.litC_mat5:
    move.l  (%a0)+,(%a1)+
.litA_mat5:
    move.l  (%a0)+,(%a1)+
.lit8_mat5:
    move.l  (%a0)+,(%a1)+
.lit6_mat5:
    move.l  (%a0)+,(%a1)+
.lit4_mat5:
    move.l  (%a0)+,(%a1)+
.lit2_mat5:
    move.l  (%a0)+,(%a1)+
.lit0_mat5:
    add.w   %d1,%d1
    neg.w   %d1
    lea     -2(%a1,%d1.w),%a2       | a2 = dst - ((match offset + 1) * 2)

    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    moveq   #0,%d1
    jmp     (%a3)

.litF_mat5:
    move.l  (%a0)+,(%a1)+
.litD_mat5:
    move.l  (%a0)+,(%a1)+
.litB_mat5:
    move.l  (%a0)+,(%a1)+
.lit9_mat5:
    move.l  (%a0)+,(%a1)+
.lit7_mat5:
    move.l  (%a0)+,(%a1)+
.lit5_mat5:
    move.l  (%a0)+,(%a1)+
.lit3_mat5:
    move.l  (%a0)+,(%a1)+
.lit1_mat5:
    move.w  (%a0)+,(%a1)+

    add.w   %d1,%d1
    neg.w   %d1
    lea     -2(%a1,%d1.w),%a2       | a2 = dst - ((match offset + 1) * 2)

    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    moveq   #0,%d1
    jmp     (%a3)

.litE_mat6:
    move.l  (%a0)+,(%a1)+
.litC_mat6:
    move.l  (%a0)+,(%a1)+
.litA_mat6:
    move.l  (%a0)+,(%a1)+
.lit8_mat6:
    move.l  (%a0)+,(%a1)+
.lit6_mat6:
    move.l  (%a0)+,(%a1)+
.lit4_mat6:
    move.l  (%a0)+,(%a1)+
.lit2_mat6:
    move.l  (%a0)+,(%a1)+
.lit0_mat6:
    add.w   %d1,%d1
    neg.w   %d1
    lea     -2(%a1,%d1.w),%a2       | a2 = dst - ((match offset + 1) * 2)

    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    moveq   #0,%d1
    jmp     (%a3)

.litF_mat6:
    move.l  (%a0)+,(%a1)+
.litD_mat6:
    move.l  (%a0)+,(%a1)+
.litB_mat6:
    move.l  (%a0)+,(%a1)+
.lit9_mat6:
    move.l  (%a0)+,(%a1)+
.lit7_mat6:
    move.l  (%a0)+,(%a1)+
.lit5_mat6:
    move.l  (%a0)+,(%a1)+
.lit3_mat6:
    move.l  (%a0)+,(%a1)+
.lit1_mat6:
    move.w  (%a0)+,(%a1)+

    add.w   %d1,%d1
    neg.w   %d1
    lea     -2(%a1,%d1.w),%a2       | a2 = dst - ((match offset + 1) * 2)

    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    moveq   #0,%d1
    jmp     (%a3)

.litE_mat7:
    move.l  (%a0)+,(%a1)+
.litC_mat7:
    move.l  (%a0)+,(%a1)+
.litA_mat7:
    move.l  (%a0)+,(%a1)+
.lit8_mat7:
    move.l  (%a0)+,(%a1)+
.lit6_mat7:
    move.l  (%a0)+,(%a1)+
.lit4_mat7:
    move.l  (%a0)+,(%a1)+
.lit2_mat7:
    move.l  (%a0)+,(%a1)+
.lit0_mat7:
    add.w   %d1,%d1
    neg.w   %d1
    lea     -2(%a1,%d1.w),%a2       | a2 = dst - ((match offset + 1) * 2)

    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    moveq   #0,%d1
    jmp     (%a3)

.litF_mat7:
    move.l  (%a0)+,(%a1)+
.litD_mat7:
    move.l  (%a0)+,(%a1)+
.litB_mat7:
    move.l  (%a0)+,(%a1)+
.lit9_mat7:
    move.l  (%a0)+,(%a1)+
.lit7_mat7:
    move.l  (%a0)+,(%a1)+
.lit5_mat7:
    move.l  (%a0)+,(%a1)+
.lit3_mat7:
    move.l  (%a0)+,(%a1)+
.lit1_mat7:
    move.w  (%a0)+,(%a1)+

    add.w   %d1,%d1
    neg.w   %d1
    lea     -2(%a1,%d1.w),%a2       | a2 = dst - ((match offset + 1) * 2)

    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    moveq   #0,%d1
    jmp     (%a3)

.litE_mat8:
    move.l  (%a0)+,(%a1)+
.litC_mat8:
    move.l  (%a0)+,(%a1)+
.litA_mat8:
    move.l  (%a0)+,(%a1)+
.lit8_mat8:
    move.l  (%a0)+,(%a1)+
.lit6_mat8:
    move.l  (%a0)+,(%a1)+
.lit4_mat8:
    move.l  (%a0)+,(%a1)+
.lit2_mat8:
    move.l  (%a0)+,(%a1)+
.lit0_mat8:
    add.w   %d1,%d1
    neg.w   %d1
    lea     -2(%a1,%d1.w),%a2       | a2 = dst - ((match offset + 1) * 2)

    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    moveq   #0,%d1
    jmp     (%a3)

.litF_mat8:
    move.l  (%a0)+,(%a1)+
.litD_mat8:
    move.l  (%a0)+,(%a1)+
.litB_mat8:
    move.l  (%a0)+,(%a1)+
.lit9_mat8:
    move.l  (%a0)+,(%a1)+
.lit7_mat8:
    move.l  (%a0)+,(%a1)+
.lit5_mat8:
    move.l  (%a0)+,(%a1)+
.lit3_mat8:
    move.l  (%a0)+,(%a1)+
.lit1_mat8:
    move.w  (%a0)+,(%a1)+

    add.w   %d1,%d1
    neg.w   %d1
    lea     -2(%a1,%d1.w),%a2       | a2 = dst - ((match offset + 1) * 2)

    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    moveq   #0,%d1
    jmp     (%a3)

.litE_mat9:
    move.l  (%a0)+,(%a1)+
.litC_mat9:
    move.l  (%a0)+,(%a1)+
.litA_mat9:
    move.l  (%a0)+,(%a1)+
.lit8_mat9:
    move.l  (%a0)+,(%a1)+
.lit6_mat9:
    move.l  (%a0)+,(%a1)+
.lit4_mat9:
    move.l  (%a0)+,(%a1)+
.lit2_mat9:
    move.l  (%a0)+,(%a1)+
.lit0_mat9:
    add.w   %d1,%d1
    neg.w   %d1
    lea     -2(%a1,%d1.w),%a2       | a2 = dst - ((match offset + 1) * 2)

    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    moveq   #0,%d1
    jmp     (%a3)

.litF_mat9:
    move.l  (%a0)+,(%a1)+
.litD_mat9:
    move.l  (%a0)+,(%a1)+
.litB_mat9:
    move.l  (%a0)+,(%a1)+
.lit9_mat9:
    move.l  (%a0)+,(%a1)+
.lit7_mat9:
    move.l  (%a0)+,(%a1)+
.lit5_mat9:
    move.l  (%a0)+,(%a1)+
.lit3_mat9:
    move.l  (%a0)+,(%a1)+
.lit1_mat9:
    move.w  (%a0)+,(%a1)+

    add.w   %d1,%d1
    neg.w   %d1
    lea     -2(%a1,%d1.w),%a2       | a2 = dst - ((match offset + 1) * 2)

    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    moveq   #0,%d1
    jmp     (%a3)

.litE_matA:
    move.l  (%a0)+,(%a1)+
.litC_matA:
    move.l  (%a0)+,(%a1)+
.litA_matA:
    move.l  (%a0)+,(%a1)+
.lit8_matA:
    move.l  (%a0)+,(%a1)+
.lit6_matA:
    move.l  (%a0)+,(%a1)+
.lit4_matA:
    move.l  (%a0)+,(%a1)+
.lit2_matA:
    move.l  (%a0)+,(%a1)+
.lit0_matA:
    add.w   %d1,%d1
    neg.w   %d1
    lea     -2(%a1,%d1.w),%a2       | a2 = dst - ((match offset + 1) * 2)

    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    moveq   #0,%d1
    jmp     (%a3)

.litF_matA:
    move.l  (%a0)+,(%a1)+
.litD_matA:
    move.l  (%a0)+,(%a1)+
.litB_matA:
    move.l  (%a0)+,(%a1)+
.lit9_matA:
    move.l  (%a0)+,(%a1)+
.lit7_matA:
    move.l  (%a0)+,(%a1)+
.lit5_matA:
    move.l  (%a0)+,(%a1)+
.lit3_matA:
    move.l  (%a0)+,(%a1)+
.lit1_matA:
    move.w  (%a0)+,(%a1)+

    add.w   %d1,%d1
    neg.w   %d1
    lea     -2(%a1,%d1.w),%a2       | a2 = dst - ((match offset + 1) * 2)

    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    moveq   #0,%d1
    jmp     (%a3)

.litE_matB:
    move.l  (%a0)+,(%a1)+
.litC_matB:
    move.l  (%a0)+,(%a1)+
.litA_matB:
    move.l  (%a0)+,(%a1)+
.lit8_matB:
    move.l  (%a0)+,(%a1)+
.lit6_matB:
    move.l  (%a0)+,(%a1)+
.lit4_matB:
    move.l  (%a0)+,(%a1)+
.lit2_matB:
    move.l  (%a0)+,(%a1)+
.lit0_matB:
    add.w   %d1,%d1
    neg.w   %d1
    lea     -2(%a1,%d1.w),%a2       | a2 = dst - ((match offset + 1) * 2)

    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    moveq   #0,%d1
    jmp     (%a3)

.litF_matB:
    move.l  (%a0)+,(%a1)+
.litD_matB:
    move.l  (%a0)+,(%a1)+
.litB_matB:
    move.l  (%a0)+,(%a1)+
.lit9_matB:
    move.l  (%a0)+,(%a1)+
.lit7_matB:
    move.l  (%a0)+,(%a1)+
.lit5_matB:
    move.l  (%a0)+,(%a1)+
.lit3_matB:
    move.l  (%a0)+,(%a1)+
.lit1_matB:
    move.w  (%a0)+,(%a1)+

    add.w   %d1,%d1
    neg.w   %d1
    lea     -2(%a1,%d1.w),%a2       | a2 = dst - ((match offset + 1) * 2)

    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    moveq   #0,%d1
    jmp     (%a3)

.litE_matC:
    move.l  (%a0)+,(%a1)+
.litC_matC:
    move.l  (%a0)+,(%a1)+
.litA_matC:
    move.l  (%a0)+,(%a1)+
.lit8_matC:
    move.l  (%a0)+,(%a1)+
.lit6_matC:
    move.l  (%a0)+,(%a1)+
.lit4_matC:
    move.l  (%a0)+,(%a1)+
.lit2_matC:
    move.l  (%a0)+,(%a1)+
.lit0_matC:
    add.w   %d1,%d1
    neg.w   %d1
    lea     -2(%a1,%d1.w),%a2       | a2 = dst - ((match offset + 1) * 2)

    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    moveq   #0,%d1
    jmp     (%a3)

.litF_matC:
    move.l  (%a0)+,(%a1)+
.litD_matC:
    move.l  (%a0)+,(%a1)+
.litB_matC:
    move.l  (%a0)+,(%a1)+
.lit9_matC:
    move.l  (%a0)+,(%a1)+
.lit7_matC:
    move.l  (%a0)+,(%a1)+
.lit5_matC:
    move.l  (%a0)+,(%a1)+
.lit3_matC:
    move.l  (%a0)+,(%a1)+
.lit1_matC:
    move.w  (%a0)+,(%a1)+

    add.w   %d1,%d1
    neg.w   %d1
    lea     -2(%a1,%d1.w),%a2       | a2 = dst - ((match offset + 1) * 2)

    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    moveq   #0,%d1
    jmp     (%a3)

.litE_matD:
    move.l  (%a0)+,(%a1)+
.litC_matD:
    move.l  (%a0)+,(%a1)+
.litA_matD:
    move.l  (%a0)+,(%a1)+
.lit8_matD:
    move.l  (%a0)+,(%a1)+
.lit6_matD:
    move.l  (%a0)+,(%a1)+
.lit4_matD:
    move.l  (%a0)+,(%a1)+
.lit2_matD:
    move.l  (%a0)+,(%a1)+
.lit0_matD:
    add.w   %d1,%d1
    neg.w   %d1
    lea     -2(%a1,%d1.w),%a2       | a2 = dst - ((match offset + 1) * 2)

    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    moveq   #0,%d1
    jmp     (%a3)

.litF_matD:
    move.l  (%a0)+,(%a1)+
.litD_matD:
    move.l  (%a0)+,(%a1)+
.litB_matD:
    move.l  (%a0)+,(%a1)+
.lit9_matD:
    move.l  (%a0)+,(%a1)+
.lit7_matD:
    move.l  (%a0)+,(%a1)+
.lit5_matD:
    move.l  (%a0)+,(%a1)+
.lit3_matD:
    move.l  (%a0)+,(%a1)+
.lit1_matD:
    move.w  (%a0)+,(%a1)+

    add.w   %d1,%d1
    neg.w   %d1
    lea     -2(%a1,%d1.w),%a2       | a2 = dst - ((match offset + 1) * 2)

    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    moveq   #0,%d1
    jmp     (%a3)

.litE_matE:
    move.l  (%a0)+,(%a1)+
.litC_matE:
    move.l  (%a0)+,(%a1)+
.litA_matE:
    move.l  (%a0)+,(%a1)+
.lit8_matE:
    move.l  (%a0)+,(%a1)+
.lit6_matE:
    move.l  (%a0)+,(%a1)+
.lit4_matE:
    move.l  (%a0)+,(%a1)+
.lit2_matE:
    move.l  (%a0)+,(%a1)+
.lit0_matE:
    add.w   %d1,%d1
    neg.w   %d1
    lea     -2(%a1,%d1.w),%a2       | a2 = dst - ((match offset + 1) * 2)

    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    moveq   #0,%d1
    jmp     (%a3)

.litF_matE:
    move.l  (%a0)+,(%a1)+
.litD_matE:
    move.l  (%a0)+,(%a1)+
.litB_matE:
    move.l  (%a0)+,(%a1)+
.lit9_matE:
    move.l  (%a0)+,(%a1)+
.lit7_matE:
    move.l  (%a0)+,(%a1)+
.lit5_matE:
    move.l  (%a0)+,(%a1)+
.lit3_matE:
    move.l  (%a0)+,(%a1)+
.lit1_matE:
    move.w  (%a0)+,(%a1)+

    add.w   %d1,%d1
    neg.w   %d1
    lea     -2(%a1,%d1.w),%a2       | a2 = dst - ((match offset + 1) * 2)

    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    moveq   #0,%d1
    jmp     (%a3)

.litE_matF:
    move.l  (%a0)+,(%a1)+
.litC_matF:
    move.l  (%a0)+,(%a1)+
.litA_matF:
    move.l  (%a0)+,(%a1)+
.lit8_matF:
    move.l  (%a0)+,(%a1)+
.lit6_matF:
    move.l  (%a0)+,(%a1)+
.lit4_matF:
    move.l  (%a0)+,(%a1)+
.lit2_matF:
    move.l  (%a0)+,(%a1)+
.lit0_matF:
    add.w   %d1,%d1
    neg.w   %d1
    lea     -2(%a1,%d1.w),%a2       | a2 = dst - ((match offset + 1) * 2)

    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    moveq   #0,%d1
    jmp     (%a3)

.litF_matF:
    move.l  (%a0)+,(%a1)+
.litD_matF:
    move.l  (%a0)+,(%a1)+
.litB_matF:
    move.l  (%a0)+,(%a1)+
.lit9_matF:
    move.l  (%a0)+,(%a1)+
.lit7_matF:
    move.l  (%a0)+,(%a1)+
.lit5_matF:
    move.l  (%a0)+,(%a1)+
.lit3_matF:
    move.l  (%a0)+,(%a1)+
.lit1_matF:
    move.w  (%a0)+,(%a1)+

    add.w   %d1,%d1
    neg.w   %d1
    lea     -2(%a1,%d1.w),%a2       | a2 = dst - ((match offset + 1) * 2)

    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+
    moveq   #0,%d1
    jmp     (%a3)

.done:
    move.w  (%a0)+,%d0              | need to copy a last byte ?
    bpl.s   .no_byte

    move.b  %d0,(%a1)+              | copy last byte
.no_byte:

    move.l  %a1,%d0
    sub.l   20(%sp),%d0             | return op - dest

    movem.l (%sp)+,%a2-%a4
    rts
