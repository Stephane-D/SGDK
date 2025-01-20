/****************************************************************************
 * \brief MegaWiFi2 example.
 * 
 * \author Juan Antonio (PaCHoN) 
 * \author Jesus Alonso (doragasu)
 * 
 * \date 01/2025
 ****************************************************************************/

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

        if(manufacturer && device){
            sprintf(buffer, "Manufacturer: %2u Device: %4u", manufacturer, device);
            VDP_drawText(buffer, 0u, 9u);
        }
    }
}

bool FLASH_doAction(u16 button, u8 max_option){    

    enum mw_err err;
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
                err = mw_flash_id_get(&manufacturer, &device); 
                if(err){
                    sprintf(buffer, "MW-ERROR: %u      ", err);
                    println(buffer);
                }else{
                    return TRUE;
                }
            break;
            case 1:{
                u8* response = mw_flash_read(0x0000, 1024U);
                if(!response){
                    println("FLASH READ 0x0000 1024B FAIL"); 
                }else{
                    paint_long_char((const char *)response, 1024U, 11u);
                }
                break;
            }                
            case 2:
                err = mw_flash_write(0x0000, (u8 *)"DATA STORE ON FLASS", 20);
                if(err){
                    sprintf(buffer, "MW-ERROR: %u      ", err);
                    println(buffer);
                }
            break;
            case 3:
                err = mw_flash_sector_erase(0);
                if(err){
                    sprintf(buffer, "MW-ERROR: %u      ", err);
                    println(buffer);
                }
            break;
            default:
        }
    }
    default:
    }    
    return FALSE;
}