/**
 *  \file xgm.h
 *  \brief XGM sound driver
 *  \author Stephane Dallongeville
 *  \date 08/2015
 *
 * This unit provides methods to use the XGM (eXtended Genesis Music) sound driver.<br>
 * This driver takes VGM (or XGM) file as input to play music.<br>
 * It supports 4 PCM channels at a fixed 14 Khz and allows to play SFX through PCM with 16 level of priority.<br>
 * The driver is designed to avoid DMA contention when possible (depending CPU load).
 */

#ifndef _XGM_H_
#define _XGM_H_


/**
 * \brief
 *      Returns play music state (XGM music player driver).
 */
u8 XGM_isPlaying(void);
/**
 *  \brief
 *      Start playing the specified XGM track (XGM music player driver).
 *
 *  \param song
 *      XGM track address.
 *
 *  \see XGM_stopPlay
 *  \see XGM_pausePlay
 *  \see XGM_nextFrame
 *  \see XGM_startPlay_FAR
 */
void XGM_startPlay(const u8 *song);
/**
 *  \brief
 *      Same as #XGM_startPlay(..) except it supports music accessible through bank switch
 *
 *  \param song
 *      XGM track address.
 *  \param size
 *      XGM track size (in byte)
 *
 *  \see XGM_startPlay
 *  \see XGM_stopPlay
 *  \see XGM_pausePlay
 *  \see XGM_nextFrame
 */
void XGM_startPlay_FAR(const u8 *song, u32 size);

/**
 *  \brief
 *      Stop playing music (XGM music player driver).
 *
 *  \see XGM_pausePlay
 */
void XGM_stopPlay(void);
/**
 * \brief
 *      Pause playing music, music can be resumed by calling #XGM_resumePlay (XGM music player driver).<br>
 *      Note that due to the nature of the music chip (FM synthesis), resume play operation will never be perfect
 *      and some notes will miss until next key-on event occurs.
 *
 *  \see XGM_resumePlay
 *  \see XGM_stopPlay
 */
void XGM_pausePlay(void);
/**
 * \brief
 *      Resume playing music after pausing with XGM_pausePlay (XGM music player driver).<br>
 *      Note that due to the nature of the music chip (FM synthesis), resume play operation will never be perfect
 *      and some notes will miss until next key-on event occurs.
 *
 *  \see XGM_pausePlay
 *  \see XGM_nextFrame
 */
void XGM_resumePlay(void);

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
 *      <code>isPlayingPCM(SOUND_PCM_CH1_MSK | SOUND_PCM_CH2_MSK)</code><br>
 *      will actually return play state for channel 1 and channel 2.
 *
 *  \return
 *      Return the channel mask of current playing channel(s).<br>
 *      For instance it returns (SOUND_PCM_CH1_MSK | SOUND_PCM_CH3_MSK) if channels 1 and 3 are currently playing.
 */
u8 XGM_isPlayingPCM(const u16 channel_mask);
/**
 *  \brief
 *      Declare a new PCM sample (maximum = 255) for the XGM music player driver.<br>
 *      Sample id < 64 are reserved for music while others are used for SFX
 *      so if you want to declare a new SFX PCM sample use an id >= 64
 *
 *  \param id
 *      Sample id:<br>
 *      value 0 is not allowed<br>
 *      values from 1 to 63 are used for music
 *      values from 64 to 255 are used for SFX
 *  \param sample
 *      Sample address, should be 256 bytes boundary aligned<br>
 *      SGDK automatically align sample resource as needed
 *  \param len
 *      Size of sample in bytes, should be a multiple of 256<br>
 *      SGDK automatically adjust resource size as needed
 */
void XGM_setPCM(const u8 id, const u8 *sample, const u32 len);
/**
 *  \brief
 *      Same as #XGM_setPCM but fast version.<br>
 *      This method assume that XGM driver is loaded and that 68000 has access to Z80 bus
 *
 *  \param id
 *      Sample id:<br>
 *      value 0 is not allowed<br>
 *      values from 1 to 63 are used for music
 *      values from 64 to 255 are used for SFX
 *  \param sample
 *      Sample address, should be 256 bytes boundary aligned<br>
 *      SGDK automatically align sample resource as needed
 *  \param len
 *      Size of sample in bytes, should be a multiple of 256<br>
 *      SGDK automatically adjust resource size as needed
 */
void XGM_setPCMFast(const u8 id, const u8 *sample, const u32 len);
/**
 *  \brief
 *      Play a PCM sample on specified channel (XGM music player driver).<br>
 *      If a sample was currently playing on this channel then priority of the newer sample should be are compared then it's stopped and the new sample is played instead.<br>
 *      Note that music may use the first PCM channel so it's better to use channel 2 to 4 for SFX.
 *
 *  \param id
 *      Sample id (use #XGM_setPCM(..) method first to set id)
 *  \param priority
 *      Value should go from 0 to 15 where 0 is lowest priority and 15 the highest one.<br>
 *      If the channel was already playing the priority is used to determine if the new SFX should replace the current one (new priority >= old priority).
 *  \param channel
 *      Channel where we want to play sample.<br>
 *      #SOUND_PCM_CH1    = channel 1 (usually used by music)<br>
 *      #SOUND_PCM_CH2    = channel 2<br>
 *      #SOUND_PCM_CH3    = channel 3<br>
 *      #SOUND_PCM_CH4    = channel 4<br>
 */
