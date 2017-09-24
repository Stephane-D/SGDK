#include "genesis.h"

#include "ym_state.h"


static void YM_write(YM_CH *ch, YM_SLOT *slot, u16 r, u16 v);
static void YM_keyOFF(YM_CH *ch, YM_SLOT *slot);
static void YM_keyON(YM_CH *ch, YM_SLOT *slot);


static const s16 ar_tab[0x20] =
{
    0x0, 0x1B, 0x48, 0xA3,
    0x123, 0x222, 0x369, 0x5F9,
    0x91A, 0xF5C, 0x16C1, 0x1FC0,
    0x1FC0, 0x1FC0, 0x1FC0, 0x1FC0,
    0x1FC0, 0x1FC0, 0x1FC0, 0x1FC0,
    0x1FC0, 0x1FC0, 0x1FC0, 0x1FC0,
    0x1FC0, 0x1FC0, 0x1FC0, 0x1FC0,
    0x1FC0, 0x1FC0, 0x1FC0, 0x1FC0
};

static const s16 dr_tab[0x20] =
{
    0x0, 0x1, 0x5, 0xB,
    0x15, 0x27, 0x3F, 0x6E,
    0xA8, 0x11C, 0x1A5, 0x2B7,
    0x3F3, 0x66C, 0x939, 0xED2,
    0x1515, 0x1FC0, 0x1FC0, 0x1FC0,
    0x1FC0, 0x1FC0, 0x1FC0, 0x1FC0,
    0x1FC0, 0x1FC0, 0x1FC0, 0x1FC0,
    0x1FC0, 0x1FC0, 0x1FC0, 0x1FC0
};

void YM_init(YM *ym)
{
    u16 c, s;

    ym->ch3_special = 0;

    for(c = 0; c < 6; c++)
    {
        YM_CH *ch = &(ym->channels[c]);

        ch->algo = 0;
        ch->freq = 0;
        ch->ext_freq = 0;
        ch->pan = 0;

        for(s = 0; s < 4; s++)
        {
            YM_SLOT *slot = &(ch->slots[s]);

            slot->mul = 0;

            slot->ep = EG_OFF;

            slot->env = MAX_ATT_LEVEL;
            slot->env_step = 0;

            slot->ar = 0;
            slot->d1r = 0;
            slot->d2r = 0;
            slot->rr = 0;

            slot->tl = MAX_ATT_LEVEL;
            slot->sl = MAX_ATT_LEVEL;

            slot->key = 0;
            // default for algo 0
            slot->out = (s == 3) ? 1 : 0;
        }
    }
}

void YM_XGMKeyWrites(YM *ym, u8* data, u16 num)
{
    u8 *src;
    YM_CH *ch;

    src = data;
    while(num--)
    {
        u16 value = *src++;
        u16 c = value & 0x03;

        if (c == 3) break;
        if (value & 0x04) c += 3;

        ch = &(ym->channels[c]);

        if (value & 0x10) YM_keyON(ch, &(ch->slots[0]));
        else YM_keyOFF(ch, &(ch->slots[0]));
        if (value & 0x20) YM_keyON(ch, &(ch->slots[1]));
        else YM_keyOFF(ch, &(ch->slots[1]));
        if (value & 0x40) YM_keyON(ch, &(ch->slots[2]));
        else YM_keyOFF(ch, &(ch->slots[2]));
        if (value & 0x80) YM_keyON(ch, &(ch->slots[3]));
        else YM_keyOFF(ch, &(ch->slots[3]));
    }
}

void YM_XGMWrites(YM *ym, u16 port, u8* data, u16 num)
{
    u8 *src;

    src = data;
    while(num--)
    {
        u16 reg = *src++;
        u16 value = *src++;

        if ((reg & 0xF0) > 0x20)
        {
            YM_CH *ch;
            u16 c;

            c = reg & 3;
            // invalid channel
            if (c == 3) return;
            // set channel number
            if (port) c += 3;

            ch = &(ym->channels[c]);

            YM_write(ch, &(ch->slots[(reg >> 2) & 3]), reg, value);
        }
        // special mode
        else if (reg == 0x27)
            ym->ch3_special = value & 0x40;
    }
}

