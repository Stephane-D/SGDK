/****************************************************************************
 * \brief MegaWiFi2 example.
 * 
 * \author Juan Antonio (PaCHoN) 
 * \author Jesus Alonso (doragasu)
 * 
 * \date 01/2025
 ****************************************************************************/

#include "mw-api/configuration_cert.h"

void CONFIG_CERT_start(){
    
    u16 button;
    bool repaint = TRUE;
    option = 0;
    
    do{
        CONFIG_CERT_paint(repaint);
        button = readButton(JOY_1);
        repaint = CONFIG_CERT_doAction(button, 2);
        print();
    }while(button != BUTTON_A);

}

void CONFIG_CERT_paint(bool repaint){
    if(repaint){
        clearScreen();
        println("Checking Cert...");
        delay_ms(DEFAULT_DELAY);
        u32 hash = mw_http_cert_query();
        sprintf(buffer, "Store Cert Hash: %08lx", hash);        
        VDP_drawText(buffer, 0u, 2u);
        VDP_drawText("Press START to Set Cert", 0u, 4u);
        VDP_drawText("Press A to return", 0u, 6u);
        VDP_drawText("Press B to clear Cert", 0u, 7u);
        sprintf(buffer, "Cert Hash: %08lx  len: %u", cert_hash, cert_len);        
        VDP_drawText(buffer, 0u, 9u);
        u8 line = 11u;
        paint_long_char(cert, cert_len, &line);
        repaint = false;
    }
}

bool CONFIG_CERT_doAction(u16 button, u8 max_option){    
    switch (button){
    case BUTTON_B:    
        CONFIG_CERT_clear();
        break;
    case BUTTON_START:{
        int rety = 3;
        while(mw_http_cert_set(cert_hash, cert, cert_len) && rety-->0);
        return TRUE;
    }
    default:
    }    
    return FALSE;
}

void CONFIG_CERT_clear(){   
    char mycert[20];
    memset(&mycert, 0,20);
    int rety = 3;
    while(mw_http_cert_set(23, mycert, 20) && rety--);
}
