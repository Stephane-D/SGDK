/****************************************************************************
 * \brief MegaWiFi2 example.
 * 
 * \author Juan Antonio (PaCHoN) 
 * \author Jesus Alonso (doragasu)
 * 
 * \date 01/2025
 ****************************************************************************/

#ifndef _DT_H_
#define _DT_H_

#include "genesis.h"
#include "utils.h"

void DT_start();

void DT_paint(bool repaint);

bool DT_doAction(u16 button, u8 max_option);

void DT_test();

#endif // _DT_H_
