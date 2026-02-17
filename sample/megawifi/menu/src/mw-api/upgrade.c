/****************************************************************************
 * \brief MegaWiFi2 example.
 * 
 * \author Juan Antonio (PaCHoN) 
 * \author Jesus Alonso (doragasu)
 * 
 * \date 01/2025
 ****************************************************************************/

#include "mw-api/upgrade.h"

void UPGRADE_start(){
    u16 button;
    bool repaint = TRUE;
    option = 0;    
    do{
        UPGRADE_paint(repaint);
        button = readButton(JOY_1);
        repaint = UPGRADE_doAction(button, 2u);
        VDP_drawText("*", 0u, option + 2);
        print();
    }while(button != BUTTON_A);

}

void UPGRADE_paint(bool repaint){
    if(repaint){
        clearScreen();        
        VDP_drawText("List Firmwares", 1u, 2u);
        VDP_drawText("Upgrade (CAN TAKE 10 MIN. BE PATIENT)", 1u, 3u);
        VDP_drawText("Press START to select", 0u, 4u);
        VDP_drawText("Press A to return", 0u, 5u);
    }
}
static uint8_t len = 0;
static uint8_t total = 0;
static char listUpgrades[256] = {0};

bool UPGRADE_doAction(u16 button, u8 max_option){    
    switch (button){
    case BUTTON_UP:
        VDP_drawText(" ", 0u, option + 2);
        if(option > 0) {
            option--;
        }
        break;
    case BUTTON_DOWN:    
        VDP_drawText(" ", 0u, option + 2);
        if(option < max_option - 1){
            option = option+1u % max_option;
        }
        break;
    case BUTTON_START:{
        switch(option){
            case 0:
                char* tmp;
                memset(listUpgrades, 0, sizeof(listUpgrades));
                if(mw_fw_list_upgrades(0u, 10u, 0u, &tmp, &len, &total)){
                    println("Upgrade Error");
                }else{
                    VDP_clearTextArea(0, 9, 40, 10);
                    println("Upgrade OK   ");
                    memcpy(listUpgrades, tmp, len);
                    char buffer[6] = {0};
                    sprintf(buffer, "%2u", total); 
                    VDP_drawText("Total:", 1u, (u16)( 9));
                    VDP_drawText(buffer, 8u, (u16)( 9));
                    u16 headName = 0;
                    u16 count = 0;
                    u16 i = 0;

                    while (i < len && count < total) {
                        if(listUpgrades[i] == '\0') {
                            u16 nameLen = i - headName;
                            if(nameLen > 0) {
                                VDP_drawText(&listUpgrades[headName], 1u, (u16)(count + 11));                                
                                count++;
                            }
                            headName = i + 1;
                        }
                        i++;
                    }
                }
            break;
            case 1:
                if(len){
                    u16 i = 0;
                    char rtosName[64];
                    while(listUpgrades[i]){
                        rtosName[i] = listUpgrades[i];
                        i++;
                    }
                    rtosName[i] = '\0';
                    if(mw_fw_upgrade(rtosName)){
                        println("Upgrade Error");
                    }else{
                        println("Upgrade OK");
                    }
                }else{
                    println("No Upgrades available");
                }
            break;
            default:
        }
        return FALSE;
    }
    default:
    }    
    return FALSE;
}