#ifndef _AUDIO_H_
#define _AUDIO_H_


#define AUDIO_PCM_CH_AUTO   0x00

#define AUDIO_PCM_CH1       Z80_DRV_CH0_SFT
#define AUDIO_PCM_CH2       Z80_DRV_CH1_SFT
#define AUDIO_PCM_CH3       Z80_DRV_CH2_SFT
#define AUDIO_PCM_CH4       Z80_DRV_CH3_SFT

#define AUDIO_PCM_CH1_MSK   Z80_DRV_CH0
#define AUDIO_PCM_CH2_MSK   Z80_DRV_CH1
#define AUDIO_PCM_CH3_MSK   Z80_DRV_CH2
#define AUDIO_PCM_CH4_MSK   Z80_DRV_CH3

#define AUDIO_PAN_LEFT      0x80
#define AUDIO_PAN_RIGHT     0x40
#define AUDIO_PAN_CENTER    0xC0

#define AUDIO_MVS_SILENCE   0x00
#define AUDIO_MVS_ONCE      0x01
#define AUDIO_MVS_LOOP      0x02


// Z80_DRIVER_PCM
u8   isPlaying_PCM();
void startPlay_PCM(const u8 *sample, const u32 len, const u16 rate, const u8 pan);
void stopPlay_PCM();

// Z80_DRIVER_2ADPCM
u8 isPlaying_2ADPCM(const u16 channel_mask);
void startPlay_2ADPCM(const u8 *sample, const u32 len, const u16 channel, const u16 loop);
void stopPlay_2ADPCM(const u16 channel);

// Z80_DRIVER_4PCM
u8   isPlaying_4PCM(const u16 channel_mask);
void startPlay_4PCM(const u8 *sample, const u32 len, const u16 channel, const u16 loop);
void stopPlay_4PCM(const u16 channel);
void setChannel_4PCM(const u16 channel, const u8 volume);
u8   getChannel_4PCM(const u16 channel);

// Z80_DRIVER_4PCM_VOL
u8   isPlaying_4PCM_VOL(const u16 channel_mask);
void startPlay_4PCM_VOL(const u8 *sample, const u32 len, const u16 channel, const u16 loop);
void stopPlay_4PCM_VOL(const u16 channel);
void setVolume_4PCM_VOL(const u16 channel, const u8 volume);
u8   getVolume_4PCM_VOL(const u16 channel);

// Z80_DRIVER_MVSTRACKER
u8   isPlaying_MVSTRACKER();
void stopPlay_MVSTRACKER();
void startPlay_MVSTRACKER(const u8 *song, const u8 cmd);

// Z80_DRIVER_TFMTRACKER
void startPlay_TFMTRACKER(const u8 *song);


#endif // _AUDIO_H_
