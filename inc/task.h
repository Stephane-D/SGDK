/**
 * \file task.h
 * \brief User task support
 * \date 12/2021
 * \author doragasu
 *
 * This module allows assigning a function that will run as an user task.
 * CPU time is given to that function in several ways:
 * - When the supervisor task calls TSK_userYield(), the user task executes
 *   immediately until the next VBlank interrupt. Then supervisor task is
 *   resumed.
 * - When the supervisor task call TSK_superPend(), the user task executes
 *   immediately until one of the following happens:
 *   a) The user task calls TSK_superPend().
 *   b) The timeout specified in TSK_superPost() expires.
 *
 * For the scheduler to work, VBlank interrupts must be enabled.
 *
 * These functions are implemented in task.s
 */

#ifndef __TASK_H__
#define __TASK_H__

#include "task_cst.h"


/**
 * \brief Configure the user task callback function.<br>
 *  Must be set with a not NULL callback before calling any TSK_xxx functions.
 *
 * \param task A function pointer to the user task (or NULL to disable multitasking).
 */
void TSK_userSet(VoidCallback *task);

/**
 * \brief Stop the user task.<br>
 *  This has the same effect than using TSK_setUser(NULL).
 */
void TSK_stop(void);

/**
 * \brief Yield from supervisor task to user task. The user task will resume
 * and will use all the available CPU time until the next vertical blanking
 * interrupt, that will resume the supervisor task.
 */
void TSK_userYield(void);

/**
 * \brief Block supervisor task and resume user task. Supervisor task will
 * not resume execution until #TSK_superPost() is called from user task or a
 * timeout happens..
 *
 * \param wait Maximum number of frames to wait while blocking. Use
 *            TSK_PEND_FOREVER for an infinite wait, or a positive number
 *            (greater than 0) for a specific number of frames.
 *
 * \return false if task was awakened from user task, or true if timeout
 * occurred.
 */
bool TSK_superPend(s16 wait);

/**
 * \brief Resume a blocked supervisor task. Must be called from user task.
 *
 * \param immediate If true, immediately causes a context switch to
 *            supervisor task. If false, context switch will not occur until
 *            the VBLANK interrupt.
 */
void TSK_superPost(bool immediate);

#endif /*__TASK_H__*/
