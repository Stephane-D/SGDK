#include "genesis.h"

#include "resources.h"

#define NUM_DRIVER      6
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


static void joyEvent(u16 joy, u16 changed, u16 state);
static void vintEvent();

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
            {"PAN ", 3, {{"center ", SOUND_PAN_CENTER}, {"left ", SOUND_PAN_LEFT}, {"right ", SOUND_PAN_RIGHT}}},
            {"RATE ", 6, {{"8k ", SOUND_PCM_RATE_8000}, {"11k ", SOUND_PCM_RATE_11025}, {"13.4k ", SOUND_PCM_RATE_13400}, {"16k ", SOUND_PCM_RATE_16000}, {"22k ", SOUND_PCM_RATE_22050}, {"32k", SOUND_PCM_RATE_32000}}}
        }
    },
    {
        Z80_DRIVER_DPCM2,
        "2 channels DPCM driver",
        1,
        {
            {"LOOP ", 2, {{"off ", 0}, {"on ", 1}}},
        }
    },
    {
        Z80_DRIVER_PCM4,
        "4 channels PCM driver",
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
        Z80_DRIVER_XGM,
        "XGM driver",
        1,
        {
            {"LOOP ", 2, {{"off ", 0}, {"on ", 1}}}
        }
    },
    {
        Z80_DRIVER_XGM2,
        "XGM2 driver",
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
static u16 dmaMethod;
static u32 musicSize;

int main()
{
    u16 i, j;
    const driver_def *cur_driver;
    const cmd_def *cur_cmd;

    JOY_setEventHandler(joyEvent);

    VDP_setScreenWidth320();
    VDP_setHInterrupt(0);
    VDP_setHilightShadow(0);
    SYS_setVIntCallback(vintEvent);

    // point to first driver
    driver = &drivers[0];
    cmd = NULL;
    dmaMethod = 0;

    for(i = 0, cur_driver = drivers; i < NUM_DRIVER; i++, cur_driver++)
        for(j = 0, cur_cmd = cur_driver->cmds; j < MAX_CMD; j++, cur_cmd++)
            params_value[i][j] = cur_cmd->params;

    PAL_setPalette(PAL0, font_pal_lib.data, CPU);
    PAL_setColor((PAL1 * 16) + 15, 0x0888);
    VDP_setTextPalette(PAL0);
    VDP_drawText("Current Z80 driver", 10, 1);
    VDP_drawText("DMA Method:", 1, 26);

    refreshDriverInfos();

    while(TRUE)
    {
        SYS_doVBlankProcess();
    }
}


static void setTextPalette(u16 selected)
{
    if (selected) VDP_setTextPalette(PAL0);
    else VDP_setTextPalette(PAL1);
}


static void refreshDriverInfos()
{

    refreshDriverParams();
    refreshDriverCmd();

    char size[16];
    char str[40];

    // extra infos
    switch(driver->id)
    {
        case Z80_DRIVER_XGM:
            if (musicSize)
            {
                fix32ToStr(intToFix32(musicSize / 1024), size, 1);
                sprintf(str, "Music size = %s KB", size);
                VDP_drawText(str, 2, 7);
            }
            break;

        case Z80_DRIVER_XGM2:
            if (musicSize)
            {
                fix32ToStr(intToFix32(musicSize / 1024), size, 1);
                sprintf(str, "Music size = %s KB", size);
                VDP_drawText(str, 2, 7);
            }
            break;
    }
}

static void refreshDriverParams()
{
    const u16 driver_ind = getCurrentDriverIndex();
    const char* str;
    u16 start;
    u16 len;
    u16 posY;
    u16 i, j;

    VDP_clearTileMapRect(BG_A, 0, 3, 40, 8);

    str = driver->name;
    len = strlen(str);
    start = 20 - (len / 2);

    setTextPalette(driver != &drivers[0]);
    VDP_drawText("<", start - 2, 3);
    VDP_setTextPalette(PAL0);
    VDP_drawText(str, start, 3);
    setTextPalette(driver != &drivers[NUM_DRIVER - 1]);
    VDP_drawText(">", start + len + 1, 3);

    posY = 5;

    for(i = 0; i < driver->num_cmd; i++)
    {
        const cmd_def *cur_cmd = &driver->cmds[i];
        u16 posX = 2;

        str = cur_cmd->name;
        VDP_setTextPalette(PAL0);
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

    VDP_setTextPalette(PAL0);

    if (cmd == NULL) VDP_drawText("*", 0, 3);
    else VDP_drawText("*", 0, 5 + getCurrentCmdIndex());
}

static void refreshDriverCmd()
{
    VDP_clearTileMapRect(BG_A, 0, 12, 40, 14);
    setTextPalette(1);

    switch(driver->id)
    {
        case Z80_DRIVER_PCM:
            VDP_drawText("press A to start/end playback", 1, 12);
            break;

        case Z80_DRIVER_DPCM2:
            VDP_drawText("press A to start/end channel 1", 1, 12);
            VDP_drawText("press B to start/end channel 2", 1, 13);
            break;

        case Z80_DRIVER_PCM4:
            VDP_drawText("press A to start/end channel 1", 1, 12);
            VDP_drawText("press B to start/end channel 2", 1, 13);
            VDP_drawText("press C to start/end channel 3", 1, 14);
            VDP_drawText("press START to start/end channel 4", 1, 15);
            break;

        case Z80_DRIVER_XGM:
        case Z80_DRIVER_XGM2:
            VDP_drawText("press A to play Streets Of Rage 2 XGM", 1, 12);
            VDP_drawText("press B to play Bad Apple (PCM) XGM", 1, 13);
            VDP_drawText("press C to play Midnight Resistance XGM", 1, 14);
            VDP_drawText("press START to pause/resume XGM music", 1, 15);

            if (driver->id == Z80_DRIVER_XGM)
            {
                VDP_drawText("press X to play PCM SFX ch 2", 1, 17);
                VDP_drawText("press Y to play PCM SFX ch 3", 1, 18);
                VDP_drawText("press Z to play PCM SFX ch 4", 1, 19);
            }
            else
            {
                VDP_drawText("press X to play PCM SFX ch 1", 1, 17);
                VDP_drawText("press Y to play PCM SFX ch 2", 1, 18);
                VDP_drawText("press Z to play PCM SFX ch 3", 1, 19);
            }

            VDP_drawText("Z80 CPU load:", 1, 21);
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

static void joyEvent(u16 joy, u16 changed, u16 state)
{
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
                setCurrentParam(param - 1);
        }

        // reset music size
        musicSize = 0;
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
                setCurrentParam(param + 1);
        }

        // reset music size
        musicSize = 0;
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

    const u16 driver_ind = getCurrentDriverIndex();
    const u16 loop = params_value[driver_ind][0]->value;

    // driver commands
    switch(driver->id)
    {
        case Z80_DRIVER_PCM:
        {
            const u16 pan = params_value[driver_ind][1]->value;

            if (changed & state & BUTTON_A)
            {
                if (SND_PCM_isPlaying())
                    SND_PCM_stopPlay();
                else
                {
                    // RATE parameter value
                    switch(params_value[driver_ind][2]->value)
                    {
                        case SOUND_PCM_RATE_8000:
                            SND_PCM_startPlay(india_8k, sizeof(india_8k), SOUND_PCM_RATE_8000, pan, loop);
                            break;

                        case SOUND_PCM_RATE_11025:
                            SND_PCM_startPlay(india_11k, sizeof(india_11k), SOUND_PCM_RATE_11025, pan, loop);
                            break;

                        case SOUND_PCM_RATE_13400:
                            SND_PCM_startPlay(india_13k, sizeof(india_13k), SOUND_PCM_RATE_13400, pan, loop);
                            break;

                        case SOUND_PCM_RATE_16000:
                            SND_PCM_startPlay(india_16k, sizeof(india_16k), SOUND_PCM_RATE_16000, pan, loop);
                            break;

                        case SOUND_PCM_RATE_22050:
                            SND_PCM_startPlay(india_22k, sizeof(india_22k), SOUND_PCM_RATE_22050, pan, loop);
                            break;

                        case SOUND_PCM_RATE_32000:
                            SND_PCM_startPlay(india_32k, sizeof(india_32k), SOUND_PCM_RATE_32000, pan, loop);
                            break;
                    }
                }
            }
            break;
        }

        case Z80_DRIVER_DPCM2:
        {
            if (changed & state & BUTTON_A)
            {
                if (SND_DPCM2_isPlaying(SOUND_PCM_CH1_MSK))
                    SND_DPCM2_stopPlay(SOUND_PCM_CH1);
                else
                    SND_DPCM2_startPlay(india_pcm_22k, sizeof(india_pcm_22k), SOUND_PCM_CH1, loop);
            }

            if (changed & state & BUTTON_B)
            {
                if (SND_DPCM2_isPlaying(SOUND_PCM_CH2_MSK))
                    SND_DPCM2_stopPlay(SOUND_PCM_CH2);
                else
                    SND_DPCM2_startPlay(loop3_pcm_22k, sizeof(loop3_pcm_22k), SOUND_PCM_CH2, loop);
            }
            break;
        }

        case Z80_DRIVER_PCM4:
        {
            // set volume values for driver PCM4_ENV
            SND_PCM4_setVolume(SOUND_PCM_CH1, params_value[driver_ind][1]->value);
            SND_PCM4_setVolume(SOUND_PCM_CH2, params_value[driver_ind][2]->value);
            SND_PCM4_setVolume(SOUND_PCM_CH3, params_value[driver_ind][3]->value);
            SND_PCM4_setVolume(SOUND_PCM_CH4, params_value[driver_ind][4]->value);

            if (changed & state & BUTTON_START)
            {
                if (SND_PCM4_isPlaying(SOUND_PCM_CH4_MSK))
                    SND_PCM4_stopPlay(SOUND_PCM_CH4);
                else
                    SND_PCM4_startPlay(loop2_16k, sizeof(loop2_16k), SOUND_PCM_CH4, loop);
            }

            if (changed & state & BUTTON_A)
            {
                if (SND_PCM4_isPlaying(SOUND_PCM_CH1_MSK))
                    SND_PCM4_stopPlay(SOUND_PCM_CH1);
                else
                    SND_PCM4_startPlay(hat2_16k, sizeof(hat2_16k), SOUND_PCM_CH1, loop);
            }

            if (changed & state & BUTTON_B)
            {
                if (SND_PCM4_isPlaying(SOUND_PCM_CH2_MSK))
                    SND_PCM4_stopPlay(SOUND_PCM_CH2);
                else
                    SND_PCM4_startPlay(snare2_16k, sizeof(snare2_16k), SOUND_PCM_CH2, loop);
            }

            if (changed & state & BUTTON_C)
            {
                if (SND_PCM4_isPlaying(SOUND_PCM_CH3_MSK))
                    SND_PCM4_stopPlay(SOUND_PCM_CH3);
                else
                    SND_PCM4_startPlay(hat1_16k, sizeof(hat1_16k), SOUND_PCM_CH3, loop);
            }

            break;
        }

        case Z80_DRIVER_XGM:
        {
            if (changed & state & BUTTON_X)
            {
                XGM_setPCM(64, snare1_14k, sizeof(snare1_14k));
//                XGM_setPCM(64, sonic_jump_14k, sizeof(sonic_jump_14k));
                XGM_startPlayPCM(64, 10, SOUND_PCM_CH2);
            }
            if (changed & state & BUTTON_Y)
            {
                XGM_setPCM(65, hat1_14k, sizeof(hat1_14k));
//                XGM_setPCM(65, sonic_stop_14k, sizeof(sonic_stop_14k));
                XGM_startPlayPCM(65, 10, SOUND_PCM_CH3);
            }
            if (changed & state & BUTTON_Z)
            {
                XGM_setPCM(66, cri_14k, sizeof(cri_14k));
                XGM_startPlayPCM(66, 10, SOUND_PCM_CH4);
            }

            if (changed & state & BUTTON_A)
            {
                if (XGM_isPlaying()) XGM_stopPlay();
                else XGM_startPlay(sor2_xgm);
                musicSize = sizeof(sor2_xgm);
            }
            if (changed & state & BUTTON_B)
            {
                if (XGM_isPlaying()) XGM_stopPlay();
                else XGM_startPlay(bapcm_xgm);
                musicSize = sizeof(bapcm_xgm);
            }
            if (changed & state & BUTTON_C)
            {
                if (XGM_isPlaying()) XGM_stopPlay();
                else XGM_startPlay(midnight_xgm);
//                XGM_startPlay(toystory_xgm);
                musicSize = sizeof(midnight_xgm);
            }

            if (changed & state & BUTTON_START)
            {
                if (XGM_isPlaying()) XGM_pausePlay();
                else XGM_resumePlay();
            }
            break;
        }

        case Z80_DRIVER_XGM2:
        {
            if (changed & state & BUTTON_X)
            {
                XGM2_playPCM(snare1_13k, sizeof(snare1_13k), SOUND_PCM_CH1);
//                XGM2_playPCM(sonic_jump_13k, sizeof(sonic_jump_13k), SOUND_PCM_CH1);
            }
            if (changed & state & BUTTON_Y)
            {
                XGM2_playPCM(hat1_13k, sizeof(hat1_13k), SOUND_PCM_CH2);
//                XGM2_playPCM(sonic_stop_13k, sizeof(sonic_stop_13k), SOUND_PCM_CH2);
            }
            if (changed & state & BUTTON_Z)
            {
                XGM2_playPCM(cri_13k, sizeof(cri_13k), SOUND_PCM_CH3);
            }

            if (changed & state & BUTTON_A)
            {
                if (XGM2_isPlaying()) XGM2_stop();
                else XGM2_play(sor2_xgm2);
                musicSize = sizeof(sor2_xgm2);
            }
            if (changed & state & BUTTON_B)
            {
                if (XGM2_isPlaying()) XGM2_stop();
                else XGM2_play(bapcm_xgm2);
                musicSize = sizeof(bapcm_xgm2);
            }
            if (changed & state & BUTTON_C)
            {
                if (XGM2_isPlaying()) XGM2_stop();
                else XGM2_play(midnight_xgm2);
//                XGM_startPlay(toystory);
                musicSize = sizeof(midnight_xgm2);
            }

            if (changed & state & BUTTON_START)
            {
                if (XGM2_isPlaying()) XGM2_fadeOutAndPause(30);
                else
                {
                    XGM2_resume();
                    XGM2_fadeIn(30);
                }
            }
            break;
        }
    }

    // MODE button state changed
    if (changed & state & BUTTON_MODE)
    {
        dmaMethod++;
        dmaMethod &= 3;
    }

    if (changed & state)
        refreshDriverInfos();
}


static void vintEvent()
{
    u16 i;
    u16 in, out;
    char strNum[8];
    char str[40];

    // set BUS protection
    Z80_enableBusProtection();

    in = GET_VCOUNTER;

    if ((in >= 224) && (in <= 230))
    {
        switch(dmaMethod)
        {
            case 1:
                DMA_doDma(DMA_VRAM, 0, 0x8000, (6 * 1024) / 2, 2);
                break;

            case 2:
                for(i = 0; i < 5; i++)
                    DMA_doDma(DMA_VRAM, 0, 0x8000, 1024 / 2, 2);
                break;

            case 3:
                for(i = 0; i < 11; i++)
                    DMA_doDma(DMA_VRAM, 0, 0x8000, 256 / 2, 2);
                break;
        }
    }

    out = GET_VCOUNTER;

    // remove BUS protection
    Z80_disableBusProtection();

    if (driver->id == Z80_DRIVER_XGM)
    {
        // get Z80 cpu estimated load
        u16 load = XGM_getCPULoad();

        uintToStr(load, str, 3);
        strcat(str, " %");
        VDP_clearText(16, 21, 10);
        VDP_drawText(str, 16, 21);
    }
    else if (driver->id == Z80_DRIVER_XGM2)
    {
        // get Z80 cpu estimated load
        u16 load = XGM2_getCPULoad(TRUE);

        uintToStr(load, str, 3);
        strcat(str, " %");
        VDP_clearText(16, 21, 10);
        VDP_drawText(str, 16, 21);
    }
    else
    {
        VDP_clearTextLine(22);
        VDP_clearTextLine(23);
        VDP_clearTextLine(24);
    }

    if ((in >= 224) && (in <= 230))
    {
        switch(dmaMethod)
        {
            case 0:
                VDP_drawText("NONE     ", 13, 26);
                break;

            case 1:
                VDP_drawText("1 x 6KB  ", 13, 26);
                break;

            case 2:
                VDP_drawText("5 x 1KB  ", 13, 26);
                break;

            case 3:
                VDP_drawText("11 x 256B", 13, 26);
                break;
        }
    }
    else
        VDP_drawText("NOT DONE ", 13, 26);

    if (dmaMethod)
    {
        strcpy(str, "DMA start at ");
        uintToStr(in, strNum, 3);
        strcat(str, strNum);
        strcat(str, " - end at ");
        uintToStr(out, strNum, 3);
        strcat(str, strNum);

        VDP_drawText(str, 1, 27);
    }
    else
        VDP_clearTextLine(27);
}
