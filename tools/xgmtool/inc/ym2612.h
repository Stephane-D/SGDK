#ifndef YM2612_H_
#define YM2612_H_


typedef struct
{
    int registers[2][256];
    bool init[2][256];
} YM2612;


YM2612* YM2612_create();
YM2612* YM2612_copy(YM2612* source);
void YM2612_clear(YM2612* source);
void YM2612_initialize(YM2612* source);

int YM2612_get(YM2612* source, int port, int reg);
bool YM2612_set(YM2612* source, int port, int reg, int value);
bool YM2612_isSame(YM2612* source, YM2612* state, int port, int reg);
bool YM2612_isDiff(YM2612* source, YM2612* state, int port, int reg);
LList* YM2612_getDelta(YM2612* source, YM2612* state);

bool YM2612_canIgnore(int port, int reg);
int* YM2612_getDualReg(int reg);


#endif // YM2612_H_
