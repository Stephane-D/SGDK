# MiniMusic on Teradrive

**Disclaimer:** these are only observations. Use of MiniMusic on a Teradrive is *not* officially supported (outside of its Mega Drive compatibility).

## Available Z80 RAM

MiniMusic is normally limited to 6KB of sound data. This limit comes from the fact that Z80 RAM on Mega Drive is 8KB, and the first 2KB are reserved for the sound program, leaving 6KB to use for data.

Teradrive however has 16KB of Z80 RAM. This means that 14KB should be available for data. MiniMusic should be able to use all 14KB of data just fine, only bear into mind that it's going to be a *very* narrow part of your userbase.

## Using MiniMusic from DOS

In theory, MiniMusic could be run directly on the Z80 from DOS if you *really* know what you're doing.

To initialize the sound driver:

1. Copy the Z80 program to Z80 address `$0000` onwards
2. Copy the sound data to Z80 address `$0800` onwards
3. Reset and start running the Z80

To send a command:

1. Pause the Z80
2. If Z80 address `$0000` = 0 then write command there, resume Z80 and quit
3. If Z80 address `$0001` = 0 then write command there, resume Z80 and quit
4. If Z80 address `$0002` = 0 then write command there, resume Z80 and quit
5. Let the Z80 run for some time and retry from step 1 (or give up and fail if you don't want to risk the CPU being stuck for a long time)

Additionally, you can implement `MiniMusic_GetStatus` this way (the value you read from Z80 RAM will be the value the function returns):

1. Pause the Z80
2. Read Z80 address `$0003`
3. Resume the Z80
