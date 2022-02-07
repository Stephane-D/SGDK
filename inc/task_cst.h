/**
 * \file task_cst.h
 * \brief User task constantes definition
 * \date 12/2021
 * \author doragasu
 *
 * This module define constantes for the task unit (see task.h file).
 * This needs to be separated as we include it from task.a assembly file !
 */

#ifndef __TASK_CST_H__
#define __TASK_CST_H__

/**
 *  \brief
 *      Byte length for the user task stack
 */
#define USER_STACK_LENGTH   512

/**
 *  \brief
 *      User task registers save buffer size
 */
#define UTSK_REGS_LEN       (15 * 4)

/**
 *  \brief
 *      Timeout value to use for infinite waits
 */
#define TSK_PEND_FOREVER    -1


#endif /*__TASK_CST_H__*/
