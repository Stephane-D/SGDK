/****************************************************************************
 * \brief MegaWiFi RTOS.
 * 
 * \author Juan Antonio (PaCHoN) 
 * \author Jesus Alonso (doragasu)
 * 
 * \date 01/2025
 ****************************************************************************/

#pragma once
#include <WiFiUdp.h> //WOKAROUND PARA LOS DATOS s16
#include <netdb.h>
#include "util.h"

#define NETUTILS_TAG "NETUTILS"

int net_dns_lookup(const char* addr, const char *port, struct addrinfo** addr_info);