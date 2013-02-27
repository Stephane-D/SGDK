	.globl	SRAM_readWord
	.type	SRAM_readWord, @function
    | extern u16 SRAM_readWord(u32 offset);
SRAM_readWord:
    move.l  4(%sp),%d1
    add.l   %d1,%d1
    lea     0x200001,%a0
    lea     (%a0,%d1.l),%a0
    moveq   #0,%d0
    movep.w 0(%a0),%d0
    rts

	.globl	SRAM_readLong
	.type	SRAM_readLong, @function
    | extern u32 SRAM_readLong(u32 offset);
SRAM_readLong:
    move.l  4(%sp),%d1
    add.l   %d1,%d1
    lea     0x200001,%a0
    lea     (%a0,%d1.l),%a0
    movep.l 0(%a0),%d0
    rts

	.globl	SRAM_writeWord
	.type	SRAM_writeWord, @function
    | extern void SRAM_writeWord(u32 offset, u16 val);
SRAM_writeWord:
    move.l  4(%sp),%d1
    add.l   %d1,%d1
    move.l  8(%sp),%d0              | values on stack are always long
    lea     0x200001,%a0
    lea     (%a0,%d1.l),%a0
    movep.w %d0,0(%a0)
    rts

	.globl	SRAM_writeLong
	.type	SRAM_writeLong, @function
    | extern void SRAM_writeLong(u32 offset, u32 val);
SRAM_writeLong:
    move.l  4(%sp),%d1
    add.l   %d1,%d1
    move.l  8(%sp),%d0              | values on stack are always long
    lea     0x200001,%a0
    lea     (%a0,%d1.l),%a0
    movep.l %d0,0(%a0)
    rts
