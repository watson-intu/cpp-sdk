add_subdirectory(lib)
add_subdirectory(wdc)

qi_create_bin(unit_test wdc/tests/TestMain.cpp)
qi_use_lib(unit_test wdc)

