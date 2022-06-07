/**
 *  \file ext/fractal/visual_dbg.h
 *  \brief Visual Debugger
 *  \author Aurora*Fields
 *  \date 06/2022
 *
 * Provides definitions, settings and functions for Fractal Sound Visual Debugger.
 */

#ifndef _VISUAL_DBG_H_
#define _VISUAL_DBG_H_

#include "config.h"
#include "types.h"
#include "ext/fractal/fractal.h"


#if (MODULE_FRACTAL != 0)

#define VISUAL_DBG_PIANOSPRCNT	18
#define VISUAL_DBG_GRAPHLEN		320
#define VISUAL_DBG_GRAPHLEN2	(VISUAL_DBG_GRAPHLEN / 4)
#define VISUAL_DBG_GRAPHHEIGHT	20

#define VISUAL_DBG_TRACKLINES	15

/**
 * ROM data structures
 */

typedef struct {
	s16* fractionBuffer;
	s16* frquencyBuffer;
	s16* volumeBuffer;
	s16* delayBuffer;
	s16* voiceSampleIdBuffer;
	s16* panningBuffer;

} VisualDbg_HistoryInfo;

typedef struct {
	union {
		u8 musicId;
		Fractal_ChannelInfo* musicInfo;
	};

	union {
		u8 sfxId;
		Fractal_ChannelInfo* sfxInfo;
	};

	union {
		u8 noteOffset;
		VisualDbg_HistoryInfo* musicHistory;
	};

	VisualDbg_HistoryInfo* sfxHistory;
	u32 iconVDPCommand;
	u32 volumeVDPCommand;
	u8 shortName[];

} VisualDbg_Channel;

typedef struct {
	u32* voiceTable;
	u32* sampleTable;
	u32* vibratoTable;
	u32* envelopeTable;

} VisualDbg_SongTable;

/**
 * RAM data structures
 */

typedef struct {
#if VISUAL_DEBUG
	u16 rawPad1Hold;
	u16 rawPad1Press;
	u16 pad1Hold;
	u16 pad1Press;

	u8 intFlag;
	u8 frameCounter;
	u8 advanceMode;
	u8 _unused1;			// not used... yet!

	union {
		u8 graphPosition;
		VisualDbg_SongTable* table;
	};

	union {
		Fractal_MusicHeader* musicHeader;	// warning: could be either(!)
		Fractal_SFXHeader* sfxHeader;
	};

	void* externalProcessor;
	u16 soundTestId;
	u8 repeatPressDelay;
	u8 graphParam;
	u8 selectedChannelEntry;

	VisualDbg_Channel* selectChannelAddress;
	u16* selectChannelOffset;			// will point to VisualDbg_Channel.sfxInfo or VisualDbg_Channel.musicInfo

	union {
		u8 modeId;
		void* modeUpdate;
	};

	// testscale screen variables
	u16 testSelection;
	u16 testType;
	u16 testSelectionMax;
	void* testUpdate;
	u16 testHistoryCount;

	// graph screen variables
	u16 fullHistoryPosition;
	u16 graphScale;
	u16 graphOffset;
	void* graphRoutine;
	u16 graphColumn;
	u16 graphLastColumn;

	// warning! shared variables
	union {
		u8 trackerLinesLeft;
		u16 testItemId;
		u16 graphLastColumn2;
	};

	union {
		u8 trackerDotCount;
		u16 testEditId;
		u16 graphColumn2;
	};

	u16 graphHistoryPosition2;
	u16 trackerFlags;
	u16 graphTilesLeft;

	union {
		u16* trackerLineAddress;
		u16 graphHistoryPosition;
	};

	union {
		s16 graphHistory[VISUAL_DBG_GRAPHLEN];
		u16 pianoSpriteTable[VISUAL_DBG_PIANOSPRCNT*4];
		u32* trackerLineBuffer[VISUAL_DBG_TRACKLINES];
	};

	union {
		u16 trackerCallStackAddress;
		u32 graphBufferAbove;
	};

	union {
		u32 graphBuffer[VISUAL_DBG_GRAPHHEIGHT*8];
		Fractal_StackItem trackerCallStack[FEATURE_STACK_DEPTH];
	};

	u32 graphBufferBelow;
	u32 graphBufferAbove2;

	union {
		u32 graphBuffer2[VISUAL_DBG_GRAPHHEIGHT*8];
		u32 testBuffer[VISUAL_DBG_GRAPHHEIGHT*8];
	};

	union {
		u32 trackerEndAddress;
		u32 graphBufferBelow2;
	};
#endif

} VisualDbg_Memory;

/**
 * Functions
 */

#if VISUAL_DEBUG

// WARNING: doesn't actually work :(
// v-int handler memory and Visual Debugger memor conflict causing a crash!!
/* noreturn */ void VisualDbg_Run(void* externalProcessor);
s8* VisualDbg_GetName(u16 sound);

#endif

#endif // MODULE_FRACTAL

#endif // _VISUAL_DBG_H_
