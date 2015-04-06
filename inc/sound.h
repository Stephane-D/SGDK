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
 * <b>Z80_DRIVER_4PCM_ENV</b><br>
 * 4 channels 8 bits signed sample driver with volume support.<br>
 * It can mix up to 4 samples (8 bit signed) at a fixed 16 Khz rate.<br>
 * with volume support (16 levels du to memory limitation).<br>
 * Address and size of samples have to be 256 bytes boundary.<br>
 * The driver does support "cutoff" when mixing so you can use true 8 bits samples :)<br>
 *<br>
 * <b>Z80_DRIVER_MVS</b><br>
 * MVS music player driver.<br>
 *<br>
 * <b>Z80_DRIVER_TFM</b><br>
 * TFM music player driver.<br>
 *<br>
 * <b>Z80_DRIVER_VGM</b><br>
 * VGM music player driver.<br>
 * It supports 1 PCM channel at a fixed 8 Khz and allows to play SFX through the named PCM channel.<br>
 * Written by Sigflup and kubilus1.
 *<br>
 * <b>Z80_DRIVER_XGM</b><br>
 * eXtended VGM music player driver.<br>
 * This driver takes VGM (or XGM) file as input to play music.<br>
 * It supports 4 PCM channels at a fixed 14 Khz and allows to play SFX through PCM with 16 level of priority.<br>
 * The driver is designed to avoid DMA contention when possible (depending CPU load).
 */

#ifndef _SOUND_H_
#define _SOUND_H_


/**
 *  \def SOUND_PCM_CH_AUTO
 *      Auto select PCM channel to use.
 */
#define SOUND_PCM_CH_AUTO   0x00

/**
 *  \def SOUND_PCM_CH1
 *      PCM channel 1.
 */
#define SOUND_PCM_CH1       Z80_DRV_CH0_SFT
/**
 *  \def SOUND_PCM_CH2
 *      PCM channel 2.
 */
#define SOUND_PCM_CH2       Z80_DRV_CH1_SFT
/**
 *  \def SOUND_PCM_CH3
 *      PCM channel 3.
 */
#define SOUND_PCM_CH3       Z80_DRV_CH2_SFT
/**
 *  \def SOUND_PCM_CH4
 *      PCM channel 4.
 */
#define SOUND_PCM_CH4       Z80_DRV_CH3_SFT

/**
 *  \def SOUND_PCM_CH1_MSK
 *      PCM channel 1 selection mask.
 */
#define SOUND_PCM_CH1_MSK   Z80_DRV_CH0
/**
 *  \def SOUND_PCM_CH2_MSK
 *      PCM channel 2 selection mask.
 */
#define SOUND_PCM_CH2_MSK   Z80_DRV_CH1
/**
 *  \def SOUND_PCM_CH3_MSK
 *      PCM channel 3 selection mask.
 */
#define SOUND_PCM_CH3_MSK   Z80_DRV_CH2
/**
 *  \def SOUND_PCM_CH4_MSK
 *      PCM channel 4 selection mask.
 */
#define SOUND_PCM_CH4_MSK   Z80_DRV_CH3

/**
 *  \def SOUND_RATE_32000
 *      PCM sample rate set to 32 Khz.<br>
 *      Best quality but take lot of rom space.
 */
#define SOUND_RATE_32000    0
/**
 *  \def SOUND_RATE_22050
 *      PCM sample rate set to 22050 Hz.<br>
 *      Best quality but take lot of rom space.
 */
#define SOUND_RATE_22050    1
/**
 *  \def SOUND_RATE_16000
 *      PCM sample rate set to 16 Khz.<br>
 */
#define SOUND_RATE_16000    2
/**
 *  \def SOUND_RATE_13400
 *      PCM sample rate set to 13400 Hz.<br>
 *      Good compromise for rom space and quality.
 */
