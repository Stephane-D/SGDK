cmake_minimum_required(VERSION 3.22)

# Set the compiler prefix
if(DEFINED ENV{PREFIX})
  set(PREFIX "$ENV{PREFIX}")
endif()

# Set the compiler/assembler name
set(CMAKE_C_COMPILER ${PREFIX}gcc)
set(CMAKE_ASM_COMPILER ${PREFIX}gcc)
set(CMAKE_AR ${PREFIX}ar)
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

project(md C ASM)

include(ExternalProject)
ExternalProject_Add(
  BINTOS
  SOURCE_DIR ${PROJECT_SOURCE_DIR}/tools/bintos/src
  BINARY_DIR ${PROJECT_SOURCE_DIR}/tools/bintos/src/build
  INSTALL_COMMAND install bintos ${PROJECT_SOURCE_DIR}/bin
  BUILD_ALWAYS TRUE)
ExternalProject_Add(
  XGMTOOL
  SOURCE_DIR ${PROJECT_SOURCE_DIR}/tools/xgmtool/src
  BINARY_DIR ${PROJECT_SOURCE_DIR}/tools/xgmtool/src/build
  INSTALL_COMMAND install xgmtool ${PROJECT_SOURCE_DIR}/bin
  BUILD_ALWAYS TRUE)
ExternalProject_Add(
  SJASM
  CONFIGURE_COMMAND ""
  DOWNLOAD_COMMAND ""
  BUILD_COMMAND make -C ${PROJECT_SOURCE_DIR}/tools/sjasm/src
  INSTALL_COMMAND install ${PROJECT_SOURCE_DIR}/tools/sjasm/src/sjasm ${PROJECT_SOURCE_DIR}/bin
  BUILD_ALWAYS TRUE)

include_directories(${PROJECT_SOURCE_DIR}/inc)
include_directories(${PROJECT_SOURCE_DIR}/res)
include_directories(${PROJECT_SOURCE_DIR}/src)

set(RESCOMP java -jar ${PROJECT_SOURCE_DIR}/bin/rescomp.jar)
set(ASMZ80 ${PROJECT_SOURCE_DIR}/bin/sjasm -q)
set(BINTOS_EXE ${PROJECT_SOURCE_DIR}/bin/bintos)

add_compile_options(
  -DSGDK_GCC
  -m68000
  -Wall
  -Wextra
  -Wno-main
  -Wno-shift-negative-value
  -Wno-unused-parameter
  -fno-builtin
  -nostdlib
  -nodefaultlibs)

add_compile_options(
  "$<$<CONFIG:Release,>:-fuse-linker-plugin;-fno-web;-fno-gcse;-fomit-frame-pointer;-ffat-lto-objects;-flto>"
  "$<$<CONFIG:Debug>:-O0;-DDEBUG=1>"
  "$<$<COMPILE_LANGUAGE:ASM>:-xassembler-with-cpp;-Wa,-I${PROJECT_SOURCE_DIR};-Wa,--register-prefix-optional,--bitwise-or>"
)

# If CMAKE_BUILD_TYPE is not set
add_compile_options("$<$<CONFIG:>:-O3>")

# Gather sources
file(GLOB_RECURSE SRC_LIB_C src/*.c)
string(REPLACE "src/boot/rom_head.c" "" SRC_LIB_C "${SRC_LIB_C}")
file(GLOB_RECURSE SRC_LIB_S src/*.s)
string(REPLACE "src/boot/sega.s" "" SRC_LIB_S "${SRC_LIB_S}")
file(GLOB_RECURSE SRC_LIB_S80 src/*.s80)
string(REGEX REPLACE "[.]s80" ".s;" Z80SRC ${SRC_LIB_S80})
file(GLOB_RECURSE RES_LIB_RES res/*.res)
string(REGEX REPLACE "[.]res" ".s;" RESSRC ${RES_LIB_RES})

# Rules to build Z80 source code
foreach(z80_source ${Z80SRC})
  get_filename_component(file ${z80_source} NAME_WE)
  get_filename_component(dir ${z80_source} DIRECTORY)
  set(file ${dir}/${file})
  add_custom_command(
    OUTPUT ${z80_source}
    COMMAND ${ASMZ80} -i${PROJECT_SOURCE_DIR}/inc/snd ${file}.s80 ${file}.out &&
            ${BINTOS_EXE} ${file}.out ${file}.s
    DEPENDS ${file}.s80 BINTOS SJASM
    BYPRODUCTS ${file}.out ${file}.h)
endforeach()

# Rules to build resources
foreach(res_source ${RESSRC})
  string(REGEX REPLACE "[.]s" ".h" res_header ${res_source})
  string(REGEX REPLACE "[.]s" ".res" res ${res_source})
  add_custom_command(
    OUTPUT ${res_source} ${res_header}
    COMMAND ${RESCOMP} ${res}
    DEPENDS ${res} XGMTOOL)
endforeach()

add_library(md STATIC ${SRC_LIB_C} ${SRC_LIB_S} ${Z80SRC} ${RESSRC})
set_target_properties(
  md
  PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib
             OUTPUT_NAME "md"
             DEBUG_OUTPUT_NAME "md_debug")
