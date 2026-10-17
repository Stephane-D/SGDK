#include <model/app/app.h>
#include "ext/mw/megawifi_async.h"
#include "timer.h"

#define APP_MW_BUF_LEN 1400

enum app_operation {
    APP_OP_NONE,
    APP_OP_READ_SLOT,
    APP_OP_READ_DEFAULT,
    APP_OP_INTERFACE,
    APP_OP_SCAN,
    APP_OP_SCAN_STATUS,
    APP_OP_SCAN_DISCONNECT,
    APP_OP_SCAN_WAIT_IDLE,
    APP_OP_CONFIGURE,
    APP_OP_CONFIGURE_SAVE,
    APP_OP_DEFAULT_SLOT,
    APP_OP_DEFAULT_SAVE,
    APP_OP_READ_STATUS,
    APP_OP_CONNECT,
    APP_OP_CONNECT_WAIT,
    APP_OP_DISCONNECT
};

enum app_event {
    APP_EVENT_NONE,
    APP_EVENT_DETECT,
    APP_EVENT_SLOT,
    APP_EVENT_DEFAULT,
    APP_EVENT_INTERFACE,
    APP_EVENT_SCAN,
    APP_EVENT_RESULT,
    APP_EVENT_STATUS
};

static App app;
static char cmd_buf[APP_MW_BUF_LEN] __attribute__((aligned(2)));
static const CommDriver *drivers;
static const CommDriver *activeDriver;
static size_t driverCount;
static size_t driverIndex;
static enum app_operation operation;
static volatile enum app_event pendingEvent;
static enum mw_err callbackError;
static uint8_t slotCursor;
static uint8_t callbackSlot;
static int16_t callbackDefaultSlot;
static enum mw_state callbackSystemState;
static uint8_t scanStatusRetries;
static uint8_t scanTotalCount;
static uint32_t operationDeadline;
static uint32_t nextStatusPoll;
static bool detectionSucceeded;
static bool redrawPending;
static volatile bool requestPending;

static enum mw_err APP_requestStartResult(enum mw_err err)
{
    if (err != MW_ERR_NONE) {
        requestPending = FALSE;
    }
    return err;
}

#define APP_START_REQUEST(expression) \
    (requestPending = TRUE, APP_requestStartResult(expression))

static void APP_setStatus(const char *status)
{
    strncpy(app.status, status, sizeof(app.status) - 1);
    app.status[sizeof(app.status) - 1] = '\0';
    redrawPending = TRUE;
}

static void APP_idleTask(void)
{
    while (TRUE) {
        mw_process();
        mw_process_async_timeout();
    }
}

static void APP_detectCallback(enum mw_err err, uint8_t *major,
        uint8_t *minor, char **variant)
{
    requestPending = FALSE;
    (void)major;
    (void)minor;
    (void)variant;
    callbackError = err;
    detectionSucceeded = err == MW_ERR_NONE;
    pendingEvent = APP_EVENT_DETECT;
}

static void APP_slotCallback(enum mw_err err, struct mw_msg_ap_cfg *config)
{
    AppSlot *slot = &app.slots[slotCursor];

    requestPending = FALSE;
    callbackError = err;
    callbackSlot = slotCursor;
    slot->configured = err == MW_ERR_NONE && config != NULL;
    if (slot->configured) {
        memcpy(slot->ssid, config->ssid, MW_SSID_MAXLEN);
        slot->ssid[MW_SSID_MAXLEN] = '\0';
        slot->phy_type = config->phy_type;
    } else {
        slot->ssid[0] = '\0';
    }
    pendingEvent = APP_EVENT_SLOT;
}

static void APP_defaultSlotCallback(enum mw_err err, u8 slot)
{
    requestPending = FALSE;
    callbackError = err;
    callbackDefaultSlot = err == MW_ERR_NONE ? slot : -1;
    pendingEvent = APP_EVENT_DEFAULT;
}

static void APP_interfaceCallback(enum mw_err err, struct mw_ip_cfg *ip)
{
    requestPending = FALSE;
    callbackError = err;
    app.hasInterfaceIp = err == MW_ERR_NONE && ip != NULL;
    if (app.hasInterfaceIp) {
        app.interfaceIp = *ip;
    }
    pendingEvent = APP_EVENT_INTERFACE;
}

