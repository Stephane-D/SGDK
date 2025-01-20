#include "mw-api/http.h"


/// Receives data from the HTTP test
static int http_recv(uint32_t len)
{
	int16_t recv_last;
	int err = FALSE;
	uint32_t recvd = 0;
	uint8_t ch = MW_HTTP_CH;

	u8 line = 11u;

	while (recvd < len && !err) {
		recv_last = MW_BUFLEN;
		err = mw_recv_sync(&ch, cmd_buf, &recv_last, TSK_PEND_FOREVER) != MW_ERR_NONE;
		paint_long_char(cmd_buf, recv_last - recvd, line);
		line++;
		recvd += recv_last;
	}

	return err;
}

void HTTP_start(){
    
    u16 button;
    bool repaint = TRUE;
    
    do{
        HTTP_paint(repaint);
        button = readButton(JOY_1);
        repaint = HTTP_doAction(button, 0);
        print();
    }while(button != BUTTON_A);

}

void HTTP_paint(bool repaint){
    if(repaint){
        clearScreen();        
        VDP_drawText("HTTPs Test", 1u, 2u);
        VDP_drawText("Press START to launch", 0u, 3u);
        VDP_drawText("Press A to return", 0u, 4u);
        VDP_drawText("Press B by Http", 0u, 5u);
    }
}

bool HTTP_doAction(u16 button, u8 max_option){    
    switch (button){
    case BUTTON_START:
        HTTP_test("https://www.example.com", TRUE);
		break;
    case BUTTON_B:
        HTTP_test("http://www.example.com", FALSE);
		break;
    default:
    }    
    return FALSE;
}

/// Sets the certificate for the HTTPS TLS connection
static void http_cert_set(void)
{
	uint32_t hash = mw_http_cert_query();
	if (hash != cert_hash) {
		mw_http_cert_set(cert_hash, cert, cert_len);
	}
}


/// Test an HTTP GET request to https://www.example.com
void HTTP_test(const char* url, bool ssl) {
	uint32_t len = 0;
	enum mw_err err = MW_ERR_NONE;
	s16 errHttp = -1;
	if(ssl){		
		http_cert_set();
	}else{
		CONFIG_CERT_clear();
	}

	println("Setting URL               ");
	print();
	delay_ms(1000);
	err = mw_http_url_set(url); 
	if (err) goto err_out;
	println("Setting Method            ");
	print();
	delay_ms(1000);
	err = mw_http_method_set(MW_HTTP_METHOD_GET);
	if (err) goto err_out;
	println("Open URL                  ");
	print();
	delay_ms(1000);
	err = mw_http_open(0);
	if (err) goto err_out;
	println("Finish URL                ");
	print();
	delay_ms(1000);
	errHttp = mw_http_finish(&len, MS_TO_FRAMES(20000));
	if (errHttp < 100) goto err_out;
	if (len) {
		if (http_recv(len)) goto err_out;
	}
	println("HTTP test SUCCESS         ");
	return;
err_out:
	sprintf(buffer, "MW-ERROR: %u HTTP-ERROR: %d", err, errHttp);
	println(buffer);
}
