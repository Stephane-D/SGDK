/****************************************************************************
 * \brief MegaWiFi2 example.
 * 
 * \author Juan Antonio (PaCHoN) 
 * \author Jesus Alonso (doragasu)
 * 
 * \date 01/2025
 ****************************************************************************/

#ifndef _FLASH_H_
#define _FLASH_H_

#include "genesis.h"
#include "utils.h"

static u8 manufacturer  __attribute__((unused));
static u16 device  __attribute__((unused));

void FLASH_start();

void FLASH_paint(bool repaint);

bool FLASH_doAction(u16 button, u8 max_option);

#endif // _FLASH_H_
