;// ---------------------------------------------------------------
;// Quicksort for MC68000
;// by Stephane Dallongeville @2016
;// original implementation by (c) 2014 Dale Whinham
;//
;// void qsort(void** src, u16 size, _comparatorCallback* cb);
;// ---------------------------------------------------------------

#include "asm_mac.i"

func qsort
    movem.l %a2-%a6,-(%sp)

    move.l  24(%sp),%a2             ;// a2 = src
    move.l  28(%sp),%d0             ;// d0 = size (limited to 0x3FFF)
    move.l  32(%sp),%a6             ;// a6 = comparator

    add.w   %d0,%d0
    add.w   %d0,%d0
    lea     -4(%a2,%d0.w),%a3       ;// a3 = &src[size - 1]

    bsr quicksort                   ;// start sort !

    movem.l (%sp)+,%a2-%a6
    rts

;// ---------------------------------------------------------------
;// Quicksort
;// a2 - left pointer
;// a3 - right pointer
;// ---------------------------------------------------------------

quicksort:
    cmp.l   %a2,%a3                 ;// L >= R ?
    ble     .endqsort               ;// done !

    bsr     partition               ;// index of partition is in a1

    movem.l %a1-%a3,-(%sp)          ;// save P,L,R

    lea     -4(%a1),%a3             ;// R = P-1

    bsr     quicksort               ;// quicksort(L, P-1)

    movem.l (%sp),%a1-%a3           ;// restore P,L,R
    lea     4(%a1),%a2              ;// L = P+1
    move.l  %a2,4(%sp)              ;// save changed L

    bsr     quicksort               ;// quicksort(P+1, R)

    movem.l (%sp)+,%a1-%a3          ;// restore P,L,R

.endqsort:
    rts

;// ---------------------------------------------------------------
;// Partition
;// a2 - left pointer
;// a3 - right pointer
;// return pivot in a1
;// ---------------------------------------------------------------
partition:
    move.l  %a2,%a4                 ;// a4 = L
    move.l  %a3,%a5                 ;// a5 = R = P

;//    move.l  %a3,%d0
;//    add.l   %a2,%d0
;//    asr.l   %d0                     ;// d0 = P = (L+R)/2
;//    and.w   0xFFFC,%d0              ;// clear bit 0 & 1
;//    move.l  %d0,%a0                 ;// a0 = P

    move.l  (%a5),-(%sp)            ;// reserve space for comparator arguments, put (*L,*P) by default
    move.l  (%a4),-(%sp)

    lea     -4(%a5),%a0             ;// tmp = next R
    cmp.l   %a4,%a0                 ;// L >= R ?
    ble     .finish                 ;// done !

.loop:
.findleft:
    move.l  (%a4)+,(%sp)            ;// first argument comparator = *L
    jsr     (%a6)                   ;// compare

    tst.w   %d0                     ;// *L < *P
    blt     .findleft

    lea     -4(%a4),%a4             ;// L fix (put on value to swap)

.findright:
    move.l  -(%a5),(%sp)            ;// first argument comparator = *R
    jsr     (%a6)                   ;// compare

    tst.w   %d0                     ;// *R > *P
    bgt     .findright

    cmp.l   %a4,%a5                 ;// L >= R ?
    ble     .finish                 ;// done !

    move.l  (%a4),%d0               ;// swap *R / *L
    move.l  (%a5),(%a4)+            ;// L++
    move.l  %d0,(%a5)

    lea     -4(%a5),%a0             ;// tmp = next R
    cmp.l   %a4,%a0                 ;// R > L ?
    bhi     .loop                   ;// continue !

.finish:
    move.l  (%a4),%d0               ;// swap *L / *P
    move.l  (%a3),(%a4)
    move.l  %d0,(%a3)

    move.l  %a4,%a1                 ;// a1 = L = new pivot

    lea     8(%sp),%sp              ;// release space for comparator arguments
    rts


;// -------------------------------------------------------------------------------------------------
;// Aplib decruncher for MC68000 "gcc version"
;// by MML 2010
;// Size optimized (164 bytes) by Franck "hitchhikr" Charlet.
;// More optimizations by r57shell.
;//
;// aplib_decrunch: A0 = Source / A1 = Destination / D0 Returns unpacked size
;// u32 aplib_unpack(u8 *src, u8 *dest); /* c prototype */
;//
;// -------------------------------------------------------------------------------------------------

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
    moveq   #2,%d1                  ;// Initialize LWM

.next_sequence:
    bsr.b   .get_bit
    bcc.b   .copy_byte              ;// if bit sequence is %0..., then copy next byte

    bsr.b   .get_bit
    bcc.b   .code_pair              ;// if bit sequence is %10..., then is a code pair

    moveq   #0,%d0                  ;// offset = 0 (eor.l %d0,%d0)
    bsr.b   .get_bit
    bcc.b   .short_match            ;// if bit sequence is %110..., then is a short match

    moveq   #4-1,%d5                ;// The sequence is %111..., the next 4 bits are the offset (0-15)
.get_3_bits:
    bsr.b   .get_bit
    roxl.l  #1,%d0                  ;// addx.l  %d0,%d0 <- my bug, Z flag only cleared, not SET
    dbf     %d5,.get_3_bits         ;// (dbcc doesn't modify flags)
    beq.b   .write_byte             ;// if offset == 0, then write 0x00

                                    ;// If offset != 0, then write the byte on destination - offset
    move.l  %a1,%a2
    suba.l  %d0,%a2
    move.b  (%a2),%d0

