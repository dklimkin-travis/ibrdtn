cmake_minimum_required(VERSION 3.2)

project (dtnd)

include_directories("${CMAKE_SOURCE_DIR}/ibrdtn/daemon/src")

# TODO: set up CURL
# TODO: set up VMIME
# TODO: set up wifip2p
# TODO: set up sqlite
# TODO: set up HAVE_LIBDAEMON, HAVE_DAEMON_RESET_SIGS
# TODO: set up dtndht

option(EnableTls "Enable TLS support" ${Openssl})
if (EnableTls)
  add_definitions(-DWITH_TLS=1)
  message(STATUS "TLS support enabled")
endif()

option(EnableGrpc "Enable gRPC API" OFF)
if (EnableGrpc)
  add_definitions(-DPROTO_API_SERVER=1)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
  message(STATUS "gRPC API support enabled")
  message(STATUS "Note: gRPC module requires C++14 support.")
endif()

add_subdirectory("src")

if(Tests)
  add_subdirectory("tests")
endif()
