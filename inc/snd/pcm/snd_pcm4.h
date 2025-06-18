/**
 *  \file snd_pcm4.h
 *  \brief Z80_DRIVER_PCM4 sound driver API
 *  \author Stephane Dallongeville
 *  \date 08/2011
 *
 * This unit provides playback method for the <b>Z80_DRIVER_PCM4</b> sound driver.<br>
 * Sound driver description:<br>
 * 4 channels 8 bits signed sample driver with volume support.<br>
 * It can mix up to 4 samples (8 bit signed) at a fixed 16 Khz rate.<br>
 * with volume support (16 levels du to memory limitation).<br>
 * Address and size of samples have to be 256 bytes boundary.<br>
 * The driver does support "cutoff" when mixing so you can use true 8 bits samples :)<br>
 *<br>
 * Note that SGDK sound drivers may not fit your needs so it's important to know<br>
 * that some alternatives sound drivers exist (see SGDK readme.md for more info)
 */

#ifndef _SND_PCM4_H_
#define _SND_PCM4_H_

#define SND_isPlaying_4PCM_ENV      _Pragma("GCC error \"This method is deprecated, use SND_PCM4_isPlaying instead.\"")
#define SND_startPlay_4PCM_ENV      _Pragma("GCC error \"This method is deprecated, use SND_PCM4_startPlay instead.\"")
#define SND_stopPlay_4PCM_ENV       _Pragma("GCC error \"This method is deprecated, use SND_PCM4_stopPlay instead.\"")
#define SND_getVolume_4PCM_ENV      _Pragma("GCC error \"This method is deprecated, use SND_PCM4_getVolume instead.\"")
#define SND_setVolume_4PCM_ENV      _Pragma("GCC error \"This method is deprecated, use SND_PCM4_setVolume instead.\"")

#define SND_isPlaying_4PCM          _Pragma("GCC error \"This method is deprecated, use SND_PCM4_isPlaying instead.\"")
#define SND_startPlay_4PCM          _Pragma("GCC error \"This method is deprecated, use SND_PCM4_startPlay instead.\"")
#define SND_stopPlay_4PCM           _Pragma("GCC error \"This method is deprecated, use SND_PCM4_stopPlay instead.\"")
#define SND_getVolume_4PCM          _Pragma("GCC error \"This method is deprecated, use SND_PCM4_getVolume instead.\"")
#define SND_setVolume_4PCM          _Pragma("GCC error \"This method is deprecated, use SND_PCM4_setVolume instead.\"")

/**
 *  \brief
 *      Load the Z80_DRIVER_PCM4 sound driver.
 */
void SND_PCM4_loadDriver(const bool waitReady);

/**
 *  \brief
 *      Return play status of specified channel (4 channels PCM player driver).
 *
 *  \param channel_mask
 *      Channel(s) we want to retrieve play state.<br>
 *      SOUND_PCM_CH1_MSK    = channel 1<br>
 *      SOUND_PCM_CH2_MSK    = channel 2<br>
 *      SOUND_PCM_CH3_MSK    = channel 3<br>
 *      SOUND_PCM_CH4_MSK    = channel 4<br>
 *      <br>
 *      You can combine mask to retrieve state of severals channels at once:<br>
 *      <code>SND_PCM4_isPlaying(SOUND_PCM_CH1_MSK | SOUND_PCM_CH2_MSK)</code><br>
 *      will actually return play state for channel 1 and channel 2.
 *
 *  \return
 *      Return TRUE if specified channel(s) is(are) playing.
 */
bool SND_PCM4_isPlaying(const u16 channel_mask);
/**
 *  \brief
 *      Start playing a sample on specified channel (4 channels PCM player driver).<br>
 *      If a sample was currently playing on this channel then it's stopped and the new sample is played instead.
 *
 *  \param sample
 *      Sample address, should be 256 bytes boundary aligned<br>
 *      SGDK automatically align resource as needed
 *  \param len
 *      Size of sample in bytes, should be a multiple of 256<br>
 *      SGDK automatically adjust resource size as needed
 *  \param channel
 *      Channel where we want to play sample, accepted values are:<br>
 *      SOUND_PCM_CH_AUTO  = auto selection from current channel usage<br>
 *      SOUND_PCM_CH1      = channel 1<br>
 *      SOUND_PCM_CH2      = channel 2<br>
 *      SOUND_PCM_CH3      = channel 3<br>
 *      SOUND_PCM_CH4      = channel 4<br>
 *  \param loop
 *      Loop flag.<br>
 *      If TRUE then the sample will be played in loop (else sample is played only once).
 */
void SND_PCM4_startPlay(const u8 *sample, const u32 len, const SoundPCMChannel channel, const bool loop);
/**
 *  \brief
 *      Stop playing the specified channel (4 channels PCM player driver).<br>
 *      No effect if no sample was currently playing on this channel.
 *
 *  \param channel
 *      Channel we want to stop, accepted values are:<br>
 *      SOUND_PCM_CH1    = channel 1<br>
 *      SOUND_PCM_CH2    = channel 2<br>
 *      SOUND_PCM_CH3    = channel 3<br>
 *      SOUND_PCM_CH4    = channel 4<br>
 */
void SND_PCM4_stopPlay(const SoundPCMChannel channel);
/**
 *  \brief
 *      Change envelop / volume of specified channel (4 channels PCM player driver).
 *
 *  \param channel
 *      Channel we want to set envelop, accepted values are:<br>
 *      SOUND_PCM_CH1    = channel 1<br>
 *      SOUND_PCM_CH2    = channel 2<br>
 *      SOUND_PCM_CH3    = channel 3<br>
 *      SOUND_PCM_CH4    = channel 4<br>
 *  \param volume
 *      Volume to set: 16 possible level from 0 (minimum) to 15 (maximum).
 */
void SND_PCM4_setVolume(const SoundPCMChannel channel, const u8 volume);
/**
 *  \brief
 *      Return envelop / volume level of specified channel (4 channels PCM player driver).
 *
 *  \param channel
 *      Channel we want to retrieve envelop level, accepted values are:<br>
 *      SOUND_PCM_CH1    = channel 1<br>
 *      SOUND_PCM_CH2    = channel 2<br>
 *      SOUND_PCM_CH3    = channel 3<br>
 *      SOUND_PCM_CH4    = channel 4<br>
 *  \return
 *      Envelop of specified channel.<br>
 *      The returned value is comprised between 0 (quiet) to 15 (loud).
 */
u8   SND_PCM4_getVolume(const SoundPCMChannel channel);

#endif // _SND_PCM4_H_
