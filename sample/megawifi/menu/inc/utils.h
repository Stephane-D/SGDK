/****************************************************************************
 * \brief MegaWiFi2 example.
 * 
 * \author Juan Antonio (PaCHoN) 
 * \author Jesus Alonso (doragasu)
 * 
 * \date 01/2025
 ****************************************************************************/

#ifndef _UTILS_H_
#define _UTILS_H_

#include <genesis.h>

#define DEFAULT_DELAY 500
#define DEFAULT_MW_DELAY 1000

/// Shut compiler warnings for unused parameters
#define UNUSED_PARAM(par) (void)(par)

/// Tuned for 60 Hz, change it for PAL consoles
#define MS_TO_FRAMES(ms)  ((((ms) * 60 / 500) + 1)/2)

/// Certificate for www.example.org
static const char cert[] = "-----BEGIN CERTIFICATE-----\n"
"MIIDeTCCAv+gAwIBAgIQCwDpLU1tcx/KMFnHyx4YhjAKBggqhkjOPQQDAzBhMQsw"
"CQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3d3cu"
"ZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBHMzAe"
"Fw0yMTA0MTQwMDAwMDBaFw0zMTA0MTMyMzU5NTlaMFkxCzAJBgNVBAYTAlVTMRUw"
"EwYDVQQKEwxEaWdpQ2VydCBJbmMxMzAxBgNVBAMTKkRpZ2lDZXJ0IEdsb2JhbCBH"
"MyBUTFMgRUNDIFNIQTM4NCAyMDIwIENBMTB2MBAGByqGSM49AgEGBSuBBAAiA2IA"
"BHipnHWuiF1jpK1dhtgQSdavklljQyOF9EhlMM1KNJWmDj7ZfAjXVwUoSJ4Lq+vC"
"05ae7UXSi4rOAUsXQ+Fzz21zSDTcAEYJtVZUyV96xxMH0GwYF2zK28cLJlYujQf1"
"Z6OCAYIwggF+MBIGA1UdEwEB/wQIMAYBAf8CAQAwHQYDVR0OBBYEFIoj655r1/k3"
"XfltITl2mqFn3hCoMB8GA1UdIwQYMBaAFLPbSKT5ocXYrjZBzBFjaWIpvEvGMA4G"
"A1UdDwEB/wQEAwIBhjAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwdgYI"
"KwYBBQUHAQEEajBoMCQGCCsGAQUFBzABhhhodHRwOi8vb2NzcC5kaWdpY2VydC5j"
"b20wQAYIKwYBBQUHMAKGNGh0dHA6Ly9jYWNlcnRzLmRpZ2ljZXJ0LmNvbS9EaWdp"
"Q2VydEdsb2JhbFJvb3RHMy5jcnQwQgYDVR0fBDswOTA3oDWgM4YxaHR0cDovL2Ny"
"bDMuZGlnaWNlcnQuY29tL0RpZ2lDZXJ0R2xvYmFsUm9vdEczLmNybDA9BgNVHSAE"
"NjA0MAsGCWCGSAGG/WwCATAHBgVngQwBATAIBgZngQwBAgEwCAYGZ4EMAQICMAgG"
"BmeBDAECAzAKBggqhkjOPQQDAwNoADBlAjB+Jlhu7ojsDN0VQe56uJmZcNFiZU+g"
"IJ5HsVvBsmcxHcxyeq8ickBCbmWE/odLDxkCMQDmv9auNIdbP2fHHahv1RJ4teaH"
"MUSpXca4eMzP79QyWBH/OoUGPB2Eb9P1+dozHKQ="
"-----END CERTIFICATE-----";

static const uint16_t cert_len = sizeof(cert);
/// Certificate hash, obtained with command:
/// openssl x509 -hash in <cert_file_name> -noout
static const uint32_t cert_hash = 0xa6570d26;

static u16 option __attribute__((unused)) = 0; 
static char buffer[128] __attribute__((unused));
static long ciclo __attribute__((unused)) = 0;

/// Command buffer. Must be word aligned
static char cmd_buf[MW_BUFLEN] __attribute__((unused));

#define SCREEN_COLUMNS 20U // Número de columnas en pantalla
#define SCREEN_ROWS 28U    // Número de filas en pantalla

void clearScreen();
void println(const char *str);
void print();
void paint_long_char(const char *cert, u16 len, u8 line);
u16 readButton(u16 joy);
int readText(char* buffer, size_t lengthMax);
void delay_ms(u16 milliseconds);
void printStatus(union mw_msg_sys_stat * status);


#endif // _UTILS_H_
