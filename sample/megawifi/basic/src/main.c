/****************************************************************************
 * \brief MegaWiFi example.
 *
 * This example demonstrates the following:
 * - How to initialize MegaWiFi
 * - How to do TCP connections
 * - How to do HTTP queries
 * - How to get the date/time (synchronized to NTP servers)
 * - How to send and receive data using UDP protocol
 *
 * To build this example set MODULE_MEGAWIFI to 1 in config.h and
 * rebuild the library.
 *
 * \author Jesus Alonso (doragasu)
 * \date 01/2022
 ****************************************************************************/

#include "genesis.h"

#if (MODULE_MEGAWIFI == 0)
#error "Set MODULE_MEGAWIFI to 1 in config.h and rebuild the library"
#endif

/// Length of the wflash buffer
#define MW_BUFLEN	1460

/// TCP port to use (set to Megadrive release year ;-)
#define MW_CH_PORT 	1985

/// Shut compiler warnings for unused parameters
#define UNUSED_PARAM(par) (void)(par)

/// Tuned for 60 Hz, change it for PAL consoles
#define MS_TO_FRAMES(ms)  ((((ms) * 60 / 500) + 1)/2)

/// Command buffer. Must be word aligned
static char cmd_buf[MW_BUFLEN] __attribute__((aligned(2)));

/// UDP receive function callback
static void udp_recv_cb(enum lsd_status stat, uint8_t ch,
		char *data, uint16_t len, void *ctx);

/// Certificate for www.example.org
static const char cert[] = "-----BEGIN CERTIFICATE-----\n"
"MIIElDCCA3ygAwIBAgIQAf2j627KdciIQ4tyS8+8kTANBgkqhkiG9w0BAQsFADBh"
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3"
"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD"
"QTAeFw0xMzAzMDgxMjAwMDBaFw0yMzAzMDgxMjAwMDBaME0xCzAJBgNVBAYTAlVT"
"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxJzAlBgNVBAMTHkRpZ2lDZXJ0IFNIQTIg"
"U2VjdXJlIFNlcnZlciBDQTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB"
"ANyuWJBNwcQwFZA1W248ghX1LFy949v/cUP6ZCWA1O4Yok3wZtAKc24RmDYXZK83"
"nf36QYSvx6+M/hpzTc8zl5CilodTgyu5pnVILR1WN3vaMTIa16yrBvSqXUu3R0bd"
"KpPDkC55gIDvEwRqFDu1m5K+wgdlTvza/P96rtxcflUxDOg5B6TXvi/TC2rSsd9f"
"/ld0Uzs1gN2ujkSYs58O09rg1/RrKatEp0tYhG2SS4HD2nOLEpdIkARFdRrdNzGX"
"kujNVA075ME/OV4uuPNcfhCOhkEAjUVmR7ChZc6gqikJTvOX6+guqw9ypzAO+sf0"
"/RR3w6RbKFfCs/mC/bdFWJsCAwEAAaOCAVowggFWMBIGA1UdEwEB/wQIMAYBAf8C"
"AQAwDgYDVR0PAQH/BAQDAgGGMDQGCCsGAQUFBwEBBCgwJjAkBggrBgEFBQcwAYYY"
"aHR0cDovL29jc3AuZGlnaWNlcnQuY29tMHsGA1UdHwR0MHIwN6A1oDOGMWh0dHA6"
"Ly9jcmwzLmRpZ2ljZXJ0LmNvbS9EaWdpQ2VydEdsb2JhbFJvb3RDQS5jcmwwN6A1"
"oDOGMWh0dHA6Ly9jcmw0LmRpZ2ljZXJ0LmNvbS9EaWdpQ2VydEdsb2JhbFJvb3RD"
"QS5jcmwwPQYDVR0gBDYwNDAyBgRVHSAAMCowKAYIKwYBBQUHAgEWHGh0dHBzOi8v"
"d3d3LmRpZ2ljZXJ0LmNvbS9DUFMwHQYDVR0OBBYEFA+AYRyCMWHVLyjnjUY4tCzh"
"xtniMB8GA1UdIwQYMBaAFAPeUDVW0Uy7ZvCj4hsbw5eyPdFVMA0GCSqGSIb3DQEB"
"CwUAA4IBAQAjPt9L0jFCpbZ+QlwaRMxp0Wi0XUvgBCFsS+JtzLHgl4+mUwnNqipl"
"5TlPHoOlblyYoiQm5vuh7ZPHLgLGTUq/sELfeNqzqPlt/yGFUzZgTHbO7Djc1lGA"
"8MXW5dRNJ2Srm8c+cftIl7gzbckTB+6WohsYFfZcTEDts8Ls/3HB40f/1LkAtDdC"
"2iDJ6m6K7hQGrn2iWZiIqBtvLfTyyRRfJs8sjX7tN8Cp1Tm5gr8ZDOo0rwAhaPit"
"c+LJMto4JQtV05od8GiG7S5BNO98pVAdvzr508EIDObtHopYJeS4d60tbvVS3bR0"
"j6tJLp07kzQoH3jOlOrHvdPJbRzeXDLz\n"
"-----END CERTIFICATE-----";
static const uint16_t cert_len = sizeof(cert) - 1;
/// Certificate hash, obtained with command:
/// openssl x509 -hash in <cert_file_name> -noout
static const uint32_t cert_hash = 0x85cf5865;