.write_byte:
    move.b  %d0,(%a1)+
    bra.b   .next_sequence_init

.short_match:                       ;// Short match %110...
    moveq   #3,%d2                  ;// length = 3
    move.b  (%a0)+,%d0              ;// Get offset (offset is 7 bits + 1 bit to mark if copy 2 or 3 bytes)
    lsr.b   #1,%d0
    beq.b   .end_decrunch           ;// if offset == 0, end of decrunching
    bcs.b   .domatch_new_lastpos
    moveq   #2,%d2                  ;// length = 2
    bra.b   .domatch_new_lastpos

.code_pair:                         ;// Code pair %10...
    bsr.b   .decode_gamma
    sub.l   %d1,%d2                 ;// offset -= LWM
    bne.b   .normal_code_pair
    move.l  %d4,%d0                 ;// offset = old_offset
    bsr.b   .decode_gamma
    bra.b   .copy_code_pair

.normal_code_pair:
    subq.l  #1,%d2                  ;// offset -= 1
    lsl.l   #8,%d2                  ;// offset << 8
    move.b  (%a0)+,%d2              ;// get the least significant byte of the offset (16 bits)
    move.l  %d2,%d0
    bsr.b   .decode_gamma
    cmp.l   %a3,%d0                 ;// >=32000
    bge.b   .domatch_with_2inc

.compare_1280:
    cmp.l   %a4,%d0                 ;// >=1280 <32000
    bge.b   .domatch_with_inc

.compare_128:
    cmp.l   %a5,%d0                 ;// >=128 <1280
    bge.b   .domatch_new_lastpos

.domatch_with_2inc:
    addq.l  #1,%d2
.domatch_with_inc:
    addq.l  #1,%d2
.domatch_new_lastpos:
    move.l  %d0,%d4                 ;// old_offset = offset
.copy_code_pair:
    subq.l  #1,%d2                  ;// length--
    move.l  %a1,%a2
    suba.l  %d0,%a2

.loop_do_copy:
    move.b  (%a2)+,(%a1)+
    dbf     %d2,.loop_do_copy
    moveq   #1,%d1                  ;// LWM = 1
    bra.b   .next_sequence          ;// Process next sequence

.get_bit:                           ;// Get bits from the crunched data (D3) and insert the most significant bit in the carry flag.
    add.b   %d3,%d3
    bne.b   .still_bits_left
    move.b  (%a0)+,%d3              ;// Read next crunched byte
    addx.b  %d3,%d3

.still_bits_left:
    rts

.decode_gamma:                      ;// Decode values from the crunched data using gamma code
    moveq   #1,%d2

.get_more_gamma:
    bsr.b   .get_bit
    addx.l  %d2,%d2
    bsr.b   .get_bit
    bcs.b   .get_more_gamma
    rts

.end_decrunch:
    move.l %a1,%d0
    sub.l %a6,%d0                   ;// d0 = unpacked size

    movem.l (%a7)+,%a2-%a6/%d2-%d5
    rts


;// ---------------------------------------------------------------------------
;// LZ4W unpacker for MC68000
;// by Stephane Dallongeville @2017
;// decomp code tweaked by HpMan, optimized further by Malachi
;//
;// lz4w_unpack_a: A0 = Source / A1 = Destination / D0 Returns unpacked size
;// u16 lz4w_unpack(const u8 *src, u8 *dest);  /* c prototype */
;// ---------------------------------------------------------------------------

.macro  LZ4W_NEXT
    moveq  #0, d1
    moveq  #0, d0
    move.b  (a0)+, d0               ;// d0 = literal & match length
    move.b  (a0)+, d1               ;// d1 = match offset

    add.w  d0, d0
    add.w  d0, d0
    move.l  (a3,d0.w), a4
    jmp  (a4)
  .endm

func lz4w_unpack
    movem.l 4(%sp),%a0-%a1          ;// a0 = src, // a1 = dst

lz4w_unpack_a:
    movem.l  a1-a4, -(sp)           ;// save dst for lz4w_unpack_a

    lea  .jump_table(pc), a3        ;// for LZ4W_NEXT macro
    LZ4W_NEXT

