#include "config.h"
#include "types.h"

#include "ext/fractal/visual_dbg.h"

#if (MODULE_FRACTAL != 0)

#if VISUAL_DEBUG
#ifdef __INTELLISENSE__
    #pragma diag_suppress 1118
#endif

/* noreturn */ void VisualDebugger(void* externalProcessor) {
	register u16* a0 asm ("a0") = 0 /* we need to create this in RAM!!! */;
	register u32* a1 asm ("a1") = externalProcessor;

	asm volatile (
		"jsr	dVisualDebugger"
		: "=a" (a0), "=a" (a1)
		:
		: "cc", "memory"
	);
}

#endif

#endif // MODULE_FRACTAL
