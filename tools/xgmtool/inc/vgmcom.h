#ifndef VGMCOM_H_
#define VGMCOM_H_


#define VGM_DATA_BLOCK          0x67
#define VGM_END                 0x66
#define VGM_SEEK                0xE0

#define VGM_WRITE_SN76489       0x50
#define VGM_WRITE_YM2612_PORT0  0x52
#define VGM_WRITE_YM2612_PORT1  0x53

#define VGM_WAIT_NTSC_FRAME     0x62
#define VGM_WAIT_PAL_FRAME      0x63

#define VGM_STREAM_CONTROL      0x90
#define VGM_STREAM_DATA         0x91
#define VGM_STREAM_FREQUENCY    0x92
#define VGM_STREAM_START_LONG   0x93
#define VGM_STREAM_STOP         0x94
#define VGM_STREAM_START        0x95

#define VGM_LOOP                0x30


typedef struct
{
    unsigned char* data;
    int offset;
    int command;
    int size;
} VGMCommand;


#include <stdbool.h>
#include "util.h"


VGMCommand* VGMCommand_create(int command);
VGMCommand* VGMCommand_createEx(unsigned char* data, int offset);

bool VGMCommand_isDataBlock(VGMCommand* source);
int VGMCommand_getDataBankId(VGMCommand* source);
int VGMCommand_getDataBlockLen(VGMCommand* source);
bool VGMCommand_isSeek(VGMCommand* source);
int VGMCommand_getSeekAddress(VGMCommand* source);
bool VGMCommand_isEnd(VGMCommand* source);
bool VGMCommand_isLoop(VGMCommand* source);
bool VGMCommand_isPCM(VGMCommand* source);
bool VGMCommand_isWait(VGMCommand* source);
bool VGMCommand_isWaitNTSC(VGMCommand* source);
bool VGMCommand_isWaitPAL(VGMCommand* source);
bool VGMCommand_isShortWait(VGMCommand* source);
int VGMCommand_getWaitValue(VGMCommand* source);
int VGMCommand_computeSize(VGMCommand* source);
unsigned char* VGMCommand_asByteArray(VGMCommand* source);
bool VGMCommand_isPSGWrite(VGMCommand* source);
bool VGMCommand_isPSGEnvWrite(VGMCommand* source);
bool VGMCommand_isPSGToneWrite(VGMCommand* source);
int VGMCommand_getPSGValue(VGMCommand* source);
bool VGMCommand_isYM2612Port0Write(VGMCommand* source);
bool VGMCommand_isYM2612Port1Write(VGMCommand* source);
bool VGMCommand_isYM2612Write(VGMCommand* source);
int VGMCommand_getYM2612Port(VGMCommand* source);
int VGMCommand_getYM2612Register(VGMCommand* source);
int VGMCommand_getYM2612Value(VGMCommand* source);
bool VGMCommand_isYM2612KeyWrite(VGMCommand* source);
bool VGMCommand_isYM2612KeyOffWrite(VGMCommand* source);
int VGMCommand_getYM2612KeyChannel(VGMCommand* source);
bool VGMCommand_isYM26120x2XWrite(VGMCommand* source);
bool VGMCommand_isYM2612TimersWrite(VGMCommand* source);
bool VGMCommand_isYM2612TimersNoSpecialNoCSMWrite(VGMCommand* source);
bool VGMCommand_isStream(VGMCommand* source);
bool VGMCommand_isStreamControl(VGMCommand* source);
bool VGMCommand_isStreamData(VGMCommand* source);
bool VGMCommand_isStreamFrequency(VGMCommand* source);
bool VGMCommand_isStreamStart(VGMCommand* source);
bool VGMCommand_isStreamStartLong(VGMCommand* source);
bool VGMCommand_isStreamStop(VGMCommand* source);
int VGMCommand_getStreamId(VGMCommand* source);
int VGMCommand_getStreamBankId(VGMCommand* source);
int VGMCommand_getStreamBlockId(VGMCommand* source);
int VGMCommand_getStreamFrenquency(VGMCommand* source);
int VGMCommand_getStreamSampleAddress(VGMCommand* source);
int VGMCommand_getStreamSampleSize(VGMCommand* source);
bool VGMCommand_isSame(VGMCommand* source, VGMCommand* com);

bool VGMCommand_contains(List* commands, VGMCommand* command);
VGMCommand* VGMCommand_getKeyCommand(List* commands, int channel);
VGMCommand* VGMCommand_createYMCommand(int port, int reg, int value);
List* VGMCommand_createYMCommands(int port, int baseReg, int value);


#endif // VGMCOM_H_
