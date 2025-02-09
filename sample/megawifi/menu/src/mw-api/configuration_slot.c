/****************************************************************************
 * \brief MegaWiFi2 example.
 * 
 * \author Juan Antonio (PaCHoN) 
 * \author Jesus Alonso (doragasu)
 * 
 * \date 01/2025
 ****************************************************************************/

#include "mw-api/configuration_slot.h"

void CONFIG_SLOT_start(){
    u16 button;
    bool repaint = TRUE;
    option = 0;    
    do{
        CONFIG_SLOT_paint(repaint);
        button = readButton(JOY_1);
        repaint = CONFIG_SLOT_doAction(button, MW_NUM_CFG_SLOTS);
        VDP_drawText("*", 0u, option + 5);
        print();
    }while(button != BUTTON_A);

}

void CONFIG_SLOT_paint(bool repaint){
    char *ssid, *pass;
    enum mw_phy_type phyType;
    if(repaint){
        clearScreen();
        println("Searching Configs...");
        int i = 0;
        for(i = 0; i < MW_NUM_CFG_SLOTS; i++){
            while(mw_ap_cfg_get((u8)i, &ssid, &pass, &phyType));
            sprintf(buffer, "%u: SSID:%s", (u8)i, ssid);
            VDP_drawText(buffer, 1u, (u16)i + 5);      
            sprintf(buffer, "PASS: %.1s**", pass);          
            VDP_drawText(buffer, 30u, (u16)i + 5);
            strclr(buffer);
        }
        s16 slotDefault = mw_def_ap_cfg_get();
        sprintf(buffer, "Default Slot: %d", slotDefault);
        VDP_drawText(buffer, 1u, (u16)10);
        VDP_drawText("Press Start to Configure Slot", 0u, (u16)16);
        VDP_drawText("Press A to Return", 0u, (u16)17);
        VDP_drawText("Press B associate/des to AP from Slot", 0u, (u16)18);
        VDP_drawText("Press C to Set Default AP Config Slot", 0u, (u16)19);
        repaint = false;
    }
}

bool CONFIG_SLOT_doAction(u16 button, u8 max_option){    
    switch (button){
    case BUTTON_UP:
        VDP_drawText(" ", 0u, option + 5);
        if(option > 0) {
            option--;
        }
        break;
    case BUTTON_DOWN:    
        VDP_drawText(" ", 0u, option + 5);
        if(option < max_option - 1){
            option = option+1u % max_option;
        }
        break;
    case BUTTON_B:{
        CONFIG_SLOT_toogleConnection();
        union mw_msg_sys_stat *status;
        while (!(status = mw_sys_stat_get()));
        printStatus(status);
        break;
    }
    case BUTTON_C:
        mw_def_ap_cfg((u8)option);
        mw_cfg_save();
        return TRUE;
    case BUTTON_START:
        CONFIG_AP_start((u8)option);
        return TRUE;
    default:
    }    
    return FALSE;
}

void CONFIG_SLOT_toogleConnection(){
    
        union mw_msg_sys_stat *status;
        while (!(status = mw_sys_stat_get()));
        printStatus(status);        
        if(status->online){
            println("Disconecting... ");            
            SYS_doVBlankProcess();
            mw_sleep(MS_TO_FRAMES(DEFAULT_MW_DELAY));
            while (!mw_ap_disassoc());    
            println("Disconected     ");            
            SYS_doVBlankProcess();  
        }else{
            struct mw_ip_cfg *ipConf;
            println("AP Connecting...");            
            SYS_doVBlankProcess();
            mw_sleep(MS_TO_FRAMES(DEFAULT_MW_DELAY));    
            int i =3;
            println("AP Waiting...   ");            
            SYS_doVBlankProcess();  
            while(mw_ap_assoc((u8)option) && i--){
                sprintf(buffer, "%d", i);
                VDP_drawText(buffer, 22u, 0);          
                SYS_doVBlankProcess();  
            }
            i =3;
            if(mw_ap_assoc_wait(MS_TO_FRAMES(60000))){
                println("ERROR         ");   
                SYS_doVBlankProcess();  
            }else{ 
                println("Checking IP...");            
                SYS_doVBlankProcess();
                while(mw_ip_current(&ipConf));           
                sprintf(buffer, "IP: %u.%u.%u.%u", ipConf->addr.byte[0], ipConf->addr.byte[1], ipConf->addr.byte[2], ipConf->addr.byte[3]);
                VDP_drawText(buffer, 0u, 21u);    
                sprintf(buffer, "D1: %u.%u.%u.%u", ipConf->dns1.byte[0], ipConf->dns1.byte[1], ipConf->dns1.byte[2], ipConf->dns1.byte[3]);     
                VDP_drawText(buffer, 0u, 22u); 
                sprintf(buffer, "D2: %u.%u.%u.%u", ipConf->dns2.byte[0], ipConf->dns2.byte[1], ipConf->dns2.byte[2], ipConf->dns2.byte[3]);  
                VDP_drawText(buffer, 0u, 23u); 
                sprintf(buffer, "GW: %u.%u.%u.%u", ipConf->gateway.byte[0], ipConf->gateway.byte[1], ipConf->gateway.byte[2], ipConf->gateway.byte[3]);  
                VDP_drawText(buffer, 0u, 24u);  
                sprintf(buffer, "MK: %u.%u.%u.%u", ipConf->mask.byte[0], ipConf->mask.byte[1], ipConf->mask.byte[2], ipConf->mask.byte[3]); 
                VDP_drawText(buffer, 0u, 25u);  
                println("IP Checked    ");      
                SYS_doVBlankProcess();
            }  
        }
}