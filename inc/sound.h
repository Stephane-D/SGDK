/**
 *  \file sound.h
 *  \brief Audio / Sound stuff
 *  \author Stephane Dallongeville
 *  \date 08/2011
 *
 * This unit provides advanced sound playback methods through differents Z80 drivers.<br>
 *<br>
 * <b>Z80_DRIVER_PCM</b><br>
 * Single channel 8 bits signed sample driver.<br>
 * It can play a sample (8 bit signed) from 8 Khz up to 32 Khz rate.<br>
 *<br>
 * <b>Z80_DRIVER_2ADPCM</b><br>
 * 2 channels 4 bits ADPCM sample driver.<br>
 * It can mix up to 2 ADCPM samples at a fixed 22050 Hz Khz rate.<br>
 * Address and size of samples have to be 256 bytes boundary.<br>
 *<br>
 * <b>Z80_DRIVER_4PCM</b><br>
 * 4 channels 8 bits signed sample driver with volume support.<br>
 * It can mix up to 4 samples (8 bit signed) at a fixed 16 Khz rate.<br>
 * with volume support (16 levels du to memory limitation).<br>
 * Address and size of samples have to be 256 bytes boundary.<br>
 * The driver does support "cutoff" when mixing so you can use true 8 bits samples :)<br>
 *<br>
 * <b>Z80_DRIVER_XGM</b><br>
 * eXtended VGM music player driver.<br>
 * <i>Moved to xgm.h unit</i>
 */

#ifndef _SOUND_H_
#define _SOUND_H_


/**
 *  \brief
 *      Auto select PCM channel to use.
 */
#define SOUND_PCM_CH_AUTO   0xFF

/**
 *  \brief
 *      PCM channel 1.
 */
#define SOUND_PCM_CH1       Z80_DRV_CH0_SFT
/**
 *  \brief
 *      PCM channel 2.
 */
#define SOUND_PCM_CH2       Z80_DRV_CH1_SFT
/**
 *  \brief
 *      PCM channel 3.
 */
#define SOUND_PCM_CH3       Z80_DRV_CH2_SFT
/**
 *  \brief
 *      PCM channel 4.
 */
#define SOUND_PCM_CH4       Z80_DRV_CH3_SFT

/**
 *  \brief
 *      PCM channel 1 selection mask.
 */
#define SOUND_PCM_CH1_MSK   Z80_DRV_CH0
/**
 *  \brief
 *      PCM channel 2 selection mask.
 */
#define SOUND_PCM_CH2_MSK   Z80_DRV_CH1
/**
 *  \brief
 *      PCM channel 3 selection mask.
 */
#define SOUND_PCM_CH3_MSK   Z80_DRV_CH2
/**
 *  \brief
 *      PCM channel 4 selection mask.
 */
#define SOUND_PCM_CH4_MSK   Z80_DRV_CH3

/**
 *  \brief
 *      PCM sample rate set to 32 Khz.<br>
 *      Best quality but take lot of rom space.
 */
#define SOUND_RATE_32000    0
/**
 *  \brief
 *      PCM sample rate set to 22050 Hz.<br>
 *      Best quality but take lot of rom space.
 */
#define SOUND_RATE_22050    1
/**
 *  \brief
 *      PCM sample rate set to 16 Khz.<br>
 */
#define SOUND_RATE_16000    2
/**
 *  \brief
 *      PCM sample rate set to 13400 Hz.<br>
 *      Good compromise for rom space and quality.
 */
#define SOUND_RATE_13400    3
/**
 *  \brief
 *      PCM sample rate set to 11025 Hz.<br>
 */
#define SOUND_RATE_11025    4
/**
 *  \brief
 *      PCM sample rate set to 8 Khz.<br>
 *      Worst quality but take less rom place.
 */
#define SOUND_RATE_8000     5

/**
 *  \brief
 *      Left speaker panning.
 */
#define SOUND_PAN_LEFT      0x80
/**
 *  \brief
 *      Right speaker panning.
 */
#define SOUND_PAN_RIGHT     0x40
/**
 *  \brief
 *      Center (laft and right) speaker panning.
 */
#define SOUND_PAN_CENTER    0xC0


/**
 *  \brief
 *      Return play status (Single channel PCM player driver).
 *
 *  \return
 *      Return non zero if PCM player is currently playing a sample
 */
