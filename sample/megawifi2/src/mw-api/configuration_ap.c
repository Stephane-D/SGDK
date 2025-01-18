#include "mw-api/configuration_ap.h"

void CONFIG_AP_paintApData(struct mw_ap_data apData, u8 line){

    sprintf(buffer, "%d", apData.rssi);
    VDP_drawText(buffer , 1, line);
    switch(apData.auth){
        case MW_SEC_OPEN:
            VDP_drawText("OPEN", 5, line);
        break;
        case MW_SEC_WEP:
            VDP_drawText("WEP", 5, line);
        break;
        case MW_SEC_WPA_PSK:
            VDP_drawText("WPA", 5, line);
        break;
        case MW_SEC_WPA2_PSK:
            VDP_drawText("WPA2", 5, line);
        break;
        case MW_SEC_WPA_WPA2_PSK:
            VDP_drawText("MIX", 5, line);
        break;
        default:
            VDP_drawText("UNK" , 5, line);
    }
    char apBuff[apData.ssid_len];
    memcpy(apBuff,apData.ssid, apData.ssid_len);
    VDP_drawText(apBuff, 10, line);
}

u8 CONFIG_AP_paint(bool repaint, char* ap_data, u16 apLength, u8 page, u8 total_pages, s16 *selected_ap){
    u8 max_option = AP_MAX_ITEMS_PER_PAGE;
    if(repaint){
        clearScreen();
        u8 count = 0, aux_count = 0;
        u8 pageAux = 0;
        s16 pos = 0;
        do{                    
            if(option == count && pageAux == page) *selected_ap = pos;
            pos = mw_ap_fill_next(ap_data, pos ,&apData, apLength);
            if(pos){
                if(pageAux == page){   
                    CONFIG_AP_paintApData(apData, count + 1U);
                    count++;
                }else{
                    aux_count++;
                    if(aux_count == AP_MAX_ITEMS_PER_PAGE){
                        aux_count = 0;
                        pageAux++;
                    }
                }
            }
        }while(pos && pos < apLength && count < AP_MAX_ITEMS_PER_PAGE);
        max_option = count - 1;
        VDP_drawText("*", 0, option + 1U);        
        VDP_drawText("Pulse A to return", 1u, AP_MAX_ITEMS_PER_PAGE + 2u);
        repaint = FALSE;
    }
    
    sprintf(buffer, "%u/%u", page + 1,total_pages);
    VDP_drawText(buffer, 0u, 27u);
    print();
    return max_option;
}

void CONFIG_AP_save(const char *ap_data, u16 data_len, u16 pos, char *pass, u8 slot, enum mw_phy_type phy_type){
    struct mw_ap_data apd;
    mw_ap_fill_next(ap_data, pos ,&apd, data_len);
    char apBuff[apd.ssid_len + 1u];
    memcpy(apBuff,apd.ssid, apd.ssid_len);
    apBuff[apd.ssid_len]= '\0';
    while(mw_ap_cfg_set(slot,apBuff, pass, phy_type) + mw_cfg_save());
}

bool CONFIG_AP_doAction(u16 button, u8 max_option, u8 *page, u8 total_pages, s16 selected_ap, char* ap_data, u16 apLength, u8 slot){
    switch (button){
        case BUTTON_UP:
            if(option > 0) {
                option--;
            }
            return TRUE;
        case BUTTON_DOWN:    
            if(option < max_option ){
                option = option+1u % max_option;
            }
            return TRUE;
        case BUTTON_LEFT:
            option = 0;
            if(*page > 0) {
                (*page)--;
            }
            return TRUE;
        case BUTTON_RIGHT: 
            option = 0;   
            if((*page) < total_pages -1){
                (*page) = (*page)+1u % total_pages;
            }
            return TRUE;
        case BUTTON_START:{
            if(selected_ap >= 0){
                char bufferPass[MW_PASS_MAXLEN];
                int length = readText(bufferPass, MW_PASS_MAXLEN);
                char pass[length];
                memcpy(pass,bufferPass, length);
                CONFIG_AP_save(ap_data, apLength, selected_ap, pass, slot, MW_PHY_11BGN);
                button = BUTTON_A;
            }
            return TRUE;
        }
        default:
    }   
    return FALSE;
}

void CONFIG_AP_start(u8 slot){
    char* ap_data;
    s16 selected_ap = -1;
    u8 aps = 0, page = 0;
    bool repaint = TRUE;
    u16 button;
    option = 0;
    u8 max_option = AP_MAX_ITEMS_PER_PAGE;
    clearScreen();
	println("Searching Aps...");
	s16 apLength = mw_ap_scan(MW_PHY_11BGN, &ap_data, &aps);
	if (!apLength) println("No Ap");
    u8 total_pages = aps / AP_MAX_ITEMS_PER_PAGE;
    if (aps % AP_MAX_ITEMS_PER_PAGE > 0) total_pages++;
    do{
        if (apLength) max_option = CONFIG_AP_paint(repaint, ap_data, apLength,page,total_pages, &selected_ap);
        button = readButton(JOY_1);
        if (apLength) repaint = CONFIG_AP_doAction(button, max_option, &page, total_pages, selected_ap, ap_data, apLength, slot);             
        ciclo++;
    }while(button != BUTTON_A);
}