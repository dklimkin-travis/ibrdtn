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

add_subdirectory("src")

if(Tests)
  add_subdirectory("tests")
endif()
