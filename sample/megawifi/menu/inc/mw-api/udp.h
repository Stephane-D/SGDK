/****************************************************************************
 * \brief MegaWiFi2 example.
 * 
 * \author Juan Antonio (PaCHoN) 
 * \author Jesus Alonso (doragasu)
 * 
 * \date 01/2025
 ****************************************************************************/

#ifndef _UDP_H_
#define _UDP_H_

#include <genesis.h>
#include "utils.h"

void UDP_start();

void UDP_paint(bool repaint);

bool UDP_doAction(u16 button, u8 max_option);

void UDP_normal_test();

void UDP_reuse_test();

/// UDP receive functions callback
void udp_recv_cb(enum lsd_status stat, uint8_t ch, char *data, uint16_t len, void *ctx);
void udp_send_complete_cb(enum lsd_status stat, void *ctx);

#endif // _UDP_H_
