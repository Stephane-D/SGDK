/****************************************************************************
 * \brief MegaWiFi2 example.
 * 
 * \author Juan Antonio (PaCHoN) 
 * \author Jesus Alonso (doragasu)
 * 
 * \date 01/2025
 ****************************************************************************/

#ifndef _CONFIGURATION_CERT_H_
#define _CONFIGURATION_CERT_H_

#include "genesis.h"
#include "utils.h"
#include "random.h"

void CONFIG_CERT_start();

void CONFIG_CERT_paint(bool repaint);

bool CONFIG_CERT_doAction(u16 button, u8 max_option);

void CONFIG_CERT_clear();

#endif // _CONFIGURATION_CERT_H_
