add_directory(lib)
add_directory(wdc)

qi_create_bin(unit_test wdc/tests/TestMain.cpp)
qi_use_lib(unit_test wdc)

