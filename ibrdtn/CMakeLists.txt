cmake_minimum_required(VERSION 3.2)

include_directories("${CMAKE_SOURCE_DIR}/ibrcommon")
include_directories("${CMAKE_SOURCE_DIR}/ibrdtn/ibrdtn")

if (Lowpan)
  add_definitions(-DIBRCOMMON_SUPPORT_LOWPAN=1)
  message(STATUS "6lowpan support enabled")
endif()

if (Openssl)
  add_definitions(-DHAVE_OPENSSL=1)
  add_definitions(-DIBRCOMMON_SUPPORT_SSL=1)
  message(STATUS "OpenSSL support enabled")
endif()

if (DtnSec)
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

if (EnableGrpc)
  set(protobuf_MODULE_COMPATIBLE TRUE)
  # Protobuf
  find_package(protobuf REQUIRED)
  INCLUDE_DIRECTORIES(${PROTOBUF_INCLUDE_DIRS})

  add_subdirectory("prototools")
endif()