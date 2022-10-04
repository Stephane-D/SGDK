# Introduction

This repository contains a flash driver and a save manager that allows saving and loading data to/from the persistent flash memory available in many cheap flash cartridges (such as the FlashKit Carts from Krikkzz).

The `flash` module contains the flash driver supporting raw reading, writing and erasing contents from flash memory, while the `saveman` module contains the save manager implementing wear leveling, save slots, safe save, etc. Usually you will not have to use the `flash` module directly: you just use the `saveman` module that abstracts the complexities of the `flash` module under the hood.

The `main.c` and `lipsum.h` files are just used to build the sample and test code.

## Features

The save manager supports the following features:

* Multiple slot support: you can keep independent save files in different slots.
* Wear leveling: the module intelligently appends data in an efficient way to avoid flash chip wear as much as possible. The mechanism is specially efficient if you use small slots with big flash sectors.
* Safe save: the module avoids save data loss even in the case of a power cut while data was being saved. In this case, the incomplete save condition is detected on initialization, and data is corrected, so you can keep the last data set properly recorded before the incomplete save was initiated.

## Requirements

You will need a flash cart with the `#WE` signal properly wired and a supported chip that:
* Is compatible with the AMD specification.
* Implements a CFI interface.

This should translate in most flash carts you can find around!

## Understanding the restrictions