#define SOUND_RATE_13400    3
/**
 *  \def SOUND_RATE_11025
 *      PCM sample rate set to 11025 Hz.<br>
 */
#define SOUND_RATE_11025    4
/**
 *  \def SOUND_RATE_8000
 *      PCM sample rate set to 8 Khz.<br>
 *      Worst quality but take less rom place.
 */
#define SOUND_RATE_8000     5

/**
 *  \def SOUND_PAN_LEFT
 *      Left speaker panning.
 */
#define SOUND_PAN_LEFT      0x80
/**
 *  \def SOUND_PAN_RIGHT
 *      Right speaker panning.
 */
#define SOUND_PAN_RIGHT     0x40
/**
 *  \def SOUND_PAN_CENTER
 *      Center (laft and right) speaker panning.
 */
#define SOUND_PAN_CENTER    0xC0


// Z80_DRIVER_PCM

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
 *      #SOUND_PCM_CH1    = channel 1<br>
 *      #SOUND_PCM_CH2    = channel 2<br>
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


// Z80_DRIVER_4PCM_ENV

/**
 *  \brief
 *      Return play status of specified channel (4 channels PCM ENV player driver).
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
u8   SND_isPlaying_4PCM_ENV(const u16 channel_mask);
/**
 *  \brief
 *      Start playing a sample on specified channel (4 channels PCM ENV player driver).<br>
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
 *      #SOUND_PCM_CH1    = channel 1<br>
 *      #SOUND_PCM_CH2    = channel 2<br>
 *      #SOUND_PCM_CH3    = channel 3<br>
 *      #SOUND_PCM_CH4    = channel 4<br>
 *  \param loop
 *      Loop flag.<br>
 *      If non zero then the sample will be played in loop (else sample is played only once).
 */
void SND_startPlay_4PCM_ENV(const u8 *sample, const u32 len, const u16 channel, const u8 loop);
/**
 *  \brief
 *      Stop playing the specified channel (4 channels PCM ENV player driver).<br>
 *      No effect if no sample was currently playing on this channel.
 *
 *  \param channel
 *      Channel we want to stop.<br>
 *      #SOUND_PCM_CH1    = channel 1<br>
 *      #SOUND_PCM_CH2    = channel 2<br>
 *      #SOUND_PCM_CH3    = channel 3<br>
 *      #SOUND_PCM_CH4    = channel 4<br>
 */
void SND_stopPlay_4PCM_ENV(const u16 channel);
/**
 *  \brief
 *      Change envelop / volume of specified channel (4 channels PCM ENV player driver).
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
void SND_setVolume_4PCM_ENV(const u16 channel, const u8 volume);
/**
 *  \brief
 *      Return envelop / volume level of specified channel (4 channels PCM ENV player driver).
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
u8   SND_getVolume_4PCM_ENV(const u16 channel);


// Z80_DRIVER_MVS

/**
 *  \brief
 *      Return play status for FM (MVS music player driver).
 *
 *  \return
 *      Return non zero if FM is currently playing.
 */
u8 SND_isPlaying_MVS();
/**
 *  \brief
 *      Start playing the specified FM track (MVS music player driver).
 *
 *  \param music
 *      FM track address.
 *  \param loop
 *      Loop flag.<br>
 *      If non zero then the sample will be played in loop (else sample is played only once).
 */
void SND_startPlay_MVS(const u8 *music, const u8 loop);
/**
 *  \brief
 *      Stop playing FM music (MVS music player driver).
 */
void SND_stopPlay_MVS();
/**
 *  \brief
 *      Set the FM music tempo (MVS music player driver).
 */
void SND_setTempo_MVS(u8 tempo);

/**
 *  \brief
 *      Start playing the specified DAC sample (MVS music player driver).
 *
 *  \param sample
 *      DAC sample address.
 *  \param size
 *      Sample size.<br>
 *      Maximum sample size is 65536 byte.
 */
