#include <model/app/app.h>
#include <res/spr_res.h>

#define APP_MW_BUF_LEN 1400

static App app;
static char cmdBuf[APP_MW_BUF_LEN] __attribute__((aligned(2)));
static Sprite *requestDonut;

static void setRequestDonut(bool visible)
{
    if (!requestDonut) {
        requestDonut = SPR_addSprite(&donut, 144, 96,
                TILE_ATTR(PAL1, FALSE, FALSE, FALSE));
    }
    SPR_setVisibility(requestDonut, visible ? VISIBLE : HIDDEN);
    SPR_update();
    SYS_doVBlankProcess();
}

#define APP_MW_REQUEST(result, expression) \
    do { \
        setRequestDonut(TRUE); \
        result = (expression); \
        setRequestDonut(FALSE); \
    } while (0)

static void setStatus(const char *status)
{
    strncpy(app.status, status, sizeof(app.status) - 1);
    app.status[sizeof(app.status) - 1] = '\0';
}

static void idleTask(void)
{
    while (TRUE) mw_process();
}

App *APP_getInstance(void)
{
    return &app;
}

void APP_init(void)
{
    memset(&app, 0, sizeof(app));
    app.defaultSlot = -1;
    setStatus("Iniciando MegaWiFi");
}

enum mw_err APP_enableMegaWiFi(void)
{
    const CommDriver *drivers;
    char *variant;
    uint8_t version[3];
    size_t driverCount;

    app.err = mw_init((uint16_t *)cmdBuf, APP_MW_BUF_LEN);
    if (app.err != MW_ERR_NONE) return app.err;
    TSK_userSet(idleTask);
    driverCount = comm_get_drivers(&drivers);
    for (size_t index = 0; index < driverCount; index++) {
        if (!drivers[index].is_present()) continue;
        drivers[index].init();
        comm_set_driver(&drivers[index]);
        APP_MW_REQUEST(app.err, mw_version_get(version, &variant));
        if (app.err == MW_ERR_NONE) {
            setStatus("MegaWiFi detectado");
            return MW_ERR_NONE;
        }
    }
    app.err = MW_ERR_NOT_READY;
    setStatus("MegaWiFi no detectado");
    return app.err;
}

void APP_readSlots(void)
{
    char *ssid;
    char *password;
    enum mw_phy_type phyType;

    for (uint8_t slot = 0; slot < APP_SLOT_COUNT; slot++) {
        AppSlot *entry = &app.slots[slot];
        enum mw_err err;

        APP_MW_REQUEST(err, mw_ap_cfg_get(slot, &ssid, &password, &phyType));
        entry->configured = err == MW_ERR_NONE;
        if (entry->configured) {
            memcpy(entry->ssid, ssid, MW_SSID_MAXLEN);
            entry->ssid[MW_SSID_MAXLEN] = '\0';
            entry->phy_type = phyType;
        } else entry->ssid[0] = '\0';
    }
    APP_MW_REQUEST(app.defaultSlot, mw_def_ap_cfg_get());
    setStatus("Slots actualizados");
}

void APP_scan(void)
{
    char *data;
    uint8_t apCount;
    int16_t dataLen;
    int16_t position = 0;

    app.scanCount = 0;
    APP_MW_REQUEST(dataLen, mw_ap_scan(MW_PHY_11BGN, &data, &apCount));
    if (dataLen < 0) {
        setStatus("Error durante escaneo");
        return;
    }
    for (uint8_t index = 0; index < apCount && index < APP_SCAN_RESULT_COUNT; index++) {
        struct mw_ap_data ap;
        uint8_t length;

        position = mw_ap_fill_next(data, position, &ap, dataLen);
        if (position <= 0) break;
        length = min(ap.ssid_len, MW_SSID_MAXLEN);
        memcpy(app.scanResults[app.scanCount], ap.ssid, length);
        app.scanResults[app.scanCount++][length] = '\0';
    }
    sprintf(app.status, "%u redes encontradas", apCount);
}

void APP_configureSlot(uint8_t slot, const char *ssid, const char *password)
{
    if (slot >= APP_SLOT_COUNT || !ssid || !ssid[0]) {
        setStatus("SSID vacio");
        return;
    }
        APP_MW_REQUEST(app.err, mw_ap_cfg_set(slot, ssid,
                password && password[0] ? password : NULL, MW_PHY_11BGN));
        if (app.err == MW_ERR_NONE) {
            APP_MW_REQUEST(app.err, mw_cfg_save());
        }
    if (app.err == MW_ERR_NONE) {
        APP_readSlots();
        sprintf(app.status, "Slot %u configurado", slot);
    } else setStatus("Error configurando slot");
}

void APP_setDefaultSlot(uint8_t slot)
{
    if (slot < APP_SLOT_COUNT) {
        APP_MW_REQUEST(app.err, mw_def_ap_cfg(slot));
    } else {
        app.err = MW_ERR_PARAM;
    }
    if (app.err == MW_ERR_NONE) {
        APP_MW_REQUEST(app.err, mw_cfg_save());
    }
    if (app.err == MW_ERR_NONE) {
        app.defaultSlot = slot;
        sprintf(app.status, "Slot %u predeterminado", slot);
    } else setStatus("Error guardando slot");
}

void APP_refreshInterface(void)
{
    struct mw_ip_cfg *ip;
    union mw_msg_sys_stat *status;

    APP_MW_REQUEST(status, mw_sys_stat_get());
    app.connected = status && status->online &&
            (status->sys_stat == MW_ST_READY ||
            status->sys_stat == MW_ST_TRANSPARENT);
    {
        enum mw_err err;

        APP_MW_REQUEST(err, mw_ip_current(&ip));
        app.hasInterfaceIp = err == MW_ERR_NONE;
    }
    if (app.hasInterfaceIp) {
        app.interfaceIp = *ip;
        setStatus("Interfaz actualizada");
    } else setStatus("Interfaz no asociada");
}

void APP_toggleConnection(uint8_t slot)
{
    if (app.connected) {
        APP_MW_REQUEST(app.err, mw_ap_disassoc());
        if (app.err == MW_ERR_NONE) {
            app.connected = FALSE;
            app.hasInterfaceIp = FALSE;
            setStatus("AP desconectado");
        } else setStatus("Error desconectando AP");
        return;
    }
    if (slot >= APP_SLOT_COUNT || !app.slots[slot].configured) {
        setStatus("Configura el slot primero");
        return;
    }
    APP_MW_REQUEST(app.err, mw_ap_assoc(slot));
    if (app.err == MW_ERR_NONE) {
        APP_MW_REQUEST(app.err, mw_ap_assoc_wait(MW_MS_TO_FRAMES(30000)));
    }
    if (app.err == MW_ERR_NONE) {
        APP_refreshInterface();
        setStatus("AP conectado");
    } else setStatus("Error conectando AP");
}