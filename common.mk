# Common definitions

BIN := $(GDK)/bin
LIB := $(GDK)/lib
export PATH := $(BIN):$(PATH)

SRC_LIB := $(GDK)/src
RES_LIB := $(GDK)/res
INCLUDE_LIB := $(GDK)/inc
MAKEFILE_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
MAKEFILE_DIR := $(subst \,/,$(MAKEFILE_DIR))

ifneq ($(OS),Windows_NT)
	PREFIX ?= m68k-elf-
endif

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

JAVA := java
ECHO := echo
SIZEBND := $(JAVA) -jar $(BIN)/sizebnd.jar
RESCOMP := $(JAVA) -jar $(BIN)/rescomp.jar
