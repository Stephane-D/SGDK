/****************************************************************************
 * \brief MegaWiFi2 example.
 * 
 * \author Juan Antonio (PaCHoN) 
 * \author Jesus Alonso (doragasu)
 * 
 * \date 01/2025
 ****************************************************************************/

#ifndef _CONFIGURATION_AP_H_
#define _CONFIGURATION_AP_H_

#include "genesis.h"
#include "utils.h"

#define AP_MAX_ITEMS_PER_PAGE  20

static struct mw_ap_data apData __attribute__((unused));

void CONFIG_AP_start(u8 slot);

u8 CONFIG_AP_paint(bool repaint, char* ap_data, u16 apLength, u8 page, u8 total_pages, s16 *selected_ap);

bool CONFIG_AP_doAction(u16 button, u8 max_option, u8 *page, u8 total_pages, s16 selected_ap, char* ap_data, u16 apLength, u8 slot);

void CONFIG_AP_paintApData(struct mw_ap_data apData, u8 line);
void CONFIG_AP_save(const char *ap_data, u16 data_len, u16 pos, char *pass, u8 slot, enum mw_phy_type phy_type);

#endif // _CONFIGURATION_AP_H_
