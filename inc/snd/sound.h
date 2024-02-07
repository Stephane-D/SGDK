/**
 *  \file sound.h
 *  \brief General / shared sound driver definitions
 *  \author Stephane Dallongeville
 *  \date 08/2011
 *
 * This unit provides basic definitions for sound drivers.<br>
 */

#ifndef _SOUND_H_
#define _SOUND_H_

/**
 *  \brief
 *      Sound PCM channel enum
 */
typedef enum
{
    SOUND_PCM_CH_AUTO = -1,       // auto-select
    SOUND_PCM_CH1 = 0,            // channel 1
    SOUND_PCM_CH2,                // channel 2
    SOUND_PCM_CH3,                // channel 3
    SOUND_PCM_CH4,                // channel 4
} SoundPCMChannel;

/**
 *  \brief
 *      Sound panning enum
 */
typedef enum
{
    SOUND_PAN_NONE = 0x00,        // mute
    SOUND_PAN_RIGHT = 0x40,       // right speaker only
    SOUND_PAN_LEFT = 0x80,        // left speaker only
    SOUND_PAN_CENTER = 0xC0       // center (both speakers)
} SoundPanning;

/**
 *  \brief
 *      PCM channel 1 (first channel) selection mask.
 */
#define SOUND_PCM_CH1_MSK   (1 << SOUND_PCM_CH1)
/**
 *  \brief
 *      PCM channel 2 selection mask.
 */
#define SOUND_PCM_CH2_MSK   (1 << SOUND_PCM_CH2)
/**
 *  \brief
 *      PCM channel 3 selection mask.
 */
#define SOUND_PCM_CH3_MSK   (1 << SOUND_PCM_CH3)
/**
 *  \brief
 *      PCM channel 4 selection mask.
 */
#define SOUND_PCM_CH4_MSK   (1 << SOUND_PCM_CH4)


#define SOUND_PCM_AUTO      _Pragma("GCC error \"This method is deprecated, use SOUND_PCM_CH_AUTO instead.\"")


/**
 *  \brief
 *      Load the dummy Z80 driver.
 *
 *      Don't use this method directly, use #Z80_loadDriver(..) instead.
 */
void SND_NULL_loadDriver(void);
/**
 *  \brief
 *      Unload the dummy Z80 driver.
 *
 *      Don't use this method directly, use #Z80_unloadDriver(..) instead.
 */
void SND_NULL_unloadDriver(void);


#endif // _SOUND_H_
