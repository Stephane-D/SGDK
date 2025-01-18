#include "mw-api/random.h"

void RND_start(){
    
    u16 button;
    bool repaint = TRUE;
    
    do{
        RND_paint(repaint);
        button = readButton(JOY_1);
        repaint = RND_doAction(button, 0);
        print();
    }while(button != BUTTON_A);

}

void RND_paint(bool repaint){
    if(repaint){
        clearScreen();        
        VDP_drawText("RANDOM Test", 1u, 2u);
        VDP_drawText("Press START to launch", 0u, 3u);
        VDP_drawText("Press A to return", 0u, 4u);
    }
}

bool RND_doAction(u16 button, u8 max_option){    
    switch (button){
    case BUTTON_START:{
        RND_test();
    }
    default:
    }    
    return FALSE;
}


/// Waits until date/time is synchronized and gets the date/time
void RND_test()
{
	u8 *rnd;

	rnd = mw_hrng_get(1u);
    if (!rnd) {
        println("Failed to get Random");
        return;
    }
	sprintf(buffer, "Random Result: %u", *rnd);
	println(buffer);
}