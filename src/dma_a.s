
#include "asm_mac.i"

func flushQueue
	move.w 6(%sp),%d0
    jeq     .fq_end

	move.l dmaQueues,%a0
	move.l #0xC00004,%a1

	subq.w #1,%d0           | prepare for dbra

.fq_loop:
	move.l (%a0)+,(%a1)
	move.l (%a0)+,(%a1)
	move.l (%a0)+,(%a1)
	move.w (%a0)+,(%a1)
	move.w (%a0)+,(%a1)     | important to use word write for command triggering DMA (see SEGA notes)

	dbra %d0,.fq_loop

.fq_end:
	rts

func flushQueueSafe
	move.w 6(%sp),%d0
    jeq     .fqs_end

	move.w 10(%sp),%d1      | z80 restore

	move.w %d2,-(%sp)       | save regs
	move.l %a2,-(%sp)

	move.l dmaQueues,%a0
	move.l #0xC00004,%a1
	move.l #0xA11100,%a2
	move.w #0x0100,%d2      | z80 off

	subq.w #1,%d0           | prepare for dbra

.fqs_loop:
	move.l (%a0)+,(%a1)
	move.l (%a0)+,(%a1)
	move.l (%a0)+,(%a1)
	move.w (%a0)+,(%a1)

	move.w %d2,(%a2)        | DISABLE Z80
	move.w %d1,(%a2)        | RESTORE Z80

	move.w (%a0)+,(%a1)     | important to use word write for command triggering DMA (see SEGA notes)

	dbra %d0,.fqs_loop

	move.l (%sp)+,%a2       | restore regs
	move.w (%sp)+,%d2

.fqs_end:
	rts
