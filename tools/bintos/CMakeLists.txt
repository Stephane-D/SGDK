cmake_minimum_required(VERSION 3.22)

project(bintos)

file(GLOB_RECURSE sources ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
add_executable(${PROJECT_NAME} ${sources})
