cmake_minimum_required(VERSION 3.2)

set(CMAKE_CXX_FLAGS_RELEASE "-O2 -pipe")
set(CMAKE_CXX_FLAGS_DEBUG  "-O0 -ggdb -pipe -Wall")

option(Debug "Build for debugging [default=OFF]" OFF)

if(Debug)
  add_definitions(-D__DEVELOPMENT_ASSERTIONS__=1)
  set(CMAKE_BUILD_TYPE Debug)
else()
  set(CMAKE_BUILD_TYPE Release)
endif()

option(Tests "Build with tests [default=OFF]" OFF)

find_package(PkgConfig REQUIRED QUIET)

message(STATUS "Build configuration: " ${CMAKE_BUILD_TYPE})

# Choose an implementation for netlink manager.
option(Netlink "Build with netlink library [default=ON]" ON)

if (Netlink)
  pkg_check_modules(PC_LIBNL libnl-3.0 libnl-route-3.0 libnl-genl-3.0)
  if(PC_LIBNL_FOUND)
    add_definitions(-DHAVE_LIBNL3=1)
    set(HAS_NETLINK 1)
  else()
    pkg_search_module(PC_LIBNL libnl-1)
    if(PC_LIBNL_FOUND)
      add_definitions(-DHAVE_LIBNL=1)
      set(HAS_NETLINK 1)
    endif()
  endif()
endif ()

option(Lowpan "Build with lowpan support [default=OFF]" OFF)

option(Openssl "Build with openssl support [default=OFF]" OFF)

subdirs("ibrcommon")
subdirs("ibrdtn")