u8   SND_isPlaying_PCM();
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
 *      #SOUND_RATE_32000 = 32 Khz (best quality but take lot of rom space)<br>
 *      #SOUND_RATE_22050 = 22 Khz<br>
 *      #SOUND_RATE_16000 = 16 Khz<br>
 *      #SOUND_RATE_13400 = 13.4 Khz<br>
 *      #SOUND_RATE_11025 = 11 Khz<br>
 *      #SOUND_RATE_8000  = 8 Khz (worst quality but take less rom place)<br>
 *  \param pan
 *      Panning :<br>
 *      #SOUND_PAN_LEFT   = play on left speaker<br>
 *      #SOUND_PAN_RIGHT  = play on right speaker<br>
 *      #SOUND_PAN_CENTER = play on both speaker<br>
 *  \param loop
 *      Loop flag.<br>
 *      If non zero then the sample will be played in loop (else sample is played only once).
 */
void SND_startPlay_PCM(const u8 *sample, const u32 len, const u8 rate, const u8 pan, const u8 loop);
/**
 *  \brief
 *      Stop playing (Single channel PCM player driver).<br>
 *      No effect if no sample was currently playing.
 */
void SND_stopPlay_PCM();


// Z80_DRIVER_2ADPCM

/**
 *  \brief
 *      Return play status of specified channel (2 channels ADPCM player driver).
 *
 *  \param channel_mask
 *      Channel(s) we want to retrieve play state.<br>
 *      #SOUND_PCM_CH1_MSK    = channel 1<br>
 *      #SOUND_PCM_CH2_MSK    = channel 2<br>
 *      <br>
 *      You can combine mask to retrieve state of severals channels at once:<br>
 *      <code>isPlaying_2ADPCM(SOUND_PCM_CH1_MSK | SOUND_PCM_CH2_MSK)</code><br>
 *      will actually return play state for channel 1 and channel 2.
 *
 *  \return
 *      Return non zero if specified channel(s) is(are) playing.
 */
u8 SND_isPlaying_2ADPCM(const u16 channel_mask);
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
 *      Channel where we want to play sample.<br>
 *      #SOUND_PCM_CH_AUTO  = auto selection from current channel usage<br>
 *      #SOUND_PCM_CH1      = channel 1<br>
 *      #SOUND_PCM_CH2      = channel 2<br>
 *  \param loop
 *      Loop flag.<br>
 *      If non zero then the sample will be played in loop (else sample is played only once).
 */
void SND_startPlay_2ADPCM(const u8 *sample, const u32 len, const u16 channel, const u8 loop);
/**
 *  \brief
 *      Stop playing the specified channel (2 channels ADPCM player driver).<br>
 *      No effect if no sample was currently playing on this channel.
 *
 *  \param channel
 *      Channel we want to stop.<br>
 *      #SOUND_PCM_CH1    = channel 1<br>
 *      #SOUND_PCM_CH2    = channel 2<br>
 */
void SND_stopPlay_2ADPCM(const u16 channel);


// Z80_DRIVER_4PCM

/**
 *  \brief
 *      Return play status of specified channel (4 channels PCM player driver).
 *
 *  \param channel_mask
 *      Channel(s) we want to retrieve play state.<br>
 *      #SOUND_PCM_CH1_MSK    = channel 1<br>
 *      #SOUND_PCM_CH2_MSK    = channel 2<br>
 *      #SOUND_PCM_CH3_MSK    = channel 3<br>
 *      #SOUND_PCM_CH4_MSK    = channel 4<br>
 *      <br>
 *      You can combine mask to retrieve state of severals channels at once:<br>
 *      <code>isPlaying_2ADPCM(SOUND_PCM_CH1_MSK | SOUND_PCM_CH2_MSK)</code><br>
 *      will actually return play state for channel 1 and channel 2.
 *
 *  \return
 *      Return non zero if specified channel(s) is(are) playing.
 */
u8   SND_isPlaying_4PCM(const u16 channel_mask);
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
 *      Channel where we want to play sample.<br>
 *      #SOUND_PCM_CH_AUTO  = auto selection from current channel usage<br>
 *      #SOUND_PCM_CH1      = channel 1<br>
 *      #SOUND_PCM_CH2      = channel 2<br>
 *      #SOUND_PCM_CH3      = channel 3<br>
 *      #SOUND_PCM_CH4      = channel 4<br>
 *  \param loop
 *      Loop flag.<br>
 *      If non zero then the sample will be played in loop (else sample is played only once).
 */
void SND_startPlay_4PCM(const u8 *sample, const u32 len, const u16 channel, const u8 loop);
/**
 *  \brief
 *      Stop playing the specified channel (4 channels PCM player driver).<br>
 *      No effect if no sample was currently playing on this channel.
 *
 *  \param channel
 *      Channel we want to stop.<br>
 *      #SOUND_PCM_CH1    = channel 1<br>
 *      #SOUND_PCM_CH2    = channel 2<br>
 *      #SOUND_PCM_CH3    = channel 3<br>
 *      #SOUND_PCM_CH4    = channel 4<br>
 */