static void APP_statusCallback(enum mw_err err, union mw_msg_sys_stat *status)
{
    requestPending = FALSE;
    callbackError = err;
    callbackSystemState = status ? status->sys_stat : MW_ST_MAX;
    app.connected = err == MW_ERR_NONE && status && status->online &&
            (status->sys_stat == MW_ST_READY ||
            status->sys_stat == MW_ST_TRANSPARENT);
    pendingEvent = APP_EVENT_STATUS;
}

static void APP_scanCallback(enum mw_err err, uint8_t *data,
        uint16_t dataLen)
{
    int16_t position = 0;
    uint8_t aps = 0;

    requestPending = FALSE;
    callbackError = err;
    app.scanCount = 0;
    scanTotalCount = 0;
    if (err == MW_ERR_NONE && data && dataLen) {
        aps = data[0];
        scanTotalCount = aps;
        data++;
        dataLen--;
        for (uint8_t index = 0; index < aps &&
                index < APP_SCAN_RESULT_COUNT; index++) {
            struct mw_ap_data ap;
            uint8_t length;

            position = mw_ap_fill_next((char *)data, position, &ap, dataLen);
            if (position <= 0) {
                break;
            }
            length = min(ap.ssid_len, MW_SSID_MAXLEN);
            memcpy(app.scanResults[app.scanCount], ap.ssid, length);
            app.scanResults[app.scanCount][length] = '\0';
            app.scanCount++;
        }
    }
    pendingEvent = APP_EVENT_SCAN;
}

static void APP_resultCallback(enum mw_err err)
{
    requestPending = FALSE;
    callbackError = err;
    pendingEvent = APP_EVENT_RESULT;
}

static void APP_startScan(void)
{
    operation = APP_OP_SCAN;
    if (APP_START_REQUEST(mw_ap_scan_async(MW_PHY_11BGN,
            APP_scanCallback)) != MW_ERR_NONE) {
        operation = APP_OP_NONE;
        APP_setStatus("No se puede iniciar escaneo");
    }
}

static void APP_checkScanIdleState(void)
{
    operation = APP_OP_SCAN_STATUS;
    if (APP_START_REQUEST(mw_sys_stat_get_async(APP_statusCallback)) !=
            MW_ERR_NONE) {
        operation = APP_OP_NONE;
        APP_setStatus("No se puede comprobar AP");
    }
}

static void APP_pollConnectionStatus(void)
{
    nextStatusPoll = vtimer + MW_MS_TO_FRAMES(250);
    APP_START_REQUEST(mw_sys_stat_get_async(APP_statusCallback));
}

static bool APP_deadlineReached(uint32_t deadline)
{
    return (int32_t)(vtimer - deadline) >= 0;
}

static void APP_connectionTimedOut(void)
{
    operation = APP_OP_NONE;
    app.connected = FALSE;
    APP_setStatus("Tiempo agotado conectando AP");
}

static void APP_readNextSlot(void)
{
    operation = APP_OP_READ_SLOT;
    if (APP_START_REQUEST(mw_ap_cfg_get_async(slotCursor,
            APP_slotCallback)) != MW_ERR_NONE) {
        callbackError = MW_ERR_NOT_READY;
        pendingEvent = APP_EVENT_SLOT;
    }
}

static void APP_readSlots(void)
{
    slotCursor = 0;
    APP_readNextSlot();
}

static void APP_tryNextDriver(void)
{
    enum mw_err err;

    while (driverIndex < driverCount) {
        activeDriver = &drivers[driverIndex++];
        if (!activeDriver->is_present()) {
            continue;
        }
        activeDriver->init();
        comm_set_driver(activeDriver);
        err = APP_START_REQUEST(mw_detect_async(APP_detectCallback));
        if (err == MW_ERR_NONE) {
            APP_setStatus("Detectando MegaWiFi");
            return;
        }
    }
    app.err = MW_ERR_NOT_READY;
    APP_setStatus("MegaWiFi no detectado");
}