.jump_table:
  .long  .lit0_mat0, .lit0_mat1, .lit0_mat2, .lit0_mat3, .lit0_mat4, .lit0_mat5, .lit0_mat6, .lit0_mat7, .lit0_mat8, .lit0_mat9, .lit0_matA, .lit0_matB, .lit0_matC, .lit0_matD, .lit0_matE, .lit0_matF
  .long  .lit1_mat0, .lit1_mat1, .lit1_mat2, .lit1_mat3, .lit1_mat4, .lit1_mat5, .lit1_mat6, .lit1_mat7, .lit1_mat8, .lit1_mat9, .lit1_matA, .lit1_matB, .lit1_matC, .lit1_matD, .lit1_matE, .lit1_matF
  .long  .lit2_mat0, .lit2_mat1, .lit2_mat2, .lit2_mat3, .lit2_mat4, .lit2_mat5, .lit2_mat6, .lit2_mat7, .lit2_mat8, .lit2_mat9, .lit2_matA, .lit2_matB, .lit2_matC, .lit2_matD, .lit2_matE, .lit2_matF
  .long  .lit3_mat0, .lit3_mat1, .lit3_mat2, .lit3_mat3, .lit3_mat4, .lit3_mat5, .lit3_mat6, .lit3_mat7, .lit3_mat8, .lit3_mat9, .lit3_matA, .lit3_matB, .lit3_matC, .lit3_matD, .lit3_matE, .lit3_matF
  .long  .lit4_mat0, .lit4_mat1, .lit4_mat2, .lit4_mat3, .lit4_mat4, .lit4_mat5, .lit4_mat6, .lit4_mat7, .lit4_mat8, .lit4_mat9, .lit4_matA, .lit4_matB, .lit4_matC, .lit4_matD, .lit4_matE, .lit4_matF
  .long  .lit5_mat0, .lit5_mat1, .lit5_mat2, .lit5_mat3, .lit5_mat4, .lit5_mat5, .lit5_mat6, .lit5_mat7, .lit5_mat8, .lit5_mat9, .lit5_matA, .lit5_matB, .lit5_matC, .lit5_matD, .lit5_matE, .lit5_matF
  .long  .lit6_mat0, .lit6_mat1, .lit6_mat2, .lit6_mat3, .lit6_mat4, .lit6_mat5, .lit6_mat6, .lit6_mat7, .lit6_mat8, .lit6_mat9, .lit6_matA, .lit6_matB, .lit6_matC, .lit6_matD, .lit6_matE, .lit6_matF
  .long  .lit7_mat0, .lit7_mat1, .lit7_mat2, .lit7_mat3, .lit7_mat4, .lit7_mat5, .lit7_mat6, .lit7_mat7, .lit7_mat8, .lit7_mat9, .lit7_matA, .lit7_matB, .lit7_matC, .lit7_matD, .lit7_matE, .lit7_matF
  .long  .lit8_mat0, .lit8_mat1, .lit8_mat2, .lit8_mat3, .lit8_mat4, .lit8_mat5, .lit8_mat6, .lit8_mat7, .lit8_mat8, .lit8_mat9, .lit8_matA, .lit8_matB, .lit8_matC, .lit8_matD, .lit8_matE, .lit8_matF
  .long  .lit9_mat0, .lit9_mat1, .lit9_mat2, .lit9_mat3, .lit9_mat4, .lit9_mat5, .lit9_mat6, .lit9_mat7, .lit9_mat8, .lit9_mat9, .lit9_matA, .lit9_matB, .lit9_matC, .lit9_matD, .lit9_matE, .lit9_matF
  .long  .litA_mat0, .litA_mat1, .litA_mat2, .litA_mat3, .litA_mat4, .litA_mat5, .litA_mat6, .litA_mat7, .litA_mat8, .litA_mat9, .litA_matA, .litA_matB, .litA_matC, .litA_matD, .litA_matE, .litA_matF
  .long  .litB_mat0, .litB_mat1, .litB_mat2, .litB_mat3, .litB_mat4, .litB_mat5, .litB_mat6, .litB_mat7, .litB_mat8, .litB_mat9, .litB_matA, .litB_matB, .litB_matC, .litB_matD, .litB_matE, .litB_matF
  .long  .litC_mat0, .litC_mat1, .litC_mat2, .litC_mat3, .litC_mat4, .litC_mat5, .litC_mat6, .litC_mat7, .litC_mat8, .litC_mat9, .litC_matA, .litC_matB, .litC_matC, .litC_matD, .litC_matE, .litC_matF
  .long  .litD_mat0, .litD_mat1, .litD_mat2, .litD_mat3, .litD_mat4, .litD_mat5, .litD_mat6, .litD_mat7, .litD_mat8, .litD_mat9, .litD_matA, .litD_matB, .litD_matC, .litD_matD, .litD_matE, .litD_matF
  .long  .litE_mat0, .litE_mat1, .litE_mat2, .litE_mat3, .litE_mat4, .litE_mat5, .litE_mat6, .litE_mat7, .litE_mat8, .litE_mat9, .litE_matA, .litE_matB, .litE_matC, .litE_matD, .litE_matE, .litE_matF
  .long  .litF_mat0, .litF_mat1, .litF_mat2, .litF_mat3, .litF_mat4, .litF_mat5, .litF_mat6, .litF_mat7, .litF_mat8, .litF_mat9, .litF_matA, .litF_matB, .litF_matC, .litF_matD, .litF_matE, .litF_matF


  .rept  127
    move.l  (a2)+, (a1)+
  .endr
.lmr_len_01:
    move.l  (a2)+, (a1)+
    move.w  (a2)+, (a1)+
    LZ4W_NEXT

  .rept  127
    move.l  (a2)+, (a1)+
  .endr
.lmr_len_00:
    move.l  (a2)+, (a1)+
    LZ4W_NEXT

  .rept  255
    move.w  (a2)+, (a1)+
  .endr
.lm_len_00:
    move.w  (a2)+, (a1)+
    move.w  (a2)+, (a1)+
    ;// .next was moved it here for .s branching range
    ;// Additionally, all branches to .next have d1 already cleared.
    ;// The easiest way to take advantage of that, is to inline the macro..
    moveq  #0, d1
.next:
    moveq  #0, d0
    move.b  (a0)+, d0               ;// d0 = literal & match length
    move.b  (a0)+, d1               ;// d1 = match offset

    add.w  d0, d0
    add.w  d0, d0
    move.l  (a3,d0.w), a4
    jmp  (a4)

.litE_mat0:  move.l  (a0)+, (a1)+
.litC_mat0:  move.l  (a0)+, (a1)+
.litA_mat0:  move.l  (a0)+, (a1)+
.lit8_mat0:  move.l  (a0)+, (a1)+
.lit6_mat0:  move.l  (a0)+, (a1)+
.lit4_mat0:  move.l  (a0)+, (a1)+
.lit2_mat0:  move.l  (a0)+, (a1)+
    add.w  d1, d1                   ;// len = len * 2, match offset null ?
    beq.s  .next                    ;// not a long match

