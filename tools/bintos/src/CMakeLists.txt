cmake_minimum_required(VERSION 3.22)

project(bintos)

set(SRC
	bintos.c
)

add_executable(${PROJECT_NAME} ${SRC})
