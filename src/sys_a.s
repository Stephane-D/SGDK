	.align	2
	.globl	SYS_assertReset
	.type	SYS_assertReset, @function
SYS_assertReset:

    reset
    rts


	.align	2
	.globl	SYS_reset
	.type	SYS_reset, @function
SYS_reset:

    move   #0x2700,%sr
    move.l (0),%a7
    move.l (4),%a0
    jmp    (%a0)


	.align	2
	.globl	SYS_getInterruptMaskLevel
	.type	SYS_getInterruptMaskLevel, @function
SYS_getInterruptMaskLevel:

    move.w  %sr,%d0
    andi.w  #0x0700,%d0
    lsr.w   #8,%d0
    rts


	.align	2
	.globl	SYS_setInterruptMaskLevel
	.type	SYS_setInterruptMaskLevel, @function
SYS_setInterruptMaskLevel:

	move.w  6(%sp),%d0                      | d0 = value
	andi.w  #0x07,%d0
	ori.w   #0x20,%d0
	lsl.w   #8,%d0
	move.w  %d0,%sr
	rts


	.align	2
	.globl	SYS_getAndSetInterruptMaskLevel
	.type	SYS_getAndSetInterruptMaskLevel, @function
SYS_getAndSetInterruptMaskLevel:

	move.w  6(%sp),%d1                      | d1 = value
	andi.w  #0x07,%d1
	ori.w   #0x20,%d1
	lsl.w   #8,%d1

	move.w  %sr,%d0                         | d0 = previous SR value
	move.w  %d1,%sr                         | SR = d1 (these 2 instructions should be canonical)

    lsr.w   #8,%d0
	andi.w  #0x07,%d0                       | d0 = previous interrupt mask
	rts