.long_match_1:
    move.w  (a0)+, d0               ;// get long offset (already negated)

    add.w  d0, d0                   ;// bit 15 contains ROM source info
    bcs.s  .lm_rom

    lea  -2(a1,d0.w), a2            ;// a2 = dst - (match offset + 2)
    neg.w  d1
    lea  .lm_len_00(pc,d1.w), a4
    jmp  (a4)

.litF_mat0:  move.l  (a0)+, (a1)+
.litD_mat0:  move.l  (a0)+, (a1)+
.litB_mat0:  move.l  (a0)+, (a1)+
.lit9_mat0:  move.l  (a0)+, (a1)+
.lit7_mat0:  move.l  (a0)+, (a1)+
.lit5_mat0:  move.l  (a0)+, (a1)+
.lit3_mat0:  move.l  (a0)+, (a1)+
.lit1_mat0:  move.w  (a0)+, (a1)+
    add.w  d1, d1                   ;// len = len * 2, match offset null ?
    beq.s  .next                    ;// not a long match

.long_match_2:
    move.w  (a0)+, d0               ;// get long offset (already negated)

    add.w  d0, d0                   ;// bit 15 contains ROM source info
    bcs.s  .lm_rom

    lea  -2(a1,d0.w), a2            ;// a2 = dst - (match offset + 2)
    neg.w  d1
    lea  .lm_len_00(pc,d1.w), a4
    jmp  (a4)

.lit0_mat0:                         ;// special case of lit=0 and mat=0
    add.w  d1, d1                   ;// len = len * 2, match offset null ?
    beq.s  .done                    ;// not a long match --> done

.long_match_3:
    move.w  (a0)+, d0               ;// get long offset (already negated)

    add.w  d0, d0                   ;// bit 15 contains ROM source info
    bcs.s  .lm_rom

    lea  -2(a1,d0.w), a2            ;// a2 = dst - (match offset + 2)
    neg.w  d1
    lea  .lm_len_00(pc,d1.w), a4
    jmp  (a4)

.lm_rom:
    add.w  d1, d1                   ;// len = len * 4
    lea  -2(a0,d0.w), a2            ;// a2 = src - (match offset + 2)
    move.l  .lmr_jump_table(pc,d1.w), a4
    jmp     (a4)

.done:
    move.w  (a0)+, d0               ;// need to copy a last byte ?
    bpl.s  .no_byte
    move.b  d0, (a1)+               ;// copy last byte
.no_byte:
    move.l  a1, d0
    sub.l   (sp)+, d0               ;// return op - dest

    movem.l (sp)+, a2-a4
    rts

