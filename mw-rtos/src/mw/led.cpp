/****************************************************************************
 * \brief MegaWiFi RTOS.
 * 
 * \author Juan Antonio (PaCHoN) 
 * \author Jesus Alonso (doragasu)
 * 
 * \date 01/2025
 ****************************************************************************/

#include <mw/led.h> 

void Led::led_toggle() {  
	u_int8_t r=0,g=0,b=0;

	enable = !enable;
	if(enable){
		switch(status->sys_stat){
			case MW_ST_INIT:
				r=255u;
			break;
			case MW_ST_IDLE:
				r = 0;
				b= 255u;
				g = 0;
			break;
			case MW_ST_AP_JOIN:
				r = 255u;
				b= 0;
				g = 170u;
			break;
			case MW_ST_READY:
				r = 0;
				b= 0;
				g = 255u;
			break;
			case MW_ST_TRANSPARENT:
				b= 255u;
				g = 255u;
			break;
		}
	}

	pixels->setPixelColor(PIXEL_INDEX, pixels->Color(r, g, b));
	pixels->show();
}

void Led::led_init() {    
	pixels->begin();
	pixels->clear();
}