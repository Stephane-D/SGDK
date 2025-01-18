#ifndef _CONFIGURATION_SLOT_H_
#define _CONFIGURATION_SLOT_H_

#include "genesis.h"
#include "utils.h"
#include "mw-api/configuration_ap.h"

void CONFIG_SLOT_start();

void CONFIG_SLOT_paint(bool repaint);

bool CONFIG_SLOT_doAction(u16 button, u8 max_option);

void CONFIG_SLOT_toogleConnection();

#endif // _CONFIGURATION_SLOT_H_