.lmr_jump_table:
  .long  .lmr_len_00-0x00, .lmr_len_01-0x00, .lmr_len_00-0x02, .lmr_len_01-0x02, .lmr_len_00-0x04, .lmr_len_01-0x04, .lmr_len_00-0x06, .lmr_len_01-0x06, .lmr_len_00-0x08, .lmr_len_01-0x08, .lmr_len_00-0x0a, .lmr_len_01-0x0a, .lmr_len_00-0x0c, .lmr_len_01-0x0c, .lmr_len_00-0x0e, .lmr_len_01-0x0e
  .long  .lmr_len_00-0x10, .lmr_len_01-0x10, .lmr_len_00-0x12, .lmr_len_01-0x12, .lmr_len_00-0x14, .lmr_len_01-0x14, .lmr_len_00-0x16, .lmr_len_01-0x16, .lmr_len_00-0x18, .lmr_len_01-0x18, .lmr_len_00-0x1a, .lmr_len_01-0x1a, .lmr_len_00-0x1c, .lmr_len_01-0x1c, .lmr_len_00-0x1e, .lmr_len_01-0x1e
  .long  .lmr_len_00-0x20, .lmr_len_01-0x20, .lmr_len_00-0x22, .lmr_len_01-0x22, .lmr_len_00-0x24, .lmr_len_01-0x24, .lmr_len_00-0x26, .lmr_len_01-0x26, .lmr_len_00-0x28, .lmr_len_01-0x28, .lmr_len_00-0x2a, .lmr_len_01-0x2a, .lmr_len_00-0x2c, .lmr_len_01-0x2c, .lmr_len_00-0x2e, .lmr_len_01-0x2e
  .long  .lmr_len_00-0x30, .lmr_len_01-0x30, .lmr_len_00-0x32, .lmr_len_01-0x32, .lmr_len_00-0x34, .lmr_len_01-0x34, .lmr_len_00-0x36, .lmr_len_01-0x36, .lmr_len_00-0x38, .lmr_len_01-0x38, .lmr_len_00-0x3a, .lmr_len_01-0x3a, .lmr_len_00-0x3c, .lmr_len_01-0x3c, .lmr_len_00-0x3e, .lmr_len_01-0x3e
  .long  .lmr_len_00-0x40, .lmr_len_01-0x40, .lmr_len_00-0x42, .lmr_len_01-0x42, .lmr_len_00-0x44, .lmr_len_01-0x44, .lmr_len_00-0x46, .lmr_len_01-0x46, .lmr_len_00-0x48, .lmr_len_01-0x48, .lmr_len_00-0x4a, .lmr_len_01-0x4a, .lmr_len_00-0x4c, .lmr_len_01-0x4c, .lmr_len_00-0x4e, .lmr_len_01-0x4e
  .long  .lmr_len_00-0x50, .lmr_len_01-0x50, .lmr_len_00-0x52, .lmr_len_01-0x52, .lmr_len_00-0x54, .lmr_len_01-0x54, .lmr_len_00-0x56, .lmr_len_01-0x56, .lmr_len_00-0x58, .lmr_len_01-0x58, .lmr_len_00-0x5a, .lmr_len_01-0x5a, .lmr_len_00-0x5c, .lmr_len_01-0x5c, .lmr_len_00-0x5e, .lmr_len_01-0x5e
  .long  .lmr_len_00-0x60, .lmr_len_01-0x60, .lmr_len_00-0x62, .lmr_len_01-0x62, .lmr_len_00-0x64, .lmr_len_01-0x64, .lmr_len_00-0x66, .lmr_len_01-0x66, .lmr_len_00-0x68, .lmr_len_01-0x68, .lmr_len_00-0x6a, .lmr_len_01-0x6a, .lmr_len_00-0x6c, .lmr_len_01-0x6c, .lmr_len_00-0x6e, .lmr_len_01-0x6e
  .long  .lmr_len_00-0x70, .lmr_len_01-0x70, .lmr_len_00-0x72, .lmr_len_01-0x72, .lmr_len_00-0x74, .lmr_len_01-0x74, .lmr_len_00-0x76, .lmr_len_01-0x76, .lmr_len_00-0x78, .lmr_len_01-0x78, .lmr_len_00-0x7a, .lmr_len_01-0x7a, .lmr_len_00-0x7c, .lmr_len_01-0x7c, .lmr_len_00-0x7e, .lmr_len_01-0x7e
  .long  .lmr_len_00-0x80, .lmr_len_01-0x80, .lmr_len_00-0x82, .lmr_len_01-0x82, .lmr_len_00-0x84, .lmr_len_01-0x84, .lmr_len_00-0x86, .lmr_len_01-0x86, .lmr_len_00-0x88, .lmr_len_01-0x88, .lmr_len_00-0x8a, .lmr_len_01-0x8a, .lmr_len_00-0x8c, .lmr_len_01-0x8c, .lmr_len_00-0x8e, .lmr_len_01-0x8e
  .long  .lmr_len_00-0x90, .lmr_len_01-0x90, .lmr_len_00-0x92, .lmr_len_01-0x92, .lmr_len_00-0x94, .lmr_len_01-0x94, .lmr_len_00-0x96, .lmr_len_01-0x96, .lmr_len_00-0x98, .lmr_len_01-0x98, .lmr_len_00-0x9a, .lmr_len_01-0x9a, .lmr_len_00-0x9c, .lmr_len_01-0x9c, .lmr_len_00-0x9e, .lmr_len_01-0x9e
  .long  .lmr_len_00-0xa0, .lmr_len_01-0xa0, .lmr_len_00-0xa2, .lmr_len_01-0xa2, .lmr_len_00-0xa4, .lmr_len_01-0xa4, .lmr_len_00-0xa6, .lmr_len_01-0xa6, .lmr_len_00-0xa8, .lmr_len_01-0xa8, .lmr_len_00-0xaa, .lmr_len_01-0xaa, .lmr_len_00-0xac, .lmr_len_01-0xac, .lmr_len_00-0xae, .lmr_len_01-0xae
  .long  .lmr_len_00-0xb0, .lmr_len_01-0xb0, .lmr_len_00-0xb2, .lmr_len_01-0xb2, .lmr_len_00-0xb4, .lmr_len_01-0xb4, .lmr_len_00-0xb6, .lmr_len_01-0xb6, .lmr_len_00-0xb8, .lmr_len_01-0xb8, .lmr_len_00-0xba, .lmr_len_01-0xba, .lmr_len_00-0xbc, .lmr_len_01-0xbc, .lmr_len_00-0xbe, .lmr_len_01-0xbe
  .long  .lmr_len_00-0xc0, .lmr_len_01-0xc0, .lmr_len_00-0xc2, .lmr_len_01-0xc2, .lmr_len_00-0xc4, .lmr_len_01-0xc4, .lmr_len_00-0xc6, .lmr_len_01-0xc6, .lmr_len_00-0xc8, .lmr_len_01-0xc8, .lmr_len_00-0xca, .lmr_len_01-0xca, .lmr_len_00-0xcc, .lmr_len_01-0xcc, .lmr_len_00-0xce, .lmr_len_01-0xce
  .long  .lmr_len_00-0xd0, .lmr_len_01-0xd0, .lmr_len_00-0xd2, .lmr_len_01-0xd2, .lmr_len_00-0xd4, .lmr_len_01-0xd4, .lmr_len_00-0xd6, .lmr_len_01-0xd6, .lmr_len_00-0xd8, .lmr_len_01-0xd8, .lmr_len_00-0xda, .lmr_len_01-0xda, .lmr_len_00-0xdc, .lmr_len_01-0xdc, .lmr_len_00-0xde, .lmr_len_01-0xde
  .long  .lmr_len_00-0xe0, .lmr_len_01-0xe0, .lmr_len_00-0xe2, .lmr_len_01-0xe2, .lmr_len_00-0xe4, .lmr_len_01-0xe4, .lmr_len_00-0xe6, .lmr_len_01-0xe6, .lmr_len_00-0xe8, .lmr_len_01-0xe8, .lmr_len_00-0xea, .lmr_len_01-0xea, .lmr_len_00-0xec, .lmr_len_01-0xec, .lmr_len_00-0xee, .lmr_len_01-0xee
  .long  .lmr_len_00-0xf0, .lmr_len_01-0xf0, .lmr_len_00-0xf2, .lmr_len_01-0xf2, .lmr_len_00-0xf4, .lmr_len_01-0xf4, .lmr_len_00-0xf6, .lmr_len_01-0xf6, .lmr_len_00-0xf8, .lmr_len_01-0xf8, .lmr_len_00-0xfa, .lmr_len_01-0xfa, .lmr_len_00-0xfc, .lmr_len_01-0xfc, .lmr_len_00-0xfe, .lmr_len_01-0xfe


  .macro  COPY_MATCH  count
    add.w  d1, d1
    neg.w  d1
    lea  -2(a1,d1.w), a2            ;// a2 = dst - ((match offset + 1) * 2)

  .rept  ((\count)+1)
    move.w  (a2)+, (a1)+
  .endr
    LZ4W_NEXT
  .endm

