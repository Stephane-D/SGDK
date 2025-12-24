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
"MIIDNDCCArmgAwIBAgIQYE2K+NALqHSLlVhTFyxfLjAKBggqhkjOPQQDAzBOMQsw\n"
"CQYDVQQGEwJVUzEYMBYGA1UECgwPU1NMIENvcnBvcmF0aW9uMSUwIwYDVQQDDBxT\n"
"U0wuY29tIFRMUyBFQ0MgUm9vdCBDQSAyMDIyMB4XDTIyMTAyMTE3MDIyM1oXDTM3\n"
"MTAxNzE3MDIyMlowTzELMAkGA1UEBhMCVVMxGDAWBgNVBAoMD1NTTCBDb3Jwb3Jh\n"
"dGlvbjEmMCQGA1UEAwwdU1NMLmNvbSBUTFMgVHJhbnNpdCBFQ0MgQ0EgUjIwdjAQ\n"
"BgcqhkjOPQIBBgUrgQQAIgNiAARk532ZA1NckR7q+NgjraG/LOJjie8oaPbt1/Ds\n"
"q2iudyvkdpcbUOvbWSgtb7g2uauNl8pMIp7uidkCP/16czqQjSvMLzo3g9oNtC1F\n"
"G3NyCWVfeCE954tmP0f9CSnWFA+jggFZMIIBVTASBgNVHRMBAf8ECDAGAQH/AgEB\n"
"MB8GA1UdIwQYMBaAFImPL6PoK6AUVHvzVrgmX2c4C5zQMEwGCCsGAQUFBwEBBEAw\n"
"PjA8BggrBgEFBQcwAoYwaHR0cDovL2NlcnQuc3NsLmNvbS9TU0xjb20tVExTLVJv\n"
"b3QtMjAyMi1FQ0MuY2VyMD8GA1UdIAQ4MDYwNAYEVR0gADAsMCoGCCsGAQUFBwIB\n"
"Fh5odHRwczovL3d3dy5zc2wuY29tL3JlcG9zaXRvcnkwHQYDVR0lBBYwFAYIKwYB\n"
"BQUHAwIGCCsGAQUFBwMBMEEGA1UdHwQ6MDgwNqA0oDKGMGh0dHA6Ly9jcmxzLnNz\n"
"bC5jb20vU1NMY29tLVRMUy1Sb290LTIwMjItRUNDLmNybDAdBgNVHQ4EFgQUMqLH\n"
"2FiL/3/APPJVaTPszswfvJcwDgYDVR0PAQH/BAQDAgGGMAoGCCqGSM49BAMDA2kA\n"
"MGYCMQC4SkI+e2cts1nTN9MCRil97z624WxLAp94hT7tNZGPZLe9YiLIyzgKqW/b\n"
"E0b2h9ACMQCvV5XMRcunAylQaCQc4J/GwR1p7yrPC0DRWWeyLAkQWi5Ylta9DxlX\n"
"74QFFksFCP0=\n"
"-----END CERTIFICATE-----\n";

static const uint16_t cert_len = sizeof(cert);
/// openssl s_client -showcerts -connect www.example.org:443 </dev/null
///   Get ROOT CA: SSL.com TLS ECC Root CA 2022
/// Certificate hash, obtained with command:
/// openssl x509 -hash -in <cert_file_name> -noout
static const uint32_t cert_hash = 0xb35d1870;

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
void paint_long_char(const char *cert, u16 len, u8* line);
u16 readButton(u16 joy);
int readText(char* buffer, size_t lengthMax);
void delay_ms(u16 milliseconds);
void printStatus(union mw_msg_sys_stat * status);


#endif // _UTILS_H_
