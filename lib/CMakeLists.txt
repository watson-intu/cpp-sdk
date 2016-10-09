include_directories(".")

qi_create_lib(tinyxml_static STATIC
                tinyxml/tinyxml.cpp
                tinyxml/tinyxmlerror.cpp
                tinyxml/tinyxmlparser.cpp)
qi_stage_lib(tinyxml_static)

qi_create_lib(base64 STATIC
		base64/cdecode.c
		base64/cdecode.h
		base64/cencode.c
		base64/cencode.h
		base64/decode.h
		base64/encode.h)
qi_stage_lib(base64)

qi_create_lib(crossguid STATIC
		crossguid/crossguid.cpp)
qi_stage_lib(crossguid)

qi_create_lib(jsoncpp STATIC
		jsoncpp/jsoncpp.cpp
		jsoncpp/json/json-forwards.h
		jsoncpp/json/json.h)
qi_stage_lib(jsoncpp)

file(GLOB_RECURSE JSONCPP_H RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "jsoncpp/*.h")
qi_install_header(${JSONCPP_H} KEEP_RELATIVE_PATHS)

qi_create_lib(tinythread++ STATIC
		tinythread++/tinythread.h
		tinythread++/tinythread.cpp)
qi_stage_lib(tinythread++)

file(GLOB_RECURSE TINYTHREAD_H RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "tinythread++/*.h")
qi_install_header(${TINYTHREAD_H} KEEP_RELATIVE_PATHS)



