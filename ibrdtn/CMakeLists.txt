cmake_minimum_required(VERSION 3.9)

if (Lowpan)
  add_definitions(-DIBRCOMMON_SUPPORT_LOWPAN=1)
  message(STATUS "6lowpan support enabled")
endif()

option(DtnSec "Enable security extensions" ${Openssl})

if(DtnSec)
  if(NOT Openssl)
    message(SEND_ERROR "Bundle Security Protocol requires OpenSSL enabled")
  endif()
  add_definitions(-DIBRDTN_SUPPORT_BSP=1)
endif()

if (Xml)
  add_definitions(-DIBRCOMMON_SUPPORT_XML=1)
  message(STATUS "XML support enabled")
endif()

option(Compression "Enable compressed bundles" OFF)

if (Compression)
  find_package(ZLIB REQUIRED)
  message(STATUS "Bundle compression support enabled")
  add_definitions(-DIBRDTN_SUPPORT_COMPRESSION=1)
  add_definitions(-DHAVE_ZLIB=1)
endif()

# TODO: set up shared options to define HAVE_LIBDAEMON,
#       HAVE_DAEMON_RESET_SIGS for daemon and tools.

add_subdirectory("ibrdtn")
add_subdirectory("daemon")
add_subdirectory("tools")
