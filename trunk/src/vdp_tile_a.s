	.globl	VDP_loadBMPTileData_A
	.type	VDP_loadBMPTileData_A, @function
VDP_loadBMPTileData_A:

	movm.l #0x3e00,-(%sp)
	move.l 28(%sp),%d2                  | d2 = index
	move.l 36(%sp),%d3                  | d3 = h
	jbeq .L17
	move.w 34(%sp),%d4                  | d4 = w
	jbeq .L17

	pea 2.w
	jbsr VDP_setAutoInc

	move.l #0xC00000,%a1                | VDP data port

 *  ((0x4000 + ((adr) & 0x3FFF)) << 16) + (((adr) >> 14) | 0x00)

	mov.l %d2,%d0                       | VDP Ctrl port
	andi.w #0x1FF,%d2                   | convert address for VDP Ctrl command
	ori.w #0x200,%d2
	lsl.l #5,%d2
	moveq #9,%d1
    lsr.w %d1,%d0
    swap %d2
    or.l %d2,%d0
    move.l %d0,0xC00004

*	andi.w #0x7FF, %d2
*	lsl.l #7,%d2
*	lea vramwrite_tab,%a0
*	move.l (%a0,%d2.l),0xC00004

	moveq #0,%d5
	move.w %d4,%d5
	lsl.l #2,%d5                        | d5 = w
	move.l %d5,%d0
	lsl.l #3,%d0
	sub.l %d5,%d0                       | d0 = w * 7

	move.l 28(%sp),%a0                  | a0 = src
	addq.l #4,%sp
	subq.w #1,%d3
	subq.w #1,%d4

	move.w %d3,%d6

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
	dbra %d6,.L15

.L17:
	movm.l (%sp)+,#0x7c
	rts
