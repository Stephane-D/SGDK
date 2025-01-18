#ifndef _CONFIGURATION_CERT_H_
#define _CONFIGURATION_CERT_H_

#include "genesis.h"
#include "utils.h"
#include "random.h"

void CONFIG_CERT_start();

void CONFIG_CERT_paint(bool repaint);

void CONFIG_CERT_paint_cert(const char *cert, u16 len, u8 line);

bool CONFIG_CERT_doAction(u16 button, u8 max_option);

void CONFIG_CERT_clear();

#endif // _CONFIGURATION_CERT_H_
