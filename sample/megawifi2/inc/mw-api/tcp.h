#ifndef _TCP_H_
#define _TCP_H_

#include "genesis.h"
#include "utils.h"

void TCP_start();

void TCP_paint(bool repaint);

bool TCP_doAction(u16 button, u8 max_option);

void TCP_test();

#endif // _TCP_H_
