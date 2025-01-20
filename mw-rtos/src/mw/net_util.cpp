/****************************************************************************
 * \brief MegaWiFi RTOS.
 * 
 * \author Juan Antonio (PaCHoN) 
 * \author Jesus Alonso (doragasu)
 * 
 * \date 01/2025
 ****************************************************************************/

#include "mw/net_util.h"

int net_dns_lookup(const char* addr, const char *port,
		struct addrinfo** addr_info)
{
	int err;
	const struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM,
	};

	err = getaddrinfo(addr, port, &hints, addr_info);

	if(err || !*addr_info) {
		ESP_LOGE(NETUTILS_TAG,"DNS lookup failure %d\n", err);
		if(*addr_info) {
			freeaddrinfo(*addr_info);
		}
		return -1;
	}

	return 0;
}
