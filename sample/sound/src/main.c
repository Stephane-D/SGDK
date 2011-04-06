#include "genesis.h"
#include "sounds.h"


#define NUM_DRIVER      7
#define MAX_CMD         8
#define MAX_PARAM       16

#define DRIVER_NAME_LEN 32
#define CMD_NAME_LEN    16
#define PARAM_NAME_LEN  8


typedef struct
{
    const char name[PARAM_NAME_LEN];
    const u16 value;
} param_def;

typedef struct
{
    const char name[CMD_NAME_LEN];
    u16 num_param;
    const param_def params[MAX_PARAM];
} cmd_def;

typedef struct
{
    const u16 id;
    const char name[DRIVER_NAME_LEN];
    const u16 num_cmd;
    const cmd_def cmds[MAX_CMD];
} driver_def;


static void handleInput();
static void joyEvent(u16 joy, u16 changed, u16 state);

static void setTextPalette(u16 selected);

static void refreshDriverInfos();
static void refreshDriverParams();
static void refreshDriverCmd();

static s16 getDriverIndex(const driver_def *d);
static s16 getCmdIndex(const cmd_def *c);
static s16 getParamIndex(const param_def *p);
static s16 getCurrentDriverIndex();
static s16 getCurrentCmdIndex();
static s16 getCurrentParamIndex();
static const param_def *getParam(const driver_def *d, const cmd_def *c);
static const param_def *getCurrentParam();
static void setCurrentParam(const param_def *param);


static const driver_def drivers[NUM_DRIVER] =
{
    {
        Z80_DRIVER_NULL,
        "Null driver",
        0,
        {{"", 0, {{"", 0}}}}
    },
    {
        Z80_DRIVER_PCM,
        "Single channel PCM driver",
        3,
        {
            {"LOOP ", 2, {{"off ", 0}, {"on ", 1}}},
            {"PAN ", 3, {{"center ", AUDIO_PAN_CENTER}, {"left ", AUDIO_PAN_LEFT}, {"right ", AUDIO_PAN_RIGHT}}},
            {"RATE ", 6, {{"8000 ", AUDIO_RATE_8000}, {"11025 ", AUDIO_RATE_11025}, {"13400 ", AUDIO_RATE_13400}, {"16000 ", AUDIO_RATE_16000}, {"22050 ", AUDIO_RATE_22050}, {"32k", AUDIO_RATE_32000}}}
        }
    },
    {
        Z80_DRIVER_2ADPCM,
        "2 channels ADPCM driver",
        1,
        {
            {"LOOP ", 2, {{"off ", 0}, {"on ", 1}}},
        }
    },
    {
        Z80_DRIVER_4PCM,
        "4 channels PCM driver",
        1,
        {
            {"LOOP ", 2, {{"off ", 0}, {"on ", 1}}},
        }
    },
    {
        Z80_DRIVER_4PCM_ENV,
        "4 channels PCM driver (env)",
        5,
        {
            {"LOOP ", 2, {{"off ", 0}, {"on ", 1}}},
            {"ENV CH0 ", 16, {{"*", 0}, {"*", 1}, {"*", 2}, {"*", 3}, {"*", 4}, {"*", 5}, {"*", 6}, {"*", 7}, {"*", 8}, {"*", 9}, {"*", 10}, {"*", 11}, {"*", 12}, {"*", 13}, {"*", 14}, {"*", 15}}},
            {"ENV CH1 ", 16, {{"*", 0}, {"*", 1}, {"*", 2}, {"*", 3}, {"*", 4}, {"*", 5}, {"*", 6}, {"*", 7}, {"*", 8}, {"*", 9}, {"*", 10}, {"*", 11}, {"*", 12}, {"*", 13}, {"*", 14}, {"*", 15}}},
            {"ENV CH2 ", 16, {{"*", 0}, {"*", 1}, {"*", 2}, {"*", 3}, {"*", 4}, {"*", 5}, {"*", 6}, {"*", 7}, {"*", 8}, {"*", 9}, {"*", 10}, {"*", 11}, {"*", 12}, {"*", 13}, {"*", 14}, {"*", 15}}},
            {"ENV CH3 ", 16, {{"*", 0}, {"*", 1}, {"*", 2}, {"*", 3}, {"*", 4}, {"*", 5}, {"*", 6}, {"*", 7}, {"*", 8}, {"*", 9}, {"*", 10}, {"*", 11}, {"*", 12}, {"*", 13}, {"*", 14}, {"*", 15}}}
        }
    },
    {
        Z80_DRIVER_MVS,
        "MVS tracker driver",
        1,
        {
            {"LOOP ", 2, {{"off ", 0}, {"on ", 1}}}
        }
    },
    {
        Z80_DRIVER_TFM,
        "TFM tracker driver",
        1,
        {
            {"LOOP ", 2, {{"off ", 0}, {"on ", 1}}}
        }
    }
};


