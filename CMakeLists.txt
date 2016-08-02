cmake_minimum_required(VERSION 2.8.12.2)
project(wdc)
find_package(qibuild)
include_directories(lib wdc)

add_subdirectory(lib)
add_subdirectory(wdc)

qi_create_bin(unit_test wdc/tests/TestMain.cpp)
qi_use_lib(unit_test wdc)

file(GLOB_RECURSE WDC_H RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "wdc/*.h")
qi_install_header(${WDC_H} KEEP_RELATIVE_PATHS)

