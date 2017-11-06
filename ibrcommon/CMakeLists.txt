cmake_minimum_required(VERSION 3.2)

project (ibrcommon)

include_directories("${CMAKE_SOURCE_DIR}/ibrcommon")
add_subdirectory("ibrcommon")

if(Tests)
  add_subdirectory("tests")
endif()
