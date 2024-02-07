#ifndef YM_STATE_H
#define YM_STATE_H


#define MIN_ATT_LEVEL       0
#define MAX_ATT_LEVEL       (0X7F << 6)


#define EG_ATT      4
#define EG_DEC      3
#define EG_SUS      2
#define EG_REL      1
#define EG_OFF      0


typedef struct
{
    u16 mul;        // freq multiplier

    u16 ep;         // current envelop phase

    s16 env;        // current envelop level (comparable to tl and sl)
    s16 env_step;   // envelop step (ar, d1r, d2r or rr depending env phase)

    s16 ar;         // attack rate
    s16 d1r;        // decay rate
    s16 d2r;        // substain rate
    s16 rr;         // release rate

    u16 tl;         // total level (min = 0, max = 32768)
    u16 sl;         // substain level (comparable to tl)

    u16 key;        // key on/off state
    u16 out;        // out slot type (depending algo)
} YM_SLOT;

typedef struct
{
    YM_SLOT slots[4];

    u16 algo;
    u16 freq;
    u16 ext_freq;       // (for CH3 special mode)
    u16 pan;
} YM_CH;

typedef struct
{
    YM_CH channels[6];
    u16 ch3_special;
} YM;



void YM_init(YM *ym);

void YM_XGMKeyWrites(YM *ym, u8* data, u16 num);
void YM_XGMWrites(YM *ym, u16 port, u8* data, u16 num);
void YM_updateEnv(YM *ym);


#endif // YM_STATE_H
