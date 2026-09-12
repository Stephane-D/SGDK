#include <genesis.h>
#include "global.h"
#include "sound.h"

// Play sound effect for row removal with rising tone
void Sound_PlayGameOver()
{
    if (!g_game.isSoundOn)
        return;
    
    g_soundEffectDownCounter = 70;
    PSG_reset();
    
    PSG_setTone(1, g_game.soundRowRemovingTone);
    PSG_setEnvelope(1, PSG_ENVELOPE_MAX);
}

// Update sound effect for game over with descending tone
void Sound_UpdateGameOver()
{
    if (g_soundEffectDownCounter)
    {
        g_soundEffectDownCounter--;
        PSG_setTone(1, g_game.soundRowRemovingTone + (g_soundEffectDownCounter + 10) * 142);
        
        if (!g_soundEffectDownCounter)
            PSG_reset();
    }
}

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

// Update sound effect for row removal during animation, with different tones based on number of rows removed
void Sound_UpdateRowRemoved()
{
    if (g_game.isSoundOn)
    {
        if (g_rowsToRemoveCount < 4)
        {
            if (g_rowsRemoveDownCounter == ROW_REMOVE_FX_DELAY / 9 * 1)
                PSG_setEnvelope(1, PSG_ENVELOPE_MIN);
            else
            {
                PSG_setTone(1, g_game.soundRowRemovingTone + (g_rowsRemoveDownCounter + 10) * 30);
            }
        }
        else
        {
            PSG_setTone(1, g_game.soundRowRemovingTone - (g_rowsRemoveDownCounter + 10) * 497);
        }
        
        if (g_rowsRemoveDownCounter == ROW_REMOVE_FX_DELAY / 2)
            PSG_setEnvelope(3, PSG_ENVELOPE_MIN);
    }
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