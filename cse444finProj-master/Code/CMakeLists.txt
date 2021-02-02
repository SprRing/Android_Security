cmake_minimum_required(VERSION 3.14)
project(CSE444P C)

set(CMAKE_C_STANDARD 99)

add_executable(CSE444P main.c util/util.c util/util.h object/universal_node.c object/universal_node.h object/stack.c object/stack.h unit_tests/ut_stack.c unit_tests/ut_stack.h object/dirScanner.c object/dirScanner.h unit_tests/ut_dirScanner.c unit_tests/ut_dirScanner.h object/fileCus.c object/fileCus.h object/fnMeta.c object/fnMeta.h)