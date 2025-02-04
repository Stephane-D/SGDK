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

static char *listUpgrades __attribute__((unused)) = NULL;
static uint8_t len __attribute__((unused)) = 0;
static uint8_t total __attribute__((unused)) = 0;

void UPGRADE_start();

void UPGRADE_paint(bool repaint);

bool UPGRADE_doAction(u16 button, u8 max_option);

#endif // _UPGRADE_H_
