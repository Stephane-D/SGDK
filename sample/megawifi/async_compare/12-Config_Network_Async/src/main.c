#include "genesis.h"
#include <model/app/app.h>
#include <utils/virtual_keyboard.h>
#include <res/spr_res.h>

static uint8_t selectedSlot;
static volatile uint16_t pendingActions;
static bool screenDirty;
static bool keyboardActive;
static bool editingPassword;
static char networkSsid[MW_SSID_MAXLEN];
static char networkPassword[MW_PASS_MAXLEN];
static VirtualKeyboard keyboard;
static Sprite *requestDonut;
static bool requestDonutVisible;

#define ACTION_SLOT_UP       BUTTON_UP
#define ACTION_SLOT_DOWN     BUTTON_DOWN
#define ACTION_CONNECTION    BUTTON_A
#define ACTION_INTERFACE     BUTTON_B
#define ACTION_CONFIGURE     BUTTON_C
#define ACTION_DEFAULT_SLOT  BUTTON_START
#define ACTION_SCAN          BUTTON_MODE

static void onJoyEvent(u16 joy, u16 changed, u16 state)
{
    if (joy == JOY_1) {
        pendingActions |= changed & state;
    }
}

static void drawScreen(void)
{
    App *app = APP_getInstance();
    char text[40];

    VDP_clearTextArea(0, 0, 40, 28);
    VDP_drawText("MegaWiFi - Configuracion asincrona", 0, 0);
    VDP_drawText(app->status, 0, 2);
    for (uint8_t slot = 0; slot < APP_SLOT_COUNT; slot++) {
        AppSlot *entry = &app->slots[slot];
        sprintf(text, "%c Slot %u %c %.28s", slot == selectedSlot ? '>' : ' ',
                slot, slot == app->defaultSlot ? '*' : ' ',
                entry->configured ? entry->ssid : "<vacio>");
        VDP_drawText(text, 0, 4 + slot);
    }
        VDP_drawText(app->connected ? "A desconectar B interfaz C configurar" :
            "A conectar B interfaz C configurar", 0, 9);
    VDP_drawText("Arriba/abajo slot START predeterminado", 0, 10);
    VDP_drawText("Redes detectadas:", 0, 12);
    for (uint8_t index = 0; index < app->scanCount; index++) {
        VDP_drawText(app->scanResults[index], 2, 13 + index);
    }
    if (app->hasInterfaceIp) {
        sprintf(text, "IP %u.%u.%u.%u", app->interfaceIp.addr.byte[0], app->interfaceIp.addr.byte[1],
                app->interfaceIp.addr.byte[2], app->interfaceIp.addr.byte[3]);
        VDP_drawText(text, 0, 19);
        sprintf(text, "GW %u.%u.%u.%u", app->interfaceIp.gateway.byte[0],
                app->interfaceIp.gateway.byte[1], app->interfaceIp.gateway.byte[2],
                app->interfaceIp.gateway.byte[3]);
        VDP_drawText(text, 0, 20);
    }
    VDP_drawText("MODE escanear C editar SSID y password", 0, 26);
}

static void beginConfiguration(void)
{
    AppSlot *slot = &APP_getInstance()->slots[selectedSlot];

    if (!APP_getInstance()->ready) return;
    if (slot->configured) {
        strncpy(networkSsid, slot->ssid, sizeof(networkSsid) - 1);
        networkSsid[sizeof(networkSsid) - 1] = '\0';
    } else {
        networkSsid[0] = '\0';
    }
    networkPassword[0] = '\0';
    editingPassword = FALSE;
    keyboardActive = TRUE;
    VIRTUAL_KEYBOARD_begin(&keyboard, networkSsid, sizeof(networkSsid), "SSID");
    screenDirty = TRUE;
}

static void processKeyboard(uint16_t actions)
{
    VIRTUAL_KEYBOARD_handle(&keyboard, actions);
    if (keyboard.result == VIRTUAL_KEYBOARD_EDITING) {
        if (actions) screenDirty = TRUE;
        return;
    }
    if (keyboard.result == VIRTUAL_KEYBOARD_CANCELLED) {
        keyboardActive = FALSE;
        screenDirty = TRUE;
        return;
    }
    if (!editingPassword) {
        editingPassword = TRUE;
        VIRTUAL_KEYBOARD_begin(&keyboard, networkPassword, sizeof(networkPassword),
                "Password (vacia para red abierta)");
        screenDirty = TRUE;
        return;
    }
    keyboardActive = FALSE;
    APP_configureSlot(selectedSlot, networkSsid, networkPassword);
    screenDirty = TRUE;
}

void main(bool reset)
{
    SYS_disableInts();
    APP_init();
    JOY_init();
        SPR_init();
    JOY_setEventHandler(onJoyEvent);
    SYS_enableInts();

        requestDonut = SPR_addSprite(&donut, 144, 96,
            TILE_ATTR(PAL1, FALSE, FALSE, FALSE));
        SPR_setVisibility(requestDonut, HIDDEN);
    APP_enableMegaWiFi();

    while (TRUE) {
        uint16_t actions = pendingActions;

        pendingActions = 0;
        APP_process();
        requestDonutVisible = APP_isRequestPending();
        SPR_setVisibility(requestDonut, requestDonutVisible ? VISIBLE : HIDDEN);
        if (APP_takeRedraw()) {
            screenDirty = TRUE;
        }

        if (keyboardActive) {
            processKeyboard(actions);
        } else if (actions & ACTION_SLOT_UP) {
            selectedSlot = selectedSlot == 0 ? APP_SLOT_COUNT - 1 : selectedSlot - 1;
            screenDirty = TRUE;
        } else if (actions & ACTION_SLOT_DOWN) {
            selectedSlot = (selectedSlot + 1) % APP_SLOT_COUNT;
            screenDirty = TRUE;
        } else {
            if (actions & ACTION_CONNECTION) APP_toggleConnection(selectedSlot);
            if (actions & ACTION_SCAN) APP_scan();
            if (actions & ACTION_INTERFACE) APP_refreshInterface();
            if (actions & ACTION_CONFIGURE) beginConfiguration();
            if (actions & ACTION_DEFAULT_SLOT) APP_setDefaultSlot(selectedSlot);
            if (actions & (ACTION_CONNECTION | ACTION_SCAN | ACTION_INTERFACE | ACTION_CONFIGURE |
                    ACTION_DEFAULT_SLOT)) {
                screenDirty = TRUE;
            }
        }
        if (screenDirty) {
            if (keyboardActive) VIRTUAL_KEYBOARD_draw(&keyboard);
            else drawScreen();
            screenDirty = FALSE;
        }
        SYS_doVBlankProcess();
        SPR_update();
    }
}