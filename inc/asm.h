/**
 *  \file asm.h
 *  \brief Assembly helper
 *  \author Stephane Dallongeville
 *  \date 08/2011
 *
 * This unit provides methods to help dealing with inline assembly or header inclusion GAS.
 */

#ifndef _ASM_H_
#define _ASM_H_


#define VAR2REG_B(var, reg)       asm ("move.b %0, %/"reg"" :: "r" (var) : ""reg"")
#define VAR2REG_W(var, reg)       asm ("move.w %0, %/"reg"" :: "r" (var) : ""reg"")
#define VAR2REG_L(var, reg)       asm ("move.l %0, %/"reg"" :: "r" (var) : ""reg"")

#define REG2VAR_B(reg, var)       asm ("move.b %/"reg", %0" : "=r" (var))
#define REG2VAR_W(reg, var)       asm ("move.w %/"reg", %0" : "=r" (var))
#define REG2VAR_L(reg, var)       asm ("move.l %/"reg", %0" : "=r" (var))

// enumeration helper for GAS
#if defined(__ASSEMBLY__) || defined(__ASSEMBLER__)

  .set last_enum_value, 0
  .macro enum_val name
  .equiv \name, last_enum_value
  .set last_enum_value, last_enum_value + 1
  .endm

    #define ENUM_BEGIN  .set last_enum_value, 0
    #define ENUM_BEGIN_EX(tmpName)  .set last_enum_value, 0
    #define ENUM_VAL(name) enum_val name
    #define ENUM_VALASSIGN(name, value)            \
      .set last_enum_value, value                 ;\
      enum_val name
    #define ENUM_END(enum_name)
#else
    #define ENUM_BEGIN typedef enum {
    #define ENUM_BEGIN_EX(tmpName) typedef enum tmpName {
    #define ENUM_VAL(name) name,
    #define ENUM_VALASSIGN(name, value) name = value,
    #define ENUM_END(enum_name) } enum_name;
#endif


#endif // _ASM_H_

