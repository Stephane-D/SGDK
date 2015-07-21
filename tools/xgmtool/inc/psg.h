#ifndef PSG_H_
#define PSG_H_

#include <stdbool.h>

typedef struct
{
    int registers[4][2];
    bool init[4][2];
    int index;
    int type;
} PSG;


PSG* PSG_create();
PSG* PSG_copy(PSG* state);

void PSG_clear(PSG* psg);
int PSG_get(PSG* psg, int ind, int typ);
void PSG_write(PSG* psg, int value);
bool PSG_isSame(PSG* psg, PSG* state, int ind, int typ);
bool PSG_isLowSame(PSG* psg, PSG* state, int ind, int typ);
bool PSG_isHighSame(PSG* psg, PSG* state, int ind, int typ);
bool PSG_isDiff(PSG* psg, PSG* state, int ind, int typ);

bool PSG_isLowDiffOnly(PSG* psg, PSG* state, int ind, int typ);

#include "util.h"

LList* PSG_getDelta(PSG* psg, PSG* state);


#endif // PSG_H_
