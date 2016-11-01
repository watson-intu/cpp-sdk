cmake_minimum_required(VERSION 2.8.12.2)
project(wdc)
find_package(qibuild)
include_directories(lib src)

add_subdirectory(lib)
add_subdirectory(src)

file(GLOB_RECURSE WDC_TESTS_CPP RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "tests/*.cpp")
qi_create_bin(unit_test ${WDC_TESTS_CPP})
qi_use_lib(unit_test wdc)

#file(GLOB_RECURSE WDC_H RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/*.h")
#qi_install_header(${WDC_H} KEEP_RELATIVE_PATHS)

