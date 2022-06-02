/**
 *  \file ext/fractal/fractal.h
 *  \brief Fractal definitions
 *  \author Aurora*Fields
 *  \date 06/2022
 *
 * Provides definitions, settings and functions for Fractal Sound.
 */

#ifndef _FRACTAL_H_
#define _FRACTAL_H_

#include "config.h"
#include "types.h"
#include "ext/fractal/fractal_cfg.h"


#if (MODULE_FRACTAL != 0)

// forward
typedef struct Fractal_Data_ Fractal_Data;

/**
 *  \brief
 *      Define the memory address reserved for the Fractal sound driver work buffer
 */
#define FRACTAL_MEMORY  (MEMORY_HIGH - ((sizeof(Fractal_Data) + 1) & 0xFFFE))


/**
 * ROM data structures
 */

typedef struct {
	u16 displacement;
	u16 shape;
	u16 multiply;
	u8 delay;
	u8 step;

} Fractal_VibratoEntry;

typedef struct {
	u16 freqOffset;
	u8 start[6];
	u8 loop[6];
	u8 rest[6];
	u8 restLoop[6];
	char debug[6];				// should contain "SAMPLE"

} Fractal_SampleEntry;

typedef struct {
	char debug[2];				// should contain "VO"
	u8 amsfms;
	u8 algorithmFeedback;

	u8 detuneMultiple[4];
	u8 rateScaleAttackRate[4];
	u8 ampModDecay1Rate[4];
	u8 decay2Rate[4];
	u8 decay1LevelReleaseRate[4];
	u8 ssgeg[4];
	u8 totalLevel[4];

} Fractal_VoiceEntry;

typedef enum {
	Fractal_ChannelType_FM1 = 0,		// valid for SFX
	Fractal_ChannelType_FM2 = 1,		// valid for SFX
	Fractal_ChannelType_FM3 = 2,
	Fractal_ChannelType_FM4 = 4,		// valid for SFX
	Fractal_ChannelType_FM5 = 5,		// valid for SFX
	Fractal_ChannelType_FM6 = 6,

	Fractal_ChannelType_BitFM3SM = 3,
	Fractal_ChannelType_FM3op1 = 0x8,
	Fractal_ChannelType_FM3op2 = 0x9,
	Fractal_ChannelType_FM3op3 = 0xA,
	Fractal_ChannelType_FM3op4 = 0xB,

	Fractal_ChannelType_BitDAC = 4,
	Fractal_ChannelType_DAC1 = 0x13,
	Fractal_ChannelType_DAC2 = 0x16,	// valid for SFX

	Fractal_ChannelType_BitSpecial = 6,
	Fractal_ChannelType_TA = 0x40,

	Fractal_ChannelType_PSG1 = 0x80,	// valid for SFX
	Fractal_ChannelType_PSG2 = 0xA0,	// valid for SFX
	Fractal_ChannelType_PSG3 = 0xC0,	// valid for SFX
	Fractal_ChannelType_PSG4 = 0xE0,	// valid for SFX

} Fractal_ChannelType;

typedef enum {
	Fractal_SongType_Command = -1,
	Fractal_SongType_Music = 0,
	Fractal_SongType_SFX = 1,

} Fractal_SongType;

typedef struct {
	union {
		Fractal_SongType type;
		void* routine;
	};

} Fractal_CommandHeader;

typedef enum {
	Fractal_HeaderFlag_NoMasterFraction = 0,
	Fractal_HeaderFlag_NoMasterVolume = 1,
	Fractal_HeaderFlag_NoUnderwater = 2,
	Fractal_HeaderFlag_FM3Special = 3,
	Fractal_HeaderFlag_BackUp = 4,
	Fractal_HeaderFlag_Continuous = 5,

} Fractal_HeaderFlags;

typedef struct {
	union {
		Fractal_SongType type;
		Fractal_VoiceEntry* voices;
	};

	union {
		u8 priority;
		Fractal_SampleEntry* samples;
	};

	union {
		u8 flags;
		Fractal_VibratoEntry* vibrato;
	};

	u32* envelopes;
	u16 tempoNTSC;
	u16 tempoPAL;
	u32 channels[];

} Fractal_MusicHeader;

typedef struct {
	union {
		Fractal_SongType type;
		Fractal_VoiceEntry* voices;
	};

	union {
		u8 priority;
		Fractal_SampleEntry* samples;
	};

	union {
		u8 flags;
		Fractal_VibratoEntry* vibrato;
	};

	u32* envelopes;
	u32 channels[];

} Fractal_SFXHeader;

