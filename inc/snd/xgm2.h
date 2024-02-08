/**
 *  \file xgm2.h
 *  \brief XGM2 sound driver
 *  \author Stephane Dallongeville
 *  \date 04/2023
 *
 * This unit provides methods to use the XGM2 (eXtended Genesis Music) sound driver.<br>
 * It takes VGM (or XGM2) file as input to play music.<br>
 * It supports 3 PCM channels at either 13.3 Khz or 6.65 Khz and envelop control for both FM and PSG.<br>
 * It allows to play SFX through PCM with 16 level of priority.<br>
 * The driver supports renforced protection against DMA contention.
 */

#ifndef _XGM2_H_
#define _XGM2_H_


/**
 *  \brief
 *      Load the XGM2 sound driver.
 *
 *      Don't use this method directly, use #Z80_loadDriver(..) instead.
 */
void XGM2_loadDriver(bool waitReady);
/**
 *  \brief
 *      Unload the XGM2 sound driver.
 *
 *      Don't use this method directly, use #Z80_unloadDriver(..) instead.
 */
void XGM2_unloadDriver(void);

/**
 * \brief
 *      Returns play music state.
 */
bool XGM2_isPlaying(void);

/**
 *  \brief
 *      Load the specified XGM2 music blob (prepare for play, useful for multi tracks XGM2 music)
 *
 *  \param song
 *      XGM2 music blob address
 *
 *  \see XGM2_playTrack
 *  \see XGM2_stop
 *  \see XGM2_load_FAR
 */
void XGM2_load(const u8 *song);
/**
 *  \brief
 *      Same as #XGM2_load(..) except it supports access through bank switch
 *
 *  \param song
 *      XGM2 music blob address
 *  \param len
 *      XGM2 music blob size (in byte)
 *
 *  \see XGM2_playTrack
 *  \see XGM2_stop
 *  \see XGM2_load
 */
void XGM2_load_FAR(const u8 *song, const u32 len);
/**
 *  \brief
 *      Start playing the specified track (need to call XGM2_load(..) first)
 *
 *  \param track
 *      track index (for multi track XGM2 music blob)
 *
 *  \see XGM2_load
 *  \see XGM2_load_FAR
 *  \see XGM2_stop
 */
void XGM2_playTrack(const u16 track);
/**
 *  \brief
 *      Start playing the specified XGM2 music blob (fast play for single track XGM2 music)
 *
 *  \param song
 *      XGM2 track address.
 *
 *  \see XGM2_stop
 *  \see XGM2_play_FAR
 *  \see XGM2_load
 */
void XGM2_play(const u8* song);
/**
 *  \brief
 *      Same as #XGM2_play(..) except it supports music accessible through bank switch
 *
 *  \param song
 *      XGM2 track address.
 *  \param len
 *      XGM2 track size (in byte)
 *
 *  \see XGM2_stop
 *  \see XGM2_play
 *  \see XGM2_load_FAR
 */
void XGM2_play_FAR(const u8* song, const u32 len);

/**
 *  \brief
 *      Stop playing music (cannot be resumed).<br>
 *
 *  \see XGM2_pause
 *  \see XGM2_resume
 *  \see XGM2_play
 *  \see XGM2_playTrack
 */
void XGM2_stop(void);
/**
 * \brief
 *      Pause playing music, music can be resumed by calling #XGM2_resume().<br>
 *      Note that due to the nature of the music chip (FM synthesis), resume play operation will never be perfect
 *      and some notes will miss until next key-on event occurs.
 *
 *  \see XGM2_resume
 *  \see XGM2_stop
 */
void XGM2_pause(void);
/**
 * \brief
 *      Resume playing music after pausing with XGM2_stop().<br>
 *      Note that due to the nature of the music chip (FM synthesis), resume play operation will never be perfect
 *      and some notes will miss until next key-on event occurs.
 *
 *  \see XGM2_pause
 *  \see XGM2_stop
 */
void XGM2_resume(void);

/**
 *  \brief
 *      Return play status of specified PCM channel
 *
 *  \param channel_mask
 *      Channel(s) we want to retrieve play state.<br>
 *      SOUND_PCM_CH1_MSK    = channel 1<br>
 *      SOUND_PCM_CH2_MSK    = channel 2<br>
 *      SOUND_PCM_CH3_MSK    = channel 3<br>
 *      <br>
 *      You can combine mask to retrieve state of severals channels at once:<br>
 *      <code>isPlayingPCM(SOUND_PCM_CH1_MSK | SOUND_PCM_CH2_MSK)</code><br>
 *      will actually return play state for channel 1 and channel 2.
 *
 *  \return
 *      Return the channel mask of current playing channel(s).<br>
 *      For instance it returns (SOUND_PCM_CH1_MSK | SOUND_PCM_CH3_MSK) if channels 1 and 3 are currently playing.
 *
 *  \see XGM2_playPCM
 *  \see XGM2_stopPCM
 */
