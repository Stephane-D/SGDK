#ifndef _HTTP_H_
#define _HTTP_H_

#include "genesis.h"
#include "utils.h"

void HTTP_start();

void HTTP_paint(bool repaint);

bool HTTP_doAction(u16 button, u8 max_option);

void HTTP_test();

#endif // _HTTP_H_