typedef struct Fractal_ChanneInfo {
	u16 address;
	struct Fractal_ChanneInfo* mirror;
	u16 nextOffset;

	union {
		u8 frequencyRegister;
		void* sendFrequency;
	};

	union {
		u8 volumeRegister;
		void* sendVolume;
	};

	u16 frequencyData;
	u16 volumeData;

	union {
		Fractal_ChannelType type;
		void* table;
	};

	void* extraRoutine;

	union {
		u8 initialDelay;
		void* muteRoutine;
	};

	union {
		u8 unmuteRegister;
		void* unmuteRoutine;
	};

	void* pauseRoutine;

	union {
		u8 initialTransposition;
		void* keyOffRoutine;
	};

	union {
		u8 keyOffRegister;
		void* keyOnRoutine;
	};

	char* name;
	void* loadVoiceRoutine;

	union {
		u8 panningRegister;
		void* loadPanningRoutine;
	};

	void* restoreRoutine;

	// following is FM only
	union {
		u8 part;
		void* voiceRegisterList;
	};

	u8 D1LRegister;
	u8 voiceOffset;
	u8 keyBit;

} Fractal_ChanneInfo;

/**
 * RAM data structures
 */

typedef enum {
	Fractal_TrackFlag_VolumeUpdate = 0,
	Fractal_TrackFlag_FractionUpdate = 1,
	Fractal_TrackFlag_Cut = 2,
	Fractal_TrackFlag_Tie = 3,
	Fractal_TrackFlag_Rest = 4,
	Fractal_TrackFlag_Active = 7,

} Fractal_TrackFlagBits;

typedef enum {
	Fractal_ModeFlag_NoMasterFraction = 0,
	Fractal_ModeFlag_NoMasterVolume = 1,
	Fractal_ModeFlag_NoUnderwater = 2,
	Fractal_ModeFlag_Continuous = 3,
	Fractal_ModeFlag_Muted = 4,

} Fractal_ModeFlagBits;

typedef union {
	u8 counter;
	void* address;

} Fractal_StackItem;

typedef struct {
	union {
		u8 trackFlags;
		u32 address;
	};
	union {
		u8 noiseMode;
		u8 operatorMask;
	};

	u8 LFO;
	u8 delay;
	u8 lastDelay;

	union {
		u8 voice;
		u8 sample;
	};

	u8 volume;

#if FEATURE_SOUNDTEST
	u16 chipVolume;
	u16 chipFraction;
	u16 chipFrequency;
#endif

	u8 callStack;
	u8 modeFlags;
	Fractal_StackItem stack[FEATURE_STACK_DEPTH];

	u16 note;
	u16 fraction;

} Fractal_ChannelMusic;

typedef struct {
	Fractal_ChannelMusic;

	// SFX ONLY
	union {
		u8 priority;
		u32 songTable;
	};

	u16 soundId;

} Fractal_ChannelSFX;

typedef enum {
	Fractal_DataFlag_Portamento = 0,
	Fractal_DataFlag_Delay = 1,
	Fractal_DataFlag_Vibrato = 7,

} Fractal_DataFlagBits;

typedef struct {
	u8 flags;
	u8 envId;
	u8 envDelay;
	u8 envLastDelay;
	u16 envPosition;

	u8 vibId;
	u8 vibDelay;
	u16 vibPosition;

} Fractal_VolumeData;

typedef struct {
	Fractal_VolumeData;

	u16 portamentoTarget;
	u16 portamentoDisplacement;

} Fractal_FractionData;

typedef enum {
	paused = 0,
	FM3Special2 = 3,
	backUp2 = 4,
	underwater = 5,
	executing = 6,

} Fractal_DriverFlagBits;

