# Common definitions

ifeq ($(GDK),)
	BIN := bin
	LIB := lib

	SRC_LIB := src
	RES_LIB := res
	INC_LIB := inc
else ifeq ($(GDK),.)
	BIN := bin
	LIB := lib

	SRC_LIB := src
	RES_LIB := res
	INC_LIB := inc
else
	BIN := $(GDK)/bin
	LIB := $(GDK)/lib

	SRC_LIB := $(GDK)/src
	RES_LIB := $(GDK)/res
	INC_LIB := $(GDK)/inc
endif

# Toolchain location (gcc, as, ld, ar, nm, objcopy...).
#  - Windows: bundled binaries from $(BIN), not overridable from the environment (a stray CC/AR from a
#    MinGW/MSYS shell must not hijack the bundled toolchain).
#  - Linux / macOS: tools are taken from PATH using PREFIX (default 'm68k-elf-').
#    TOOLCHAIN_DIR can give the toolchain folder, and CC, AR, LD, NM, OBJCPY, CONVSYM, ASMZ80, MACCER, BINTOS,
#    LTO_PLUGIN, LIBGCC can be overridden from the environment or the make command line.
#    The external GCC must be >= 10 (LTO) and target 68000 (-m68000 multilib, including libgcc).
ifeq ($(OS),Windows_NT)
	# Native Windows
	SHELL := $(BIN)/sh
	RM := $(BIN)/rm
	CP := $(BIN)/cp
	MKDIR := $(BIN)/mkdir

	AR := $(BIN)/ar
	CC := $(BIN)/gcc
	LD:= $(BIN)/ld
	NM:= $(BIN)/nm
	OBJCPY := $(BIN)/objcopy
	CONVSYM := $(BIN)/convsym
	ASMZ80 := $(BIN)/sjasm
	MACCER := $(BIN)/mac68k
	BINTOS := $(BIN)/bintos
	LTO_PLUGIN := --plugin=liblto_plugin.dll
	LIBGCC := $(LIB)/libgcc.a
	GCC_B := -B$(BIN)
else
	# Native Linux, macOS and Docker
	PREFIX ?= m68k-elf-
	TOOLCHAIN_DIR ?=
	TC := $(if $(TOOLCHAIN_DIR),$(TOOLCHAIN_DIR)/)
	SHELL := sh
	RM := rm
	CP := cp
	MKDIR := mkdir

	# AR, CC and LD are predefined by make (origin 'default'), so '?=' can't be used for them
	ifneq ($(filter default undefined,$(origin AR)),)
		AR := $(TC)$(PREFIX)ar
	endif
	ifneq ($(filter default undefined,$(origin CC)),)
		CC := $(TC)$(PREFIX)gcc
	endif
	ifneq ($(filter default undefined,$(origin LD)),)
		LD := $(TC)$(PREFIX)ld
	endif
	NM ?= $(TC)$(PREFIX)nm
	OBJCPY ?= $(TC)$(PREFIX)objcopy
	CONVSYM ?= convsym
	ASMZ80 ?= sjasm
	MACCER ?= mac68k
	BINTOS ?= bintos
	LTO_PLUGIN ?=
	LIBGCC ?= -lgcc
	# makes gcc look for its sub-programs (cc1, as, ld, lto-wrapper...) in the toolchain folder
	GCC_B := $(if $(TOOLCHAIN_DIR),-B$(TOOLCHAIN_DIR))
endif

JAVA := java
ECHO := echo
SIZEBND := $(JAVA) -jar $(BIN)/sizebnd.jar
RESCOMP := $(JAVA) -jar $(BIN)/rescomp.jar
