# MiniMusic sound data format

This document describes the format of the data that's loaded in Z80 RAM for use by the sound driver. The following notes apply:

* Numbers starting with `$` are hexadecimal.
* All the data must fit in 6144 bytes (6KB).
* All 16-bit values are little endian (first low byte, then high byte).
* Data will be placed starting from Z80 address `$800` (all pointers must be adjusted accordingly).

The only thing that needs to be at a specific address is the main header (see below), which must be at the very beginning. You can place everything else anywhere within that 6KB space (`$800` to `$1FFF`).

## Main header

The overall header for all the sound data is located at `$800` in Z80 RAM (i.e. the first eight bytes of the sound data) and is as follows:

* **1 byte:** number of music
* **1 byte:** number of sound effects
* **2 bytes:** pointer to music list
* **2 bytes:** pointer to sound effect list
* **2 bytes:** pointer to instrument list

Each of the three lists is a list of 16-bit pointers.

# Music and sound effect data

## Music header

* **1 byte:** number of FM tracks (from 0 to 6)
* **2 bytes × number of FM tracks:** pointers to each FM track
* **1 byte:** number of PSG tracks (from 0 to 4)
* **2 bytes × number of PSG tracks:** pointers to each PSG track

FM channels are assigned from channel 1  to 6, in order. Sound effects are allowed to take over channels 5 and 6, so allocate the less important instruments there.

PSG channels are assigned from channel 1 to 4: square, square, square, noise. If you need to use noise in music, you have to allocate all four channels. Sound effects are allowed to take over channels 3 and 4.

## Sound effect header

* **1 byte:** flags (see below)
* **2 bytes (if used):** pointer to FM channel 6 track
* **2 bytes (if used):** pointer to FM channel 5 track
* **2 bytes (if used):** pointer to PSG channel 3 track
* **2 bytes (if used):** pointer to PSG noise track

The first byte indicates two things. The lower nibble (bits 3-0) indicates which channels are used:

* **Bit 0:** uses FM channel 5
* **Bit 1:** uses FM channel 6
* **Bit 2:** uses PSG channel 3
* **Bit 3:** uses PSG noise

The upper nibble (bits 7-4) are the SFX priority, from 0 (lowest) to 15 (highest). When a SFX is already using a channel and we want to play a new one, it will be replaced if the new SFX has same or higher priority.

Multiple sound effects can play at the same time if they use different channels.

## Track data

Each "track" corresponds to a hardware channel and consists of a list of "opcodes", each of which indicate an action such as key-on, change the note length, change the volume, etc.

The following opcodes are available:

* **`$80` to `$FF`: set note length.** Sets the wait duration for any following key-on/off. Substract 127 to get the number of ticks to wait (from `$80` = 1 tick to `$FF` = 128 ticks).
* **`$00` to `$5F`: key-on.** It starts a new note and waits for the current note length. The pitch is given by the byte itself, in semitones (from `$00` = C1 to `$5F` = B8).
* **`$60`: key-off.** It stops the current note and waits for the current note length.
* **`$61`: cancel next key.** It removes the key part of the next key-on/off. For key-on, this changes the pitch and inserts a wait. For key-off, this only inserts the wait.
* **`$62`: start of main loop.**
* **`$63`: end of main loop.** Returns back to the main loop point (see above), or if it hasn't been set, it stops the track here.
* **`$64 nn`: start of sub-loop.** `nn` is the number of times to play this loop. Sub-loops can't be nested (you can't have a sub-loop inside another sub-loop).
* **`$65`: end of sub-loop.** If there are any iterations left it returns to the sub-loop point (see above), otherwise it continues forwards.
* **`$66 nn` or `$67 nn nn`: call to subroutine.** For `$66` the offset is 8-bit (sign extended), for `$67` the offset is 16-bit (little endian). The offset is counted from the end of the opcode. Subroutine calls can't be nested (you can't call to a subroutine from another subroutine).
* **`$68`: return from subroutine.** It continues where the last call left off.
* **`$69 nn`: set instrument.** `nn` is the instrument ID.
* **`$6A nn` or `$6B nn`: set transpose** (pitch offset), measured in semitones (in two's complement). It takes effect on the next key-on. `$6A` is absolute (`nn` is used as-is), `$6B` is relative (`nn` is added to the current transpose).
* **`$6C nn` or `$6D nn`: set attenuation** (volume), measured in -0.75dB steps. `$6C` is absolute (`nn` is used as-is), `$6D` is relative (`nn` is added to the current attenuation, in two's complement)
* **`$6E nn`: set panning.** `nn` bit 7 = left channel on, bit 6 = right channel on, bits 5-0 should be 0. Ignored for PSG channels.

## PSG noise channel

PSG noise channel may be used on its own or together with PSG channel 3. When entering notes with the PSG noise channel, rather than using semitones use these values:

- `$00`: periodic noise, high pitch
- `$01`: periodic noise, medium pitch
- `$02`: periodic noise, low pitch
- `$03`: periodic noise, channel 3
- `$04`: white noise, high pitch
- `$05`: white noise, medium pitch
- `$06`: white noise, low pitch
- `$07`: white noise, channel 3

When using `$03` or `$07`, the pitch comes from the PSG channel 3 track. You should also mute PSG channel 3, however by default instrument 0 is loaded which is already mute (so it only matters if you're already using another instrument).

# Instrument data

The instrument list is a list of 16-bit pointers with the Z80 addresses of every instrument. The list starts counting from instrument number 1, _not_ 0.

## FM instrument format

FM instruments use the same format as EIF (Echo Instrument Format). It consists of 29 bytes with the values of YM2612 registers as-is, in the following order:

* Register `$B0` (algorithm and feedback)
* Registers `$30`, `$34`, `$38`, `$3C` (MUL and DT)
* Registers `$40`, `$44`, `$48`, `$4C` (TL)
* Registers `$50`, `$54`, `$58`, `$5C` (AR and KS)
* Registers `$60`, `$64`, `$68`, `$6C` (DR and AMON)
* Registers `$70`, `$74`, `$78`, `$7C` (SR)
* Registers `$80`, `$84`, `$88`, `$8C` (RR and SL)
* Registers `$90`, `$94`, `$98`, `$9C` (SSG-EG)

Any unused bits must be left as 0.

## PSG instrument format

A PSG instrument consists of 6 bytes:

* TL (total level)
* SL (sustain level)
* AR (attack rate)
* DR (decay rate)
* SR (sustain rate)
* RR (release rate)

TL, SL, AR, DR, SR, RR all mean the same thing as for FM instruments. TL and SL are measured as the threshold level, in 0.125dB steps (note: this is volume, *not* attenuation). AR, DR, SR, RR are also measured in 0.125dB steps, this amount is added or substracted every time the envelope ticks.

A flat PSG envelope would be:

```
FF 00 FF 00 00 FF
```

## Instrument number 0

Instrument number 0 is reserved for the "null" instrument, which is used to mute the channel. You can't redefine this instrument (it isn't included in the list), but tracks are allowed to use it.
