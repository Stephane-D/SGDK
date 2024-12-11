# Common definitions

BIN := $(GDK)/bin
LIB := $(GDK)/lib

SRC_LIB := $(GDK)/src
RES_LIB := $(GDK)/res
INCLUDE_LIB := $(GDK)/inc

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
else
	# Native Linux and Docker
	PREFIX ?= m68k-elf-
	SHELL := sh
	RM := rm
	CP := cp
	MKDIR := mkdir

	AR := $(PREFIX)ar
	CC := $(PREFIX)gcc
	LD := $(PREFIX)ld
	NM := $(PREFIX)nm
	OBJCPY := $(PREFIX)objcopy
	CONVSYM := convsym
	ASMZ80 := sjasm
	MACCER := mac68k
	BINTOS := bintos
	LTO_PLUGIN :=
	LIBGCC := -lgcc
endif

JAVA := java
ECHO := echo
SIZEBND := $(JAVA) -jar $(BIN)/sizebnd.jar
RESCOMP := $(JAVA) -jar $(BIN)/rescomp.jar
