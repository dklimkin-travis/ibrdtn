cmake_minimum_required(VERSION 3.2)

project(dtnprotosend)

set(CMAKE_CXX_STANDARD 11)

# include dirs with generated files.
include_directories("${CMAKE_BINARY_DIR}/ibrdtn/daemon/src")
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

add_subdirectory("protos")

set(PROTOTOOL_ALL dtnprotoping dtnprotorecv dtnprotosend)
set(PROTOTOOL_LIBS proto_lib prototool_lib gflags)

find_package(gflags REQUIRED)
message(STATUS "Using gflags ${gflags_VERSION}")
list(APPEND PROTOTOOL_LIBS ${gflags_LIBRARIES})

if (Compression)
  list(APPEND PROTOTOOL_LIBS ${ZLIB_LIBRARIES})
endif()

set(PROTOTOOL_BASE_SRC dtnprototool.cpp)
add_library(PROTOTOOL_BASE STATIC ${PROTOTOOL_BASE_SRC})
list(APPEND PROTOTOOL_LIBS ${PROTOTOOL_BASE})

foreach(PROTOTOOL ${PROTOTOOL_ALL})
  add_executable(${PROTOTOOL} "${PROTOTOOL}.cpp" ${PROTOTOOL_BASE_SRC})
  target_link_libraries(${PROTOTOOL} ${PROTOTOOL_LIBS})
endforeach()
