include_directories(.)
add_definitions("-DBOOST_ASIO_DISABLE_STD_CHRONO -DBOOST_FILESYSTEM_VERSION=3")

file(GLOB_RECURSE WDC_CPP RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.cpp")
qi_create_lib(wdc SHARED ${WDC_CPP} )

if (CMAKE_HOST_APPLE)
	qi_use_lib(wdc dl jsoncpp tinythread++ crossguid base64 boost boost_filesystem boost_thread boost_system openssl)
else()
	qi_use_lib(wdc dl jsoncpp tinythread++ crossguid base64 boost boost_filesystem boost_thread boost_system openssl rt)
endif()
qi_stage_lib(wdc)

