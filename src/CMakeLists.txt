include_directories(.)
add_definitions("-DBOOST_ASIO_DISABLE_STD_CHRONO -DBOOST_FILESYSTEM_VERSION=3")

file(GLOB_RECURSE UTILS_CPP RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.cpp")
qi_create_lib(utils SHARED ${UTILS_CPP} )

if (CMAKE_HOST_APPLE)
	qi_use_lib(utils dl zlib minizip jsoncpp tinythread++ crossguid base64 boost boost_filesystem boost_thread boost_system openssl tinyxml_static kafka)
else()
	qi_use_lib(utils dl zlib minizip jsoncpp tinythread++ crossguid base64 boost boost_filesystem boost_thread boost_system openssl rt tinyxml_static kafka)
endif()
qi_stage_lib(utils)

file(GLOB_RECURSE UTILS_H RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.h")
qi_install_header(${UTILS_H} KEEP_RELATIVE_PATHS)