void SND_startDAC_MVS(const u8 *sample, u16 size);
/**
 *  \brief
 *      Stop playing DAC sample (MVS music player driver).
 */
void SND_stopDAC_MVS();

/**
 *  \brief
 *      Return play status for PSG (MVS music player driver).
 *
 *  \return
 *      Return non zero if PSG is currently playing.
 */
u8 SND_isPlayingPSG_MVS();
/**
 *  \brief
 *      Start playing the specified PSG music (MVS music player driver).
 *
 *  \param music
 *      PSG music address.
 */
void SND_startPSG_MVS(const u8 *music);
/**
 *  \brief
 *      Stop playing PSG music.
 */
void SND_stopPSG_MVS();
/**
 *  \brief
 *      Enable the specified PSG channel.
 *
 *  \param chan
 *      PSG channel to enable.
 */
void SND_enablePSG_MVS(u8 chan);
/**
 *  \brief
 *      Disable the specified PSG channel.
 *
 *  \param chan
 *      PSG channel to disable.
 */
void SND_disablePSG_MVS(u8 chan);


// Z80_DRIVER_TFM

/**
 *  \brief
 *      Start playing the specified TFM track (TFM music player driver).
 *
 *  \param song
 *      TFM track address.
 */
void SND_startPlay_TFM(const u8 *song);
/**
 *  \brief
 *      Stop playing music (TFM music player driver).
 */
void SND_stopPlay_TFM();


// Z80_DRIVER_VGM

/**
 * \brief
 *      Return play state of VGM driver.
 */
u8 SND_isPlaying_VGM();
/**
 *  \brief
 *      Start playing the specified VGM track (VGM music player driver).
 *
 *  \param song
 *      VGM track address.
 */
void SND_startPlay_VGM(const u8 *song);
/**
 *  \brief
 *      Stop playing music (VGM music player driver).
 */
void SND_stopPlay_VGM();
/**
 * \brief
 *      Resume playing music after stopping with SND_stopPlay_VGM.
 */
void SND_resumePlay_VGM();

/**
 * \brief
 *      Play a PCM sound effect while a VGM track is playing.
 */
void SND_playSfx_VGM(const u8 *sfx, u16 len);


// Z80_DRIVER_XGM

/**
 * \brief
 *      Returns play music state (XGM music player driver).
 */
u8 SND_isPlaying_XGM();
/**
 *  \brief
 *      Start playing the specified XGM track (XGM music player driver).
 *
 *  \param song
 *      XGM track address.
 */
void SND_startPlay_XGM(const u8 *song);
/**
 *  \brief
 *      Stop playing music (XGM music player driver).
 */
void SND_stopPlay_XGM();
/**
 * \brief
 *      Resume playing music after stopping with SND_stopPlay_XGM (XGM music player driver).
 */
void SND_resumePlay_XGM();

/**
 *  \brief
 *      Return play status of specified PCM channel (XGM music player driver).
 *
 *  \param channel_mask
 *      Channel(s) we want to retrieve play state.<br>
 *      #SOUND_PCM_CH1_MSK    = channel 1<br>
 *      #SOUND_PCM_CH2_MSK    = channel 2<br>
 *      #SOUND_PCM_CH3_MSK    = channel 3<br>
 *      #SOUND_PCM_CH4_MSK    = channel 4<br>
 *      <br>
 *      You can combine mask to retrieve state of severals channels at once:<br>
 *      <code>isPlayingPCM_XGM(SOUND_PCM_CH1_MSK | SOUND_PCM_CH2_MSK)</code><br>
 *      will actually return play state for channel 1 and channel 2.
 *
 *  \return
 *      Return non zero if specified channel(s) is(are) playing.
 */
