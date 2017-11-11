cmake_minimum_required(VERSION 3.2)

project (ibrdtn)

include_directories("${CMAKE_SOURCE_DIR}/ibrcommon")
include_directories("${CMAKE_SOURCE_DIR}/ibrdtn/ibrdtn")

add_subdirectory("ibrdtn")

if(Tests)
#  add_subdirectory("tests")
endif()
