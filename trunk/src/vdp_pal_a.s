	.align	2
	.globl	VDP_getPaletteColors
	.type	VDP_getPaletteColors, @function
VDP_getPaletteColors:
	pea 2.w
	jbsr VDP_setAutoInc
	addq.l #4,%sp
	move.w 6(%sp),%d1               | d1 = index

	add.w %d1,%d1                   | d1 = adr
	move.w %d1,%d0                  | d0 = adr

	andi.w #0x3FFF,%d1              | d1 = adr & 0x3FFF
	rol.w #2,%d0
	andi.w #3,%d0                   | d0 = adr >> 14
	ori.w #0x20,%d0                 | d0 = (adr >> 14) | 0x20

	swap %d1                        | d1 = (adr & 0x3FFF) << 16
    move.w %d0,%d1                  | d1 = ((adr & 0x3FFF) << 16) | ((adr >> 14) | 0x20)

	move.l %d1,0xC00004             | *0xC000004 = GFX_READ_CRAM_ADDR(adr)

	move.l 8(%sp),%a0               | a0 = dest
	move.l #0xC00000,%a1            | a1 = 0xC00000

	move.w 14(%sp),%d1              | d1 = count
	move.w %d1,%d0
	lsr.w #4,%d0
	jeq .L11

	subq.w #1,%d0

.L13:
	move.l (%a1),(%a0)+             | *dest++ = *0xC00000
	move.l (%a1),(%a0)+
	move.l (%a1),(%a0)+
	move.l (%a1),(%a0)+
	move.l (%a1),(%a0)+
	move.l (%a1),(%a0)+
	move.l (%a1),(%a0)+
	move.l (%a1),(%a0)+
    dbra %d0,.L13

.L11:
    andi.w #15,%d1
    jeq .L12

	subq.w #1,%d1

.L14:
	move.w (%a1),(%a0)+
    dbra %d1,.L14

.L12:
	rts


	.align	2
	.globl	VDP_setPaletteColors
	.type	VDP_setPaletteColors, @function
VDP_setPaletteColors:
	pea 2.w
	jbsr VDP_setAutoInc
	addq.l #4,%sp
	move.w 6(%sp),%d1               | d1 = index

	add.w %d1,%d1                   | d1 = adr
	move.w %d1,%d0                  | d0 = adr

	andi.w #0x3FFF,%d1              | d1 = adr & 0x3FFF
	ori.w #0xC000,%d1               | d1 = (adr & 0x3FFF) | 0xC000
	rol.w #2,%d0
	andi.w #3,%d0                   | d0 = adr >> 14

	swap %d1
    move.w %d0,%d1                  | d1 = (((adr & 0x3FFF) | 0xC000) << 16) | (adr >> 14)

	move.l %d1,0xC00004             | *0xC000004 = GFX_WRITE_CRAM_ADDR(adr)

	move.l 8(%sp),%a0               | a0 = src
	move.l #0xC00000,%a1            | a1 = 0xC00000

	move.w 14(%sp),%d1              | d1 = count
	move.w %d1,%d0
	lsr.w #4,%d0
	jeq .L1

	subq.w #1,%d0

.L3:
	move.l (%a0)+,(%a1)             | *0xC00000 = *src++
	move.l (%a0)+,(%a1)
	move.l (%a0)+,(%a1)
	move.l (%a0)+,(%a1)
	move.l (%a0)+,(%a1)
	move.l (%a0)+,(%a1)
	move.l (%a0)+,(%a1)
	move.l (%a0)+,(%a1)
    dbra %d0,.L3

.L1:
    andi.w #15,%d1
    jeq .L2

	subq.w #1,%d1

.L4:
	move.w (%a0)+,(%a1)
    dbra %d1,.L4

.L2:
	rts


	.align	2
	.globl	VDP_getPalette
	.type	VDP_getPalette, @function
VDP_getPalette:
	pea 2.w
	jbsr VDP_setAutoInc
	addq.l #4,%sp
	move.w 6(%sp),%d1               | d1 = index

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
	move.w 6(%sp),%d1               | d1 = index

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
