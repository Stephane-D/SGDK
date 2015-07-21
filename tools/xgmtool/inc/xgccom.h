#ifndef XGCCOM_H
#define XGCCOM_H


#define XGC_PSG_TONE    0x10
#define XGC_PSG_ENV     0x18
#define XGC_PCM         0x50
#define XGC_STATE       0x60

#define XGC_FRAME_SIZE  1


XGMCommand* XGCCommand_createFrameSizeCommand(int size);
XGMCommand* XGCCommand_createFromCommand(XGMCommand* command);

int XGCCommand_getFrameSizeSize(XGMCommand* source);
void XGCCommand_setFrameSizeSize(XGMCommand* source, int value);
int XGCCommand_getType(XGMCommand* source);
bool XGCCommand_isFrameSize(XGMCommand* source);
bool XGCCommand_isPSGEnvWrite(XGMCommand* source);
bool XGCCommand_isPSGToneWrite(XGMCommand* source);
bool XGCCommand_isPCM(XGMCommand* source);
int XGCCommand_getPCMId(XGMCommand* source);

LList* XGCCommand_createPSGEnvCommands(LList* commands);
LList* XGCCommand_createPSGToneCommands(LList* commands);
LList* XGCCommand_createYMKeyCommands(LList* commands);
LList* XGCCommand_createStateCommands(LList* commands);

LList* XGCCommand_convertSingle(XGMCommand* command);
LList* XGCCommand_convert(LList* commands);


#endif // XGCCOM_H