static const driver_def *driver;
static const cmd_def *cmd;
// params value
static const param_def *params_value[NUM_DRIVER][MAX_CMD];


int main()
{
    u16 i, j;
    const driver_def *cur_driver;
    const cmd_def *cur_cmd;

    JOY_setEventHandler(joyEvent);

    VDP_setScreenWidth320();
    VDP_setHInterrupt(0);
    VDP_setHilightShadow(0);

    // init volume to 0 for driver 4PCM_ENV
    setVolume_4PCM_ENV(AUDIO_PCM_CH1, 0);
    setVolume_4PCM_ENV(AUDIO_PCM_CH2, 0);
    setVolume_4PCM_ENV(AUDIO_PCM_CH3, 0);
    setVolume_4PCM_ENV(AUDIO_PCM_CH4, 0);

    // point to first driver
    driver = drivers;
    cmd = NULL;

    for(i = 0, cur_driver = drivers; i < NUM_DRIVER; i++, cur_driver++)
        for(j = 0, cur_cmd = cur_driver->cmds; j < MAX_CMD; j++, cur_cmd++)
            params_value[i][j] = cur_cmd->params;

    VDP_setPaletteColor(1, 15, 0x0888);

    VDP_setTextPalette(0);
    VDP_drawText("Current Z80 driver", 10, 1);

    refreshDriverInfos();

    while(1)
    {
        handleInput();
        VDP_waitVSync();
    }
}


static void setTextPalette(u16 selected)
{
    if (selected) VDP_setTextPalette(0);
    else VDP_setTextPalette(1);
}


static void refreshDriverInfos()
{
    refreshDriverParams();
    refreshDriverCmd();
}

static void refreshDriverParams()
{
    const u16 driver_ind = getCurrentDriverIndex();
    const char* str;
    u16 start;
    u16 len;
    u16 posY;
    u16 i, j;

    VDP_clearTileMapRect(APLAN, 0, 3, 40, 8);

    str = driver->name;
    len = strlen(str);
    start = 20 - (len / 2);

    setTextPalette(driver != &drivers[0]);
    VDP_drawText("<", start - 2, 3);
    VDP_setTextPalette(0);
    VDP_drawText(str, start, 3);
    setTextPalette(driver != &drivers[NUM_DRIVER - 1]);
    VDP_drawText(">", start + len + 1, 3);

    posY = 5;

    for(i = 0; i < driver->num_cmd; i++)
    {
        const cmd_def *cur_cmd = &driver->cmds[i];
        u16 posX = 2;

        str = cur_cmd->name;
        VDP_setTextPalette(0);
        VDP_drawText(str, posX, posY);
        posX += strlen(str);

        for(j = 0; j < cur_cmd->num_param; j++)
        {
            const param_def *cur_param = &cur_cmd->params[j];

            str = cur_param->name;
            setTextPalette(params_value[driver_ind][i] == cur_param);
            VDP_drawText(str, posX, posY);
            posX += strlen(str);
        }

        posY++;
    }

    VDP_setTextPalette(0);

    if (cmd == NULL) VDP_drawText("*", 0, 3);
    else VDP_drawText("*", 0, 5 + getCurrentCmdIndex());
}

