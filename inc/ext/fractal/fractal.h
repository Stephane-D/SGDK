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

/**
 *  \brief
 *      Define the memory address reserved for the Fractal sound driver work buffer
 */
#define FRACTAL_MEMORY  (MEMORY_HIGH - ((sizeof(Fractal_Data) + 1) & 0xFFFE))


/**
 * ROM data structures
 */

/**
 *  \brief
 *          Struct that represents a vibrato table entry
 *
 *  \param displacement
 *          Displacement per frame. Eg this is how fast the vibrato is.
 *
 *  \param shape
 *          Lower 8 bits should be 0 and upper 8 bits be the shape ID.
 *
 *  \param multiply
 *          Multiplier for vibrato entries. This essentially is depth.
 *
 *  \param delay
 *          How many ticks to delay before vibrato starts
 *
 *  \param step
 *          Initial offset for the LUT.
 */
typedef struct {
	u16 displacement;
	u16 shape;
	u16 multiply;
	u8 delay;
	u8 step;

} Fractal_VibratoEntry;

/**
 *  \brief
 *          Struct that represents a sample table entry
 *
 *  \param freqOffsetHi
 *          High byte of frequency offset of the sample
 *
 *  \param freqOffsetLo
 *          Low byte of frequency offset of the sample
 *
 *  \param loop
 *          Address of the key on sample
 *
 *  \param start
 *          Address of the key on loop sample
 *
 *  \param rest
 *          Address of the key off sample
 *
 *  \param restLoop
 *          Address of the key off loop sample
 */
typedef struct {
	union {
		u8 freqOffsetHi;
		u8* start;
	};

	union {
		u8 freqOffsetLo;
		u8* loop;
	};

	u8* rest;
	u8* restLoop;

} Fractal_SampleEntry;

/**
 *  \brief
 *          Struct that represents a voice table entry
 */
typedef struct {
	s8 debug[2];				// should contain "VO"
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

/**
 *  \brief
 *          Enum for channel types
 */
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

	Fractal_ChannelType_End = 0xFF,		// indicates end of channels list

} __attribute__ ((__packed__))  Fractal_ChannelType;

/**
 *  \brief
 *          Enum for song types
 */
typedef enum {
	Fractal_SongType_Command = -1,
	Fractal_SongType_Music = 0,
	Fractal_SongType_SFX = 1,

} __attribute__ ((__packed__)) Fractal_SongType;

/**
 *  \brief
 *          Command song header
 *
 *  \param type
 *          Fractal_SongType entry
 *
 *  \param routine
 *          The 24-bit address that handles this command
 */
typedef struct {
	union {
		Fractal_SongType type;
		void* routine;
	};

} Fractal_CommandHeader;

/**
 *  \brief
 *          Enum for various flags in song headers
 */
typedef enum {
	Fractal_HeaderFlag_NoMasterFraction = 0,
	Fractal_HeaderFlag_NoMasterVolume = 1,
	Fractal_HeaderFlag_NoUnderwater = 2,
	Fractal_HeaderFlag_FM3Special = 3,
	Fractal_HeaderFlag_BackUp = 4,
	Fractal_HeaderFlag_Continuous = 5,

} __attribute__ ((__packed__)) Fractal_HeaderFlags;

/**
 *  \brief
 *          Command song header
 *
 *  \param type
 *          Fractal_ChannelType entry
 *
 *  \param routine
 *          The 24-bit address where the channel data starts at
 */
typedef union {
	Fractal_ChannelType type;
	void* data;

} Fractal_HeaderChannel;

/**
 *  \brief
 *          Music song header
 *
 *  \param type
 *          Fractal_SongType entry
 *
 *  \param voices
 *          The 24-bit address for voice data
 *
 *  \param samples
 *          The 24-bit address for sample data
 *
 *  \param flags
 *          bit field that refers to flags in Fractal_HeaderFlags
 *
 *  \param vibrato
 *          The 24-bit address for vibrato data
 *
 *  \param envelopes
 *          The 24-bit address for envelope pointer table
 *
 *  \param tempoNTSC
 *          tempo on 60hz systems
 *
 *  \param tempoPAL
 *          tempo on 50hz systems
 *
 *  \param channels
 *          list of Fractal_HeaderChannel entries
 */
