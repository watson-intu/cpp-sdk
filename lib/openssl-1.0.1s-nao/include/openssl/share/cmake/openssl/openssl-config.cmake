set(_root "${CMAKE_CURRENT_LIST_DIR}/../../..")
get_filename_component(_root ${_root} ABSOLUTE)

set(OPENSSL_LIBRARIES

  CACHE INTERNAL "" FORCE
)

set(OPENSSL_INCLUDE_DIRS
  ${_root}/include
  CACHE INTERNAL "" FORCE
)

# qi_persistent_set(OPENSSL_DEPENDS "")
export_lib(OPENSSL)
