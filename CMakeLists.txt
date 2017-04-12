cmake_minimum_required(VERSION 2.8.12.2)
project(self)
find_package(qibuild)
include_directories(lib src)

add_subdirectory(lib)
add_subdirectory(src)

file(GLOB_RECURSE TESTS_CPP RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "tests/*.cpp")
qi_create_bin(unit_test ${TESTS_CPP})
qi_use_lib(unit_test wdc)


