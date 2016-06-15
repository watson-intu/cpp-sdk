add_definitions(" -DAUDIOIMPL_IS_REMOTE -DNAO_ENABLED -DBOOST_ASIO_DISABLE_STD_CHRONO -DBOOST_FILESYSTEM_VERSION=3")

file(GLOB_RECURSE WDC_CPP RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.cpp")
qi_create_lib(wdc SHARED
	${WDC_CPP})

if (CMAKE_HOST_APPLE)
	message(STATUS "WDC: Running on Apple")
	qi_use_lib(wdc dl jsoncpp tinythread++ crossguid base64 boost boost_filesystem boost_thread boost_system openssl)
else (CMAKE_HOST_APPLE)
	message(STATUS "WDC: NOT running on Apple")
	qi_use_lib(wdc dl jsoncpp tinythread++ crossguid base64 boost boost_filesystem boost_thread boost_system openssl rt)
endif (CMAKE_HOST_APPLE)
qi_stage_lib(wdc)


