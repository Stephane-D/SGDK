#include "mw-api/ping.h"

void PING_start(){
    
    u16 button;
    bool repaint = TRUE;
    
    do{
        PING_paint(repaint);
        button = readButton(JOY_1);
        repaint = PING_doAction(button, 0);
        print();
    }while(button != BUTTON_A);

}

void PING_paint(bool repaint){
    if(repaint){
        clearScreen();        
        VDP_drawText("PING Test", 1u, 2u);
        VDP_drawText("Press START to launch", 0u, 3u);
        VDP_drawText("Press A to return", 0u, 4u);
    }
}

bool PING_doAction(u16 button, u8 max_option){    
    switch (button){
    case BUTTON_START:{
        PING_test();
        break;
    }
    default:
    }    
    return FALSE;
}


/// Waits until date/time is synchronized and gets the date/time
void PING_test()
{
	struct mw_ping_response *ping_response;
    
    println("PING to www.example.com ...");
    print();
    delay_ms(DEFAULT_DELAY);
	ping_response = mw_ping("www.example.com", 5u);
    if(ping_response){
        sprintf(buffer, "Finish: %u, ok: %u fail: %u", ping_response->finish, ping_response->ok, ping_response->fail);
	    println(buffer);
    }else{
	    println("FAIL PING to www.example.com");
    }
}