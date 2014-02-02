/**
 *  \file tfcplay.h
 *  \brief TFM Music Maker compiled data player (68000)
 *  \author Alone Coder and Shiru
 *  \date 12/2007
 *
 * This unit provides TFM music player through the 68000 CPU.
 */

#ifndef _TFCPLAY_H_
#define _TFCPLAY_H_


/**
 * \brief Initialize and load the specified TFC music (68000 driver)
 *  The 68000 should have the Z80 bus before calling this method.
 *
 * \see TFC_play()
 * \see Z80_requestBus()
 */
void TFC_init(const u8* music);

/**
 * \brief Update frame for 68000 TFC player.<br>
 *  The 68000 should have the Z80 bus before calling this method.
 *
 * \see Z80_requestBus()
 */
void TFC_frame();
/**
 * \brief Returns TRUE if a TFC lmusic is currently playing.
 *
 * \see Z80_requestBus()
 */
u16 TFC_isPlaying();
/**
 * \brief Start/Stop play TFC music (68000 driver).
 *  The 68000 should have the Z80 bus before calling this method.
 *
 * \see Z80_requestBus()
 */
void TFC_play(u16 play);

#endif // _TFCPLAY_H_