static void APP_processStatusEvent(void)
{
    switch (operation) {
    case APP_OP_SCAN_STATUS:
        if (callbackError != MW_ERR_NONE) {
            operation = APP_OP_NONE;
            APP_setStatus("No se puede comprobar AP");
        } else if (callbackSystemState == MW_ST_IDLE) {
            APP_startScan();
        } else if (scanStatusRetries == 0) {
            operation = APP_OP_NONE;
            APP_setStatus("AP no se desconecta");
        } else {
            scanStatusRetries--;
            operation = APP_OP_SCAN_DISCONNECT;
                if (APP_START_REQUEST(mw_ap_disassoc_async(APP_resultCallback)) !=
                    MW_ERR_NONE) {
                operation = APP_OP_NONE;
                APP_setStatus("No se puede desconectar AP");
            }
        }
        break;
    case APP_OP_CONNECT_WAIT:
        if (callbackError == MW_ERR_NONE && app.connected) {
            operation = APP_OP_INTERFACE;
                if (APP_START_REQUEST(mw_ip_current_async(APP_interfaceCallback)) !=
                    MW_ERR_NONE) {
                operation = APP_OP_NONE;
                APP_setStatus("AP conectado");
            }
        } else if (APP_deadlineReached(operationDeadline)) {
            APP_connectionTimedOut();
        } else {
            nextStatusPoll = vtimer + MW_MS_TO_FRAMES(250);
        }
        break;
    default:
        operation = APP_OP_NONE;
        APP_setStatus(app.connected ? "AP conectado" : "AP desconectado");
        break;
    }
}

static void APP_processResultEvent(void)
{
    if (callbackError != MW_ERR_NONE) {
        operation = APP_OP_NONE;
        APP_setStatus("Error configurando red");
        return;
    }

    switch (operation) {
    case APP_OP_CONFIGURE:
        operation = APP_OP_CONFIGURE_SAVE;
        mw_cfg_save_async(APP_resultCallback);
        break;
    case APP_OP_CONFIGURE_SAVE:
        APP_setStatus("Slot configurado");
        APP_readSlots();
        break;
    case APP_OP_DEFAULT_SLOT:
        operation = APP_OP_DEFAULT_SAVE;
        mw_cfg_save_async(APP_resultCallback);
        break;
    case APP_OP_DEFAULT_SAVE:
        APP_setStatus("Slot predeterminado guardado");
        APP_readSlots();
        break;
    case APP_OP_CONNECT:
        operation = APP_OP_CONNECT_WAIT;
        operationDeadline = vtimer + MW_MS_TO_FRAMES(30000);
        nextStatusPoll = vtimer;
        break;
    case APP_OP_DISCONNECT:
        app.connected = FALSE;
        app.hasInterfaceIp = FALSE;
        operation = APP_OP_NONE;
        APP_setStatus("AP desconectado");
        break;
    case APP_OP_SCAN_DISCONNECT:
        app.connected = FALSE;
        app.hasInterfaceIp = FALSE;
        operation = APP_OP_SCAN_WAIT_IDLE;
        operationDeadline = vtimer + MW_MS_TO_FRAMES(250);
        break;
    default:
        break;
    }
}

static void APP_processDeadlines(void)
{
    if (operation == APP_OP_SCAN_WAIT_IDLE) {
        if (APP_deadlineReached(operationDeadline)) {
            APP_checkScanIdleState();
        }
        return;
    }

    if (operation != APP_OP_CONNECT_WAIT) return;
    if (APP_deadlineReached(operationDeadline)) {
        APP_connectionTimedOut();
    } else if (APP_deadlineReached(nextStatusPoll)) {
        APP_pollConnectionStatus();
    }
}

App *APP_getInstance(void)
{
    return &app;
}

bool APP_isRequestPending(void)
{
    return requestPending;
}

void APP_init(void)
{
    memset(&app, 0, sizeof(app));
    app.defaultSlot = -1;
    APP_setStatus("Iniciando MegaWiFi");
}

void APP_enableMegaWiFi(void)
{
    app.err = mw_init((uint16_t *)cmd_buf, APP_MW_BUF_LEN);
    if (app.err != MW_ERR_NONE) {
        APP_setStatus("Buffer MegaWiFi invalido");
        return;
    }

    TSK_userSet(APP_idleTask);
    drivers = NULL;
    driverCount = comm_get_drivers(&drivers);
    driverIndex = 0;
    APP_tryNextDriver();
}

