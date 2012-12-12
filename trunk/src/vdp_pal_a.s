	.align	2
	.globl	VDP_getPalette
	.type	VDP_getPalette, @function
VDP_getPalette:
	pea 2.w
	jbsr VDP_setAutoInc
	addq.l #4,%sp
	move.w 6(%sp),%d1               | d1 = num

	lsl.w #5,%d1                    | d1 = adr
	move.w %d1,%d0                  | d0 = adr

	and.w #0x3FFF,%d1               | d1 = adr & 0x3FFF
	rol.w #2,%d0
	andi.w #3,%d0                   | d0 = adr >> 14
	ori.w #0x20,%d0                 | d0 = (adr >> 14) | 0x20

	swap %d1                        | d1 = (adr & 0x3FFF) << 16
    move.w %d0,%d1                  | d1 = ((adr & 0x3FFF) << 16) | ((adr >> 14) | 0x20)

	move.l %d1,0xC00004             | *0xC000004 = GFX_READ_CRAM_ADDR(adr)

	move.l 8(%sp),%a0               | a0 = dest
	move.l #0xC00000,%a1            | a1 = 0xC00000

	move.l (%a1),(%a0)+             | *dest++ = *0xC00000
	move.l (%a1),(%a0)+
	move.l (%a1),(%a0)+
	move.l (%a1),(%a0)+
	move.l (%a1),(%a0)+
	move.l (%a1),(%a0)+
	move.l (%a1),(%a0)+
	move.l (%a1),(%a0)+
	rts


	.align	2
	.globl	VDP_setPalette
	.type	VDP_setPalette, @function
VDP_setPalette:
	pea 2.w
	jbsr VDP_setAutoInc
	addq.l #4,%sp
	move.w 6(%sp),%d1               | d1 = num

	lsl.w #5,%d1                    | d1 = adr
	move.w %d1,%d0                  | d0 = adr

	and.w #0x3FFF,%d1               | d1 = adr & 0x3FFF
	ori.w #0xC000,%d1               | d1 = (adr & 0x3FFF) | 0xC000
	rol.w #2,%d0
	andi.w #3,%d0                   | d0 = adr >> 14

	swap %d1
    move.w %d0,%d1                  | d1 = (((adr & 0x3FFF) | 0xC000) << 16) | (adr >> 14)

	move.l %d1,0xC00004             | *0xC000004 = GFX_WRITE_CRAM_ADDR(adr)

	move.l 8(%sp),%a0               | a0 = src
	move.l #0xC00000,%a1            | a1 = 0xC00000

	move.l (%a0)+,(%a1)             | *0xC00000 = *src++
	move.l (%a0)+,(%a1)
	move.l (%a0)+,(%a1)
	move.l (%a0)+,(%a1)
	move.l (%a0)+,(%a1)
	move.l (%a0)+,(%a1)
	move.l (%a0)+,(%a1)
	move.l (%a0)+,(%a1)
	rts
