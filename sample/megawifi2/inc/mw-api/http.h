#ifndef _HTTP_H_
#define _HTTP_H_

#include "genesis.h"
#include "utils.h"
#include "mw-api/configuration_cert.h"

void HTTP_start();

void HTTP_paint(bool repaint);

bool HTTP_doAction(u16 button, u8 max_option);

void HTTP_test(const char* url, bool ssl);

#endif // _HTTP_H_
