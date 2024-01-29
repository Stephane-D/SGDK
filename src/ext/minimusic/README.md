# MiniMusic

Version 1.10

A tiny Z80 sound driver for Mega Drive that runs entirely off Z80 RAM and doesn't access the cartridge slot. This may be useful if you want to play sound while writing back to cartridge Flash, or if you simply want to keep things tiny.

This driver is under the zlib license (see `LICENSE.txt`). You are *not* required to credit the author, just don't lie about it.

## How to use

* If using C: check `doc/api-c.md`
* If using asm: check `doc/api-asm.md`

The format for the sound data is documented in `doc/format.md`. A sample program (asm API only) is available in the `sample-asm` directory.

## Github

https://github.com/sikthehedgehog/minimusic