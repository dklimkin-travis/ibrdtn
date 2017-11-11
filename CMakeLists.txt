cmake_minimum_required(VERSION 3.2)

SET(CMAKE_CXX_FLAGS_RELEASE "-O2 -pipe")
SET(CMAKE_CXX_FLAGS_DEBUG  "-O0 -ggdb -pipe")

option(Debug "Build for debugging [default=OFF]" OFF)

if(Debug)
  add_definitions(-D __DEVELOPMENT_ASSERTIONS__)
  SET(CMAKE_BUILD_TYPE Debug)
else()
  SET(CMAKE_BUILD_TYPE Release)
endif()

option(Tests "Build with tests [default=OFF]" OFF)

find_package(PkgConfig REQUIRED QUIET)

message(STATUS "Build configuration: " ${CMAKE_BUILD_TYPE})

subdirs("ibrcommon")
