cmake_minimum_required(VERSION 3.9)

set(CMAKE_CXX_FLAGS_RELEASE "-O2 -pipe")
set(CMAKE_CXX_FLAGS_DEBUG  "-O0 -ggdb -pipe -Wall -Wextra -Wpedantic")

if (NOT CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "Build configuration: " ${CMAKE_BUILD_TYPE})

if (CMAKE_BUILD_TYPE EQUAL Debug)
  add_definitions(-D__DEVELOPMENT_ASSERTIONS__=1)
endif()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED QUIET)

option(Tests "Build with tests [default=OFF]" OFF)
if (Tests)
  pkg_check_modules(CPPUNIT REQUIRED cppunit)
  enable_testing()
endif()

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

execute_process(
    COMMAND sh -c "git rev-parse --short HEAD | tr -d '\\n'"
    OUTPUT_VARIABLE BUILD_NUMBER
)
message( STATUS "Release commit: " ${BUILD_NUMBER} )
add_definitions(-DBUILD_NUMBER=\"${BUILD_NUMBER}\")

set(CHANGELOG ${CMAKE_SOURCE_DIR}/ibrdtn/daemon/debian/changelog)
execute_process(
    COMMAND sh -c "cat ${CHANGELOG} | head -n 1 | sed 's/[a-z\\-]* (\\(.*\\)) .*$/\\1/' | tr -d '\\n'"
    OUTPUT_VARIABLE VERSION
)
message( STATUS "Package version: " ${VERSION} )
add_definitions(-DVERSION="${VERSION}")
add_definitions(-DPACKAGE_VERSION="${VERSION}")

include(CheckIncludeFileCXX)
CHECK_INCLUDE_FILE_CXX("regex.h" HAVE_REGEX_H)
if (HAVE_REGEX_H)
    add_definitions(-DHAVE_REGEX_H=1)
endif()

subdirs("ibrcommon")
subdirs("ibrdtn")
