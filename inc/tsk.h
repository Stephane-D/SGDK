/************************************************************************//**
 * \file tsk.h
 * \brief User task support
 * \date 12/2021
 *
 * This module allows assigning a function that will run as an user task.
 * CPU time is given to that function in several ways:
 * - When the supervisor task calls tsk_user_yield(), the user task executes
 *   immediately until the next VBlank interrupt. Then supervisor task is
 *   resumed.
 * - When the supervisor task call tsk_super_pend(), the user task executes
 *   immediately until one of the following happens:
 *   a) The user task calls tsk_super_pend().
 *   b) The timeout specified in tsk_super_post() expires.
 *
 * These functions are implemented in boot/sega.s
 ****************************************************************************/

#ifndef __TSK_H__
#define __TSK_H__

#include "types.h"

#define TSK_PEND_FOREVER -1

/************************************************************************//**
 * \brief Configure the task used as user task. Must be invoked once before
 * calling tsk_user_yield().
 *
 * \param[in] user_tsk A function pointer to the user task to configure.
 ****************************************************************************/
void tsk_user_set(void (*user_tsk)(void));

/************************************************************************//**
 * \brief Yield from supervisor task to user task. The user task will resume
 * and will use all the available CPU time until the next vertical blanking
 * interrupt, that will resume the supervisor task.
 ****************************************************************************/
void tsk_user_yield(void);

/************************************************************************//**
 * \brief Block supervisor task and resume user task. Supervisor task will
 * not resume execution until super_tsk_post() is called from user task or a
 * timeout happens..
 *
 * \param[in] wait_tout Maximum number of frames to wait while blocking. Use
 *            TSK_PEND_FOREVER for an infinite wait, or a positive number
 *            (greater than 0) for a specific number of frames.
 *
 * \return false if task was awakened from user task, or true if timeout
 * occurred.
 ****************************************************************************/
s8 tsk_super_pend(int16_t wait_tout);

/************************************************************************//**
 * \brief Resume a blocked supervisor task. Must be called from user task.
 *
 * \param[in] force_ctx_sw If true, immediately causes a context switch to
 *            supervisor task. If false, context switch will not occur until
 *            the VBLANK interrupt.
 ****************************************************************************/
void tsk_super_post(s8 force_ctx_sw);

#endif /*__TSK_H__*/
