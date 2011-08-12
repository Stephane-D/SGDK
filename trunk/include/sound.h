#ifndef _SOUND_H_
#define _SOUND_H_


#define SOUND_PCM_CH_AUTO   0x00

#define SOUND_PCM_CH1       Z80_DRV_CH0_SFT
#define SOUND_PCM_CH2       Z80_DRV_CH1_SFT
#define SOUND_PCM_CH3       Z80_DRV_CH2_SFT
#define SOUND_PCM_CH4       Z80_DRV_CH3_SFT

#define SOUND_PCM_CH1_MSK   Z80_DRV_CH0
#define SOUND_PCM_CH2_MSK   Z80_DRV_CH1
#define SOUND_PCM_CH3_MSK   Z80_DRV_CH2
#define SOUND_PCM_CH4_MSK   Z80_DRV_CH3

#define SOUND_RATE_32000    0
#define SOUND_RATE_22050    1
#define SOUND_RATE_16000    2
#define SOUND_RATE_13400    3
#define SOUND_RATE_11025    4
#define SOUND_RATE_8000     5

#define SOUND_PAN_LEFT      0x80
#define SOUND_PAN_RIGHT     0x40
#define SOUND_PAN_CENTER    0xC0

#define SOUND_MVS_SILENCE   0x00
#define SOUND_MVS_ONCE      0x01
#define SOUND_MVS_LOOP      0x02


// Z80_DRIVER_PCM
u8   SND_isPlaying_PCM();
void SND_startPlay_PCM(const u8 *sample, const u32 len, const u8 rate, const u8 pan, const u8 loop);
void SND_stopPlay_PCM();

// Z80_DRIVER_2ADPCM
u8 SND_isPlaying_2ADPCM(const u16 channel_mask);
void SND_startPlay_2ADPCM(const u8 *sample, const u32 len, const u16 channel, const u8 loop);
void SND_stopPlay_2ADPCM(const u16 channel);

// Z80_DRIVER_4PCM
u8   SND_isPlaying_4PCM(const u16 channel_mask);
void SND_startPlay_4PCM(const u8 *sample, const u32 len, const u16 channel, const u8 loop);
void SND_stopPlay_4PCM(const u16 channel);

// Z80_DRIVER_4PCM_ENV
u8   SND_isPlaying_4PCM_ENV(const u16 channel_mask);
void SND_startPlay_4PCM_ENV(const u8 *sample, const u32 len, const u16 channel, const u8 loop);
void SND_stopPlay_4PCM_ENV(const u16 channel);
void SND_setVolume_4PCM_ENV(const u16 channel, const u8 volume);
u8   SND_getVolume_4PCM_ENV(const u16 channel);

// Z80_DRIVER_MVS
u8   SND_isPlaying_MVS();
void SND_stopPlay_MVS();
void SND_startPlay_MVS(const u8 *song, const u8 cmd);

// Z80_DRIVER_TFM
void SND_startPlay_TFM(const u8 *song);


#endif // _SOUND_H_
