cmake_minimum_required(VERSION 3.22)

project(xgmtool)

file(GLOB_RECURSE sources ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
file(GLOB_RECURSE headers ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

if (CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(STATUS "Building xgmtool for 32-bit")
    add_compile_options(-m32)
    add_link_options(-m32)
endif()

link_libraries(m)
add_executable(${PROJECT_NAME} ${sources} ${headers})
