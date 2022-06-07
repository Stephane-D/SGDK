/**
 *  \file memory_base.h
 *  \brief Memory base definition
 *  \author Stephane Dallongeville
 *  \date 06/2022
 *
 * This unit provides basic memory address definitions.<br>
 * We keep this H unit separated so it could eventually be included / used in assembly file if needed.
 */

#ifndef _MEMORY_BASE_H_
#define _MEMORY_BASE_H_

/**
 *  \brief
 *      Define start of ROM region
 */
#define ROM      0x00000000
/**
 *  \brief
 *      Define start of RAM region
 */
#define RAM      0xE0FF0000

/**
 *  \brief
 *      Define memory allocated for stack (default = 0xA00)
 */
#define STACK_SIZE      0x0A00
/**
 *  \brief
 *      Define the memory high address limit for dynamic allocation
 */
#define MEMORY_HIGH     (0xE1000000 - STACK_SIZE)


#endif // _MEMORY_BASE_H_