void XGM_startPlayPCM(const u8 id, const u8 priority, const u16 channel);
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
void XGM_stopPlayPCM(const u16 channel);

/**
 *  \brief
 *      Return the elapsed play time since the last #XGM_startPlay(..) call.<br>
 *      The returned value is in music frame which can be 50/60 per second depending the base music play rate (NTSC/PAL).
 *
 *  \see XGM_startPlay(..)
 *  \see XGM_setMusicTempo()
 */
u32 XGM_getElapsed(void);
/**
 *  \brief
 *      Get the current music tempo (in tick per second).<br>
 *      Default value is 60 or 50 depending the system is NTSC or PAL.<br>
 *      This method is meaningful only if you use the automatic music sync mode (see XGM_setManualSync() method)
 *      which is the default mode.<br>
 *      Note that using specific tempo (not 60 or 50) will affect performance of DMA contention and external command parsing
 *      so it's recommended to stand with default one.
 *
 *  \see XGM_setManualSync(..)
 *  \see XGM_setMusicTempo(..)
 */
u16 XGM_getMusicTempo(void);
/**
 *  \brief
 *      Set the music tempo (in tick per second).<br>
 *      Default value is 60 or 50 depending the system is NTSC or PAL.
 *      This method is meaningful only if you use the automatic music sync mode (see XGM_setManualSync() method)
 *      which is the default mode.<br>
 *      Note that using specific tempo (not 60 or 50) can completely distord FM instruments sound and affect
 *      performance of DMA contention and external command parsing so it's recommended to stand with default one.
 *
 *  \see XGM_setManualSync(..)
 *  \see XGM_getMusicTempo(void)
 */
void XGM_setMusicTempo(u16 value);

/**
 *  \brief
 *      Returns manual sync mode state of XGM driver (by default auto sync is used).
 *
 *  \see XGM_setManualSync(void)
 */
u16 XGM_getManualSync(void);
/**
 *  \brief
 *      Set manual sync mode of XGM driver (by default auto sync is used).
 *
 *  \param value TRUE or FALSE
 *  \see XGM_getManualSync(void)
 *  \see XGM_nextFrame(void)
 */
void XGM_setManualSync(u16 value);
/**
 *  \brief
 *      Notify the Z80 a new frame just happened (XGM music player driver).
 *
 *  Sound synchronization was initially 100% done by Z80 itself using the V-Interrupt but
 *  if the Z80 is stopped right at V-Int time (bus request from 68000 or DMA stall) then
 *  the V-Int can be missed by the Z80 and music timing affected.<br>
 *  To fix that issue and also to offer more flexibility the music timing should now be handled by the 68k.<br>
 *  By default this method is called automatically by SGDK at V-Int time but you can decide to handle sync
 *  manually (see XGM_setManualSync(..) method).<br>
 *  When you are in manual sync you normally should call this method once per frame (in the V-Int callback for instance)
 *  but you are free to play with it to increase or decrease music tempo.<br>
 *  Note that it's better to call this method a bit before (3/4 scanlines should be fine) doing DMA operation for best
 *  main bus contention protection (see #XGM_set68KBUSProtection() and #XGM_setForceDelayDMA() methods).
 *
 * \see XGM_setManualSync(..)
 * \see XGM_nextXFrame(..)
 * \see XGM_set68KBUSProtection(..)
 * \see XGM_setForceDelayDMA(..)
 */
#define XGM_nextFrame()  XGM_nextXFrame(1)
/**
 *  \brief
 *      Same as XGM_nextFrame() except you can specify the numer of frame.
 *
 * \see XGM_nextFrame(..)
 */
void XGM_nextXFrame(u16 num);

/**
 *  \brief
 *      Set the loop number for music with loop command.<br>
 *      Default value is -1 for pseudo unfinite (255) loops plays.
 *      A value of 0 means single play without any loop, 1 = single play + 1 loop...
 */
void XGM_setLoopNumber(s8 value);

/**
 *  \brief
 *      Same as Z80_setBusProtection(..) (generic version of the method)
 *
 *  \see XGM_setForceDelayDMA(..)
 */
void XGM_set68KBUSProtection(u8 value);
/**
 *  \brief
 *      Same as Z80_getForceDelayDMA() (generic version of the method)
 *
 *  \see XGM_setForceDelayDMA()
 */
bool XGM_getForceDelayDMA(void);
/**
 *  \brief
 *      Same as Z80_getForceDelayDMA(..) (generic version of the method)
 */
void XGM_setForceDelayDMA(bool value);

/**
 *  \brief
 *      Returns an estimation of the Z80 CPU load (XGM driver).<br>
 *      The low 16 bits returns the estimated Z80 CPU load where the high 16 bits returns the part
 *      spent waiting in the DMA contention (see #XGM_set68KBUSProtection method).<br>
 *      The method computes CPU load mean over 32 frames and so it's important to call it at
 *      each frame (on VInt for instance) to get meaningful value.<br>
 *      Note that it returns CPU load only for the XGM music parsing part as PCM channel mixing is always ON.<br>
 *      Idle usage is 40% on NTSC and 30% on PAL, 100% usage usually mean overrun and may result in music slowdown
 *      and incorrect PCM operations.
 */
u32 XGM_getCPULoad(void);


#endif // _XGM_H_
