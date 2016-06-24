add_definitions("-DBOOST_ASIO_DISABLE_STD_CHRONO -DBOOST_FILESYSTEM_VERSION=3")
add_directory(lib)

file(GLOB_RECURSE WDC_SERVICES_CPP RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/services "*.cpp")
file(GLOB_RECURSE WDC_TESTS_CPP RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/tests "*.cpp")
file(GLOB_RECURSE WDC_UTILS_CPP RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/utils "*.cpp")
qi_create_lib(wdc SHARED
	${WDC_SERVICES_CPP}
	${WDC_TESTS_CPP}
	${WDC_UTILS_CPP}
	)

if (CMAKE_HOST_APPLE)
	qi_use_lib(wdc dl jsoncpp tinythread++ crossguid base64 boost boost_filesystem boost_thread boost_system openssl)
else()
	qi_use_lib(wdc dl jsoncpp tinythread++ crossguid base64 boost boost_filesystem boost_thread boost_system openssl rt)
endif()
qi_stage_lib(wdc)


