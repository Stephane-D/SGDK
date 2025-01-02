
# ConvSym - Symbol extraction and conversion utility

**ConvSym** is a command-line utility aimed to extract symbol lists from various assembler-specific file formats and convert them into DEB1/DEB2 formats supported by the "Advanced Error Handler and Debugger" or human-readable plain-text files.

It was originally designed to be used with the **ASM68K** and **The AS Macroassembler** assemblers, however, ConvSym's high configurability makes it possible to use with other tools and environments, including SGDK and other projects with **GNU AS**.

The utility supports various input and output processing and transformation options, allowing for a high level of flexibility.

## Table of contents

* [Usage](#usage)
  * [Supported options](#supported-options)
  * [Examples](#examples)
* [Converting SGDK symbols](#converting-sgdk-symbols)
* [Input formats](#input-formats)
  * [`asm68k_sym` format](#asm68k_sym-format)
  * [`asm68k_lst` format](#asm68k_lst-format)
  * [`as_lst` format](#as_lst-format)
  * [`as_lst_exp` format](#as_lst_exp-format)
  * [`log` format](#log-format)
  * [`txt` format](#txt-format)
* [Output formats](#output-formats)
  * [`deb2` output format](#deb2-output-format)
  * [`deb1` output format](#deb1-output-format)
  * [`asm` output format](#asm-output-format)
  * [`log` output format](#log-output-format)
* [Version history](#version-history)

## Usage

**ConvSym** uses the following command-line arguments format:

	convsym [input_file|-] [output_file|-] <options>

*Input* and *output files* are mandatory arguments. When run without arguments, options summary is displayed.

When using `-` as input and/or output file name, the I/O is redirected to STDIN and/or STDOUT respectively.

### Supported options

```
  -in [format]
  -input [format]
    Selects input file format. Supported formats: asm68k_sym, asm68k_lst,
    as_lst, as_lst_exp, log, txt
    Default: asm68k_sym

  -out [format]
  -output [format]
    Selects output file format. Supported formats: asm, deb1, deb2, log
    Default: deb2

  -inopt [options]
    Additional options specific for the input format.
    Default options (depending on -in [format]):
      -in asm68k_sym -inopt "/localSign=@ /localJoin=. /processLocals+"
      -in asm68k_lst -inopt "/localSign=@ /localJoin=. /ignoreMacroDefs+ /ignoreMacroExp- /addMacrosAsOpcodes+ /processLocals+"
      -in as_lst -inopt "/localJoin=. /processLocals+ /ignoreInternalSymbols+"
      -in log -inopt "/separator=: /useDecimal-"
      -in txt -inopt "/fmt='%s %X' /offsetFirst-"

  -outopt [options]
    Additional options specific for the output format.
    Default options (depending on -out [format]):
      -out deb2 -outopt "/favorLastLabels-"
      -out deb1 -outopt "/favorLastLabels-"
      -out asm -outopt "/fmt='%s:       equ     $%X'"
      -out log -outopt "/fmt='%X: %s'"

  -debug
    Enables verbose debug-level logging. Can be useful for troubleshooting.

Offsets conversion options:
  -base [offset]
    Sets the base offset for the input data: it is subtracted from every 
    symbol's offset found in [input_file] to form the final offset.
    Default: 0

  -mask [offset]
    Sets the mask for the offsets in the input data: it's applied to every 
    offset found in [input_file] after the base offset subtraction (if occurs).
    Default: FFFFFF

  -range [bottom] [upper]
    Determines the range for offsets allowed in a final symbol file (after 
    subtraction of the base offset).
    Default: 0 3FFFFF

  -a
    Enables "Append mode": symbol data is appended to the end of the 
    [output_file]. Data overwrites file contents by default.
    This is usually used to append symbols to ROMs.

  -noalign
    Don't align symbol data in "Append mode", which is aligned to nearest
    even offset by default. Using this option is not recommended, it's only 
    there to retain compatilibity with older ConvSym versions.

Symbol table dump options:
  -org [offset]
  -org @[symbolName]
    If set, symbol data will placed at the specified [offset] in the output 
    file. This option cannot be used in "append mode".
    You can specify @SomeSymbol instead of plain offset, in this case ConvSym 
    will resolve that symbol's offset.

  -ref [offset]
  -ref @[symbolName]
    If set, a 32-bit Big Endian offset pointing to the beginning of symbol
    data will be written at specified offset. This is can be used, 
    if symbol data pointer must be written somewhere in the ROM header.
    You can specify @SomeSymbol instead of plain offset, in this case ConvSym 
    will resolve that symbol's offset.

Symbols conversion and filtering options:
  -toupper
    Converts all symbol names to uppercase.

  -tolower
    Converts all symbol names to lowercase.

  -addprefix [string]
    Prepends a specified prefix string to every symbol in the resulting table.
    Done after all other transformations.

  -filter [regex]
    Enables filtering of the symbol list fetched from the [input_file]
    based on a regular expression.

  -exclude
    If set, filter works in "exclude mode": all labels that DO match
    the -filter regex are removed from the list, everything else stays.
```

### Examples

**Default options.** Converts ASM68K symbol file to DEB2 format (implies `-input asm68k_sym` and `-output deb2` for Defaults respectively):

```sh
convsym symbols.sym symbols.deb2
```

**Append to ROM.** Converts ASM68K symbol file to DEB2 format (`-output deb2`) and appends it (`-a`) to the end of ROM (for use with MD Debugger):

```sh
convsym symbols.sym rom.bin -output deb2 -a
```

**AS Listing to Log file.** Convert listing file `listing.lst` in `as_lst` format to `symbols.log` of `log` format:

```sh
convsym listing.lst symbols.log -input as_lst -output log
```

**Symbol filtering and transformation.** Converts from plaintext symbol table (log file), excludes symbols starting with "z80" (`-filter "z80.+" -exclude`), converts everything to lowercase for better compression (`-tolower`) and appends to ROM and writes a 32-bit Big-Endian pointer to the symbol table (where it was appended) at offset 0x200 of ROM (`-ref 200`):

```sh
convsym symbols.log rom.bin -input log -a -filter "z80.+" -exclude -tolower -ref 200
```

## Converting SGDK symbols

Since **version 2.11** ConvSym supports converting symbols in SGDK projects.

SGDK logs all symbols to `symbol.txt` file upon build. Since version 2.11 ConvSym introduced `txt` input format which can be configured to recognize this file's format.

Here's how ConvSym can be invoked in the command line to store symbols from `out/symbol.txt` file in `out/rom.bin`:

```sh
convsym out/symbol.txt out/rom.bin -in txt -inopt "/fmt='%X %*[TtBbCcDd] %511s /offsetFirst+" -range 0 FFFFFF -a -ref @MDDBG__SymbolTablePtr
```

Let's break down command line options:

- `out/symbol.txt out/rom.bin` - specifies path to input and output files respectively;
- `-in txt` - selects `txt` format for the input file;
- `-inopt "/fmt='%X %*[TtBbCcDd] %511s /offsetFirst+"` - specifies `/fmt` and `/offsetFirst` format options (they are exclusive to `txt` format), `/fmt` describes line format for the file, which includes:
   - A hex offset (`%X`);
   - One of the following symbol type specifiers `T`, `t`, `B`, `b`, `C`, `c`, `D`, `d` (others are ignored);
   - A label name which shouldn't exceed 512 characters with null terminator (`%511s`);
   - For more information, search reference for `sscanf` format (C standard library), which is used under the hood here.
- `-range 0 FFFFFF` - adds both ROM and RAM symbols to the output (covers the entire 68K 24-bit address space), it would've been only ROM by default (`-range 0 3FFFFF`);
- `-a` - appends output file (the ROM) instead of overwriting it;
- `-ref @MDDBG__SymbolTablePtr` - writes reference pointer to the symbol table (where ROM was appended) at offset specified by the `MDDBG__SymbolTablePtr` symbol in ROM so MD Debugger can read it (**WARNING!** Make sure this symbol exists, or ConvSym will fail).


## Input formats

Summary of currently supported input formats (input data format can be specified via `-input` option, or its shorthand: `-in`):

* `asm68k_sym`, `asm68k_lst` - **ASM68K** assembler symbol and listing files;
* `as_lst`, `as_lst_exp` - **The AS Macro Assembler** listing files (*stable* since **version 2.8**, and *experimental*);
* `log` - Plain-text symbol tables (since **version 2.1**);
* `txt` - Generic text format parser with configurable format string (since **version 2.12**).

Some formats support additional options, which can be specified via `-inopt` option. These options are described below.

### `asm68k_sym` format

Expects a symbol file produced by the **ASM68K** assembler for input. It's the recommended input format for projects using **ASM68K**.

It also supports local symbols, if produced by the assembler.

> [!NOTE]
> 
> In order to include local labels in the symbol file, `/o v+` assembly option should be used.

**Options:**

Since **version 2.6**, the following options are supported:

```
  /localSign=[x]
    determines character used to specify local labels

  /localJoin=[x]
    character used to join local label and its global "parent"

  /processLocals[+|-]
    specify whether local labels will processed (if not, the above
    options have no effect)
```

Default options can be expressed as follows:

	-inopt "/localSign=@ /localJoin=. /processLocals+"

### `asm68k_lst` format

Expects a listing file produced by the **ASM68K** assembler for input. Local symbols are also supported by default. Since parsing of listing files is less reliable, symbol files and `asm68_sym` format is recommended instead (see above).

> [!NOTE]
>
> In order for some macro-related parsing options to work correctly, `/m` argument should be used on the assembler side to properly expand macros in the listing file (please consult the assembler manual for more information).

**Known issues**:

* The format will ignore line break character `&`, as line continuations aren't properly listed by the **ASM68K** assembler; some information may be lost.
* Labels before the `if` directive (and its derivatives) may not be included in the listing file due to the assembler bug, hence they will be missing from the symbols table generated by **ConvSym**.
* format doesn't tolerate `SECTION` directives in the listing files, as assembler generates incorrect offsets whenever they are used; expect a lot of missing or misplaced symbols if you use them. The fix for this cannot be provided in the current implementation of the format.

**Options:**

```
  /localSign=[x]
    determines character used to specify local labels; default: @

  /localJoin=[x]
    character used to join local label and its global "parent"; default: .

  /ignoreMacroDefs[+|-]
    specify whether macro definitions listings should be ignored (lines between
    "macro" and "endm"); default: +

  /ignoreMacroExp[+|-]
    specify if lines representing macro expansions should be ignored; default: -

  /addMacrosAsOpcodes[+|-]
    set if macros that process label as parameter (defined as "macro *") should 
    be recognized when used; default: +

  /processLocals[+|-]
    specify whether local labels will processed; default: +
```

Default options can be expressed as follows:

	-inopt "/localSign=@ /localJoin=. /ignoreMacroDefs+ /ignoreMacroExp- /addMacrosAsOpcodes+ /processLocals+"


### `as_lst` format

Expects a listing file produced by the **AS** assembler for input. It's the recommended format for projects using **AS**.

Since version **version 2.8**, it works by processing a symbol table at the end of the file. This parser superseded the old experimental one, which is now available as `as_lst_exp` (so if you're looking for pre-v2.8 behaviour for some reason, use that instead).

It also supports local symbols, if produced by the assembler.

**Known issues**:

* Sonic 2 and Sonic 3K disassemly also compile the Z80 driver in the same project using `org`/`phase` to locate Z80-related labels starting from offset $000000. This causes Z80 and M68K symbols to be interleaved, thus messing up the symbol table for up to the first 8 kb of ROM data. There's no clean way to get rid of conflicting labels, but thankfully the disassemblies have most of Z80-labels start with the letter `z`, so you can add the following command-line options to filter them out: `-exclude -filter "z.+"'`

**Options:**

Since **version 2.8**, the following options are supported:

```
  /localJoin=[x]
    character used to join local label and its global "parent"

  /processLocals[+|-]
    specify whether local labels will processed (if not, the above
    options have no effect)

  /ignoreInternalSymbols[+|-]
    whether to ignore internal symbols (e.g. `__FORW123`) generated by AS in 
    place of nameless labels (+, - etc)
```

Default options can be expressed as follows:

	-inopt "/localJoin=. /processLocals+ /ignoreInternalSymbols+"


### `as_lst_exp` format

*Available since **version 2.8**.*

This is an experimental version of listing files parser for the AS assembler. Like `as_lst` format, it expects a listing file produced by the **AS** assembler for input. 

Using this parser is currently not recommended and its implementation may drastically change in future versions of ConvSym.

Before **version 2.8** it was actually in place of the `as_lst` format, but the latter has since been replaced with a more stable and refined implementation.


### `log` format

*Available since **version 2.1**.*

Expects a plain-text file, where each row logs an individual symbol name and its offset, in order.

The default format is the following:

```
[HexOffset]: [SymbolName]
```

Whitespaces and tabulation are ignored and don't affect parsing.

**Options:**

```
  /separator=[x]
    determines character that separates labes and offsets, default: ":"

  /useDecimal[+|-]
    sets whether offsets should be parsed as decimal numbers; default: -
```

Default options can be expressed as follows:

	-inopt "/separator=: /useDecimal-"

### `txt` format

*Available since **version 2.11**.*

Used for generic text files and can be flexibly configured for arbitrary line formats using `printf`-like syntax (`scanf` format from the C standard library to be exact). It defaults to `%s %X` (e.g. `MyLabel 1C2`).

With additional configuration it can be used to parse SGDK's `symbols.txt` file (see [Converting SGDK symbols](#converting-sgdk-symbols) section).

**Options:**

```
  /fmt='format-string'
    specifies format string to parse input file lines, default: "%s %X"

  /offsetFirst[+|-]
    specifies whether offset comes first in the input string; default: -
```

Default options can be expressed as follows:

	-inopt "/fmt='%s %X' /offsetFirst-"

## Output formats

Summary of currently supported output formats (output data format can be specified via `-output` option, or its shorthand: `-out`):

* `deb2` - Debug symbols database format for "The Advanced Error Handler and Debugger 2.x";
* `deb1` - Debug symbols database format for "The Advanced Error Handler and Debugger 1.x";
* `asm`, `log` - Plain-text **.asm** and **.log**/**.txt** files.

Some formats support additional options, which can be specified via `-outopt` option. These options are described below.

### `deb2` output format

Outputs debug symbols database in DEB2 format, which is the format supported by the MD Debugger and Error Handler. This is the default output format.

**Options:**

Since **version 2.7**, the following options are supported:

```
  /favorLastLabels[+|-]
    sets whether to prefer the last processed label when multiple labels share 
    the same offset (the first processed label is chosen otherwise); default: -
```

Default options can be expressed as follows:

	-outopt "/favorLastLabels-"

### `deb1` output format

Outputs debug symbols database in old DEB1 format. This is an outdated and limited format which is not supported by the current version of MD Debugger and Error Handler. This format only aims to retain compatibility with the Error Handler 1.0.

**Options:**

Since **version 2.7**, the following options are supported:

```
  /favorLastLabels[+|-]
    sets whether to prefer the last processed label when multiple labels share 
    the same offset (the first processed label is chosen otherwise); default: -
```

Default options can be expressed as follows:

	-outopt "/favorLastLabels-"


### `asm` output format

By default, outputs a symbol list in assembly format recognized by both **ASM68K** and **AS** assemblers.

The default format is the following:

```
[SymbolName]:	equ	$[HexOffset]
```

This format can be altered by passing **printf**-compatible format string, where the first argument corresponds to the symbol name and the second corresponds to the offset.

**Options:**

Since **version 2.11**, the following options are supported:

```
  /fmt='format-string'
    specifies format string to print (label, offset) pairs, default: "%s:	equ	$%X"
```

Default options can be expressed as follows:

	-outopt "/fmt='%s:	equ	$%X'"

Before **version 2.11** format string option could be specified as follows (this legacy syntax is still recognized):

	-outopt "%s:	equ	$%X"


### `log` output format

By default, outputs symbol list in plain-text format that is compatible with the input format of the same name (see "`log` input format").

The default format is the following:

```
[HexOffset]: [SymbolName]
```

This format can be altered by passing **printf**-compatible format string, where the first argument corresponds to the symbol name and the second corresponds to the offset.

**Options:**

Since **version 2.11**, the following options are supported:

```
  /fmt='format-string'
    specifies format string to print (offset, label) pairs, default: "%X: %s"
```

Default options can be expressed as follows:

	-outopt "/fmt='%X: %s'"

Before **version 2.11** format string option could be specified as follows (this legacy syntax is still recognized):

	-outopt "%X: %s"


## Version history

See [CHANGES.md](CHANGES.md).