static void refreshDriverCmd()
{
    VDP_clearTileMapRect(APLAN, 0, 12, 40, 8);
    setTextPalette(1);

    switch(driver->id)
    {
        case Z80_DRIVER_PCM:
            VDP_drawText("press A to start/end playback", 1, 12);
            break;

        case Z80_DRIVER_2ADPCM:
            VDP_drawText("press A to start/end channel 1 play", 1, 12);
            VDP_drawText("press B to start/end channel 2 play", 1, 13);
            break;

        case Z80_DRIVER_4PCM:
            VDP_drawText("press A to start/end channel 1 play", 1, 12);
            VDP_drawText("press B to start/end channel 2 play", 1, 13);
            VDP_drawText("press C to start/end channel 3 play", 1, 14);
            VDP_drawText("press START to start/end channel 4 play", 1, 15);
            break;

        case Z80_DRIVER_4PCM_ENV:
            VDP_drawText("press A to start/end channel 1 play", 1, 12);
            VDP_drawText("press B to start/end channel 2 play", 1, 13);
            VDP_drawText("press C to start/end channel 3 play", 1, 14);
            VDP_drawText("press START to start/end channel 4 play", 1, 15);
            break;

        case Z80_DRIVER_MVS:
            VDP_drawText("press A to start/end play", 1, 12);
            break;

        case Z80_DRIVER_TFM:
            VDP_drawText("press A to start/end play", 1, 12);
            break;
    }
}


static s16 getDriverIndex(const driver_def *d)
{
    u16 i;
    const driver_def *cur_driver;

    for(i = 0, cur_driver = drivers; i < NUM_DRIVER; i++, cur_driver++)
        if (cur_driver == d) return i;

    return -1;
}

static s16 getCmdIndex(const cmd_def *c)
{
    u16 i, j;
    const driver_def *cur_driver;
    const cmd_def *cur_cmd;

    for(i = 0, cur_driver = drivers; i < NUM_DRIVER; i++, cur_driver++)
        for(j = 0, cur_cmd = cur_driver->cmds; j < cur_driver->num_cmd; j++, cur_cmd++)
            if (cur_cmd == c) return j;

    return -1;
}

static s16 getParamIndex(const param_def *p)
{
    u16 i, j, k;
    const driver_def *cur_driver;
    const cmd_def *cur_cmd;
    const param_def* cur_param;

    for(i = 0, cur_driver = drivers; i < NUM_DRIVER; i++, cur_driver++)
        for(j = 0, cur_cmd = cur_driver->cmds; j < cur_driver->num_cmd; j++, cur_cmd++)
            for(k = 0, cur_param = cur_cmd->params; k < cur_cmd->num_param; k++, cur_param++)
                if (cur_param == p) return k;

    return -1;
}

static s16 getCurrentDriverIndex()
{
    return getDriverIndex(driver);
}

static s16 getCurrentCmdIndex()
{
    return getCmdIndex(cmd);
}

static s16 getCurrentParamIndex()
{
    return getParamIndex(getParam(driver, cmd));
}

static const param_def *getParam(const driver_def *d, const cmd_def *c)
{
    const s16 driver_ind = getDriverIndex(d);
    const s16 cmd_ind = getCmdIndex(c);

    if ((driver_ind != -1) && (cmd_ind != -1))
        return params_value[driver_ind][cmd_ind];

    return NULL;
}

static const param_def *getCurrentParam()
{
    return getParam(driver, cmd);
}

static void setCurrentParam(const param_def *param)
{
    const s16 driver_ind = getCurrentDriverIndex();
    const s16 cmd_ind = getCurrentCmdIndex();

    if ((driver_ind != -1) && (cmd_ind != -1))
        params_value[driver_ind][cmd_ind] = param;
}


static void handleInput()
{
    u16 value;

    value = JOY_readJoypad(JOY_1);

    if (value & BUTTON_A)
    {
        if (value & BUTTON_UP)
        {

        }

        if (value & BUTTON_DOWN)
        {

        }
    }
    else if (value & BUTTON_B)
    {
        if (value & BUTTON_UP)
        {

        }

        if (value & BUTTON_DOWN)
        {

        }
    }
    else
    {
        if (value & BUTTON_UP)
        {

        }

        if (value & BUTTON_DOWN)
        {

        }

        if (value & BUTTON_LEFT)
        {

        }

        if (value & BUTTON_RIGHT)
        {

        }
    }
}


