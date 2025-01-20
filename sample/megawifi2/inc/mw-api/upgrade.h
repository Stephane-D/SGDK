/****************************************************************************
 * \brief MegaWiFi2 example.
 * 
 * \author Juan Antonio (PaCHoN) 
 * \author Jesus Alonso (doragasu)
 * 
 * \date 01/2025
 ****************************************************************************/

#ifndef _UPGRADE_H_
#define _UPGRADE_H_

#include "genesis.h"
#include "utils.h"

void UPGRADE_start();

void UPGRADE_paint(bool repaint);

bool UPGRADE_doAction(u16 button, u8 max_option);

#endif // _UPGRADE_H_
