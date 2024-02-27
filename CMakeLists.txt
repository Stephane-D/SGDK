cmake_minimum_required(VERSION 3.22)

project(md C ASM)

if(DEFINED ENV{PREFIX})
	set(PREFIX "$ENV{PREFIX}")
endif()

include(ExternalProject)
ExternalProject_Add(BINTOS
  SOURCE_DIR        ${PROJECT_SOURCE_DIR}/tools/bintos/src
  BINARY_DIR        ${PROJECT_SOURCE_DIR}/tools/bintos/src/build
  INSTALL_COMMAND	install bintos ${PROJECT_SOURCE_DIR}/bin
  BUILD_ALWAYS		TRUE
)
ExternalProject_Add(XGMTOOL
  SOURCE_DIR        ${PROJECT_SOURCE_DIR}/tools/xgmtool/src
  BINARY_DIR        ${PROJECT_SOURCE_DIR}/tools/xgmtool/src/build
  INSTALL_COMMAND	install xgmtool ${PROJECT_SOURCE_DIR}/bin
  BUILD_ALWAYS		TRUE
)
ExternalProject_Add(SJASM
  CONFIGURE_COMMAND ""
  DOWNLOAD_COMMAND	""
  BUILD_COMMAND		make -C ${PROJECT_SOURCE_DIR}/tools/sjasm/Sjasm
  INSTALL_COMMAND	install ${PROJECT_SOURCE_DIR}/tools/sjasm/Sjasm/sjasm ${PROJECT_SOURCE_DIR}/bin
  BUILD_ALWAYS		TRUE
)

# Set the compiler/assembler name
set(CMAKE_C_COMPILER ${PREFIX}gcc)
set(CMAKE_ASM_COMPILER ${PREFIX}gcc)
set(CMAKE_AR ${PREFIX}ar)

include_directories(${PROJECT_SOURCE_DIR}/inc)
include_directories(${PROJECT_SOURCE_DIR}/res)
include_directories(${PROJECT_SOURCE_DIR}/src)

set(RESCOMP java -jar ${PROJECT_SOURCE_DIR}/bin/rescomp.jar)
set(ASMZ80 ${PROJECT_SOURCE_DIR}/bin/sjasm -q)
set(BINTOS ${PROJECT_SOURCE_DIR}/bin/bintos)

set(DEFAULT_FLAGS_LIB
	-DSGDK_GCC
	-m68000
	-Wall
	-Wextra
	-Wno-shift-negative-value
	-Wno-unused-parameter
	-fno-builtin
	-nostdlib -nodefaultlibs)
set(FLAGS_LIB
	${DEFAULT_FLAGS_LIB}
	-fuse-linker-plugin
	-fno-web
	-fno-gcse
	-fno-unit-at-a-time
	-fomit-frame-pointer
	-ffat-lto-objects
	-flto)

add_compile_options(${FLAGS_LIB})

# If CMAKE_BUILD_TYPE is not set
add_compile_options(
	"$<$<CONFIG:>:-O3>"
)

# -g causes problems for asm files so remove it
string(REPLACE "-g" "" CMAKE_ASM_FLAGS_DEBUG "${CMAKE_ASM_FLAGS_DEBUG}")

set(CMAKE_ASM_FLAGS "-x assembler-with-cpp -Wa,--register-prefix-optional,--bitwise-or")

# Z80 code converted to 68K .s files
set(Z80SRC
	${PROJECT_SOURCE_DIR}/src/snd/drv_null.s
	${PROJECT_SOURCE_DIR}/src/snd/drv_xgm.s
	${PROJECT_SOURCE_DIR}/src/snd/xgm2/drv_xgm2.s
	${PROJECT_SOURCE_DIR}/src/snd/pcm/drv_pcm.s
	${PROJECT_SOURCE_DIR}/src/snd/pcm/drv_pcm4.s
	${PROJECT_SOURCE_DIR}/src/snd/pcm/drv_dpcm2.s
)

set(SRC
	src/bmp.c
	src/dma.c
	src/joy.c
	src/map.c
	src/mapper.c
	src/maths.c
	src/maths3D.c
	src/memory.c
	src/object.c
	src/pal.c
	src/pool.c
	src/psg.c
	src/sprite_eng.c
	src/sprite_eng_legacy.c
	src/sram.c
	src/string.c
	src/sys.c
	src/tab_cnv.c
	src/tab_log10.c
	src/tab_log2.c
	src/tab_sin.c
	src/tab_sqrt.c
	src/timer.c
	src/tools.c
	src/types.c
	src/vdp.c
	src/vdp_bg.c
	src/vdp_spr.c
	src/vdp_tile.c
	src/vram.c
	src/ym2612.c
	src/z80_ctrl.c
	src/bmp_a.s
	src/dma_a.s
	src/kdebug.s
	src/maths3D_a.s
	src/memory_a.s
	src/sram_a.s
	src/sys_a.s
	src/task.s
	src/tools_a.s
	src/vdp_tile_a.s
	src/snd/smp_null.s
	src/snd/smp_null_dpcm.s
	src/snd/sound.c
	src/snd/xgm.c
	src/snd/xgm2.c
	src/snd/pcm/snd_pcm.c
	src/snd/pcm/snd_dpcm2.c
	src/snd/pcm/tab_vol.c
	src/snd/pcm/snd_pcm4.c
	# Resources
	res/libres.s
)

# Create custom commands for Z80 source code
foreach(z80_source ${Z80SRC})
    get_filename_component(file ${z80_source} NAME_WE)
    get_filename_component(dir ${z80_source} DIRECTORY)

    set(file_o "${file}.out")
    set(file_s80 "${file}.s80")
    set(file_s "${file}.s")
    set(file_h "${file}.h")

    add_custom_command( OUTPUT ${z80_source}
					COMMAND ${ASMZ80} -i${PROJECT_SOURCE_DIR}/inc/snd ${dir}/${file_s80} ${dir}/${file_o} && ${BINTOS} ${dir}/${file_o} ${dir}/${file_s}
					DEPENDS ${dir}/${file_s80}
					BYPRODUCTS ${dir}/${file_o} ${dir}/${file_h}
					)
endforeach()

# Build resources
add_custom_command(OUTPUT ${PROJECT_SOURCE_DIR}/res/libres.s ${PROJECT_SOURCE_DIR}/res/libres.h
					COMMAND ${RESCOMP} ${PROJECT_SOURCE_DIR}/res/libres.res
					DEPENDS ${PROJECT_SOURCE_DIR}/res/libres.res
					)

set(ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
add_library(md STATIC ${SRC} ${Z80SRC})
set_target_properties(md PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib OUTPUT_NAME "md" DEBUG_OUTPUT_NAME "md_debug")
