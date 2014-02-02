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


    .global lzkn_unpack



# disable for legal issue
#
# u32 lzkn_unpack(u8 *src, u8 *dest);
#
# input:
# a5 - input
# a6 - output
# returns:
# a6 - end of extracted

# lzkn_unpack:
#   movem.l %a5-%a6/%d2,-(%a7)
#   movem.l 16(%a7),%a5-%a6
#   move.l  %a6,%a1                 | save destination address in a1
#
# 	addq	#2,%a5                  | skip out size
# 	lea	    (.loop).l,%a0
# 	moveq	#0,%d7
#
# .loop:
# 	dbf	    %d7, .skipmask
# 	moveq	#7,%d7
# 	move.b	(%a5)+,%d1
#
# .skipmask:
# 	lsr.w	#1,%d1
# 	bcs.s	.not_raw1
#
# .raw1:
# 	move.b	(%a5)+,(%a6)+
# 	jmp	    (%a0)
#
# .not_raw1:
# 	moveq	#0,%d0
# 	move.b	(%a5)+,%d0
# 	bmi.w	.small_copy
# 	cmpi.b	#0x1F,%d0
# 	beq.s	.end
#
# .long_copy:
# 	move.l	%d0,%d2
# 	lsl.w	#3,%d0
# 	move.b	(%a5)+,%d0
# 	andi.w	#0x1F,%d2
# 	addq.w	#2,%d2
# 	bra.s	.copy
#
# .small_copy:
# 	btst	#6,%d0
# 	bne.w	.copy_input
# 	move.l	%d0,%d2
# 	lsr.w	#4,%d2
# 	subq.w	#7,%d2
# 	andi.w	#0xF,%d0
#
# .copy:
# 	neg.w	%d0
#
# .loop1:
# 	move.b	(%a6,%d0.w),(%a6)+
# 	dbf	    %d2,.loop1
# 	jmp	    (%a0)
#
# .copy_input:
# 	subi.b	#0xB9,%d0
#
# .loop2:
# 	move.b	(%a5)+,(%a6)+
# 	dbf	    %d0, .loop2
# 	jmp	    (%a0)
#
# .end:
#   suba.l  %a6,%a1
#   move.l   %a1,%d0
#   movem.l (%a7)+,%a5-%a6/%d2
#   rts
