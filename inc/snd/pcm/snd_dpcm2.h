/**
 *  \file SND_DPCM2.h
 *  \brief Z80_DRIVER_DPCM2 sound driver API
 *  \author Stephane Dallongeville
 *  \date 08/2011
 *
 * This unit provides playback method for the <b>Z80_DRIVER_DPCM2</b> sound driver.<br>
 * Sound driver description:<br>
 * 2 channels 4 bits DPCM sample driver, it can mix up to 2 DCPM samples at a fixed 22050 Hz Khz rate.<br>
 * Address and size of samples have to be 256 bytes boundary.<br>
 *<br>
 * Note that SGDK sound drivers may not fit your needs so it's important to know<br>
 * that some alternatives sound drivers exist (see SGDK readme.md for more info)
 */

#ifndef _SND_DPCM2_H_
#define _SND_DPCM2_H_

#define SND_isPlaying_2ADPCM    _Pragma("GCC error \"This method is deprecated, use SND_DPCM2_isPlaying instead.\"")
#define SND_startPlay_2ADPCM    _Pragma("GCC error \"This method is deprecated, use SND_DPCM2_startPlay instead.\"")
#define SND_stopPlay_2ADPCM     _Pragma("GCC error \"This method is deprecated, use SND_DPCM2_stopPlay instead.\"")

/**
 *  \brief
 *      Load the Z80_DRIVER_DPCM2 sound driver.
 *
 *      Don't use this method directly, use #Z80_loadDriver(..) instead.
 */
void SND_DPCM2_loadDriver(const bool waitReady);
/**
 *  \brief
 *      Unload the Z80_DRIVER_DPCM2 sound driver.
 *
 *      Don't use this method directly, use #Z80_unloadDriver(..) instead.
 */
void SND_DPCM2_unloadDriver(void);

/**
 *  \brief
 *      Return play status of specified channel (2 channels ADPCM player driver).
 *
 *  \param channel_mask
 *      Channel(s) we want to retrieve play state.<br>
 *      SOUND_PCM_CH1_MSK    = channel 1<br>
 *      SOUND_PCM_CH2_MSK    = channel 2<br>
 *      <br>
 *      You can combine mask to retrieve state of severals channels at once:<br>
 *      <code>isPlaying_2ADPCM(SOUND_PCM_CH1_MSK | SOUND_PCM_CH2_MSK)</code><br>
 *      will actually return play state for channel 1 and channel 2.
 *
 *  \return
 *      Return TRUE if specified channel(s) is(are) playing.
 */
bool SND_DPCM2_isPlaying(const u16 channel_mask);
/**
 *  \brief
 *      Start playing a sample on specified channel (2 channels ADPCM player driver).<br>
 *      If a sample was currently playing on this channel then it's stopped and the new sample is played instead.
 *
 *  \param sample
 *      Sample address, should be 128 bytes boundary aligned<br>
 *      SGDK automatically align resource as needed
 *  \param len
 *      Size of sample in bytes, should be a multiple of 128<br>
 *      SGDK automatically adjust resource size as needed
 *  \param channel
 *      Channel where we want to play sample, accepted values are:<br>
 *      SOUND_PCM_CH_AUTO  = auto selection from current channel usage<br>
 *      SOUND_PCM_CH1      = channel 1<br>
 *      SOUND_PCM_CH2      = channel 2<br>
 *  \param loop
 *      Loop flag.<br>
 *      If TRUE then the sample will be played in loop (else sample is played only once).
 */
void SND_DPCM2_startPlay(const u8 *sample, const u32 len, const SoundPCMChannel channel, const bool loop);
/**
 *  \brief
 *      Stop playing the specified channel (2 channels ADPCM player driver).<br>
 *      No effect if no sample was currently playing on this channel.
 *
 *  \param channel
 *      Channel we want to stop, accepted values are:<br>
 *      SOUND_PCM_CH1    = channel 1<br>
 *      SOUND_PCM_CH2    = channel 2<br>
 */
void SND_DPCM2_stopPlay(const SoundPCMChannel channel);

#endif // _SND_DPCM2_H_
