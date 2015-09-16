#ifndef XGCCOM_H
#define XGCCOM_H


#define XGC_PSG_TONE    0x10
#define XGC_PSG_ENV     0x18
#define XGC_PCM         0x50
#define XGC_STATE       0x60

#define XGC_FRAME_SKIP  0x7D

#define XGC_FRAME_SIZE  1


XGMCommand* XGCCommand_createFrameSizeCommand(int size);
XGMCommand* XGCCommand_createFrameSkipCommand();
XGMCommand* XGCCommand_createFromCommand(XGMCommand* command);
XGMCommand* XGCCommand_createFromData(unsigned char* data);

int XGCCommand_getFrameSizeSize(XGMCommand* source);
void XGCCommand_setFrameSizeSize(XGMCommand* source, int value);
int XGCCommand_getType(XGMCommand* source);
bool XGCCommand_isFrameSize(XGMCommand* source);
bool XGCCommand_isFrameSkip(XGMCommand* source);
bool XGCCommand_isPSGEnvWrite(XGMCommand* source);
bool XGCCommand_isPSGToneWrite(XGMCommand* source);
bool XGCCommand_isPCM(XGMCommand* source);
int XGCCommand_getPCMId(XGMCommand* source);
bool XGCCommand_isState(XGMCommand* source);

LList* XGCCommand_createPSGEnvCommands(LList* commands);
LList* XGCCommand_createPSGToneCommands(LList* commands);
LList* XGCCommand_createYMKeyCommands(LList* commands);
LList* XGCCommand_createStateCommands(LList* commands);

LList* XGCCommand_convertSingle(XGMCommand* command);
LList* XGCCommand_convert(LList* commands);


#endif // XGCCOM_H
