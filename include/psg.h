#ifndef _PSG_H_
#define _PSG_H_

#define PSG_PORT            0xC00011

#define PSG_ENVELOPE_MIN    15
#define PSG_ENVELOPE_MAX    0

void PSG_init();

void PSG_write(u8 data);

void PSG_setEnvelope(u8 channel, u8 value);
void PSG_setTone(u8 channel, u16 value);
void PSG_setFrequency(u8 channel, u16 value);


#endif // _PSG_H_
