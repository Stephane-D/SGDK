/**
 *  \file snd_pcm.h
 *  \brief Z80_DRIVER_PCM sound driver API
 *  \author Stephane Dallongeville
 *  \date 08/2011
 *
 * This unit provides playback method for the <b>Z80_DRIVER_PCM</b> sound driver.<br>
 *<br>
 * Sound driver description:<br>
 * This sound driver allow Single PCM channel 8 bits signed sample driver.<br>
 * It can play a sample (8 bit signed) from 8 Khz up to 32 Khz rate.<br>
 *<br>
 * Note that SGDK sound drivers may not fit your needs so it's important to know<br>
 * that some alternatives sound drivers exist (see SGDK readme.md for more info)
 */

#ifndef _SND_PCM_H_
#define _SND_PCM_H_

#include "snd/z80_driver.h"
#include "snd/sound.h"

/**
 *  \brief
 *      PCM sample rate values
 */
typedef enum
{
    SOUND_PCM_RATE_32000,   /** Best quality but take lot of rom space */
    SOUND_PCM_RATE_22050,
    SOUND_PCM_RATE_16000,
    SOUND_PCM_RATE_13400,
    SOUND_PCM_RATE_11025,
    SOUND_PCM_RATE_8000     /** Worst quality but take less rom place */
} SoundPcmSampleRate;


#define SOUND_RATE_32000    _Pragma("GCC error \"This method is deprecated, use SOUND_PCM_RATE_32000 instead.\"")
#define SOUND_RATE_22050    _Pragma("GCC error \"This method is deprecated, use SOUND_PCM_RATE_22050 instead.\"")
#define SOUND_RATE_16000    _Pragma("GCC error \"This method is deprecated, use SOUND_PCM_RATE_16000 instead.\"")
#define SOUND_RATE_13400    _Pragma("GCC error \"This method is deprecated, use SOUND_PCM_RATE_13400 instead.\"")
#define SOUND_RATE_11025    _Pragma("GCC error \"This method is deprecated, use SOUND_PCM_RATE_11025 instead.\"")
#define SOUND_RATE_8000     _Pragma("GCC error \"This method is deprecated, use SOUND_PCM_RATE_8000 instead.\"")


#define SND_isPlaying_PCM   _Pragma("GCC error \"This method is deprecated, use SND_PCM_isPlaying instead.\"")
#define SND_startPlay_PCM   _Pragma("GCC error \"This method is deprecated, use SND_PCM_startPlay instead.\"")
#define SND_stopPlay_PCM    _Pragma("GCC error \"This method is deprecated, use SND_PCM_stopPlay instead.\"")


extern const Z80Driver SND_PCM_driver;
/**
 *  \brief
 *      Variable rate sample player Z80 driver.<br>
 *      It can play a sample (8 bit signed) from 8 Khz up to 32 Khz rate.
 */
#define Z80_DRIVER_PCM (&SND_PCM_driver)
/**
 *  \brief
 *      Retrieve the Z80 PCM driver instance.
 *
 *  \return
 *      Pointer to the Z80Driver instance representing the Z80 PCM driver.
 */
const Z80Driver* SND_PCM_getDriver(void);
/**
 *  \brief
 *      Load the PCM sound driver.
 *
 *      Don't use this method directly, use #Z80_loadDriver(..) instead.
 */
void SND_PCM_loadDriver(const Z80DriverBoot boot);
/**
 *  \brief
 *      Unload the PCM sound driver.
 *
 *      Don't use this method directly, use #Z80_unloadDriver(..) instead.
 */
void SND_PCM_unloadDriver(void);

/**
 *  \brief
 *      Return play status (Single channel PCM player driver).
 *
 *  \return
 *      Return non zero if PCM player is currently playing a sample
 */
bool SND_PCM_isPlaying(void);
/**
 *  \brief
 *      Start playing a sample (Single channel PCM player driver).<br>
 *      If a sample was currently playing then it's stopped and the new sample is played instead.
 *
 *  \param sample
 *      Sample address, should be 256 bytes boundary aligned<br>
 *      SGDK automatically align resource as needed
 *  \param len
 *      Size of sample in bytes, should be a multiple of 256<br>
 *      SGDK automatically adjust resource size as needed
 *  \param rate
 *      Playback rate :<br>
 *      SOUND_PCM_RATE_32000 = 32 Khz (best quality but take lot of rom space)<br>
 *      SOUND_PCM_RATE_22050 = 22 Khz<br>
 *      SOUND_PCM_RATE_16000 = 16 Khz<br>
 *      SOUND_PCM_RATE_13400 = 13.4 Khz<br>
 *      SOUND_PCM_RATE_11025 = 11 Khz<br>
 *      SOUND_PCM_RATE_8000  = 8 Khz (worst quality but take less rom place)<br>
 *  \param pan
 *      Panning :<br>
 *      SOUND_PAN_NONE   = mute<br>
 *      SOUND_PAN_LEFT   = play on left speaker<br>
 *      SOUND_PAN_RIGHT  = play on right speaker<br>
 *      SOUND_PAN_CENTER = play on both speaker<br>
 *  \param loop
 *      Loop flag.<br>
 *      If TRUE then the sample will be played in loop (else sample is played only once).
 */
void SND_PCM_startPlay(const u8 *sample, const u32 len, const SoundPcmSampleRate rate, const SoundPanning pan, const bool loop);
/**
 *  \brief
 *      Stop playing (Single channel PCM player driver).<br>
 *      No effect if no sample was currently playing.
 */
void SND_PCM_stopPlay(void);

#endif // _SND_PCM_H_
