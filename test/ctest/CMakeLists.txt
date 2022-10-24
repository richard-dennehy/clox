cmake_minimum_required(VERSION 3.23 FATAL_ERROR)

add_executable(ctest_write_chunk test_write_chunk.c)

target_link_libraries(ctest_write_chunk PRIVATE clox_lib)

add_test(ctest_write_chunk ctest_write_chunk)