u8 XGM2_isPlayingPCM(const u16 channel_mask);
/**
 *  \brief
 *      Play a PCM sample on specified channel (XGM2 music player driver).<br>
 *      The method use a default priority value of 6 which is below the minimum music PCM priority (7)
 *
 *  \param sample
 *      Sample address, should be 256 bytes boundary aligned<br>
 *      (SGDK automatically align WAV resource as needed)
 *  \param len
 *      Size of sample in bytes, should be a multiple of 256<br>
 *      (SGDK automatically adjust WAV resource size as needed)
 *  \param channel
 *      Channel to use to play sample.<br>
 *      SOUND_PCM_CH_AUTO = auto selection from current channel usage<br>
 *      SOUND_PCM_CH1     = channel 1 (usually used by music)<br>
 *      SOUND_PCM_CH2     = channel 2<br>
 *      SOUND_PCM_CH3     = channel 3<br>
 *  \return FALSE if there is no channel available to play the PCM, TRUE otherwise
 *
 *  \see XGM2_playPCMEx
 *  \see XGM2_stopPCM
 *  \see XGM2_isPlayingPCM
 */
bool XGM2_playPCM(const u8 *sample, const u32 len, const SoundPCMChannel channel);
/**
 *  \brief
 *      Play a PCM sample on specified channel (XGM2 music player driver).<br>
 *      If a sample was currently playing on this channel then priority of the newer sample should be are compared then it's stopped and the new sample is played instead.<br>
 *      Note that music may use the first PCM channel so it's better to use channel 2 to 4 for SFX.
 *
 *  \param sample
 *      Sample address, should be 256 bytes boundary aligned<br>
 *      (SGDK automatically align WAV resource as needed)
 *  \param len
 *      Size of sample in bytes, should be a multiple of 256<br>
 *      (SGDK automatically adjust WAV resource size as needed)
 *  \param channel
 *      Channel to use to play sample.<br>
 *      SOUND_PCM_CH_AUTO = auto selection from current channel usage<br>
 *      SOUND_PCM_CH1     = channel 1 (usually used by music)<br>
 *      SOUND_PCM_CH2     = channel 2<br>
 *      SOUND_PCM_CH3     = channel 3<br>
 *  \param priority
 *      Value should go from 0 to 15 where 0 is lowest priority and 15 the highest one (music PCM priority can either be set to 7 or 15).<br>
 *      If the channel was already playing the priority is used to determine if the new SFX should replace the current one (new priority >= old priority).
 *  \param halfRate
 *      Set to TRUE to play the sample at half rate (6.65 Khz) instead of default 13.3 Khz
 *  \param loop
 *      Set to TRUE to enable looping sample play
 *  \return FALSE if there is no channel available to play the PCM, TRUE otherwise
 *
 *  \see XGM2_playPCM
 *  \see XGM2_stopPCM
 *  \see XGM2_isPlayingPCM
 */
bool XGM2_playPCMEx(const u8 *sample, const u32 len, const SoundPCMChannel channel, const u8 priority, const bool halfRate, const bool loop);
/**
 *  \brief
 *      Stop play PCM on specified channel (XGM2 music player driver).<br>
 *      No effect if no sample was currently playing on this channel.
 *
 *  \param channel
 *      Channel we want to stop.<br>
 *      SOUND_PCM_CH1 = channel 1<br>
 *      SOUND_PCM_CH2 = channel 2<br>
 *      SOUND_PCM_CH3 = channel 3<br>
 *
 *  \see XGM2_playPCM
 *  \see XGM2_isPlayingPCM
 */
void XGM2_stopPCM(const SoundPCMChannel channel);

/**
 *  \return
 *      TRUE if currently processing a volume fade effect, FALSE otherwise.
 */
bool XGM2_isProcessingFade(void);
/**
 *  \brief
 *      Process music volume "fade-in" effect, must be called right after a "start play" or "resume" command.<br>
 *      Gradually increase FM and PSG volume starting from 0 up to the current volume levels.
 *
 *  \param numFrame
 *      Duration of music fade-in effect in number of frame.
 */
void XGM2_fadeIn(const u16 numFrame);
/**
 *  \brief
 *      Process music volume "fade-out" effect.<br>
 *      Gradually decrease FM and PSG volume starting from current levels down to 0.
 *
 *  \param numFrame
 *      Duration of music fade-out effect in number of frame.
 */
void XGM2_fadeOut(const u16 numFrame);
/**
 *  \brief
 *      Process music volume "fade-out" effect and stop.<br>
 *      Gradually decrease FM and PSG volume starting from current levels down to 0, then issue a 'stop play' command.
 *
 *  \param numFrame
 *      Duration of music fade-out effect in number of frame.
 */
