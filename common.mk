# Common definitions

BIN := $(GDK)/bin
LIB := $(GDK)/lib

SRC_LIB := $(GDK)/src
RES_LIB := $(GDK)/res
INCLUDE_LIB := $(GDK)/inc
MAKEFILE_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
MAKEFILE_DIR := $(subst \,/,$(MAKEFILE_DIR))

ifeq ($(OS),Windows_NT)
	# Native Windows
	SHELL := $(BIN)/sh.exe
	RM := $(BIN)/rm.exe
	CP := $(BIN)/cp.exe
	MKDIR := $(BIN)/mkdir.exe

	AR := $(BIN)/ar.exe
	CC := $(BIN)/gcc.exe
	LD:= $(BIN)/ld.exe
	NM:= $(BIN)/nm.exe
	OBJCPY := $(BIN)/objcopy.exe
	ASMZ80 := $(BIN)/sjasm.exe
	MACCER := $(BIN)/mac68k.exe
	BINTOS := $(BIN)/bintos.exe
	LTO_PLUGIN := --plugin=liblto_plugin-0.dll
	LIBGCC := $(LIB)/libgcc.a
else
	ifeq ($(SGDK_DOCKER),y)
		# Linux docker
		SHELL = sh
		RM = rm
		CP = cp
		MKDIR = mkdir

		AR := $(BIN)/ar
		CC := $(BIN)/gcc
		LD := $(BIN)/ld
		NM := $(BIN)/nm
		OBJCPY := $(BIN)/objcopy
		ASMZ80 := $(BIN)/sjasm
		MACCER := $(BIN)/mac68k
		BINTOS := $(BIN)/bintos
		LTO_PLUGIN := --plugin=liblto_plugin-0.dll
		LIBGCC := $(LIB)/libgcc.a
	else
		# Native Linux
		PREFIX := m68k-elf-
		SHELL = sh
		RM = rm
		CP = cp
		MKDIR = mkdir

		AR := $(PREFIX)ar
		CC := $(PREFIX)gcc
		LD := $(PREFIX)ld
		NM := $(PREFIX)nm
		OBJCPY := $(PREFIX)objcopy
		ASMZ80 := sjasm
		MACCER := mac68k
		BINTOS := bintos
	    LTO_PLUGIN :=
		LIBGCC := -lgcc
	endif
endif

JAVA := java
ECHO := echo
SIZEBND := $(JAVA) -jar $(BIN)/sizebnd.jar
RESCOMP := $(JAVA) -jar $(BIN)/rescomp.jar
