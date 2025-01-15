#pragma once

static const char *TAG = "USBHost";

class USBHost {
public:
    USBHost(){};

    static void host_lib_daemon_task(void *arg);
};