void XGM2_fadeOutAndStop(const u16 numFrame);
/**
 *  \brief
 *      Process music volume "fade-out" effect and pause.<br>
 *      Gradually decrease FM and PSG volume starting from current levels down to 0, then issue a 'stop play' command.
 *
 *  \param numFrame
 *      Duration of music fade-out effect in number of frame.
 */
void XGM2_fadeOutAndPause(const u16 numFrame);
/**
 *  \brief
 *      Process a specific music volume "fade" effect.<br>
 *      Gradually change FM and PSG volume starting from current levels to the specified ones.
 *
 *  \param fmVolume
 *      FM volume to reach at the end of fade effect.
 *  \param psgVolume
 *      PSG volume to reach at the end of fade effect.
 *  \param numFrame
 *      Duration of music fade effect in number of frame.
 */
void XGM2_fadeTo(const u16 fmVolume, const u16 psgVolume, const u16 numFrame);

/**
 *  \brief
 *      Set the loop number for music with loop command.<br>
 *      Default value is -1 for pseudo unfinite (255) loops plays.
 *      A value of 0 means single play without any loop, 1 = single play + 1 loop...
 */
void XGM2_setLoopNumber(const s8 value);

/**
 *  \brief
 *      Return the elapsed play time since the last #XGM2_play(..) call.<br>
 *      The returned value is in music frame which can be 50/60 per second depending the base music play rate (NTSC/PAL).
 *
 *  \see XGM2_play(..)
 *  \see XGM2_playTrack(..)
 *  \see XGM2_setMusicTempo(..)
 */
u32 XGM2_getElapsed(void);

/**
 *  \brief
 *      Get the current music tempo (in tick per second).<br>
 *      Default value is 60 or 50 depending the system is NTSC or PAL.<br>
 *      This method is meaningful only if you use the automatic music sync mode (see XGM2_setManualSync() method)
 *      which is the default mode.<br>
 *      Note that using specific tempo (not 60 or 50) will affect performance of DMA contention and external command parsing
 *      so it's recommended to stand with default one.
 *
 *  \see XGM2_setMusicTempo
 */
u16 XGM2_getMusicTempo(void);
/**
 *  \brief
 *      Set the music tempo (in tick per second).<br>
 *      Default value is 60 or 50 depending the system is NTSC or PAL.
 *      This method is meaningful only if you use the automatic music sync mode (see XGM2_setManualSync() method)
 *      which is the default mode.<br>
 *      Note that using specific tempo (not 60 or 50) can completely distord FM instruments sound and affect
 *      performance of DMA contention and external command parsing so it's recommended to stand with default one.
 *
 *  \see XGM2_getMusicTempo
 */
void XGM2_setMusicTempo(const u16 value);

/**
 *  \brief
 *      Set the volume level for the FM music part
 *
 *  \param value
 *      volume level (0 to 100)
 *
 *  \see XGM2_setPSGVolume
 */
void XGM2_setFMVolume(const u16 value);
/**
 *  \brief
 *      Set the <i>volume</i> level for the PSG music part
 *
 *  \param value
 *      volume level (0 to 100)
 *
 *  \see XGM2_setFMVolume
 */
void XGM2_setPSGVolume(const u16 value);

/**
 * \brief
 *      Returns TRUE if specified xgm2 use PAL timing.
 */
bool XGM2_isPAL(const u8 *xgm2);

/**
 *  \brief
 *      Returns an estimation of the Z80 CPU load (XGM2 driver).
 *
 *  \param mean
 *      if set to TRUE then return a mean load computed on the last 8 frames otherwise return instant last frame load
 */
u16 XGM2_getCPULoad(const bool mean);
/**
 *  \brief
 *      Returns an estimation of the Z80 CPU time spent in waiting for DMA completion (see #Z80_setBusProtection(bool) method).
*
 *  \param mean
 *      if set to TRUE then return a mean wait computed on the last 8 frames otherwise return instant last frame DMA wait
 */
u16 XGM2_getDMAWaitTime(const bool mean);

/**
 *  \brief
 *      Returns the internal frame counter (v-int process number).<br>
 *      Debug function to verify the driver is working optimally.
 */
u16 XGM2_getDebugFrameCounter(void);
/**
 *  \brief
 *      Returns the real PCM playback rate (XGM2 driver).<br>
 *      Debug function to verify the driver is working optimally.
 */
u16 XGM2_getDebugPCMRate(void);
/**
 *  \brief
 *      Returns the number of missed frames since last startPlay command.<br>
 *      Debug function to verify the driver is working optimally.
 */
u8 XGM2_getDebugMissedFrames(void);
/**
 *  \brief
 *      Returns the ending process time in number of sample for the specified v-int process (v-int process number).<br>
 *      Debug function to verify the driver is working optimally.
 */
u8 XGM2_getDebugProcessDuration(const u16 ind);


#endif // _XGM2_H_
