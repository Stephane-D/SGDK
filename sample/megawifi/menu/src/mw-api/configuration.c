/****************************************************************************
 * \brief MegaWiFi2 example.
 * 
 * \author Juan Antonio (PaCHoN) 
 * \author Jesus Alonso (doragasu)
 * 
 * \date 01/2025
 ****************************************************************************/

#include "mw-api/configuration.h"

void CONFIG_start(){
    u16 button;
    bool repaint = TRUE;
    option = 0;    
    do{
        CONFIG_paint(repaint);
        button = readButton(JOY_1);
        repaint = CONFIG_doAction(button, 2u);
        VDP_drawText("*", 0u, option + 2);
        print();
    }while(button != BUTTON_A);

}

void CONFIG_paint(bool repaint){
    if(repaint){
        clearScreen();        
        VDP_drawText("Slots Configuration", 1u, 2u);
        VDP_drawText("Cert Configuration", 1u, 3u);
        VDP_drawText("Press START to selec", 0u, 4u);
        VDP_drawText("Press A to return", 0u, 5u);
    }
}

bool CONFIG_doAction(u16 button, u8 max_option){    
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
                CONFIG_SLOT_start((u8)option);
            break;
            case 1:
                CONFIG_CERT_start((u8)option);
            break;
            default:
        }
        return TRUE;
    }
    default:
    }    
    return FALSE;
}