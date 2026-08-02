#include <genesis.h>
#include "global.h"
#include "sound.h"


// Play sound effect for row removal with rising tone
void Sound_PlayRowRemoved()
{
    if (!g_game.isSoundOn)
        return;

    PSG_reset();

    PSG_setNoise(PSG_NOISE_TYPE_WHITE, PSG_NOISE_FREQ_CLOCK8);
    PSG_setEnvelope(3, PSG_ENVELOPE_MAX);

    PSG_setTone(1, g_game.soundRowRemovingTone);
    PSG_setEnvelope(1, PSG_ENVELOPE_MAX);
}

// Play sound effect for figure being fixed with different tones based on drop type
void Sound_PlayFigureFixed()
{
    if (!g_game.isSoundOn)
        return;

    if (g_input.dropType)
    {
        PSG_setNoise(PSG_NOISE_TYPE_PERIODIC, PSG_NOISE_FREQ_CLOCK8);
        PSG_setTone(0, 1023);
    }
    else
    {
        PSG_setNoise(PSG_NOISE_TYPE_PERIODIC, PSG_NOISE_FREQ_CLOCK4);
        PSG_setTone(0, 800);
    }

    PSG_setEnvelope(0, PSG_ENVELOPE_MAX);
    PSG_setEnvelope(3, PSG_ENVELOPE_MAX);
}