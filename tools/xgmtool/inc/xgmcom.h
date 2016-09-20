#ifndef XGMCOM_H_
#define XGMCOM_H_


#define XGM_FRAME           0x00
#define XGM_PSG             0x10
#define XGM_YM2612_PORT0    0x20
#define XGM_YM2612_PORT1    0x30
#define XGM_YM2612_REGKEY   0x40
#define XGM_PCM             0x50

// don't use command above 0x7F
#define XGM_LOOP            0x7E
#define XGM_END             0x7F


typedef struct
{
    unsigned char* data;
    int offset;
    int command;
    int size;
} XGMCommand;


XGMCommand* XGMCommand_create(unsigned char* data, int size);
XGMCommand* XGMCommand_createEx(int command, unsigned char* data, int size);
XGMCommand* XGMCommand_createLoopCommand(int offset);
XGMCommand* XGMCommand_createFrameCommand();
XGMCommand* XGMCommand_createEndCommand();
XGMCommand* XGMCommand_createFromData(unsigned char* data);

int XGMCommand_getType(XGMCommand* source);
int XGMCommand_getSize(XGMCommand* source);
bool XGMCommand_isFrame(XGMCommand* source);
bool XGMCommand_isLoop(XGMCommand* source);
int XGMCommand_getLoopOffset(XGMCommand* source);
bool XGMCommand_isEnd(XGMCommand* source);
bool XGMCommand_isPCM(XGMCommand* source);
int XGMCommand_getPCMId(XGMCommand* source);
int XGMCommand_getPCMChannel(XGMCommand* source);
int XGMCommand_getPCMPrio(XGMCommand* source);
bool XGMCommand_isPSGWrite(XGMCommand* source);
int XGMCommand_getPSGWriteCount(XGMCommand* source);
bool XGMCommand_isYM2612Port0Write(XGMCommand* source);
bool XGMCommand_isYM2612Port1Write(XGMCommand* source);
bool XGMCommand_isYM2612Write(XGMCommand* source);
int XGMCommand_getYM2612Port(XGMCommand* source);
int XGMCommand_getYM2612WriteCount(XGMCommand* source);
bool XGMCommand_isYM2612RegKeyWrite(XGMCommand* source);
void XGMCommand_setOffset(XGMCommand* source, int value);

bool XGMCommand_removeYM2612RegWrite(XGMCommand* source, int port, int reg);

#include "vgmcom.h"
#include "util.h"

XGMCommand* XGMCommand_createYMKeyCommand(LList** pcommands, int max);
LList* XGMCommand_createYMKeyCommands(LList* commands);
LList* XGMCommand_createYMPort0Commands(LList* commands);
LList* XGMCommand_createYMPort1Commands(LList* commands);
LList* XGMCommand_createPSGCommands(LList* commands);

#include "xgm.h"

LList* XGMCommand_createPCMCommands(XGM* xgm, VGM* vgm, LList* commands);

char* XGMCommand_toString(XGMCommand* command);
void XGMCommand_logCommand(FILE *file, XGMCommand* command);
bool XGMCommand_logCommands(char* fileName, LList* commands);


#endif // XGMCOM_H_
