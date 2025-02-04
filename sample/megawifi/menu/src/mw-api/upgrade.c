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
        VDP_drawText("Upgrade", 1u, 3u);
        VDP_drawText("Press START to select", 0u, 4u);
        VDP_drawText("Press A to return", 0u, 5u);
    }
}

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
                if(mw_fw_list_upgrades(0u, 10u, 0u, &listUpgrades, &len, &total)){
                    println("Upgrade Error");
                }else{
                    len = 0;
                    println("Upgrade OK");
                }
            break;
            case 1:
                if(len){
                    u16 i = 0;
                    while(listUpgrades[i++]);
                    char rtosName[64];
                    memcpy(rtosName, listUpgrades, i - 1);
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
        return TRUE;
    }
    default:
    }    
    return FALSE;
}