# C API for MiniMusic

## Adding MiniMusic to your game

You need to add the following files from the `api-c` directory into your program:

* `minimus.c` is the API code proper
* `minimus.h` is the API header file
* `minimusb.inc` is the Z80 program

Whenever you need to call a MiniMusic function, make sure to have the following declaration at the beginning of that file:

```
#include "minimus.h"
```

## Initialization

To initialize the sound driver, you need to call `minimusic_init`. You can pass your sound data (up to 6KB) by passing a pointer to it and its size as arguments.

```
minimusic_init(&sound_data, sizeof(sound_data));
```

Check `doc/format.md` for the format of the sound data (and remember it has to fit within 6144 bytes). This includes all the tracks as well as the instruments.

## Playing music

You can play music or sound effects using the `minimusic_send_cmd` function. Pass the sound number as argument:

```
minimusic_send_cmd(1);
```

Passing 0 does nothing.

The "sound number" starts at 1 and depends on how many music and sound effects you have. If you have e.g. five music and ten sound effects, then music numbers range from 1 to 5 and sound effect numbers range from 6 to 15.

## Special commands

A few special values can be passed to `minimusic_send_cmd` instead of a BGM or SFX number:

* 255: stop music
* 254: pause playback
* 253: resume playback

When pausing, all current sound effects will be stopped, but you will be able to play new sound effects *after* it's paused (if you want to play a pause jingle, make sure you do it after sending the pause command!).

## DMA and I/O port accesses

While MiniMusic does not access the cartridge slot, it *does* access the 68000 bus (to use the PSG), so you still need to guard against hardware bugs when using DMA or the I/O ports.

To prevent this, request the Z80 bus before starting DMA or accessing the I/O port, and release the bus afterwards. You don't need to wait for the Z80 to release the bus, this is just to make sure the Z80 doesn't interfere. MiniMusic provides two inline functions to do this:

```
// any other setup needed here
MINIMUSIC_Z80_GUARD_BEGIN();
// start DMA or read pads here
MINIMUSIC_Z80_GUARD_END();
```

Note: only the command that triggers DMA needs to be guarded like this (you don't need to guard around other DMA registers).

## Check if music is playing

You can check if music is still playing by calling the `minimusic_get_status` function. Mask (AND) it against the value `MINIMUSIC_STATUS_BGM`, and if it isn't 0 then music is currently playing:

```
if (minimusic_get_status() & MINIMUSIC_STATUS_BGM) {
    // Music is playing...
} else {
    // Music not playing...
}
```

Note that it will report music as *still* playing if it's paused. It only returns false when:

* Music has stopped on its own (all tracks reached the end without looping).
* Music has been interrupted by sending a stop (255) command.
* No music has ever played to begin with.  
