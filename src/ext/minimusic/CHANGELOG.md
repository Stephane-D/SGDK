# MiniMusic changelog

## 2023-12-20: Version 1.10

- (GitHub issue #12) Added a new function `MiniMusic_GetStatus` to query what the sound driver is doing. Currently it only reports if BGM is playing.
- It seems the C API was still broken under C++ (oops).
- Slowed down playback slightly to make it closer to 59.92Hz (NTSC refresh rate) than 60Hz, since it's more likely that you'll want the former instead.

## 2023-12-07: Version 1.03

- Fix C API which wasn't even *building* at all (goes to show how many people are using that API).

## 2023-11-05

- Added a very minimal sample program. Will need to update in the future with more sounds and a proper interface.
- Changed the format of the subheadings in the changelog since not all of these are version bumps (sorry).

## 2023-11-02: Version 1.02

- (GitHub issue #10) Fixed sound effect priority not resetting when stopping music (or playing new music, which implicitly stops the previous one), which prevented further lower priority sound effects from playing until a higher priority one reset it again. Last patch had a similar bugfix for pausing.
- Fixed an issue where `MuteFm` did not necessarily mute correctly FM channels 4 to 6 (and seemed to "work" by sheer coincidence only).

## 2023-08-31: Version 1.01

- Added `CHANGELOG.md` (this file).
- (GitHub issue #9) Fixed PSG channels not muting when pausing.
- (GitHub issue #8) Fixed sound effect priority not resetting when pausing, which prevented further lower priority sound effects from playing until a higher priority one reset it again.

## 2023-08-26: Version 1.00

- Initial release.
