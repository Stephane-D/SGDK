#include "mw-api/udp.h"

void UDP_start(){
    
    u16 button;
    bool repaint = TRUE;
    option = 0;
    
    do{
        UDP_paint(repaint);
        button = readButton(JOY_1);
        repaint = UDP_doAction(button, 0);
        print();
    }while(button != BUTTON_A);

}

void UDP_paint(bool repaint){
    if(repaint){
        clearScreen();        
        VDP_drawText("UDP Test", 1u, 2u);
        VDP_drawText("Press START to launch", 0u, 3u);
        VDP_drawText("Press A to return", 0u, 4u);
        VDP_drawText("Press B to reuse", 0u, 5u);
    }
}

bool UDP_doAction(u16 button, u8 max_option){    
    switch (button){
    case BUTTON_START:
        UDP_normal_test();
		break;
	case BUTTON_B:
        UDP_reuse_test();
		break;
    default:
    }    
    return FALSE;
}



/// Callback set by mw_udp_reuse_recv(), run when data is received
void udp_recv_cb(enum lsd_status stat, uint8_t ch, char *data, uint16_t len, void *ctx) {
	const struct mw_reuse_payload *udp =
		(const struct mw_reuse_payload*)data;
	UNUSED_PARAM(ctx);

	// Ignore frame if not from channel 2
	if (LSD_STAT_COMPLETE == stat && 2 == ch) {
		mw_udp_reuse_send(2, udp, len, NULL, udp_send_complete_cb);
	} else {
		mw_udp_reuse_recv((struct mw_reuse_payload*)cmd_buf,
				MW_BUFLEN, NULL, udp_recv_cb);
	}
}

/// Callback set by mw_upd_reuse_send(), run when data is sent
void udp_send_complete_cb(enum lsd_status stat, void *ctx)
{
	struct mw_reuse_payload *pkt = (struct mw_reuse_payload * const)cmd_buf;
	UNUSED_PARAM(ctx);
	UNUSED_PARAM(stat);

	// Trigger reception of another UDP packet
	mw_udp_reuse_recv(pkt, MW_BUFLEN, NULL, udp_recv_cb);
}

/// Sends "MegaWiFi UDP test!" string to 127.0.0.1:12345. If you are not using
/// an emulator, change the 127.0.0.1 address with the one for your PC. On your
/// PC you can receive UDP data e.g. by running command: $ nc -lu 12345
void UDP_normal_test(void) {
	char line[40];
	int16_t len = sizeof(line);
	uint8_t ch = 1;
	enum mw_err err = MW_ERR_NONE, errS = MW_ERR_NONE, errR = MW_ERR_NONE;

	// Make sure you are listening on the target address, e.g. with command:
	// $ nc -lu 12345
	println("Send to UDP 12345, waiting for reply");
	// Send UDP data to peer and wait for reply. Localhost works only when
	// using emulators, so change IP as needed when using the real thing.
	err = mw_udp_set(ch, "127.0.0.1", "12345", NULL);
	if (err) goto err;
	errS = mw_send_sync(ch, "MegaWiFi UDP test!\n", 20, 1000);
	errR = mw_recv_sync(&ch, line, &len, 1000);
	line[min(39, len)] = '\0';
	if (1 == ch) {
		VDP_drawText("Got UDP reply:", 0u, 7u);
        paint_long_char(line, len, 8u);
	}
	mw_close(ch);

	return;

err:
	sprintf(buffer, "MW-ERROR: %u MW-ES: %u MW-ER: %u", err, errS, errR);
	println(buffer);
	mw_close(ch);
}

/// Implements an UDP echo server at port 8007. You can send data to this port
/// and receive the echo e.g. by running command: $ nc -u <MEGAWIFI_IP_ADDR> 8007
void UDP_reuse_test(void) {
	struct mw_reuse_payload *pkt = (struct mw_reuse_payload * const)cmd_buf;
	
	enum mw_err err = MW_ERR_NONE;
	enum lsd_status errR = LSD_STAT_COMPLETE;

	// You can send text and get the echo e.g. by:
	// nc -u <dest_ip> 8007
	println("Doing echo on UDP port 8007");
	print();
	delay_ms(1000);
	// Start UDP echo task
	err = mw_udp_set(2, NULL, NULL, "8007");
	if (err) goto err;
	println("Receiving from UDP 8007, waiting for reply");
	print();
	delay_ms(1000);
	errR = mw_udp_reuse_recv(pkt, MW_BUFLEN, NULL, udp_recv_cb);
	if (errR) goto err;
	paint_long_char(pkt->payload, MW_CMD_MAX_BUFLEN - 4 - 2, 11u);	
	mw_close(2);
	return;

err:	
	sprintf(buffer, "MW-ERROR: %u LSD-ER: %d", err, errR);
	println(buffer);
	mw_close(2);
}