.litE_mat1:  move.l  (a0)+, (a1)+
.litC_mat1:  move.l  (a0)+, (a1)+
.litA_mat1:  move.l  (a0)+, (a1)+
.lit8_mat1:  move.l  (a0)+, (a1)+
.lit6_mat1:  move.l  (a0)+, (a1)+
.lit4_mat1:  move.l  (a0)+, (a1)+
.lit2_mat1:  move.l  (a0)+, (a1)+
.lit0_mat1:
    COPY_MATCH 1

.litF_mat1:  move.l  (a0)+, (a1)+
.litD_mat1:  move.l  (a0)+, (a1)+
.litB_mat1:  move.l  (a0)+, (a1)+
.lit9_mat1:  move.l  (a0)+, (a1)+
.lit7_mat1:  move.l  (a0)+, (a1)+
.lit5_mat1:  move.l  (a0)+, (a1)+
.lit3_mat1:  move.l  (a0)+, (a1)+
.lit1_mat1:  move.w  (a0)+, (a1)+
    COPY_MATCH 1

.litE_mat2:  move.l  (a0)+, (a1)+
.litC_mat2:  move.l  (a0)+, (a1)+
.litA_mat2:  move.l  (a0)+, (a1)+
.lit8_mat2:  move.l  (a0)+, (a1)+
.lit6_mat2:  move.l  (a0)+, (a1)+
.lit4_mat2:  move.l  (a0)+, (a1)+
.lit2_mat2:  move.l  (a0)+, (a1)+
.lit0_mat2:
    COPY_MATCH 2

.litF_mat2:  move.l  (a0)+, (a1)+
.litD_mat2:  move.l  (a0)+, (a1)+
.litB_mat2:  move.l  (a0)+, (a1)+
.lit9_mat2:  move.l  (a0)+, (a1)+
.lit7_mat2:  move.l  (a0)+, (a1)+
.lit5_mat2:  move.l  (a0)+, (a1)+
.lit3_mat2:  move.l  (a0)+, (a1)+
.lit1_mat2:  move.w  (a0)+, (a1)+
    COPY_MATCH 2

.litE_mat3:  move.l  (a0)+, (a1)+
.litC_mat3:  move.l  (a0)+, (a1)+
.litA_mat3:  move.l  (a0)+, (a1)+
.lit8_mat3:  move.l  (a0)+, (a1)+
.lit6_mat3:  move.l  (a0)+, (a1)+
.lit4_mat3:  move.l  (a0)+, (a1)+
.lit2_mat3:  move.l  (a0)+, (a1)+
.lit0_mat3:
    COPY_MATCH 3

.litF_mat3:  move.l  (a0)+, (a1)+
.litD_mat3:  move.l  (a0)+, (a1)+
.litB_mat3:  move.l  (a0)+, (a1)+
.lit9_mat3:  move.l  (a0)+, (a1)+
.lit7_mat3:  move.l  (a0)+, (a1)+
.lit5_mat3:  move.l  (a0)+, (a1)+
.lit3_mat3:  move.l  (a0)+, (a1)+
.lit1_mat3:  move.w  (a0)+, (a1)+
    COPY_MATCH 3

.litE_mat4:  move.l  (a0)+, (a1)+
.litC_mat4:  move.l  (a0)+, (a1)+
.litA_mat4:  move.l  (a0)+, (a1)+
.lit8_mat4:  move.l  (a0)+, (a1)+
.lit6_mat4:  move.l  (a0)+, (a1)+
.lit4_mat4:  move.l  (a0)+, (a1)+
.lit2_mat4:  move.l  (a0)+, (a1)+
.lit0_mat4:
    COPY_MATCH 4

.litF_mat4:  move.l  (a0)+, (a1)+
.litD_mat4:  move.l  (a0)+, (a1)+
.litB_mat4:  move.l  (a0)+, (a1)+
.lit9_mat4:  move.l  (a0)+, (a1)+
.lit7_mat4:  move.l  (a0)+, (a1)+
.lit5_mat4:  move.l  (a0)+, (a1)+
.lit3_mat4:  move.l  (a0)+, (a1)+
.lit1_mat4:  move.w  (a0)+, (a1)+
    COPY_MATCH 4

.litE_mat5:  move.l  (a0)+, (a1)+
.litC_mat5:  move.l  (a0)+, (a1)+
.litA_mat5:  move.l  (a0)+, (a1)+
.lit8_mat5:  move.l  (a0)+, (a1)+
.lit6_mat5:  move.l  (a0)+, (a1)+
.lit4_mat5:  move.l  (a0)+, (a1)+
.lit2_mat5:  move.l  (a0)+, (a1)+
.lit0_mat5:
    COPY_MATCH 5

