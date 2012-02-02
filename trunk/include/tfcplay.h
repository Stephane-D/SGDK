/**
 * \file tfcplay.h
 * \brief TFM Music Maker compiled data player (68000)
 * \author Alone Coder and Shiru
 * \date 12/2007
 *
 * This unit provides TFM music player through the 68000 CPU.
 */

#ifndef _TFCPLAY_H_
#define _TFCPLAY_H_


void TFC_init(const u8*);

void TFC_frame(void);
void TFC_play(u16);


#endif // _TFCPLAY_H_

