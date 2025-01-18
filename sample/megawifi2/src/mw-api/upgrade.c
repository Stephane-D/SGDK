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
        VDP_drawText("Press START to select", 0u, 3u);
        VDP_drawText("Press A to return", 0u, 4u);
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
                //mw_fw_list(); //NOT IMPLEMENTED ON SGDK
            break;
            case 1:
                //mw_fw_upgrade();
            break;
            default:
        }
        return TRUE;
    }
    default:
    }    
    return FALSE;
}