void SND_stopPlay_4PCM(const u16 channel);
/**
 *  \brief
 *      Change envelop / volume of specified channel (4 channels PCM player driver).
 *
 *  \param channel
 *      Channel we want to set envelop.<br>
 *      #SOUND_PCM_CH1    = channel 1<br>
 *      #SOUND_PCM_CH2    = channel 2<br>
 *      #SOUND_PCM_CH3    = channel 3<br>
 *      #SOUND_PCM_CH4    = channel 4<br>
 *  \param volume
 *      Volume to set : 16 possible level from 0 (minimum) to 15 (maximum).
 */
void SND_setVolume_4PCM(const u16 channel, const u8 volume);
/**
 *  \brief
 *      Return envelop / volume level of specified channel (4 channels PCM player driver).
 *
 *  \param channel
 *      Channel we want to retrieve envelop level.<br>
 *      #SOUND_PCM_CH1    = channel 1<br>
 *      #SOUND_PCM_CH2    = channel 2<br>
 *      #SOUND_PCM_CH3    = channel 3<br>
 *      #SOUND_PCM_CH4    = channel 4<br>
 *  \return
 *      Envelop of specified channel.<br>
 *      The returned value is comprised between 0 (quiet) to 15 (loud).
 */
u8   SND_getVolume_4PCM(const u16 channel);


// just to preserve backward compatibility
#define SND_isPlaying_4PCM_ENV      SND_isPlaying_4PCM
#define SND_startPlay_4PCM_ENV      SND_startPlay_4PCM
#define SND_stopPlay_4PCM_ENV       SND_stopPlay_4PCM
#define SND_getVolume_4PCM_ENV      SND_getVolume_4PCM
#define SND_setVolume_4PCM_ENV      SND_setVolume_4PCM



// Z80_DRIVER_XGM

/**
 * \deprecated use XGM_isPlaying() instead
 */
u8 SND_isPlaying_XGM();
/**
 * \deprecated use XGM_startPlay(..) instead
 */
void SND_startPlay_XGM(const u8 *song);
/**
 * \deprecated use XGM_stopPlay(..) instead
 */
void SND_stopPlay_XGM();
/**
 * \deprecated use XGM_pausePlay(..) instead
 */
void SND_pausePlay_XGM();
/**
 * \deprecated use XGM_resumePlay(..) instead
 */
void SND_resumePlay_XGM();

/**
 * \deprecated use XGM_isPlayingPCM(..) instead
 */
u8 SND_isPlayingPCM_XGM(const u16 channel_mask);
/**
 * \deprecated use XGM_setPCM(..) instead
 */
void SND_setPCM_XGM(const u8 id, const u8 *sample, const u32 len);
/**
 * \deprecated use XGM_setPCMFast(..) instead
 */
void SND_setPCMFast_XGM(const u8 id, const u8 *sample, const u32 len);
/**
 * \deprecated use XGM_startPlayPCM(..) instead
 */
void SND_startPlayPCM_XGM(const u8 id, const u8 priority, const u16 channel);
/**
 * \deprecated use XGM_stopPlayPCM(..) instead
 */
void SND_stopPlayPCM_XGM(const u16 channel);

/**
 * \deprecated use XGM_getMusicTempo(..) instead
 */
u16 SND_getMusicTempo_XGM();
/**
 * \deprecated use XGM_setMusicTempo(..) instead
 */
void SND_setMusicTempo_XGM(u16 value);

/**
 * \deprecated use XGM_getManualSync(..) instead
 */
u16 SND_getManualSync_XGM();
/**
 * \deprecated use XGM_setManualSync(..) instead
 */
void SND_setManualSync_XGM(u16 value);
/**
 * \deprecated use XGM_nextFrame(..) instead
 */
#define SND_nextFrame_XGM()  SND_nextXFrame_XGM(1)
/**
 * \deprecated use XGM_nextXFrame(..) instead
 */
void SND_nextXFrame_XGM(u16 num);

/**
 * \deprecated use XGM_setLoopNumber(..) instead
 */
void SND_setLoopNumber_XGM(u8 value);

/**
 * \deprecated use XGM_set68KBUSProtection(..) instead
 */
void SND_set68KBUSProtection_XGM(u8 value);
/**
 * \deprecated use XGM_getForceDelayDMA(..) instead
 */
u16 SND_getForceDelayDMA_XGM();
/**
 * \deprecated use XGM_setForceDelayDMA(..) instead
 */
void SND_setForceDelayDMA_XGM(u16 value);
/**
 * \deprecated use XGM_getCPULoad(..) instead
 */
u32 SND_getCPULoad_XGM();


#endif // _SOUND_H_
