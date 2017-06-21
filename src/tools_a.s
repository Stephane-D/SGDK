# ---------------------------------------------------------------
# Quicksort for MC68000
# by Stephane Dallongeville @2016
# original implementation by (c) 2014 Dale Whinham
#
# void qsort(void** src, u16 size, _comparatorCallback* cb);
# ---------------------------------------------------------------

    .global qsort

qsort:
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

    .global aplib_unpack
    .global aplib_decrunch

aplib_unpack:
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
# by Stephane Dallongeville @2016
#
# lz4w_unpack_a: A0 = Source / A1 = Destination / Returns unpacked size
# u16 lz4w_unpack(const u8 *src, u8 *dest);  /* c prototype */
# ---------------------------------------------------------------------------
    .align    2
    .globl    lz4w_unpack
    .globl    lz4w_unpack_a

lz4w_unpack:
    move.l  4(%sp),%a0              | a0 = src
    move.l  8(%sp),%a1              | a1 = dst

lz4w_unpack_a:
    movem.l %a2-%a4,-(%sp)

    lea     .next,%a3               | used for fast jump
    addq.l  #4,%a0                  | bypass unpacked length field
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

    .long .done
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
.lit0_mat0:
    jmp     (%a3)

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
    jmp     (%a3)

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

    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
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

    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
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

    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
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

    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
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

    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
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

    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
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

    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
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

    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
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

    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
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

    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
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

    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
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

    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
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

    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
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

    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
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

    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
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

    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
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

    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
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

    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
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

    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
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

    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
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

    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
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

    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
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

    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
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

    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
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

    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
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

    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
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

    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
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

    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
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

    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
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

    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
    move.w  (%a2)+,(%a1)+  ;  move.w  (%a2)+,(%a1)+
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
