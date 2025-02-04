/****************************************************************************
 * \brief MegaWiFi2 example.
 * 
 * \author Juan Antonio (PaCHoN) 
 * \author Jesus Alonso (doragasu)
 * 
 * \date 01/2025
 ****************************************************************************/

#include "mw-api/dateTime.h"

void DT_start(){
    
    u16 button;
    bool repaint = TRUE;
    
    do{
        DT_paint(repaint);
        button = readButton(JOY_1);
        repaint = DT_doAction(button, 0);
        print();
    }while(button != BUTTON_A);

}

void DT_paint(bool repaint){
    if(repaint){
        clearScreen();        
        VDP_drawText("DT Test", 1u, 2u);
        VDP_drawText("Press START to launch", 0u, 3u);
        VDP_drawText("Press A to return", 0u, 4u);
    }
}

bool DT_doAction(u16 button, u8 max_option){    
    switch (button){
    case BUTTON_START:{
        DT_test();
    }
    default:
    }    
    return FALSE;
}


/// Waits until date/time is synchronized and gets the date/time
void DT_test()
{
	const char *datetime;
	uint32_t dt_bin[2] = {};
	union mw_msg_sys_stat *stat;

	// Wait until date/time is set
	do {
		mw_sleep(60);
		stat = mw_sys_stat_get();
		if (!stat) {
			println("Failed to get date/time");
			return;
		}
	} while (!stat->dt_ok);
	datetime = mw_date_time_get(dt_bin);
	println(datetime);
}