typedef struct {
	union {
		Fractal_SongType type;
		Fractal_VoiceEntry* voices;
	};

	Fractal_SampleEntry* samples;

	union {
		u8 flags;
		Fractal_VibratoEntry* vibrato;
	};

	u32* envelopes;
	u16 tempoNTSC;
	u16 tempoPAL;
	Fractal_HeaderChannel channels[];

} Fractal_MusicHeader;

/**
 *  \brief
 *          SFX song header
 *
 *  \param type
 *          Fractal_SongType entry
 *
 *  \param voices
 *          The 24-bit address for voice data
 *
 *  \param priority
 *          Priority of all channels in this song. Higher priorities can overwrite lower priorities.
 *
 *  \param samples
 *          The 24-bit address for sample data
 *
 *  \param flags
 *          bit field that refers to flags in Fractal_HeaderFlags
 *
 *  \param vibrato
 *          The 24-bit address for vibrato data
 *
 *  \param envelopes
 *          The 24-bit address for envelope pointer table
 *
 *  \param channels
 *          list of Fractal_HeaderChannel entries
 */
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
	Fractal_HeaderChannel channels[];

} Fractal_SFXHeader;

/**
 *  \brief
 *          Channel info struct. Has various static elements about each channel
 *
 *  \param address
 *          16-bit pointer to channel RAM address
 *
 *  \param mirror
 *          Fractal_ChannelInfo pointer to the "mirror" channel. For music, is is the equivalent SFX channel, for SFX it is the music channel. NULL if not applicable
 *
 *  \param panning
 *          16-bit pointer to channel's panning RAM address
 *
 *  \param nextOffset
 *          16-bit offset to the next channel info struct
 *
 *  \param frequencyRegister
 *          The register ID of the frequency MSB for this channel. Only applicable to FM and PSG
 *
 *  \param sendFrequency
 *          The 24-bit address for the routine that handles frequency updates. Only applicable if lowest 16-bits are not 0
 *
 *  \param volumeRegister
 *          The register ID of the volume for this channel. Only applicable to FM and PSG
 *
 *  \param sendVolume
 *          The 24-bit address for the routine that handles volume updates. Only applicable if lowest 16-bits are not 0
 *
 *  \param frequencyData
 *          16-bit pointer to channel Fractal_FractionData struct RAM address. NULL if not applicable
 *
 *  \param volumeData
 *          16-bit pointer to channel Fractal_VolumeData struct RAM address. NULL if not applicable
 *
 *  \param type
 *          Fractal_ChannelType entry describing this channel
 *
 *  \param table
 *          24-bit pointer to a RAM address that contains the song header of the current music/sfx
 *
 *  \param extraRoutine
 *          24-bit address for the routine that handles extra code at the end of the channel execution
 *
 *  \param initialDelay
 *          The initial amount of delay when the channel starts up (broken!)
 *
 *  \param muteRoutine
 *          24-bit address for the routine that handles the channel being muted for various reasons
 *
 *  \param unmuteRegister
 *          The register ID used by the mute and unmute routines
 *
 *  \param unmuteRoutine
 *          24-bit address for the routine that handles the channel being unmuted
 *
 *  \param pauseRoutine
 *          24-bit address for the routine that handles the channel being paused
 *
 *  \param initialTransposition
 *          The initial transposition of the channel in notes
 *
 *  \param keyOffRoutine
 *          24-bit address for the routine that handles keying off the channel
 *
 *  \param keyRegister
 *          The register ID used by keyOffRoutine and keyOnRoutine
 *
 *  \param keyOnRoutine
 *          24-bit address for the routine that handles keying on the channel
 *
 *  \param name
 *          pointer to the string descibing the channel name
 *
 *  \param loadVoiceRoutine
 *          Routine that handles loading and reloading voice routines
 *
 *  \param panningRegister
 *          The register ID used by loadPanningRoutine
 *
 *  \param loadPanningRoutine
 *          Routine that handles loading panning for the channel
 *
 *  \param restoreRoutine
 *          Routine that helps with restoring a music channel after it's been overwritten by sfx
 *
 *  \param part
 *          ID of the YM2612 part where this FM channel writes to. 0 = part 1, 2 = part 2
 *
 *  \param voiceRegisterList
 *          24-bit pointer to list of voice registers
 *
 *  \param D1LRegister
 *          unused(?)
 *
 *  \param voiceOffset
 *          Offset for the operator in FM3 special operators, to load data of the correct operator
 *
 *  \param keyBit
 *          The bit that this FM3 special operator uses for keying on or off.
 */
