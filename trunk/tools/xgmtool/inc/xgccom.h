#ifndef XGCCOM_H
#define XGCCOM_H


#define XGC_PSG_TONE    0x10
#define XGC_PSG_ENV     0x18
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

List* XGCCommand_createPSGEnvCommands(List* commands);
List* XGCCommand_createPSGToneCommands(List* commands);
List* XGCCommand_createYMKeyCommands(List* commands);
List* XGCCommand_createStateCommands(List* commands);

List* XGCCommand_convertSingle(XGMCommand* command);
List* XGCCommand_convert(List* commands);


#endif // XGCCOM_H