void APP_process(void)
{
    enum app_event event = pendingEvent;

    pendingEvent = APP_EVENT_NONE;

    switch (event) {
    case APP_EVENT_DETECT:
        if (!detectionSucceeded) {
            APP_tryNextDriver();
            break;
        }
        app.err = MW_ERR_NONE;
        app.ready = TRUE;
        APP_setStatus("MegaWiFi detectado");
        APP_readSlots();
        break;
    case APP_EVENT_SLOT:
        if (callbackError != MW_ERR_NONE) {
            app.slots[callbackSlot].configured = FALSE;
        }
        if (++slotCursor < APP_SLOT_COUNT) {
            APP_readNextSlot();
        } else if (APP_START_REQUEST(mw_def_ap_cfg_get_async(
            APP_defaultSlotCallback)) == MW_ERR_NONE) {
            operation = APP_OP_READ_DEFAULT;
        } else {
            operation = APP_OP_NONE;
            APP_setStatus("Error leyendo slot predeterminado");
        }
        break;
    case APP_EVENT_DEFAULT:
        app.defaultSlot = callbackDefaultSlot;
        operation = APP_OP_READ_STATUS;
        if (APP_START_REQUEST(mw_sys_stat_get_async(APP_statusCallback)) !=
            MW_ERR_NONE) {
            operation = APP_OP_NONE;
            APP_setStatus("Slots actualizados");
        }
        break;
    case APP_EVENT_STATUS:
        APP_processStatusEvent();
        break;
    case APP_EVENT_INTERFACE:
        operation = APP_OP_READ_STATUS;
        if (APP_START_REQUEST(mw_sys_stat_get_async(APP_statusCallback)) !=
            MW_ERR_NONE) {
            operation = APP_OP_NONE;
            APP_setStatus(callbackError == MW_ERR_NONE ? "Interfaz actualizada" :
                "Interfaz no asociada");
        }
        break;
    case APP_EVENT_SCAN:
        operation = APP_OP_NONE;
        if (callbackError == MW_ERR_NONE) {
            sprintf(app.status, "%u redes encontradas", scanTotalCount);
            redrawPending = TRUE;
        } else {
            APP_setStatus("Error durante escaneo");
        }
        break;
    case APP_EVENT_RESULT:
        APP_processResultEvent();
        break;
    default:
        break;
    }

    APP_processDeadlines();
}

bool APP_takeRedraw(void)
{
    bool redraw = redrawPending;
    redrawPending = FALSE;
    return redraw;
}

void APP_scan(void)
{
    if (!app.ready || operation != APP_OP_NONE) return;
    scanStatusRetries = 10;
    APP_checkScanIdleState();
}

void APP_refreshInterface(void)
{
    if (!app.ready || operation != APP_OP_NONE) return;
    operation = APP_OP_INTERFACE;
        if (APP_START_REQUEST(mw_ip_current_async(APP_interfaceCallback)) !=
            MW_ERR_NONE) {
        operation = APP_OP_NONE;
        APP_setStatus("No se puede leer interfaz");
    }
}

void APP_configureSlot(uint8_t slot, const char *ssid, const char *password)
{
    if (!app.ready || operation != APP_OP_NONE) return;
    if (slot >= APP_SLOT_COUNT || !ssid || !ssid[0]) {
        APP_setStatus("SSID vacio");
        return;
    }
    operation = APP_OP_CONFIGURE;
        if (APP_START_REQUEST(mw_ap_cfg_set_async(slot, ssid,
            password && password[0] ? password : NULL,
            MW_PHY_11BGN,
            APP_resultCallback)) != MW_ERR_NONE) {
        operation = APP_OP_NONE;
        APP_setStatus("No se puede configurar slot");
    }
}

void APP_setDefaultSlot(uint8_t slot)
{
    if (!app.ready || operation != APP_OP_NONE || slot >= APP_SLOT_COUNT) return;
    operation = APP_OP_DEFAULT_SLOT;
        if (APP_START_REQUEST(mw_def_ap_cfg_async(slot, APP_resultCallback)) !=
            MW_ERR_NONE) {
        operation = APP_OP_NONE;
        APP_setStatus("No se puede seleccionar slot");
    }
}

void APP_toggleConnection(uint8_t slot)
{
    if (!app.ready || operation != APP_OP_NONE) return;
    if (app.connected) {
        operation = APP_OP_DISCONNECT;
        if (APP_START_REQUEST(mw_ap_disassoc_async(APP_resultCallback)) !=
            MW_ERR_NONE) {
            operation = APP_OP_NONE;
            APP_setStatus("No se puede desconectar AP");
        }
        return;
    }
    if (slot >= APP_SLOT_COUNT || !app.slots[slot].configured) {
        APP_setStatus("Configura el slot primero");
        return;
    }
    operation = APP_OP_CONNECT;
        if (APP_START_REQUEST(mw_ap_assoc_async(slot, APP_resultCallback)) !=
            MW_ERR_NONE) {
        operation = APP_OP_NONE;
        APP_setStatus("No se puede conectar AP");
    }
}
