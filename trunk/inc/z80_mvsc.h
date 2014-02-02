/**
 *  \file z80_mvsc.h
 *  \brief Z80 MVS driver constantes
 *  \author Ivan / Pascal / Kaneda
 *  \date 04/2007
 */

#ifndef _Z80_MVSC_H_
#define _Z80_MVSC_H_


#define MVS_FM_CMD    0xA01500
#define MVS_FM_TEMPO  0xA01514
#define MVS_FM_ADR    0xA0151E

#define MVS_DAC_CMD   0xA01521
#define MVS_DAC_ADR   0xA01524
#define MVS_DAC_SIZE  0xA01527

#define MVS_PSG_CMD   0xA01529
#define MVS_PSG_CHAN  0xA0152A
#define MVS_PSG_ADR   0xA0152B
#define MVS_PSG_STAT  0xA0152E

#define MVS_FM_STOP   0x00
#define MVS_FM_ONCE   0x01
#define MVS_FM_LOOP   0x02
#define MVS_FM_RESET  0x0F

#define MVS_DAC_PLAY  0x01
#define MVS_DAC_STOP  0xFF

#define MVS_PSG_PLAY  0x01
#define MVS_PSG_STOP  0xFF


#endif // _Z80_MVSC_H_
