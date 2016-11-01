include_directories(.)
add_definitions("-DBOOST_ASIO_DISABLE_STD_CHRONO -DBOOST_FILESYSTEM_VERSION=3")

file(GLOB_RECURSE WDC_CPP RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.cpp")
qi_create_lib(wdc SHARED ${WDC_CPP} )

if (CMAKE_HOST_APPLE)
	qi_use_lib(wdc dl jsoncpp tinythread++ crossguid base64 boost boost_filesystem boost_thread boost_system openssl tinyxml_static)
else()
	qi_use_lib(wdc dl jsoncpp tinythread++ crossguid base64 boost boost_filesystem boost_thread boost_system openssl rt tinyxml_static)
endif()
qi_stage_lib(wdc)

file(GLOB_RECURSE WDC_H RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.h")
qi_install_header(${WDC_H} KEEP_RELATIVE_PATHS)

