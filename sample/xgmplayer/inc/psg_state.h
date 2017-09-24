#ifndef PSG_STATE_H
#define PSG_STATE_H


typedef struct
{
    u16 tone[4];
    u16 env[4];
    u16 index;
    u16 type;
} PSG;


void PSGState_init(PSG *psg);

void PSGState_XGMWrites(PSG *psg, u8* data, u16 num);
void PSGState_write(PSG* psg, u16 value);


#endif // PSG_STATE_H
