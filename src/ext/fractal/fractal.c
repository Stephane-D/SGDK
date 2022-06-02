#include "config.h"
#include "types.h"

#include "ext/fractal/fractal.h"

#if (MODULE_FRACTAL != 0)

#ifdef __INTELLISENSE__
    #pragma diag_suppress 1118
#endif

void Fractal_Init(void (*decompressFunction)(u8* source, u8* destination)) {
	asm volatile (
		"move.l	%0,%%a3\n\t"
		"jsr	dFractalInit"
		:
		: "di" (decompressFunction)
		: "a0", "a1", "d0", "d1", "d2", "d3", "cc", "memory"
	);
}

void Fractal_Decompress(u8* source, u8* destination) {
	register u16 counter asm ("d0") = *((u16*)source);
	source += 2;

	asm volatile (
		"subq.w	#1,%0\n\t"
		"bmi.s	done\n\t"
		"next:\n\t"
		"move.b	(%1)+,(%2)+\n\t"
		"dbf	%0,next\n\t"
		"done:\n\t"
		:
		: "d" (counter), "a" (source), "a" (destination)
		: "cc", "memory"
	);
}

void Fractal_Update() {
	asm volatile (
		"jsr	dFractalSound"
		:
		:
		: "a0", "a1", "a2", "a3", "a4", "a5", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "cc", "memory"
	);
}

void Fractal_Queue(u16 sound) {
	asm volatile (
		"move.w	%0,%%d0\n\t"
		"jsr	dFractalQueue"
		:
		: "d"(sound)
		: "d0", "cc", "memory"
	);
}

void Fractal_UpdateMasterFraction() {
	asm volatile (
		"jsr	dUpdateMasterFrac"
		:
		:
		: "a0", "a1", "d0", "d1", "cc", "memory"
	);
}

void Fractal_ForceFractionUpdate() {
	asm volatile (
		"jsr	dSetFracFlag"
		:
		:
		: "a0", "a1", "d0", "d1", "cc", "memory"
	);
}

void Fractal_UpdateMasterVolume() {
	asm volatile (
		"jsr	dUpdateMasterVol"
		:
		:
		: "a0", "a1", "d0", "d1", "cc", "memory"
	);
}

void Fractal_ForceVolumeUpdate() {
	asm volatile (
		"jsr	dSetVolumeFlag"
		:
		:
		: "a0", "a1", "d0", "d1", "cc", "memory"
	);
}

void Fractal_UpdateMasterTempo() {
	asm volatile (
		"jsr	dUpdateMasterTempo"
		:
		:
		: "a0", "a1", "d0", "d1", "cc", "memory"
	);
}

void Fractal_UpdateTempo() {
	asm volatile (
		"jsr	dUpdateTempo"
		:
		:
		: "a0", "a1", "d0", "d1", "cc", "memory"
	);
}

void Fractal_Mute(Fractal_ChannelMusic* channel) {
	channel->modeFlags |= 1 << Fractal_TrackFlag_VolumeUpdate;
	channel->trackFlags |= 1 << Fractal_ModeFlag_Muted;
}

void Fractal_Unmute(Fractal_ChannelMusic* channel) {
	channel->modeFlags |= 1 << Fractal_TrackFlag_VolumeUpdate;
	channel->trackFlags &= ~(1 << Fractal_ModeFlag_Muted);
}

#endif // MODULE_FRACTAL
