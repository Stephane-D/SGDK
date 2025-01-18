#include "mw-api/flash.h"

void FLASH_start(){
    u16 button;
    bool repaint = TRUE;
    option = 0;    
    
    //mw_flash_info_get();   //TODO: NOT implemented ON SGDK ni RTOS  
    do{
        FLASH_paint(repaint);
        button = readButton(JOY_1);
        repaint = FLASH_doAction(button, 4u);
        VDP_drawText("*", 0u, option + 2);
        print();
    }while(button != BUTTON_A);

}

void FLASH_paint(bool repaint){
    if(repaint){
        clearScreen();      
        VDP_drawText("Get Flash IDS", 1u, 2u);
        VDP_drawText("Read Flash", 1u, 3u);
        VDP_drawText("Write Flash", 1u, 4u);
        VDP_drawText("Erase Flash", 1u, 5u);
        VDP_drawText("Press Start to exec", 0u, 6u);
        VDP_drawText("Press A to return", 0u, 7u);
    }
}

bool FLASH_doAction(u16 button, u8 max_option){    
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
                //mw_flash_id_get();  
            break;
            case 1:
                //mw_flash_read();
            break;
            case 2:
                //mw_flash_write();
            break;
            case 3:
                //mw_flash_sector_erase();
            break;
            default:
        }
        return TRUE;
    }
    default:
    }    
    return FALSE;
}