typedef struct {
	u16 tempoSong;
	u16 tempoCurrent;
	u16 tempoAccumulator;
	u8 flags;
	u8 FM3SpecialMode;
	u8 FM3SpecialKeysOn;
	u8 panMusic[7];
	u8 LFORegister;
	u32 songTable;

	Fractal_ChannelMusic chFM1;
	Fractal_VolumeData volFM1;
	Fractal_FractionData fracFM1;

	Fractal_ChannelMusic chFM2;
	Fractal_VolumeData volFM2;
	Fractal_FractionData fracFM2;

	Fractal_ChannelMusic chFM4;
	Fractal_VolumeData volFM4;
	Fractal_FractionData fracFM4;

	Fractal_ChannelMusic chFM5;
	Fractal_VolumeData volFM5;
	Fractal_FractionData fracFM5;

	Fractal_ChannelMusic chFM6;
	Fractal_VolumeData volFM6;
	Fractal_FractionData fracFM6;

	Fractal_ChannelMusic chFM3op1;
	Fractal_VolumeData volFM3op1;
	Fractal_FractionData fracFM3op1;

	Fractal_ChannelMusic chFM3op2;
	Fractal_VolumeData volFM3op2;
	Fractal_FractionData fracFM3op2;

	Fractal_ChannelMusic chFM3op3;
	Fractal_VolumeData volFM3op3;
	Fractal_FractionData fracFM3op3;

	Fractal_ChannelMusic chFM3op4;
	Fractal_VolumeData volFM3op4;
	Fractal_FractionData fracFM3op4;

	Fractal_ChannelMusic chTA;
	Fractal_FractionData fracTA;

	Fractal_ChannelMusic chDAC1;
	Fractal_VolumeData volDAC1;
	Fractal_FractionData fracDAC1;

	Fractal_ChannelMusic chDAC2;
	Fractal_VolumeData volDAC2;
	Fractal_FractionData fracDAC2;

	Fractal_ChannelMusic chPSG1;
	Fractal_VolumeData volPSG1;
	Fractal_FractionData fracPSG1;

	Fractal_ChannelMusic chPSG2;
	Fractal_VolumeData volPSG2;
	Fractal_FractionData fracPSG2;

	Fractal_ChannelMusic chPSG3;
	Fractal_VolumeData volPSG3;
	Fractal_FractionData fracPSG3;

	Fractal_ChannelMusic chPSG4;
	Fractal_VolumeData volPSG4;

} Fractal_DriverMusic;

typedef struct Fractal_Data_ {
	u8 timing[7];
	u8 panSFX[5];
	u16 queue[FEATURE_QUEUE_SIZE];
	u8 FM3SpecialKeysOff;
	u8 lastCueId;
	u8 spindashCounter;
	u8 sampleFilterId;

	u16 masterVolume;
	u16 masterVolumePSG;
	u16 masterTempo;

	Fractal_ChannelSFX chSFXFM1;
	Fractal_VolumeData volSFXFM1;
	Fractal_FractionData fracSFXFM1;

	Fractal_ChannelSFX chSFXFM2;
	Fractal_VolumeData volSFXFM2;
	Fractal_FractionData fracSFXFM2;

	Fractal_ChannelSFX chSFXFM4;
	Fractal_VolumeData volSFXFM4;
	Fractal_FractionData fracSFXFM4;

	Fractal_ChannelSFX chSFXFM5;
	Fractal_VolumeData volSFXFM5;
	Fractal_FractionData fracSFXFM5;

	Fractal_ChannelMusic chSFXDAC2;
	Fractal_VolumeData volSFXDAC2;
	Fractal_FractionData fracSFXDAC2;

	Fractal_ChannelSFX chSFXPSG1;
	Fractal_VolumeData volSFXPSG1;
	Fractal_FractionData fracSFXPSG1;

	Fractal_ChannelSFX chSFXPSG2;
	Fractal_VolumeData volSFXPSG2;
	Fractal_FractionData fracSFXPSG2;

	Fractal_ChannelSFX chSFXPSG3;
	Fractal_VolumeData volSFXPSG3;
	Fractal_FractionData fracSFXPSG3;

	Fractal_ChannelSFX chSFXPSG4;
	Fractal_VolumeData volSFXPSG4;

	Fractal_DriverMusic music;

	// these are only used while driver is updating! you can use them as temporary variables!
	u8 overwriteTL[4];

	union {
		u8 updateDAC;
		u32 sampleInfoAddress;
	};

	u16 sampleFrequency;

#if FEATURE_BACKUP
	Fractal_DriverMusic backup;
#endif

	// mMasterFrac	equ mPSG4+cFrac					; master fraction
	// mSpecMask	equ mFM3o1+cOpMask				; FM3 special mode key mask

} Fractal_Data;


/**
 * Functions
 */

void Fractal_Init(void (*decompressFunction)(u8* source, u8* destination));
void Fractal_Decompress(u8* source, u8* destination);
void Fractal_Update();
void Fractal_Queue(u16 sound);
void Fractal_UpdateMasterFraction();
void Fractal_ForceFractionUpdate();
void Fractal_UpdateMasterVolume();
void Fractal_ForceVolumeUpdate();
void Fractal_UpdateMasterTempo();
void Fractal_UpdateTempo();
void Fractal_Mute(Fractal_ChannelMusic* channel);
void Fractal_Unmute(Fractal_ChannelMusic* channel);

#endif // MODULE_FRACTAL

#endif // _FRACTAL_H_