typedef struct Fractal_ChannelInfo {
	p16 address;
	struct Fractal_ChannelInfo* mirror;
	p16 panning;
	s16 nextOffset;

	union {
		u8 frequencyRegister;
		void* sendFrequency;
	};

	union {
		u8 volumeRegister;
		void* sendVolume;
	};

	p16 frequencyData;
	p16 volumeData;

	union {
		Fractal_ChannelType type;
		void** table;
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
		u8 keyRegister;
		void* keyOnRoutine;
	};

	s8* name;
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

} Fractal_ChannelInfo;

/**
 * RAM data structures
 */

/**
 *  \brief
 *          Enum for various flags for Fractal_ChannelMusic.trackFlags
 */
typedef enum {
	Fractal_TrackFlag_VolumeUpdate = 0,
	Fractal_TrackFlag_FractionUpdate = 1,
	Fractal_TrackFlag_Cut = 2,
	Fractal_TrackFlag_Tie = 3,
	Fractal_TrackFlag_Rest = 4,
	Fractal_TrackFlag_Active = 7,

} __attribute__ ((__packed__)) Fractal_TrackFlagBits;

/**
 *  \brief
 *          Enum for various flags for Fractal_ChannelMusic.modeFlags
 */
typedef enum {
	Fractal_ModeFlag_NoMasterFraction = 0,
	Fractal_ModeFlag_NoMasterVolume = 1,
	Fractal_ModeFlag_NoUnderwater = 2,
	Fractal_ModeFlag_IsUnderwater = 3,
	Fractal_ModeFlag_Continuous = 4,
	Fractal_ModeFlag_Muted = 5,

} __attribute__ ((__packed__)) Fractal_ModeFlagBits;

/**
 *  \brief
 *          Items for each stack entry on the channel
 *
 *  \param counter
 *          How many times to still run the routine
 *
 *  \param address
 *          24-bit pointer for the routine to run
 */
typedef union {
	u8 counter;
	void* address;

} Fractal_StackItem;

/**
 *  \brief
 *          struct for music channels
 *
 *  \param trackFlags
 *          bit field that refers to flags in Fractal_TrackFlagBits
 *
 *  \param address
 *          24-bit pointer for the current track data for the channel
 *
 *  \param noiseMode
 *          noise mode for PSG4
 *
 *  \param operatorMask
 *          operator mask for FM channels
 *
 *  \param LFO
 *          AMS+FMS for music FM channels
 *
 *  \param delay
 *          Remaining number of ticks before checking the track data
 *
 *  \param lastDelay
 *          Stored delay
 *
 *  \param voice
 *          The current voice ID
 *
 *  \param sample
 *          The current sample ID
 *
 *  \param volume
 *          Channel volume
 *
 *  \param chipVolume
 *          Chip volume
 *
 *  \param chipFraction
 *          Chip fraction
 *
 *  \param chipFrequency
 *          Chip raw frequency
 *
 *  \param callStack
 *          Offset in channel RAM where the current call stack is at
 *
 *  \param modeFlags
 *          bit field that refers to flags in Fractal_ModeFlagBits
 *
 *  \param stack
 *          channel stack items array
 *
 *  \param note
 *          Channel 8.8 fixed point note offset
 *
 *  \param fraction
 *          Channel 8.8 fixed point fraction offset
 */