Being able to save directly to the flash chip has one immediate benefit: it's cheap. You don't need to add an additional SRAM + glue logic + battery to your cart. It will also keep your data for more time (there's no battery slowly draining). But there are some restrictions you must understand. In most cases they might not be troublesome, but sometimes they can be problematic enough to make you take the traditional route and use an SRAM enabled cart:

1. No emulator support. I suppose this will be solved when games using this technology start appearing, but as of today, no emulator will work with this module.
2. The space reserved for data save is discounted from the available ROM size. So for example if you have a 4 MiB cart, the default configuration will use 128 KiB for game saves, leaving you with 3968 KiB for your game instead of the usual 4096 KiB. There are ways to reduce the *waste* to 16 KiB, but this is something you have to keep in mind: if your game is tight on storage, this could be problematic.
3. Most functions in this module stop the Z80 and disable interrupts while running. The usual consequence to this restriction is you cannot play music/sound while saving data. The reason for this is that while you are modifying the flash chip contents, nobody (basically the 68000 and Z80) can read from the flash chip. If they do, they will get the erase/flash operation state instead of valid data, so disabling the Z80 and interrupts is the best way to make sure nobody tries reading.

# Usage

The save manager API uses the **slot** concept to define a location that can be written to, with the particularity that each time you write to a slot, its previous contents are completely overwritten. This allows implementing the 3 typical save slots you see in many games like **The Legend of Zelda**: when you start the game you choose a slot, and progress is written to that slot. But you have two more in case you want to start a game from scratch without losing your progress in the other slot.

In this save manager, slot length needs not to be fixed: each time you save to a slot, you specify how many bytes you want to write. This allows more complex slot configurations. For example you can define a 5 slot layout with one very small slot used only to store the game configuration (e.g. button layout, music on/off toggle, text speed, etc.), another slot to store the scoreboard, and 3 slots to keep the game progress.

The save manager API provides functions to write to, read from and delete these slots.

## Basic usage

If you just want to support a few slots with up to a few hundreds of bytes each, and if you can afford losing 128 KiB from the usual 4 MiB used for the ROM, you do not need to worry with the more advanced topics in the next subsection. You can just proceed like this:

1. Call `sm_init()` to initialize the module. Make sure it is always called with the same parameters.
2. Call `sm_load()` to load data, `sm_save()` to save data and `sm_delete()` to delete saved data.
3. In case you need to perform a factory reset, call `sm_clear()`. Note this will delete all save contents in all slots.

And that's all!

But if you want to minimize the space used by the save manager, or want to use many slots with bigger slot lengths, you need a better understanding of how the flash memory is managed, so... keep reading!

## Advanced usage

Although the save manager module abstracts most of these complexities, it is useful understanding how a flash chip works to get why the save manager works the way it does.

### How a flash chip works

Typically flash chips support at least 3 basic operations:

* Reading: not much to say, you just put the address and get the data stored in that address.
* Programming: in flash chips, writing data to a memory address is usually called programming.
* Erasing: in flash chips, erasing is the operation consisting in setting to ones all the bits in one *sector*. So reading an erased chip will get you all 0xFFs.

Words in each flash address can be programmed in a one by one basis, as long as the address you are writing to was previously erased. But if you want to program a word to an address that is not erased (i.e. has not value 0xFFFF) then you need to erase it first (well, there are some cases allowing to program words to non erased addresses, but the usual case is you can only program data if the address is erased). The bad news is that you cannot erase a single word: you must erase a complete flash sector. And flash sectors of the chips typically used for Megadrive homebrew are usually as big as 64 KiB. To make things even worse, it's pretty common for chips having odd flash sector layouts.

### The flash chip layout

To learn your chip layout, the best way is usually searching its datasheet. For example FlashKit Carts from Krikkzz usually mount an **M29W640FB** chip. [By having a look to its datasheet](https://media-www.micron.com/-/media/client/global/documents/products/data-sheet/nor-flash/parallel/m29w/m29w640f.pdf), you can learn this is a *bottom boot block* part, having 8 x 8 KiB sectors at the beginning of the chip, and 127 x 64 KiB sectors just after the initial 8 KiB ones. The datasheet has some tables that might help you better understanding this layout, but keep in mind the first 64 KiB of the chip have 8 KiB sectors, while the remaining of the chip have 64 KiB sectors.

To ease routines, by default the save manager uses the last two sectors in the addressable 4 MiB range. If you are using the chip above, these last two sectors will be 64 KiB each, taking 128 KiB total for the save functions. But you might find other chips with different layouts. For example if the cart was mounted with a S29GL032A90TFIR3, you could find in its datasheet that it's a top boot device and the last 8 sectors of the chip are 8 KiB each, so in this case by default the save manager would use just 16 KiB. This is usually an advantage (because you have more room free for your game code and data), but has also some drawbacks.

### The slot configuration

Once you have identified the sector size you will be using (typically 64 KiB, but could vary depending on the chip and where you locate the data), you have to make sure your required slot layout will fit in the sector. Add the combined length of all the slots you need and leave some room for the internal headers: the result should fit in one sector. So for example if you need 5 slots of 2 KiB each, and you are using 64 KiB sectors, they will fit without a problem. But if you are using 8 KiB sectors, you are out of luck, you need 2 KiB * 5 = 10 KiB (plus headers) and only have 8 KiB. In this case you need to reduce the amount of slots and/or their sizes, or relocate the position used to save to use bigger sectors.

### Locating where the save data is stored

The location of the save data is decided when you call `sm_init()`. The parameter `max_length_restrict` defines where the save slots are located. If you set this parameter to `0`, the function applies the default, using the two sectors at the end of the 4 MiB range (`0x400000`). But you might want to further restrict the length, for example if you are using a 2 MiB chip instead of a 4 MiB one, you will have to set the `max_length_restrict` parameter to `0x200000` (2 MiB).

Even when using a 4 MiB or bigger chip, you might want to change the `max_length_restrict` parameter to change the length of the sectors you are using. For example in the case of the **M29W640FB** chip explained before, you might want to set the `max_length_restrict` parameter to `0x6000` to use sectors 1 and 2, that are 8 KiB instead of 64 KiB (you leave sector 0 that goes from 0x0000 to 0x2000 for the boot, while sectors 1 and 2 that go from 0x2000 to 0x6000 are used for the save manager). If you do this, you must be careful and ensure no game code or data goes to these sectors, or it will be erased by the module, breaking your game! This can be achieved by modifying the linker file, but this topic goes beyond the scope of this README file.

## That's cool, but can I write to flash directly?

If you do not want to use the save manager functions, you can directly use the `flash` module to do raw program/erase operations on the flash chip. In that case do not use the `saveman` module at all, only use the `flash` module. Be warned that using this module is more complex: you have to manually disable interrupts and stop Z80, erase sectors when needed, you have to use raw memory addresses, and you lose the slot function and the safe save unless you implement them by yourself. But for simple use cases, using the `flash` module directly is another possibility.

# Author

The code in this repository has been written by Jesús Alonso (doragasu).

# License

This code is provided with NO WARRANTY under the MIT license.