.litF_mat5:  move.l  (a0)+, (a1)+
.litD_mat5:  move.l  (a0)+, (a1)+
.litB_mat5:  move.l  (a0)+, (a1)+
.lit9_mat5:  move.l  (a0)+, (a1)+
.lit7_mat5:  move.l  (a0)+, (a1)+
.lit5_mat5:  move.l  (a0)+, (a1)+
.lit3_mat5:  move.l  (a0)+, (a1)+
.lit1_mat5:  move.w  (a0)+, (a1)+
    COPY_MATCH 5

.litE_mat6:  move.l  (a0)+, (a1)+
.litC_mat6:  move.l  (a0)+, (a1)+
.litA_mat6:  move.l  (a0)+, (a1)+
.lit8_mat6:  move.l  (a0)+, (a1)+
.lit6_mat6:  move.l  (a0)+, (a1)+
.lit4_mat6:  move.l  (a0)+, (a1)+
.lit2_mat6:  move.l  (a0)+, (a1)+
.lit0_mat6:
    COPY_MATCH 6

.litF_mat6:  move.l  (a0)+, (a1)+
.litD_mat6:  move.l  (a0)+, (a1)+
.litB_mat6:  move.l  (a0)+, (a1)+
.lit9_mat6:  move.l  (a0)+, (a1)+
.lit7_mat6:  move.l  (a0)+, (a1)+
.lit5_mat6:  move.l  (a0)+, (a1)+
.lit3_mat6:  move.l  (a0)+, (a1)+
.lit1_mat6:  move.w  (a0)+, (a1)+
    COPY_MATCH 6

.litE_mat7:  move.l  (a0)+, (a1)+
.litC_mat7:  move.l  (a0)+, (a1)+
.litA_mat7:  move.l  (a0)+, (a1)+
.lit8_mat7:  move.l  (a0)+, (a1)+
.lit6_mat7:  move.l  (a0)+, (a1)+
.lit4_mat7:  move.l  (a0)+, (a1)+
.lit2_mat7:  move.l  (a0)+, (a1)+
.lit0_mat7:
    COPY_MATCH 7

.litF_mat7:  move.l  (a0)+, (a1)+
.litD_mat7:  move.l  (a0)+, (a1)+
.litB_mat7:  move.l  (a0)+, (a1)+
.lit9_mat7:  move.l  (a0)+, (a1)+
.lit7_mat7:  move.l  (a0)+, (a1)+
.lit5_mat7:  move.l  (a0)+, (a1)+
.lit3_mat7:  move.l  (a0)+, (a1)+
.lit1_mat7:  move.w  (a0)+, (a1)+
    COPY_MATCH 7

.litE_mat8:  move.l  (a0)+, (a1)+
.litC_mat8:  move.l  (a0)+, (a1)+
.litA_mat8:  move.l  (a0)+, (a1)+
.lit8_mat8:  move.l  (a0)+, (a1)+
.lit6_mat8:  move.l  (a0)+, (a1)+
.lit4_mat8:  move.l  (a0)+, (a1)+
.lit2_mat8:  move.l  (a0)+, (a1)+
.lit0_mat8:
    COPY_MATCH 8

.litF_mat8:  move.l  (a0)+, (a1)+
.litD_mat8:  move.l  (a0)+, (a1)+
.litB_mat8:  move.l  (a0)+, (a1)+
.lit9_mat8:  move.l  (a0)+, (a1)+
.lit7_mat8:  move.l  (a0)+, (a1)+
.lit5_mat8:  move.l  (a0)+, (a1)+
.lit3_mat8:  move.l  (a0)+, (a1)+
.lit1_mat8:  move.w  (a0)+, (a1)+
    COPY_MATCH 8

.litE_mat9:  move.l  (a0)+, (a1)+
.litC_mat9:  move.l  (a0)+, (a1)+
.litA_mat9:  move.l  (a0)+, (a1)+
.lit8_mat9:  move.l  (a0)+, (a1)+
.lit6_mat9:  move.l  (a0)+, (a1)+
.lit4_mat9:  move.l  (a0)+, (a1)+
.lit2_mat9:  move.l  (a0)+, (a1)+
.lit0_mat9:
    COPY_MATCH 9

.litF_mat9:  move.l  (a0)+, (a1)+
.litD_mat9:  move.l  (a0)+, (a1)+
.litB_mat9:  move.l  (a0)+, (a1)+
.lit9_mat9:  move.l  (a0)+, (a1)+
.lit7_mat9:  move.l  (a0)+, (a1)+
.lit5_mat9:  move.l  (a0)+, (a1)+
.lit3_mat9:  move.l  (a0)+, (a1)+
.lit1_mat9:  move.w  (a0)+, (a1)+
    COPY_MATCH 9

.litE_matA:  move.l  (a0)+, (a1)+
.litC_matA:  move.l  (a0)+, (a1)+
.litA_matA:  move.l  (a0)+, (a1)+
.lit8_matA:  move.l  (a0)+, (a1)+
.lit6_matA:  move.l  (a0)+, (a1)+
.lit4_matA:  move.l  (a0)+, (a1)+
.lit2_matA:  move.l  (a0)+, (a1)+
.lit0_matA:
    COPY_MATCH 10