static void YM_write(YM_CH *ch, YM_SLOT *slot, u16 r, u16 v)
{
    switch(r & 0xF0)
    {
        case 0x30:  // MUL
            slot->mul = v & 0xF;
            break;

        case 0x40:  // TL
            slot->tl = (v & 0x7F) << 6;
            break;

        case 0x50:  // AR
            slot->ar = ar_tab[v & 0x1F];
            break;

        case 0x60:  // DR
            slot->d1r = dr_tab[v & 0x1F];
            break;

        case 0x70:  // SR
            slot->d2r = dr_tab[v & 0x1F];
            break;

        case 0x80:  // SL, RR
            slot->sl = (v & 0xF0) << 5;
            slot->rr = dr_tab[((v & 0x0F) << 1) + 1];
            break;

        case 0xa0:
            switch((r >> 2) & 3)
            {
                case 0: // FNUM1
                    ch->freq = (ch->freq & 0xFF00) | v;
                    break;

                case 1: // FNUM2
                    ch->freq = (ch->freq & 0x00FF) | ((v & 0x3F) << 8);
                    break;

                case 2: // FNUM1 - CH3
                    ch->ext_freq = (ch->freq & 0xFF00) | v;
                    break;

                case 3: // FNUM2 - CH3
                    ch->ext_freq = (ch->freq & 0x00FF) | ((v & 0x3F) << 8);
                    break;
            }
            break;

        case 0xb0:
            switch((r >> 2) & 3)
            {
                case 0: // algo
                    ch->algo = v & 7;

                    switch(v & 7)
                    {
                        case 0:
                            ch->slots[0].out = 0;
                            ch->slots[1].out = 1;
                            ch->slots[2].out = 2;
                            ch->slots[3].out = 3;
                            break;

                        case 1:
                            ch->slots[0].out = 1;
                            ch->slots[1].out = 1;
                            ch->slots[2].out = 2;
                            ch->slots[3].out = 3;
                            break;

                        case 2:
                            ch->slots[0].out = 2;
                            ch->slots[1].out = 1;
                            ch->slots[2].out = 2;
                            ch->slots[3].out = 3;
                            break;

                        case 3:
                            ch->slots[0].out = 1;
                            ch->slots[1].out = 2;
                            ch->slots[2].out = 2;
                            ch->slots[3].out = 3;
                            break;

                        case 4:
                            ch->slots[0].out = 2;
                            ch->slots[1].out = 3;
                            ch->slots[2].out = 2;
                            ch->slots[3].out = 3;
                            break;

                        case 5:
                            ch->slots[0].out = 1;
                            ch->slots[1].out = 3;
                            ch->slots[2].out = 3;
                            ch->slots[3].out = 3;
                            break;

                        case 6:
                            ch->slots[0].out = 2;
                            ch->slots[1].out = 3;
                            ch->slots[2].out = 3;
                            ch->slots[3].out = 3;
                            break;

                        case 7:
                            ch->slots[0].out = 3;
                            ch->slots[1].out = 3;
                            ch->slots[2].out = 3;
                            ch->slots[3].out = 3;
                            break;
                    }
                    break;

                case 1: // panning
                    ch->pan = v & 0xC0;
                    break;
            }
            break;
    }
}

static void YM_keyOFF(YM_CH *ch, YM_SLOT *slot)
{
    // no change
    if (!slot->key) return;

    if (slot->ep > EG_REL)
        slot->ep = EG_REL;     // pass to release phase

    slot->key = 0;
}

static void YM_keyON(YM_CH *ch, YM_SLOT *slot)
{
    // no change
    if (slot->key) return;

    if (slot->ar < 94)
        slot->ep = (slot->env <= MIN_ATT_LEVEL) ? ((slot->sl == MIN_ATT_LEVEL) ? EG_SUS : EG_DEC) : EG_ATT;
    else
    {
        // set envelop to MIN ATTENUATION
        slot->env = MIN_ATT_LEVEL;
        // and directly switch to Decay (or Sustain) */
        slot->ep = (slot->sl == MIN_ATT_LEVEL) ? EG_SUS : EG_DEC;
    }

    slot->key = 1;
}

void YM_updateEnv(YM *ym)
{
    YM_CH *ch;
    YM_SLOT *slot;
    u16 c, s;

    ch = &(ym->channels[0]);
    c = 6;
    while(c--)
    {
        slot = &(ch->slots[0]);
        s = 4;
        while(s--)
        {
            s16 tmp;
            s16 env = slot->env;

            switch(slot->ep)
            {
                case EG_ATT:    // attack phase
                    env -= slot->ar;

                    tmp = slot->tl;

                    // check phase transition
                    if (env <= tmp)
                    {
                        env = tmp;
                        slot->ep = (slot->sl <= tmp) ? EG_SUS : EG_DEC;
                    }
                    break;

                case EG_DEC:    // decay phase
                    env += slot->d1r;

                    // check phase transition
                    if (env >= MAX_ATT_LEVEL)
                    {
                        env = MAX_ATT_LEVEL;
                        slot->ep = EG_OFF;
                    }
                    else if (env >= slot->sl)
                        slot->ep = EG_SUS;
                    break;

                case EG_SUS:    // sustain phase
                    env += slot->d2r;

                    // check phase transition
                    if (env >= MAX_ATT_LEVEL)
                    {
                        env = MAX_ATT_LEVEL;
                        slot->ep = EG_OFF;
                    }
                    break;

                case EG_REL:    // release phase
                    env += slot->rr;

                    // check phase transition
                    if (env >= MAX_ATT_LEVEL)
                    {
                        env = MAX_ATT_LEVEL;
                        slot->ep = EG_OFF;
                    }
                    break;
            }

            // update envelop
            slot->env = env;
            // next slot
            slot++;
        }

        // next channel
        ch++;
    }
}
