/****************************************************************************
 * \brief MegaWiFi2 example.
 * 
 * \author Juan Antonio (PaCHoN) 
 * \author Jesus Alonso (doragasu)
 * 
 * \date 01/2025
 ****************************************************************************/

#include "mw-api/tcp.h"

void TCP_start(){
    
    u16 button;
    bool repaint = TRUE;
    option = 0;
    
    do{
        TCP_paint(repaint);
        button = readButton(JOY_1);
        repaint = TCP_doAction(button, 0);
        print();
    }while(button != BUTTON_A);

}

void TCP_paint(bool repaint){
    if(repaint){
        clearScreen();        
        VDP_drawText("TCP Test", 1u, 2u);
        VDP_drawText("Press START to launch", 0u, 3u);
        VDP_drawText("Press A to return", 0u, 4u);
        VDP_drawText("Press B to TCP SERVER", 0u, 5u);
    }
}

bool TCP_doAction(u16 button, u8 max_option){    
    switch (button){
    case BUTTON_START:{
        TCP_test();
        break;
    }
    case BUTTON_B:{
        TCP_test_server();
    }
    default:
    }    
    return FALSE;
}


/// Test an TCP GET request to TCPs://www.example.com
void TCP_test() {
	enum mw_err err;

	// Connect to www.example.com on port 80
	println("Connecting to www.example.com");    
	print();
	delay_ms(DEFAULT_DELAY);
	err = mw_tcp_connect(1, "www.example.com", "80", NULL);
	if (err) {
		println("TCP test FAILED              ");
	}else{
	    println("TCP test SUCCESS             ");
    }	
    //mw_send_sync(1,"a", 1, MS_TO_FRAMES(DEFAULT_MW_DELAY));
    //mw_recv_sync(1);
	mw_tcp_disconnect(1);
}

void TCP_test_server(){
	println("Creating Server on port 80");
    print();
    delay_ms(DEFAULT_DELAY);   
	enum mw_err err = mw_tcp_bind(1u, 80u);
    if(err)goto err;
	println("Creating Server OK     ");
    print();
    delay_ms(DEFAULT_DELAY);   
	println("Waiting connections...     ");
    print();
    s16 buf_len;
    u8 ch;
    err = mw_recv_sync(&ch, cmd_buf, &buf_len, DEFAULT_MW_DELAY);
    if(err)goto err;
    println("Receving OK            ");
    u8 line = 11u;
    paint_long_char(cmd_buf, buf_len, &(line));
    mw_tcp_disconnect(1u);
    return;
err:
    sprintf(buffer, "MW-ERROR: %u      ", err);
	println(buffer);
    mw_tcp_disconnect(1u);

}