u8 SND_isPlayingPCM_XGM(const u16 channel_mask);
/**
 *  \brief
 *      Declare a new PCM sample (maximum = 255) for the XGM music player driver.<br/>
 *      Sample id < 64 are reserved for music while others are used for SFX
 *      so if you want to declare a new SFX PCM sample use an id >= 64
 *
 *  \param id
 *      Sample id:<br/>
 *      value 0 is not allowed<br/>
 *      values from 1 to 63 are used for music
 *      values from 64 to 255 are used for SFX
 *  \param sample
 *      Sample address, should be 256 bytes boundary aligned<br>
 *      SGDK automatically align sample resource as needed
 *  \param len
 *      Size of sample in bytes, should be a multiple of 256<br>
 *      SGDK automatically adjust resource size as needed
 */
void SND_setPCM_XGM(const u8 id, const u8 *sample, const u32 len);
/**
 *  \brief
 *      Same as #SND_setPCM_XGM but fast version.<br/>
 *      This method assume that XGM driver is loaded and that 68000 has access to Z80 bus
 *
 *  \param id
 *      Sample id:<br/>
 *      value 0 is not allowed<br/>
 *      values from 1 to 63 are used for music
 *      values from 64 to 255 are used for SFX
 *  \param sample
 *      Sample address, should be 256 bytes boundary aligned<br>
 *      SGDK automatically align sample resource as needed
 *  \param len
 *      Size of sample in bytes, should be a multiple of 256<br>
 *      SGDK automatically adjust resource size as needed
 */
void SND_setPCMFast_XGM(const u8 id, const u8 *sample, const u32 len);
/**
 *  \brief
 *      Play a PCM sample on specified channel (XGM music player driver).<br>
 *      If a sample was currently playing on this channel then priority of the newer sample should be are compared tthen it's stopped and the new sample is played instead.
 *
 *  \param id
 *      Sample id (set #SND_setSample_XGM method)
 *  \param priority
 *      Value should go from 0 to 15 where 0 is lowest priority and 15 the highest one.<br/>
 *      If the channel was already playing the priority is used to determine if the new SFX should replace the current one (new priority >= old priority).
 *  \param channel
 *      Channel where we want to play sample.<br>
 *      #SOUND_PCM_CH1    = channel 1<br>
 *      #SOUND_PCM_CH2    = channel 2<br>
 *      #SOUND_PCM_CH3    = channel 3<br>
 *      #SOUND_PCM_CH4    = channel 4<br>
 */
void SND_startPlayPCM_XGM(const u8 id, const u8 priority, const u16 channel);
/**
 *  \brief
 *      Stop play PCM on specified channel (XGM music player driver).<br>
 *      No effect if no sample was currently playing on this channel.
 *
 *  \param channel
 *      Channel we want to stop.<br>
 *      #SOUND_PCM_CH1    = channel 1<br>
 *      #SOUND_PCM_CH2    = channel 2<br>
 *      #SOUND_PCM_CH3    = channel 3<br>
 *      #SOUND_PCM_CH4    = channel 4<br>
 */
void SND_stopPlayPCM_XGM(const u16 channel);

/**
 *  \brief
 *      Set temporary 68K BUS protection from Z80.<br>
 *      You should protect BUS Access during DMA and restore it after:<br>
 *      SND_set68KBUSProtection_XGM(TRUE);
 *      VDP_doVRamDMA(data, 0x1000, 0x100);
 *      SND_set68KBUSProtection_XGM(FALSE);
 *
 *      This way the XGM driver will *try* to avoid using 68K BUS during DMA to
 *      avoid execution interruption and so preserve PCM playback quality.
 *      Note that the success of the operation is not 100% garantee and can fails in some condition.
 */
void SND_set68KBUSProtection_XGM(u8 value);
/**
 *  \brief
 *      Returns an estimation of the Z80 CPU load (XGM driver).<br>
 *      It returns value between ~80% (minimum value) to >100% (if we have some frames late).<br>
 *      WARNING: You should call this method at each frame to get meaningful value !
 */
u16 SND_getCPULoad_XGM();


#endif // _SOUND_H_
