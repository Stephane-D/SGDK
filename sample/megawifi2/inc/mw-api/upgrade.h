#ifndef _UPGRADE_H_
#define _UPGRADE_H_

#include "genesis.h"
#include "utils.h"

void UPGRADE_start();

void UPGRADE_paint(bool repaint);

bool UPGRADE_doAction(u16 button, u8 max_option);

#endif // _UPGRADE_H_
