cmake_minimum_required(VERSION 3.23 FATAL_ERROR)
project(clox C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_library(clox_lib chunk.c common.h memory.c debug.c value.c vm.c vm.h compiler.c compiler.h scanner.c scanner.h object.c object.h)

add_executable(clox
        main.c common.h chunk.h chunk.c memory.h memory.c debug.c debug.h value.c value.h vm.c vm.h compiler.c compiler.h scanner.c scanner.h object.c object.h)

enable_testing()
add_subdirectory(test/ctest)
