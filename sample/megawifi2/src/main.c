/****************************************************************************
 * \brief MegaWiFi example.
 *
 * This example demonstrates the following:
 * - How to initialize MegaWiFi
 * - How to do TCP connections
 * - How to do HTTP queries
 * - How to get the date/time (synchronized to NTP servers)
 * - How to send and receive data using UDP protocol
 *
 * To build this example set MODULE_MEGAWIFI to 1 in config.h and
 * rebuild the library.
 *
 * \author Jesus Alonso (doragasu)
 * \date 01/2022
 ****************************************************************************/

#include "genesis.h"
#include "utils.h"
#include "mw-api/configuration.h"
#include "mw-api/http.h"
#include "mw-api/tcp.h"
#include "mw-api/udp.h"
#include "mw-api/datetime.h"
#include "mw-api/random.h"
#include "mw-api/flash.h"
#include "mw-api/upgrade.h"
#include "mw-api/ping.h"

#if (MODULE_MEGAWIFI == 0)
#error "Set MODULE_MEGAWIFI to 1 in config.h and rebuild the library"
#endif

/// TCP port to use (set to Megadrive release year ;-)
#define MW_CH_PORT 	1985

/// Idle task, run using the spare CPU time available when the main task
/// pends or yields. See mw/tsk.h for details. Basically this task polls
/// the WiFi module to send and receive data.
static void idle_tsk(void)
{
	while (1) {
		mw_process();
	}
}

static void printMainMenu(bool repaint){
    if (repaint){
        clearScreen();
        VDP_drawText("Show Configurations", 1u, 1u);
        VDP_drawText("DateTime Test", 1u, 2u);
        VDP_drawText("Random Test", 1u, 3u);
        VDP_drawText("Flash Test", 1u, 4u);
        VDP_drawText("Upgrade Test", 1u, 5u);
        VDP_drawText("Ping Test", 1u, 6u);
        VDP_drawText("TCP Test", 1u, 7u);
        VDP_drawText("HTTP Test", 1u, 8u);
        VDP_drawText("UDP Test", 1u, 9u);
        VDP_drawText("Option:   Ciclo: ", 15u, 28u);
    }

    VDP_drawText("*", 0, option + 1U);
    print();
}

static bool doActionMainMenu(u16 button){
    switch (button){
    case BUTTON_UP:
        if(option > 0) {
            option--;
            return TRUE;
        }
        break;
    case BUTTON_DOWN:    
        if(option < 9u){
            option = option+1u % 9u;
            return TRUE;
        }
        break;
    case BUTTON_START:
        switch (option){
        case 0U:
            CONFIG_start();
            break;
        case 1U:
            DT_start();
            break;
        case 2U:
            RND_start();
            break;
        case 3U:
            FLASH_start();
            break;
        case 4U:
            UPGRADE_start();
            break;
        case 5U:
            PING_start();
            break;
        case 6U:
            TCP_start();
            break;
        case 7U:
            HTTP_start();
            break;
        case 8U:
            UDP_start();
            break;
        default:
            break;
        }
        return TRUE;
    default:
        return FALSE;
    }
    return FALSE;
}

/// Associates to the AP and runs the tests
static void run_app(void)
{
	//enum mw_err err;
    bool repaint = TRUE;
    while(1) {
        printMainMenu(repaint);
        repaint = doActionMainMenu(readButton(JOY_1));
        SYS_doVBlankProcess();
        ciclo++;
    }
}

/// MegaWiFi initialization
/// Returns true on error
static bool megawifi_init(void)
{
	uint8_t ver_major = 0, ver_minor = 0;
	char *variant = NULL;
	enum mw_err err;
	char line[] = "MegaWiFi version X.Y - zzz";
	bool ret;

	// Try detecting the module
	err = mw_detect(&ver_major, &ver_minor, &variant);

	if (MW_ERR_NONE != err) {
		// Megawifi not found
		println("MegaWiFi not found!");
		ret = TRUE;
	} else {
		// Megawifi found
		line[17] = ver_major + '0';
		line[19] = ver_minor + '0';
        memcpy(&(line[23]), variant, 3);
		println(line);
		ret = FALSE;
	}

	return ret;
}

/// Set idle task that polls WiFi module
static void tasking_init(void)
{
	TSK_userSet(idle_tsk);
}

/// Global initialization
static void init(void)
{
	// Initialize MegaWiFi
	mw_init((u16*) cmd_buf, MW_BUFLEN);
	// Initialize multitasking (for WiFi background checks)
	tasking_init();
}

/// Entry point
int main(bool hard)
{
	UNUSED_PARAM(hard);
	bool err;
	init();
	err = megawifi_init();
    delay_ms(DEFAULT_DELAY);
	if (!err) {        
        union mw_msg_sys_stat *status;
        while(!(status = mw_sys_stat_get()));
        printStatus(status);
		run_app();
	}

	return 0;
}
