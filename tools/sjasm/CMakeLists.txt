cmake_minimum_required(VERSION 3.22)

project(sjasm)

file(GLOB_RECURSE sources ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE headers ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

if (MINGW)
    add_compile_definitions(${PROJECT_NAME} PRIVATE MINGW)
else()
    add_compile_definitions(${PROJECT_NAME} PRIVATE MAX_PATH=MAXPATHLEN)
endif()

add_compile_options(-Wno-write-strings)
add_executable(${PROJECT_NAME} ${sources} ${headers})
