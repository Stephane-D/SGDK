/****************************************************************************
 * \brief MegaWiFi2 example.
 * 
 * \author Juan Antonio (PaCHoN) 
 * \author Jesus Alonso (doragasu)
 * 
 * \date 01/2025
 ****************************************************************************/

#ifndef _RANDOM_H_
#define _RANDOM_H_

#include <genesis.h>
#include "utils.h"

void RND_start();

void RND_paint(bool repaint);

bool RND_doAction(u16 button, u8 max_option);

void RND_test();

#endif // _RANDOM_H_
