
#include "asm_mac.i"

func flushQueue
	move.w 6(%sp),%d0
    jeq     .fq_end

	move.l dmaQueues,%a0
	move.l #0xC00004,%a1

	subq.w #1,%d0           // prepare for dbra

.fq_loop:
	move.l (%a0)+,(%a1)
	move.l (%a0)+,(%a1)
	move.l (%a0)+,(%a1)
	move.w (%a0)+,(%a1)
	move.w (%a0)+,(%a1)     // important to use word write for command triggering DMA (see SEGA notes)

	dbra %d0,.fq_loop

.fq_end:
	rts