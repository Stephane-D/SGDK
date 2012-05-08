/**
 * \file tools.h
 * \brief Misc tools methods
 * \author Stephane Dallongeville
 * \date 08/2011
 *
 * This unit provides some misc tools methods as getFPS()
 */

#ifndef _TOOLS_H_
#define _TOOLS_H_


/**
 * \brief
 *      Returns number of Frame Per Second.
 *
 * This function actually returns the number of time it was called in the last second.<br>
 * i.e: for benchmarking you should call this method only once per frame update.
 */
u32 getFPS();
/**
 * \brief
 *      Returns number of Frame Per Second (fix32 form).
 *
 * This function actually returns the number of time it was called in the last second.<br>
 * i.e: for benchmarking you should call this method only once per frame update.
 */
fix32 getFPS_f();


#endif // _TOOLS_H_
