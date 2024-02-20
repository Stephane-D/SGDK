cmake_minimum_required(VERSION 3.22)

project(xgmtool)

set(SRC
compress.c
gd3.c
lz77.c
psg.c
samplebank.c
util.c
vgm.c
vgmcom.c
xgc.c
xgccom.c
xgm.c
xgmcom.c
xgmsmp.c
xgmtool.c
ym2612.c
)

link_libraries(m)
add_compile_options(-m32)
add_link_options(-m32)
add_executable(${PROJECT_NAME} ${SRC})
