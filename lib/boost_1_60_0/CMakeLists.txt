include_directories(".")

file(GLOB_RECURSE BOOST_HPP RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "boost/*.*")
qi_install_header(${BOOST_HPP} KEEP_RELATIVE_PATHS)