static void joyEvent(u16 joy, u16 changed, u16 state)
{
    const u16 driver_ind = getCurrentDriverIndex();
    const u16 loop = params_value[driver_ind][0]->value;

    // LEFT button state changed
    if (changed & state & BUTTON_LEFT)
    {
        if (cmd == NULL)
        {
            // driver change
            if (driver > &drivers[0]) driver--;
        }
        else
        {
            // param change
            const param_def *param = getCurrentParam();

            if (param > &cmd->params[0])
            {
                setCurrentParam(param - 1);

                if (driver->id == Z80_DRIVER_4PCM_ENV)
                {
                    const u16 cmd_index = getCurrentCmdIndex();

                    if (cmd_index > 0)
                        setVolume_4PCM_ENV(cmd_index - 1, getParamIndex(param - 1));
                }
            }
        }
    }
    // RIGHT button state changed
    else if (changed & state & BUTTON_RIGHT)
    {
        if (cmd == NULL)
        {
            // driver change
            if (driver < &drivers[NUM_DRIVER - 1]) driver++;
        }
        else
        {
            // param change
            const param_def *param = getCurrentParam();

            if (param < &cmd->params[cmd->num_param - 1])
            {
                setCurrentParam(param + 1);

                if (driver->id == Z80_DRIVER_4PCM_ENV)
                {
                    const u16 cmd_index = getCurrentCmdIndex();

                    if (cmd_index > 0)
                        setVolume_4PCM_ENV(cmd_index - 1, getParamIndex(param + 1));
                }
            }
        }
    }

    // UP button state changed
    if (changed & state & BUTTON_UP)
    {
        if (cmd != NULL)
        {
            if (cmd > &driver->cmds[0]) cmd--;
            else cmd = NULL;
        }
    }
    // DOWN button state changed
    else if (changed & state & BUTTON_DOWN)
    {
        if (cmd == NULL)
        {
            if (driver->num_cmd > 0)
                cmd = driver->cmds;
        }
        else if (cmd < &driver->cmds[driver->num_cmd - 1]) cmd++;
    }

    // driver commands
    switch(driver->id)
    {
        case Z80_DRIVER_PCM:
        {
            const u16 pan = params_value[driver_ind][1]->value;

            if (changed & state & BUTTON_A)
            {
                if (isPlaying_PCM())
                    stopPlay_PCM();
                else
                {
                    // RATE parameter value
                    switch(params_value[driver_ind][2]->value)
                    {
                        case AUDIO_RATE_8000:
                            startPlay_PCM(india_8k, sizeof(india_8k), AUDIO_RATE_8000, pan, loop);
                            break;

                        case AUDIO_RATE_11025:
                            startPlay_PCM(india_11k, sizeof(india_11k), AUDIO_RATE_11025, pan, loop);
                            break;

                        case AUDIO_RATE_13400:
                            startPlay_PCM(india_13k, sizeof(india_13k), AUDIO_RATE_13400, pan, loop);
                            break;

                        case AUDIO_RATE_16000:
                            startPlay_PCM(india_16k, sizeof(india_16k), AUDIO_RATE_16000, pan, loop);
                            break;

                        case AUDIO_RATE_22050:
                            startPlay_PCM(india_22k, sizeof(india_22k), AUDIO_RATE_22050, pan, loop);
                            break;

                        case AUDIO_RATE_32000:
                            startPlay_PCM(india_32k, sizeof(india_32k), AUDIO_RATE_32000, pan, loop);
                            break;
                    }
                }
            }

            break;
        }

        case Z80_DRIVER_2ADPCM:
        {
            if (changed & state & BUTTON_A)
            {
                if (isPlaying_2ADPCM(AUDIO_PCM_CH1_MSK))
                    stopPlay_2ADPCM(AUDIO_PCM_CH1);
                else
                    startPlay_2ADPCM(india_pcm_22k, sizeof(india_pcm_22k), AUDIO_PCM_CH1, loop);
            }

            if (changed & state & BUTTON_B)
            {
                if (isPlaying_2ADPCM(AUDIO_PCM_CH2_MSK))
                    stopPlay_2ADPCM(AUDIO_PCM_CH2);
                else
                    startPlay_2ADPCM(loop4_pcm_22k, sizeof(loop4_pcm_22k), AUDIO_PCM_CH2, loop);
            }

            break;
        }

        case Z80_DRIVER_4PCM:
        {
            if (changed & state & BUTTON_START)
            {
                if (isPlaying_4PCM(AUDIO_PCM_CH4_MSK))
                    stopPlay_4PCM(AUDIO_PCM_CH4);
                else
                    startPlay_4PCM(sound1_16k, sizeof(sound1_16k), AUDIO_PCM_CH4, loop);
            }

            if (changed & state & BUTTON_A)
            {
                if (isPlaying_4PCM(AUDIO_PCM_CH1_MSK))
                    stopPlay_4PCM(AUDIO_PCM_CH1);
                else
                    startPlay_4PCM(loop1_16k, sizeof(loop1_16k), AUDIO_PCM_CH1, loop);
            }

            if (changed & state & BUTTON_B)
            {
                if (isPlaying_4PCM(AUDIO_PCM_CH2_MSK))
                    stopPlay_4PCM(AUDIO_PCM_CH2);
                else
                    startPlay_4PCM(loop3_16k, sizeof(loop3_16k), AUDIO_PCM_CH2, loop);
            }

            if (changed & state & BUTTON_C)
            {
                if (isPlaying_4PCM(AUDIO_PCM_CH3_MSK))
                    stopPlay_4PCM(AUDIO_PCM_CH3);
                else
                    startPlay_4PCM(violon1_16k, sizeof(violon1_16k), AUDIO_PCM_CH3, loop);
            }

            break;
        }

        case Z80_DRIVER_4PCM_ENV:
        {
            if (changed & state & BUTTON_START)
            {
                if (isPlaying_4PCM_ENV(AUDIO_PCM_CH4_MSK))
                    stopPlay_4PCM_ENV(AUDIO_PCM_CH4);
                else
                    startPlay_4PCM_ENV(loop2_16k, sizeof(loop2_16k), AUDIO_PCM_CH4, loop);
            }

            if (changed & state & BUTTON_A)
            {
                if (isPlaying_4PCM_ENV(AUDIO_PCM_CH1_MSK))
                    stopPlay_4PCM_ENV(AUDIO_PCM_CH1);
                else
                    startPlay_4PCM_ENV(piano1_16k, sizeof(piano1_16k), AUDIO_PCM_CH1, loop);
            }

            if (changed & state & BUTTON_B)
            {
                if (isPlaying_4PCM_ENV(AUDIO_PCM_CH2_MSK))
                    stopPlay_4PCM_ENV(AUDIO_PCM_CH2);
                else
                    startPlay_4PCM_ENV(violon2_16k, sizeof(violon2_16k), AUDIO_PCM_CH2, loop);
            }

            if (changed & state & BUTTON_C)
            {
                if (isPlaying_4PCM_ENV(AUDIO_PCM_CH3_MSK))
                    stopPlay_4PCM_ENV(AUDIO_PCM_CH3);
                else
                    startPlay_4PCM_ENV(beat1_16k, sizeof(beat1_16k), AUDIO_PCM_CH3, loop);
            }

            break;
        }

        case Z80_DRIVER_MVS:
        {
            if (changed & state & BUTTON_A)
            {
                if (isPlaying_MVS())
                    stopPlay_MVS();
                else
                {
//                    if (loop)
//                        startPlay_MVS(music_mvs, AUDIO_MVS_LOOP);
//                    else
//                        startPlay_MVS(music_mvs, AUDIO_MVS_ONCE);
                }
            }

            break;
        }

        case Z80_DRIVER_TFM:
        {
            if (changed & state & BUTTON_A)
            {
                startPlay_TFM(music_tfd);
            }
            break;
        }
    }

    if (changed & state)
        refreshDriverInfos();
}
