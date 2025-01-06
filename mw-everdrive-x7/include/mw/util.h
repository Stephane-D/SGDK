#pragma once

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <esp_log.h>

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

/// Stringify token, helper macro
#define _STR(x)		#x
/// Stringify token
#define STR(x)		_STR(x)

/// Remove compiler warnings when not using a function parameter
#define UNUSED_PARAM(x)		(void)x

/// Swaps bytes from a word (16 bits)
#define ByteSwapWord(w)		(uint16_t)((((uint16_t)(w))>>8) | (((uint16_t)(w))<<8))

/// Swaps bytes from a dword (32 bits)
#define ByteSwapDWord(dw)	(uint32_t)((((uint32_t)(dw))>>24) |               \
		((((uint32_t)(dw))>>8) & 0xFF00) | ((((uint32_t)(dw)) & 0xFF00)<<8) | \
	  	(((uint32_t)(dw))<<24))

/// Swaps bytes from a qword (64 bits)
#define ByteSwapQWord(qw)	(uint64_t)((((uint64_t)(qw))>>56) |                \
	((((uint64_t)(qw))>>40) & 0xFF00) | ((((uint64_t)(qw))>>24) & 0xFF0000) | \
    ((((uint64_t)(qw))>>8) & 0xFF000000) |                                    \
    ((((uint64_t)(qw)) & 0xFF000000)<<8) |                                    \
    ((((uint64_t)(qw)) & 0xFF0000)<<24)  |                                    \
    ((((uint64_t)(qw)) & 0xFF00)<< 40)   | (((uint64_t)(qw))<<56))

/// Puts a task to sleep for some milliseconds (requires FreeRTOS).
#define vTaskDelayMs(ms)	vTaskDelay((ms)/portTICK_PERIOD_MS)

#if !defined(MAX)
/// Returns the maximum of two numbers
#define MAX(a, b)	((a)>(b)?(a):(b))
#endif
#if !defined(MIN)
/// Returns the minimum of two numbers
#define MIN(a, b)	((a)<(b)?(a):(b))
#endif

int urlencode(char *out, const char *in);

// String length must be at least (2 * bin_len + 1)
void bin_to_str(const uint8_t *bin, unsigned bin_len, char *str);

#define md5_to_str(bin, str) bin_to_str(bin, 16, str)

#define sha1_to_str(bin, str) bin_to_str(bin, 20, str)

int ipv4_to_str(uint32_t ipv4, char str[14]);

/// Similar to strcpy, but returns a pointer to the last character of the
/// src input string (the ending '\0')
const char *StrCpySrc(char *dst, const char *src);

/// Similar to strcpy, but returns a pointer to the last character of the
/// dst output string (the ending '\0')
char *StrCpyDst(char *dst, const char *src);

int itemizer(const char *input, const char **item, int max_tokens);