/// Just print a line
static void println(const char *str)
{
	static unsigned int line = 2;

	if (str) {
		VDP_drawText(str, 1, line);
	}
	line++;
}

/// Idle task, run using the spare CPU time available when the main task
/// pends or yields. See mw/tsk.h for details. Basically this task polls
/// the WiFi module to send and receive data.
static void idle_tsk(void)
{
	while (1) {
		mw_process();
	}
}

/// Callback set by mw_upd_reuse_send(), run when data is sent
static void udp_send_complete_cb(enum lsd_status stat, void *ctx)
{
	struct mw_reuse_payload *pkt =
		(struct mw_reuse_payload * const)cmd_buf;
	UNUSED_PARAM(ctx);
	UNUSED_PARAM(stat);

	// Trigger reception of another UDP packet
	mw_udp_reuse_recv(pkt, MW_BUFLEN, NULL, udp_recv_cb);
}

/// Callback set by mw_udp_reuse_recv(), run when data is received
static void udp_recv_cb(enum lsd_status stat, uint8_t ch,
		char *data, uint16_t len, void *ctx)
{
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

/// Sends "MegaWiFi UDP test!" string to 127.0.0.1:12345. If you are not using
/// an emulator, change the 127.0.0.1 address with the one for your PC. On your
/// PC you can receive UDP data e.g. by running command: $ nc -lu 12345
static void udp_normal_test(void)
{
	char line[40];
	int16_t len = sizeof(line);
	uint8_t ch = 1;

	// Make sure you are listening on the target address, e.g. with command:
	// $ nc -lu 12345
	println("Send to UDP 12345, waiting for reply");
	// Send UDP data to peer and wait for reply. Localhost works only when
	// using emulators, so change IP as needed when using the real thing.
	if (mw_udp_set(ch, "127.0.0.1", "12345", NULL)) {
		goto err;
	}
	mw_send_sync(ch, "MegaWiFi UDP test!\n", 20, TSK_PEND_FOREVER);
	mw_recv_sync(&ch, line, &len, TSK_PEND_FOREVER);
	line[min(39, len)] = '\0';
	if (1 == ch) {
		println("Got UDP reply:");
		println(line);
	}
	mw_close(ch);

	return;

err:
	println("UDP test failed!");
	mw_close(ch);
}

/// Implements an UDP echo server at port 8007. You can send data to this port
/// and receive the echo e.g. by running command: $ nc -u <MEGAWIFI_IP_ADDR> 8007
static void udp_reuse_test(void)
{
	struct mw_reuse_payload *pkt =
		(struct mw_reuse_payload * const)cmd_buf;

	// You can send text and get the echo e.g. by:
	// nc -u <dest_ip> 8007
	println("Doing echo on UDP port 8007");
	// Start UDP echo task
	mw_udp_set(2, NULL, NULL, "8007");
	mw_udp_reuse_recv(pkt, MW_BUFLEN, NULL, udp_recv_cb);
}

/// Receives data from the HTTP test
static int http_recv(uint32_t len)
{
	int16_t recv_last;
	int err = FALSE;
	uint32_t recvd = 0;
	uint8_t ch = MW_HTTP_CH;

	// For the test, just read and discard the data
	while (recvd < len && !err) {
		recv_last = MW_BUFLEN;
		err = mw_recv_sync(&ch, cmd_buf, &recv_last, TSK_PEND_FOREVER) != MW_ERR_NONE;
		recvd += recv_last;
	}

	return err;
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
static void http_test(void)
{
	uint32_t len = 0;

	http_cert_set();

	if (mw_http_url_set("https://www.example.com") ||
			mw_http_method_set(MW_HTTP_METHOD_GET) ||
			mw_http_open(0) ||
			mw_http_finish(&len, MS_TO_FRAMES(20000)) < 100) {
		goto err_out;
	}
	if (len) {
		if (http_recv(len)) goto err_out;
	}

	println("HTTP test SUCCESS");
	return;

err_out:
	println("HTTP test FAILED");
	return;
}

/// Tests a direct TCP connection to www.example.com on port 80
static void tcp_test(void)
{
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
	mw_close(1);
}

/// Waits until date/time is synchronized and gets the date/time
static void datetime_test(void)
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

/// Associates to the AP and runs the tests
static void run_test(void)
{
	enum mw_err err;
	int16_t def_ap = mw_def_ap_cfg_get();

	def_ap = def_ap < 0 ? 0 : def_ap;

	// Join AP
	println("Associating to AP");
	err = mw_ap_assoc(def_ap);
	if (err != MW_ERR_NONE) {
		goto err;
	}
	err = mw_ap_assoc_wait(MS_TO_FRAMES(30000));
	if (err != MW_ERR_NONE) {
		goto err;
	}
	mw_sleep(3 * 60);
	println("DONE!");
	println(NULL);

	tcp_test();
	http_test();
	datetime_test();

	// Test UDP in normal mode
	udp_normal_test();

	// Test UDP in reuse mode
	udp_reuse_test();

	while (1) {
		// Yield forever to user task
		SYS_doVBlankProcess();
	}

	return;

err:
	println("ERROR!");
	mw_ap_disassoc();
}

/// MegaWiFi initialization
/// Returns true on error
static bool megawifi_init(void)
{
	uint8_t ver_major = 0, ver_minor = 0;
	char *variant = NULL;
	enum mw_err err;
	char line[] = "MegaWiFi version X.Y";
	bool ret;

	// Try detecting the module
	err = mw_detect(&ver_major, &ver_minor, &variant);

	if (MW_ERR_NONE != err) {
		// Megawifi not found
		println("MegaWiFi not found!");
		ret = TRUE;
	} else {
		// Megawifi found
		line[17] = ver_major + '0';
		line[19] = ver_minor + '0';
		println(line);
		println(NULL);
		ret = FALSE;
	}

	return ret;
}

/// Set idle task that polls WiFi module
static void tasking_init(void)
{
	TSK_userSet(idle_tsk);
}

/// Global initialization
static void init(void)
{
	// Initialize MegaWiFi
	mw_init((u16*) cmd_buf, MW_BUFLEN);
	// Initialize multitasking (for WiFi background checks)
	tasking_init();
}

/// Entry point
int main(bool hard)
{
	UNUSED_PARAM(hard);
	bool err;

	init();

	err = megawifi_init();
	if (!err) {
		run_test();
	}

	return 0;
}