typedef struct {
	union {
		u8 trackFlags;
		void* address;
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
	s16 chipVolume;
	s16 chipFraction;
	s16 chipFrequency;
#endif

	u8 callStack;
	u8 modeFlags;
	Fractal_StackItem stack[FEATURE_STACK_DEPTH];

	s16 note;
	s16 fraction;

} Fractal_ChannelMusic;

/**
 *  \brief
 *          struct for SFX channels
 *
 *  \param trackFlags
 *          bit field that refers to flags in Fractal_TrackFlagBits
 *
 *  \param address
 *          24-bit pointer for the current track data for the channel
 *
 *  \param noiseMode
 *          noise mode for PSG4
 *
 *  \param operatorMask
 *          operator mask for FM channels
 *
 *  \param LFO
 *          AMS+FMS for music FM channels
 *
 *  \param delay
 *          Remaining number of ticks before checking the track data
 *
 *  \param lastDelay
 *          Stored delay
 *
 *  \param voice
 *          The current voice ID
 *
 *  \param sample
 *          The current sample ID
 *
 *  \param volume
 *          Channel volume
 *
 *  \param chipVolume
 *          Chip volume
 *
 *  \param chipFraction
 *          Chip fraction
 *
 *  \param chipFrequency
 *          Chip raw frequency
 *
 *  \param callStack
 *          Offset in channel RAM where the current call stack is at
 *
 *  \param modeFlags
 *          bit field that refers to flags in Fractal_ModeFlagBits
 *
 *  \param stack
 *          channel stack items array
 *
 *  \param note
 *          Channel 8.8 fixed point note offset
 *
 *  \param fraction
 *          Channel 8.8 fixed point fraction offset
 *
 *  \param priority
 *          Priority of the channel. Higher priorities can overwrite lower priorities.
 *
 *  \param table
 *          24-bit pointer to the ROM address that contains the song header of the current sfx channel
 *
 *  \param soundId
 *          The ID of the sound that this sfx channel is playing
 */
typedef struct {
	Fractal_ChannelMusic;

	// SFX ONLY
	union {
		u8 priority;
		void* table;
	};

	u16 soundId;

} Fractal_ChannelSFX;

/**
 *  \brief
 *          Enum for various flags for Fractal_FractionData.flags and Fractal_FractionData.flags
 */
typedef enum {
	Fractal_DataFlag_Portamento = 0,
	Fractal_DataFlag_Delay = 1,
	Fractal_DataFlag_Vibrato = 7,

} __attribute__ ((__packed__)) Fractal_DataFlagBits;

/**
 *  \brief
 *          struct for channel volume data
 *
 *  \param flags
 *          bit field that refers to flags in Fractal_DataFlagBits
 *
 *  \param envId
 *          volume envelope ID
 *
 *  \param envDelay
 *          delay until the next envelope value is read
 *
 *  \param envLastDelay
 *          stored version of envDelay
 *
 *  \param envPosition
 *          Offset from envelope start position to read from
 *
 *  \param vibId
 *          Vibrato entry ID
 *
 *  \param vibDelay
 *          Number of ticks before vibrato starts
 *
 *  \param vibPosition
 *          Vibrato position accumulator in 8.8 fixed point format
 */
typedef struct {
	u8 flags;
	u8 envId;
	u8 envDelay;
	u8 envLastDelay;
	s16 envPosition;

	u8 vibId;
	u8 vibDelay;
	u16 vibPosition;

} Fractal_VolumeData;

/**
 *  \brief
 *          struct for channel volume data
 *
 *  \param flags
 *          bit field that refers to flags in Fractal_DataFlagBits
 *
 *  \param envId
 *          volume envelope ID
 *
 *  \param envDelay
 *          delay until the next envelope value is read
 *
 *  \param envLastDelay
 *          stored version of envDelay
 *
 *  \param envPosition
 *          Offset from envelope start position to read from
 *
 *  \param vibId
 *          Vibrato entry ID
 *
 *  \param vibDelay
 *          Number of ticks before vibrato starts
 *
 *  \param vibPosition
 *          Vibrato position accumulator in 8.8 fixed point format
 *
 *  \param portamentoTarget
 *          Portamento target offset in 8.8 fixed point format
 *
 *  \param portamentoTarget
 *          Portamento displacement per tick in 8.8 fixed point format
 */
typedef struct {
	Fractal_VolumeData;

	s16 portamentoTarget;
	s16 portamentoDisplacement;

} Fractal_FractionData;

/**
 *  \brief
 *          Enum for various flags for Fractal_Data.music.flags
 */
typedef enum {
	paused = 0,
	FM3Special2 = 3,
	backUp2 = 4,
	underwater = 5,
	executing = 6,

} __attribute__ ((__packed__)) Fractal_DriverFlagBits;

/**
 *  \brief
 *          struct for sound effect variables for Fractal
 */
typedef struct {
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

	Fractal_ChannelSFX chSFXDAC2;
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

} Fractal_SFXData;

/**
 *  \brief
 *          struct for sound effect variables for Fractal
 *
 *  \param tempoSong
 *          The original tempo of the song in 8.8 fixed point format
 *
 *  \param tempoCurrent
 *          The current tempo of the song scaled by Fractal_Data.masterTempo in 8.8 fixed point format
 *
 *  \param tempoAccumulator
 *          The tempo accumulator of the song in 8.8 fixed point format
 *
 *  \param flags
 *          bit field that refers to flags in Fractal_DriverFlagBits
 *
 *  \param FM3SpecialMode
 *          FM3 special mode. 0 = not special, 0x40 = special mode, 0x81 = special mode with CSM
 *
 *  \param FM3SpecialKeysOn
 *          Tracks which keys should be enabled for FM3 special mode
 *
 *  \param panMusic
 *          Music panning array. Panning goes from -$7F to $7F. $80 is special and should not be used.
 *
 *  \param LFORegister
 *          Backup of the LFO register value
 *
 *  \param table
 *          24-bit pointer to the ROM address that contains the song header for music
 */
typedef struct {
	u16 tempoSong;
	u16 tempoCurrent;
	u16 tempoAccumulator;
	u8 flags;
	u8 FM3SpecialMode;
	u8 FM3SpecialKeysOn;
	s8 panMusic[7];
	u8 LFORegister;
	void* songTable;

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

} Fractal_MusicData;

/**
 *  \brief
 *          struct for main driver data for Fractal Sound
 *
 *  \param timing
 *          Array of timings flags. Game program and music can read and write these to communicate together
 *
 *  \param panSFX
 *          SFX panning array. Panning goes from -$7F to $7F. $80 is special and should not be used.
 *
 *  \param queue
 *          The sound queue that stores what sounds will be played next
 *
 *  \param FM3SpecialKeysOff
 *          Tracks which keys are disabled for FM3 special mode
 *
 *  \param lastCueId
 *          Tracks which DualPCM cue ID was last used. This avoids cases where for some reason the cue doesn't change, so we rather delay 1 frame than corrupt YM2612 status
 *
 *  \param spindashCounter
 *          Tracks the pitch offset for the Spindash sfx
 *
 *  \param sampleFilterId
 *          The ID of the active sample filter
 *
 *  \param masterVolume
 *          The master volume offset in 8.8 fixed point format for FM and DAC
 *
 *  \param masterVolumePSG
 *          The master volume offset in 8.8 fixed point format for PSG
 *
 *  \param masterTempo
 *          The master tempo multiplier. 0x1000 = 100%
 *
 *  \param sfx
 *          Fractal_SFXData struct for Fractal
 *
 *  \param music
 *          Fractal_MusicData struct for Fractal
 *
 *  \param overwriteTL
 *          Operator TL overwrites for each operator, updated at the end of the channel execution
 *
 *  \param updateDAC
 *          Flags for updating DAC data at the end of DAC channel processing
 *
 *  \param sampleInfoAddress
 *          24-bit ROM address for the sample info to update addresses
 *
 *  \param sampleFrequency
 *          Sample frequency to update for DAC channel
 *
 *  \param backup
 *          Fractal_MusicData struct for backing up data in Fractal_Data.music
 */
typedef struct Fractal_Data_ {
	u8 timing[7];
	s8 panSFX[5];
	u16 queue[FEATURE_QUEUE_SIZE];
	u8 FM3SpecialKeysOff;
	u8 lastCueId;
	u8 spindashCounter;
	u8 sampleFilterId;

	s16 masterVolume;
	s16 masterVolumePSG;
	s16 masterTempo;

	Fractal_SFXData sfx;
	Fractal_MusicData music;

	// these are only used while driver is updating! you can use them as temporary variables!
	u8 overwriteTL[4];

	union {
		u8 updateDAC;
		void* sampleInfoAddress;
	};

	s16 sampleFrequency;

#if FEATURE_BACKUP
	Fractal_MusicData backup;
#endif

} Fractal_Data;

/**
 *  \brief
 *          The master fraction offset in 8.8 fixed point format
 */
#define Fractal_GetMasterFreq(data) (data->music.chPSG4.fraction)

/**
 *  \brief
 *          The FM3 special mode operator mask
 */
#define Fractal_GetSpecialMask(data) (data->music.chFM3op1.operatorMask)

/**
 *  \brief
 *          The FM3 special mode operator xor mask
 */
#define Fractal_GetSpecialXor(data) (data->music.chFM3op2.operatorMask)

/**
 * Functions
 */

/**
 *  \brief
 *          Initialize Fractal Sound
 *
 *  \param decompressFunction
 *          The function that handles decompression of DualPCM.dat
 */
void Fractal_Init(void (*decompressFunction)(u8* source, u8* destination));
/**
 *  \brief
 *          Example decompressor routine, loading an uncompressed file into Z80 memory
 *
 *  \param source
 *          The compressed input data
 *
 *  \param destination
 *          The target address
 */
void Fractal_Decompress(u8* source, u8* destination);
/**
 *  \brief
 *          Update Fractal at the end of v-int
 */
void Fractal_Update();
/**
 *  \brief
 *          Queue a sound for Fractal Sound
 *
 *  \param sound
 *          Sound ID. Higher ID's take precedence when playing and saving in the queue
 */
void Fractal_Queue(u16 sound);
/**
 *  \brief
 *          Set the master fraction offset
 *
 *  \param frac
 *          8.8 fixed point fraction offset. whole values = semitones
 */
void Fractal_SetMasterFraction(s16 frac);
/**
 *  \brief
 *          Force fraction update on all channels
 */
void Fractal_ForceFractionUpdate();
/**
 *  \brief
 *          Set the master volume offset
 *
 *  \param main
 *          volume offset for FM and DAC
 *
 *  \param psg
 *          volume offset for PSG
 */
void Fractal_SetMasterVolume(s16 main, s16 psg);
/**
 *  \brief
 *          Force volume update on all channels
 */
void Fractal_ForceVolumeUpdate();
/**
 *  \brief
 *          Set the master tempo multiplier
 *
 *  \param tempo
 *          tempo multiplier. 0x1000 = 100%
 */
void Fractal_SetMasterTempo(s16 tempo);
/**
 *  \brief
 *          Update active tempo
 */
void Fractal_UpdateTempo();
/**
 *  \brief
 *          Check if channel is currently muted
 *
 *  \param channel
 *          music or SFX channel to check. You can safely cast Fractal_ChannelSFX to Fractal_ChannelMusic
 *
 *  \return
 *          channel mute status
 */
bool Fractal_IsMuted(Fractal_ChannelMusic* channel);
/**
 *  \brief
 *          Toggle channel mute status
 *
 *  \param channel
 *          music or SFX channel to check. You can safely cast Fractal_ChannelSFX to Fractal_ChannelMusic
 */
void Fractal_ToggleMute(Fractal_ChannelMusic* channel);
/**
 *  \brief
 *          Set channel mute status
 *
 *  \param channel
 *          music or SFX channel to check. You can safely cast Fractal_ChannelSFX to Fractal_ChannelMusic
 */
void Fractal_Mute(Fractal_ChannelMusic* channel);
/**
 *  \brief
 *          Reset channel mute status
 *
 *  \param channel
 *          music or SFX channel to check. You can safely cast Fractal_ChannelSFX to Fractal_ChannelMusic
 */
void Fractal_Unmute(Fractal_ChannelMusic* channel);

/**
 *  \brief
 *          Fetch music channel based on Fractal_ChannelType
 *
 *  \param type
 *          one of Fractal_ChannelType
 *
 *  \return
 *          Fractal_ChannelInfo struct if valid channel, or NULL if not.
 */
Fractal_ChannelInfo* Fractal_GetMusicChannel(Fractal_ChannelType type);
/**
 *  \brief
 *          Fetch sfx channel based on Fractal_ChannelType
 *
 *  \param type
 *          one of Fractal_ChannelType
 *
 *  \return
 *          Fractal_ChannelInfo struct if valid channel, or NULL if not.
 */
Fractal_ChannelInfo* Fractal_GetSFXChannel(Fractal_ChannelType type);

/**
 *  \brief
 *          Execute a function for every valid music channel. If FM3 special mode is on, this includes fm3o1 to fm3o4, and ta, while if off, applies to fm3 instead.
 *
 *  \param func
 *          callback function that takes Fractal_ChannelInfo struct as the first and only argument
 */
extern void Fractal_ExecuteForMusicChannels(void (*func)(Fractal_ChannelInfo* channel));
/**
 *  \brief
 *          Execute a function for every valid sfx channel
 *
 *  \param func
 *          callback function that takes Fractal_ChannelInfo struct as the first and only argument
 */
extern void Fractal_ExecuteForSFXChannels(void (*func)(Fractal_ChannelInfo* channel));

#endif // MODULE_FRACTAL

#endif // _FRACTAL_H_
