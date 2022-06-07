
#include "asm_mac.i"


/* extern Fractal_ChannelInfo* Fractal_GetMusicChannel(Fractal_ChannelType type) */
func Fractal_GetMusicChannel
	move.l  4(%sp),%d0			/* load channel type from stack */

	cmp.b	#ctFM6,%d0
	bhi.s	notfm
	and.w	#7,%d0
	add.b	%d0,%d0
	add.b	%d0,%d0				/* type *= 4 */
	move.l	fmtable(%pc,%d0.w),%d0		/* load return value */
	rts

fmtable:
	dc.l dChannelInfoFM1, dChannelInfoFM2, dChannelInfoFM3, 0, dChannelInfoFM4, dChannelInfoFM5, dChannelInfoFM6

notfm:
	cmp.b	#ctPSG1,%d0
	blo.s	notpsg
	and.w	#ctPSG4,%d0
	lsr.b	#3,%d0				/* type /= 0x10 */
	move.l	psgtable-0x10(%pc,%d0.w),%d0	/* load return value */
	rts

psgtable:
	dc.l dChannelInfoPSG1, dChannelInfoPSG2, dChannelInfoPSG3, dChannelInfoPSG4

notpsg:
	btst	#ctbFM3sm,%d0
	beq.s	notop
	and.w	#3,%d0
	add.b	%d0,%d0
	add.b	%d0,%d0				/* type *= 4 */
	move.l	optable(%pc,%d0.w),%d0		/* load return value */
	rts

optable:
	dc.l dChannelInfoFM3o1, dChannelInfoFM3o2, dChannelInfoFM3o3, dChannelInfoFM3o4

special:
	move.l	#dChannelInfoTA,%d0
	rts

invalid:
	moveq	#0,%d0
	rts

notop:
	btst	#ctbSPC,%d0
	bne.s	special
	btst	#ctbDAC,%d0
	beq.s	invalid

	cmp.b	#ctDAC1,%d0
	beq.s	dac1
	move.l	#dChannelInfoDAC2,%d0
	rts

dac1:
	move.l	#dChannelInfoDAC1,%d0
	rts


/* extern Fractal_ChannelInfo* Fractal_GetSFXChannel(Fractal_ChannelType type) */
func Fractal_GetSFXChannel
	move.l  4(%sp),%d0			/* load channel type from stack */

	cmp.b	#ctFM6,%d0
	bhi.s	notfms
	and.w	#7,%d0
	add.b	%d0,%d0
	add.b	%d0,%d0				/* type *= 4 */
	move.l	fmtables(%pc,%d0.w),%d0		/* load return value */
	rts

fmtables:
	dc.l dChannelInfoSFXFM1, dChannelInfoSFXFM2, 0, 0, dChannelInfoSFXFM4, dChannelInfoSFXFM5, 0

notfms:
	cmp.b	#ctPSG1,%d0
	blo.s	notpsgs
	and.w	#ctPSG4,%d0
	lsr.b	#3,%d0				/* type /= 0x10 */
	move.l	psgtables-0x10(%pc,%d0.w),%d0	/* load return value */
	rts

psgtables:
	dc.l dChannelInfoSFXPSG1, dChannelInfoSFXPSG2, dChannelInfoSFXPSG3, dChannelInfoSFXPSG4

invalids:
	moveq	#0,%d0
	rts

notpsgs:
	cmp.b	#ctDAC2,%d0
	bne.s	invalids
	move.l	#dChannelInfoSFXDAC2,%d0
	rts


/* extern void Fractal_ExecuteForMusicChannels(void (*func)(Fractal_ChannelInfo* channel)) */
func Fractal_ExecuteForMusicChannels
		move.l  4(%sp),%a1				/* load caller routine to a1 */

		lea	dChannelInfoFM3,%a0			/* load music channel list */
		btst	#mfSpecial,mFlags.w			/* check if FM3 special mode is enabled */
		beq.s	nospec					/* if not, branch */
		lea	dChannelInfoFM3o1,%a0			/* load music channel list (FM3 special mode) */

nospec:
		bra.w	Fractal_ExecuteChannels

/* extern void Fractal_ExecuteForSFXChannels(void (*func)(Fractal_ChannelInfo* channel)) */
func Fractal_ExecuteForSFXChannels
		move.l  4(%sp),%a1				/* load caller routine to a1 */
		lea	dChannelInfoSFXFM1,%a0			/* load sfx channel list */

Fractal_ExecNext:
		move.w	ciNext(%a0),%d0				/* load the pointer to channel */
		bne.s	Fractal_ExecNotNull			/* if not null, branch */
		rts

Fractal_ExecNotNull:
		add.w	%d0,%a0					/* add channel data offset */

Fractal_ExecuteChannels:
		pea	(%a0)
		jsr	(%a1)					/* run function */
		move.l	(%sp)+,%a0				/* safely load a0 back */
		bra.s	Fractal_ExecNext			/* go to next channel */
