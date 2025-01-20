/****************************************************************************
 * \brief MegaWiFi RTOS.
 * 
 * \author Juan Antonio (PaCHoN) 
 * \author Jesus Alonso (doragasu)
 * 
 * \date 01/2025
 ****************************************************************************/

#pragma once

static const char *TAG = "USBHost";

class USBHost {
public:
    USBHost(){};

    static void host_lib_daemon_task(void *arg);
};