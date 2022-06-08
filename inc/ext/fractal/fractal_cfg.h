/**
 *  \file ext/fractal/fractal_cfg.h
 *  \brief Fractal constants
 *  \author Aurora*Fields
 *  \date 06/2022
 *
 * Provides constants / configuration for Fractal Sound.
 */

#ifndef _FRACTAL_CFG_H_
#define _FRACTAL_CFG_H_

#include "config.h"


#if (MODULE_FRACTAL != 0)

/**
 *  \brief
 *      Helper to define bottom of Fractal Sound RAM. THIS SHOULD BE IN MEMORY.H, BUT IT DOESNT COMPILE IN ASM
 */
#define _STACK_SIZE_ 0xA00
#define MEMORY_HIGH_FRAC (0 - _STACK_SIZE_)

/**
 *  \brief
 *      Safe mode controls whether extra checks are used to deteermine if something is going wrong in the driver.
 *      WARNING: currently not supported. Will fix this in the future!
 */
#define SAFE_MODE				0
/**
 *  \brief
 *      Whether Visual Debugger is built into the driver. If this is off, Visual Debugger can't be used.
 *      WARNING: currently not supported. Will fix this in the future!
 */
#define VISUAL_DEBUG			1
/**
 *  \brief
 *      If special sound test variables are built into each channel. Affects chipVolume, chipFraction and chipFrequency. If disabled, uses less RAM but does not support these variables
 */
#define FEATURE_SOUNDTEST		0+VISUAL_DEBUG
/**
 *  \brief
 *      Determines stack depth for the channel. Remember, you must also update res/Fractal/Data.json5 to contain the stack depth as well. More depth uses more RAM, but allows for better compression of sound data.
 */
#define FEATURE_STACK_DEPTH		4
/**
 *  \brief
 *      Number of entries in the sound queue. This allows multiple songs be played per frame without any losses.
 */
#define FEATURE_QUEUE_SIZE		4
/**
 *  \brief
 *      Whether music backup is enabled. If yes, some songs can pause music, and restore it after the end. This is how 1-up sound effect works in Sonic games. Uses a lot of RAM
 */
#define FEATURE_BACKUP			1
/**
 *  \brief
 *      Whether to detect redkid based hardware, and attempt to fix its sound.
 */
#define FEATURE_REDKIDFIX		1

#endif // MODULE_FRACTAL

#endif // _FRACTAL_CFG_H_
