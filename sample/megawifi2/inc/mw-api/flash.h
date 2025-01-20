#ifndef _FLASH_H_
#define _FLASH_H_

#include "genesis.h"
#include "utils.h"

static u8 manufacturer;
static u16 device;

void FLASH_start();

void FLASH_paint(bool repaint);

bool FLASH_doAction(u16 button, u8 max_option);

#endif // _FLASH_H_
