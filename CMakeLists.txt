cmake_minimum_required( VERSION 2.8 )
project( tarlib )

set( tarlib_VERSION_MAJOR 0 )
set( tarlib_VERSION_MINOR 1 )

add_library( tarlib src/tarlib.cpp )
include_directories( ${tarlib_SOURCE_DIR}/include )
list( APPEND CMAKE_CXX_FLAGS -std=c++11 )

