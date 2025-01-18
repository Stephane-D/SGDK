#include "mw-api/tcp.h"

void TCP_start(){
    
    u16 button;
    bool repaint = TRUE;
    option = 0;
    
    do{
        TCP_paint(repaint);
        button = readButton(JOY_1);
        repaint = TCP_doAction(button, 0);
        VDP_drawText("*", 0u, option + 5);
        print();
    }while(button != BUTTON_A);

}

void TCP_paint(bool repaint){
    if(repaint){
        clearScreen();        
        VDP_drawText("TCP Test", 1u, 2u);
        VDP_drawText("Press START to launch", 1u, 3u);
        VDP_drawText("Press A to return", 1u, 4u);
    }
}

bool TCP_doAction(u16 button, u8 max_option){    
    switch (button){
    case BUTTON_START:{
        TCP_test();
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
	err = mw_tcp_connect(1, "www.example.com", "80", NULL);
	if (err != MW_ERR_NONE) {
		println("TCP test FAILED");
		goto out;
	}
	println("DONE!");
	println(NULL);

	println("TCP test SUCCESS");

out:
	println("TCP test ERROR");
	mw_close(1);
}
