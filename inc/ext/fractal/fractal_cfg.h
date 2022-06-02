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

#define SAFE_MODE				0
#define VISUAL_DEBUG			1
#define FEATURE_SOUNDTEST		1|VISUAL_DEBUG
#define FEATURE_STACK_DEPTH		4
#define FEATURE_QUEUE_SIZE		4
#define FEATURE_BACKUP			1
#define FEATURE_REDKIDFIX		1

#endif // MODULE_FRACTAL

#endif // _FRACTAL_CFG_H_
