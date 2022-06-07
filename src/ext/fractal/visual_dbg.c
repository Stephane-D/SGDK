#include "config.h"
#include "types.h"
#include "sys.h"

#include "ext/fractal/visual_dbg.h"

#if (MODULE_FRACTAL != 0)

#if VISUAL_DEBUG
#ifdef __INTELLISENSE__
    #pragma diag_suppress 1118
#endif

extern VoidCallback *vintCB;

/* noreturn */ void VisualDbg_Run(void* externalProcessor) {
	register void* a0 asm ("a0") = &vintCB;
	register u32* a1 asm ("a1") = externalProcessor;

	asm volatile (
		"jsr	dVisualDebugger"
		:
		: "a" (a0), "a" (a1)
		: "cc", "memory"
	);
}

s8* VisualDbg_GetName(u16 sound) {
	register u16 d0 asm ("d0") = sound;
	s8* string;

	asm volatile (
		"lea	dvSoundNames,%%a0\n\t"
		"move.l	(%%a0,%%d0.w),%0\n\t"
		: "=a" (string)
		: "d" (d0)
		: "a0", "cc", "memory"
	);

	return string;
}

#endif

#endif // MODULE_FRACTAL
