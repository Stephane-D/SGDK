#pragma once
#include <WiFiUdp.h> //WOKAROUND PARA LOS DATOS s16
#include <netdb.h>
#include "util.h"

#define NETUTILS_TAG "NETUTILS"

int net_dns_lookup(const char* addr, const char *port, struct addrinfo** addr_info);