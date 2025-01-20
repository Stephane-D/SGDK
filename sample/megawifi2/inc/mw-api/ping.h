#ifndef _PING_H_
#define _PING_H_

#include "genesis.h"
#include "utils.h"

void PING_start();

void PING_paint(bool repaint);

bool PING_doAction(u16 button, u8 max_option);

void PING_test();

#endif // _PING_H_
