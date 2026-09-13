#ifndef _UNITY_CONFIG_H_
#define _UNITY_CONFIG_H_

#include "types.h"

#if defined(__m68k__) || defined(SGDK_GCC)
  #include "kdebug.h"
  #ifndef UNITY_EXCLUDE_SETJMP_H
    #define UNITY_EXCLUDE_SETJMP_H
  #endif
  #ifndef UNITY_EXCLUDE_MATH_H
    #define UNITY_EXCLUDE_MATH_H
  #endif
  #ifndef UNITY_EXCLUDE_STDIO_H
    #define UNITY_EXCLUDE_STDIO_H
  #endif
  #ifndef UNITY_EXCLUDE_STDBOOL_H
    #define UNITY_EXCLUDE_STDBOOL_H
  #endif
  #ifndef UNITY_EXCLUDE_STDDEF_H
    #define UNITY_EXCLUDE_STDDEF_H
  #endif
  #ifndef UNITY_EXCLUDE_STDINT_H
    #define UNITY_EXCLUDE_STDINT_H
  #endif
  #ifndef UNITY_EXCLUDE_LIMITS_H
    #define UNITY_EXCLUDE_LIMITS_H
  #endif
  #ifndef CMOCK_MEM_INDEX_TYPE
    #define CMOCK_MEM_INDEX_TYPE u32
  #endif
  
  static char sgdk_unity_buffer[256];
  static u16 sgdk_unity_buf_idx = 0;

  static inline void SGDK_UnityOutputFlush(void) {
      if (sgdk_unity_buf_idx > 0) {
          sgdk_unity_buffer[sgdk_unity_buf_idx] = '\0';
          KDebug_Alert(sgdk_unity_buffer);
          sgdk_unity_buf_idx = 0;
      }
  }

  static inline void SGDK_UnityOutputChar(char c) {
      if (c == '\r') {
          return;
      }
      if (c == '\n') {
          SGDK_UnityOutputFlush();
      } else {
          if (sgdk_unity_buf_idx >= sizeof(sgdk_unity_buffer) - 1) {
              SGDK_UnityOutputFlush();
          }
          sgdk_unity_buffer[sgdk_unity_buf_idx++] = (char)c;
      }
  }

  #ifndef UNITY_OUTPUT_CHAR
    #define UNITY_OUTPUT_CHAR(a) SGDK_UnityOutputChar(a)
  #endif
  #ifndef UNITY_OUTPUT_FLUSH
    #define UNITY_OUTPUT_FLUSH() SGDK_UnityOutputFlush()
  #endif
#endif

#endif // _UNITY_CONFIG_H_
