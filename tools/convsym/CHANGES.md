
# ConvSym version history

### Version 2.12.1 (2024-12-14)

* `deb2` output format:
	- Fixed an edge-case bug where if one block is too large and symbol heap exceeds 64 kb, all further blocks are skipped.

* Improve README, including some wording and terminology (e.g. "input/output parsers" -> "input/output formats").

### Version 2.12 (2024-12-11)

* Added support for symbol references instead of raw offsets in `-ref` and `-org` options:
  - Specify `-ref @MySymbol` to substitute offset of `MySymbol` from the symbol table (before any transformations or filtering);
  - This is useful, for example, if you need to write symbol table pointer in ROM, but you don't know pointer's offset beforehand - you can now reference it by symbol name.

* Minor stability improvements and general optimizations;
* Document more usage examples in the README.

### Version 2.11 (2024-12-08)

* Added new `txt` input parser to parse arbitrary text files; ConvSym can now parse SGDK's `symbols.txt` file.

* `asm` and `log` output formats:
  - Implement proper options support in `-outopt`. You can configure line format as `-outopt "/fmt='format-string'"` now (legacy `-outopt "format-string"` syntax is preserved). This goes in line with the new `txt` parser (which also has `/fmt` option among others and will allow to add additional options in the future;
  - Warn if line format string is incorrect (e.g. too few arguments specified).

* `log` input parser:
  - Log failed to parse lines on DEBUG level.

* `as_lst_exp` input parser:
  - Show a warning that `-inopt` is unsupported if user tries to set it.

* `deb2` and `deb1` output formats:
  - Made tree flattening algorithm introduced in 2.10 deterministic.

* Document all default parser options in ConvSym's usage message (printed when invoked without arguments), document `-debug` option.

### Version 2.10 (2024-12-07)

* Added `-addprefix` option to prepend any string to output symbols.

* `deb2` and `deb1` output formats:
  - Fixed a rare symbol encoding issue where data with unusual entropy would produce long prefix trees with some codes exceeding 16-bits. Respective characters (usually extremely rare) would then fail to decode properly corrupting a small set of symbol texts. A custom tree rebalancing algorithm was implemented to fix trees with codes longer than 16-bit;
  - Fixed a minor memory leak (<2 kb in a lifetime) on nodes in encoding function;
  - Fixed a tiny (several bytes) memory leak due to an unreleased file handle.

* `asm` and `log` output formats:
  - Properly report I/O error if output file couldn't be opened.

### Version 2.9.1 (2023-03-22)

* `asm68k_sym` parser:
  - Fixed incorrect behavior of `/processLocals-` option switch, if local labels were present in the symbol file. ConvSym would just add local labels in raw unprocessed form (e.g. `@local`, not `globalParent.local`) instead of ignoring them.

### Version 2.9 (2023-01-05)

* When appending symbol data to the end of ROM (when using `-a` flag), ConvSym now auto-aligns it on the even offset;
* `-noalign` option was added to force the old behavior (don't align on append).

* `asm68k_lst` parser:
  - Fixed missing support for multiple labels (symbols) on the same offset.

### Version 2.8 (2022-12-28)

* Completely overhauled `as_lst` parser; it's now stable and "Production-ready".
* The old experimental parser is still available as `as_lst_exp`;
* Improved built-in help (displayed in the command line): added usage examples, sorted options by groups, added README references;
* Improved README, documented `as_lst` and `as_lst_exp` parsers.

### Version 2.7.2 (2022-08-12)

* Fix SEGFAULT in `deb1` and `deb2` parsers due to out of boundary labels lookup.

### Version 2.7.1 (2022-07-23)

* Fix incorrect newlines produced by `log` and `asm` output formats on Windows;
* Fix a minor memory leak when a parser crashes;
* Overall stability and portability improvements.

### Version 2.7 (2021-04-27)

* Added support for multiple labels sharing the same offset for all input and output wrappers;

* `deb1` and `deb2` output formats:
	- Add "/favorLastLabels" option, which toggles choosing last labels when there are multiple labels at the same offset (first labels are preferred otherwise).

### Version 2.6 (2021-02-01)

* Implemented offset masks support for all the input wrappers; leave only lower 24-bits of offsets by default;
* Added `-mask` option to configure offset masking;
* Added `-in` and `-out` options as shortcuts for `-input` and `-output` respectively;
* Added STDIN and STDOUT support when processing input and output respectively.

* `asm68k_sym` input parser:
	- Added local labels support (local labels are produced when assembled with v+ option);
	- Fixed missing offset boundary and transformation logic (applied by `-range`, `-base` and `-mask` options);
	- Added "/localSign", "/localJoin" and "/processLocals" options to configure local labels processing.
* `asm68k_lst` and `as_lst` input parsers:
	- Fixed a bug that prevented offsets >=$80000000 to be added due to incorrect signed boundary check;
	- When several labels occur on the same offset, use the last label met, not the first;
	- Track last global label name correctly (it previously didn't update the label when it was filtered via boundary or other checks).
* `deb1` output format:
	- Fix memory corruption when symbol map requires more than 64 memory blocks;
	- Explicitly limit symbols map to 64 blocks, display an error when overflow was about to occur.
* `deb2` output format:
	- Fix infinite loop when the full the last block id was 0xFFFF;
	- Limit symbols map to 256 blocks, display error if more blocks were requested.

### Version 2.5.2 (2020-08-09)

* Fix SEGFAULT when attempting to write inaccesible output file.
* Minor error handling improvements.

### Version 2.5.1 (2020-01-25)

* Fix displaying of certain error messages.

### Version 2.5 (2018-10-30)

* Optimized and imporved `asm68k_lst` parser.
* Improved handling of conflicting command-line options.
* Fixed memory leaks on program termination (in both successful and failure states).
* Overall stability and error handling improvements.

### Version 2.1 (2018-07-08)

* Added `-toupper` and `-tolower` options to convert all the processed symbols to uppercase or lowecase accordingly. This helps to reduce size of symbol data in DEB1/DEB2 formats, as the compression takes advantage of it.
* Added new `log` input parser to support plain-text **.log**/**.txt** files as input.

### Version 2.0 (2018-01-14)

Initial version 2.x release.
