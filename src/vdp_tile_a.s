	.align	2
	.globl	VDP_loadTileData
	.type	VDP_loadTileData, @function
VDP_loadTileData:
	movm.l #0x3e00,-(%sp)
	move.l 24(%sp),%d6
	move.l 32(%sp),%d3
	move.w 30(%sp),%d0

	lsl.l #5,%d0
	move.l %d0,%d2
	and.l #2097120,%d2
	tst.b 39(%sp)
	jbeq .L2

	jbsr VDP_waitDMACompletion
	move.l %d3,%d0
	lsl.l #5,%d0
	and.l #65504,%d0
	move.l %d0,-(%sp)
	and.l #65504,%d2
	move.l %d2,-(%sp)
	move.l %d6,-(%sp)
	clr.l -(%sp)
	jbsr VDP_doDMA
	lea (16,%sp),%sp
	jbra .L1

	.align	2
.L2:
	pea 2.w
	jbsr VDP_setAutoInc
	move.l #12582912,%a1
	move.l %d2,%d0
	lsl.l #2,%d0
	and.l #262016,%d0
	lea vramwrite_tab,%a0
	move.l (%a0,%d0.l),12582916
	move.l %d6,%a0
	addq.l #4,%sp
	move.w %d3,%d1
	subq.w #1,%d1
	cmp.w #-1,%d1
	jbeq .L1

	.align	2
.L6:
	move.l (%a0)+,(%a1)
	move.l (%a0)+,(%a1)
	move.l (%a0)+,(%a1)
	move.l (%a0)+,(%a1)
	move.l (%a0)+,(%a1)
	move.l (%a0)+,(%a1)
	move.l (%a0)+,(%a1)
	move.l (%a0)+,(%a1)
	dbra %d1,.L6

.L1:
	movm.l (%sp)+,#0x7c
	rts

	.align	2
	.globl	VDP_loadBMPTileData
	.type	VDP_loadBMPTileData, @function
VDP_loadBMPTileData:

	movm.l #0x3c00,-(%sp)
	move.w 34(%sp),%d3                  | d3 = h
	jbeq .L17
	move.w 30(%sp),%d4                  | d4 = w
	jbeq .L17

	pea 2.w
	jbsr VDP_setAutoInc
	addq.l #4,%sp

	move.l #0xC00000,%a1                | VDP data port

	movq  #0, %d0
	move.w 26(%sp),%d0                  | d0 = index
	andi.w #0x7FF, %d0
	lsl.l #7,%d0
	lea vramwrite_tab,%a0
	move.l (%a0,%d0.l),0xC00004         | set destination address in VDP Ctrl command

    moveq #0,%d5
	move.w 38(%sp),%d5                  | d5 = bmp_w
	lsl.l #2,%d5                        | d5 = bmp_w adjusted for u32*
	move.l %d5,%d0
	lsl.l #3,%d0
	sub.l %d5,%d0                       | d0 = (bmp_w * 7) adjusted for u32*

	move.l 20(%sp),%a0                  | a0 = src
	subq.w #1,%d3
	subq.w #1,%d4

.L15:
	move.w %d4,%d2

.L14:
	move.l (%a0),(%a1)
	add.l %d5,%a0
	move.l (%a0),(%a1)
	add.l %d5,%a0
	move.l (%a0),(%a1)
	add.l %d5,%a0
	move.l (%a0),(%a1)
	add.l %d5,%a0
	move.l (%a0),(%a1)
	add.l %d5,%a0
	move.l (%a0),(%a1)
	add.l %d5,%a0
	move.l (%a0),(%a1)
	add.l %d5,%a0
	move.l (%a0),(%a1)

	sub.l %d0,%a0
	addq.l #4,%a0
	dbra %d2,.L14

.L19:
	add.l %d0,%a0
	dbra %d3,.L15

.L17:
	movm.l (%sp)+,#0x3c
	rts
