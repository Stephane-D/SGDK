/****************************************************************************
 * \brief MegaWiFi2 example.
 * 
 * \author Juan Antonio (PaCHoN) 
 * \author Jesus Alonso (doragasu)
 * 
 * \date 01/2025
 ****************************************************************************/

#ifndef _CONFIGURATION_H_
#define _CONFIGURATION_H_

#include "genesis.h"
#include "utils.h"
#include "mw-api/configuration_slot.h"
#include "mw-api/configuration_cert.h"

void CONFIG_start();

void CONFIG_paint(bool repaint);

bool CONFIG_doAction(u16 button, u8 max_option);

#endif // _CONFIGURATION_H_
