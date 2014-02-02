/**
 *  \file asm.h
 *  \brief Assembly helper
 *  \author Stephane Dallongeville
 *  \date 08/2011
 *
 * This unit provides methods to deal with ASM.
 */

#ifndef _ASM_H_
#define _ASM_H_


#define VAR2REG_B(var, reg)       asm ("move.b %0, %/"reg"" :: "r" (var) : ""reg"")
#define VAR2REG_W(var, reg)       asm ("move.w %0, %/"reg"" :: "r" (var) : ""reg"")
#define VAR2REG_L(var, reg)       asm ("move.l %0, %/"reg"" :: "r" (var) : ""reg"")

#define REG2VAR_B(reg, var)       asm ("move.b %/"reg", %0" : "=r" (var))
#define REG2VAR_W(reg, var)       asm ("move.w %/"reg", %0" : "=r" (var))
#define REG2VAR_L(reg, var)       asm ("move.l %/"reg", %0" : "=r" (var))


#endif // _ASM_H_