.litF_matA:  move.l  (a0)+, (a1)+
.litD_matA:  move.l  (a0)+, (a1)+
.litB_matA:  move.l  (a0)+, (a1)+
.lit9_matA:  move.l  (a0)+, (a1)+
.lit7_matA:  move.l  (a0)+, (a1)+
.lit5_matA:  move.l  (a0)+, (a1)+
.lit3_matA:  move.l  (a0)+, (a1)+
.lit1_matA:  move.w  (a0)+, (a1)+
    COPY_MATCH 10

.litE_matB:  move.l  (a0)+, (a1)+
.litC_matB:  move.l  (a0)+, (a1)+
.litA_matB:  move.l  (a0)+, (a1)+
.lit8_matB:  move.l  (a0)+, (a1)+
.lit6_matB:  move.l  (a0)+, (a1)+
.lit4_matB:  move.l  (a0)+, (a1)+
.lit2_matB:  move.l  (a0)+, (a1)+
.lit0_matB:
    COPY_MATCH 11

.litF_matB:  move.l  (a0)+, (a1)+
.litD_matB:  move.l  (a0)+, (a1)+
.litB_matB:  move.l  (a0)+, (a1)+
.lit9_matB:  move.l  (a0)+, (a1)+
.lit7_matB:  move.l  (a0)+, (a1)+
.lit5_matB:  move.l  (a0)+, (a1)+
.lit3_matB:  move.l  (a0)+, (a1)+
.lit1_matB:  move.w  (a0)+, (a1)+
    COPY_MATCH 11

.litE_matC:  move.l  (a0)+, (a1)+
.litC_matC:  move.l  (a0)+, (a1)+
.litA_matC:  move.l  (a0)+, (a1)+
.lit8_matC:  move.l  (a0)+, (a1)+
.lit6_matC:  move.l  (a0)+, (a1)+
.lit4_matC:  move.l  (a0)+, (a1)+
.lit2_matC:  move.l  (a0)+, (a1)+
.lit0_matC:
    COPY_MATCH 12

.litF_matC:  move.l  (a0)+, (a1)+
.litD_matC:  move.l  (a0)+, (a1)+
.litB_matC:  move.l  (a0)+, (a1)+
.lit9_matC:  move.l  (a0)+, (a1)+
.lit7_matC:  move.l  (a0)+, (a1)+
.lit5_matC:  move.l  (a0)+, (a1)+
.lit3_matC:  move.l  (a0)+, (a1)+
.lit1_matC:  move.w  (a0)+, (a1)+
    COPY_MATCH 12

.litE_matD:  move.l  (a0)+, (a1)+
.litC_matD:  move.l  (a0)+, (a1)+
.litA_matD:  move.l  (a0)+, (a1)+
.lit8_matD:  move.l  (a0)+, (a1)+
.lit6_matD:  move.l  (a0)+, (a1)+
.lit4_matD:  move.l  (a0)+, (a1)+
.lit2_matD:  move.l  (a0)+, (a1)+
.lit0_matD:
    COPY_MATCH 13

.litF_matD:  move.l  (a0)+, (a1)+
.litD_matD:  move.l  (a0)+, (a1)+
.litB_matD:  move.l  (a0)+, (a1)+
.lit9_matD:  move.l  (a0)+, (a1)+
.lit7_matD:  move.l  (a0)+, (a1)+
.lit5_matD:  move.l  (a0)+, (a1)+
.lit3_matD:  move.l  (a0)+, (a1)+
.lit1_matD:  move.w  (a0)+, (a1)+
    COPY_MATCH 13

.litE_matE:  move.l  (a0)+, (a1)+
.litC_matE:  move.l  (a0)+, (a1)+
.litA_matE:  move.l  (a0)+, (a1)+
.lit8_matE:  move.l  (a0)+, (a1)+
.lit6_matE:  move.l  (a0)+, (a1)+
.lit4_matE:  move.l  (a0)+, (a1)+
.lit2_matE:  move.l  (a0)+, (a1)+
.lit0_matE:
    COPY_MATCH 14

.litF_matE:  move.l  (a0)+, (a1)+
.litD_matE:  move.l  (a0)+, (a1)+
.litB_matE:  move.l  (a0)+, (a1)+
.lit9_matE:  move.l  (a0)+, (a1)+
.lit7_matE:  move.l  (a0)+, (a1)+
.lit5_matE:  move.l  (a0)+, (a1)+
.lit3_matE:  move.l  (a0)+, (a1)+
.lit1_matE:  move.w  (a0)+, (a1)+
    COPY_MATCH 14

.litE_matF:  move.l  (a0)+, (a1)+
.litC_matF:  move.l  (a0)+, (a1)+
.litA_matF:  move.l  (a0)+, (a1)+
.lit8_matF:  move.l  (a0)+, (a1)+
.lit6_matF:  move.l  (a0)+, (a1)+
.lit4_matF:  move.l  (a0)+, (a1)+
.lit2_matF:  move.l  (a0)+, (a1)+
.lit0_matF:
    COPY_MATCH 15

.litF_matF:  move.l  (a0)+, (a1)+
.litD_matF:  move.l  (a0)+, (a1)+
.litB_matF:  move.l  (a0)+, (a1)+
.lit9_matF:  move.l  (a0)+, (a1)+
.lit7_matF:  move.l  (a0)+, (a1)+
.lit5_matF:  move.l  (a0)+, (a1)+
.lit3_matF:  move.l  (a0)+, (a1)+
.lit1_matF:  move.w  (a0)+, (a1)+
    COPY_MATCH 15
