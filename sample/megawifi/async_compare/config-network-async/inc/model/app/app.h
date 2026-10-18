#ifndef APP_H
#define APP_H

#include "genesis.h"

#define APP_SLOT_COUNT MW_NUM_CFG_SLOTS
#define APP_SCAN_RESULT_COUNT 4

typedef struct {
    char ssid[MW_SSID_MAXLEN + 1];
    enum mw_phy_type phy_type;
    bool configured;
} AppSlot;

typedef struct {
    enum mw_err err;
    AppSlot slots[APP_SLOT_COUNT];
    char scanResults[APP_SCAN_RESULT_COUNT][MW_SSID_MAXLEN + 1];
    uint8_t scanCount;
    int16_t defaultSlot;
    struct mw_ip_cfg interfaceIp;
    bool hasInterfaceIp;
    bool connected;
    bool ready;
    char status[40];
} App;

App *APP_getInstance(void);
void APP_init(void);
void APP_enableMegaWiFi(void);
void APP_process(void);
bool APP_isRequestPending(void);
bool APP_takeRedraw(void);
void APP_scan(void);
void APP_refreshInterface(void);
void APP_configureSlot(uint8_t slot, const char *ssid, const char *password);
void APP_setDefaultSlot(uint8_t slot);
void APP_toggleConnection(uint8_t slot);

#endif
