set(protobuf_MODULE_COMPATIBLE TRUE)

# Protobuf
find_package(protobuf REQUIRED)
message(STATUS "Using protobuf ${protobuf_VERSION}")
INCLUDE_DIRECTORIES(${PROTOBUF_INCLUDE_DIRS})

# gRPC
find_package(gRPC REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")

# gRPC C++ plugin
#get_target_property(GRPC_CPP_PLUGIN_EXECUTABLE gRPC::grpc_cpp_plugin IMPORTED_LOCATION_RELEASE)
set(GRPC_CPP_PLUGIN_EXECUTABLE "/usr/local/bin/grpc_cpp_plugin")
message(STATUS "Using CPP plugin at: " ${GRPC_CPP_PLUGIN_EXECUTABLE})

# Proto file
get_filename_component(HW_PROTO "dtnservice.proto" ABSOLUTE)
get_filename_component(PROTO_PATH "${HW_PROTO}" PATH)

# Poor man's dependency. I'd like a better solution.
if (${HW_PROTO} IS_NEWER_THAN ${CMAKE_CURRENT_BINARY_DIR}/dtnservice.pb.cc)
  file(REMOVE
    ${CMAKE_CURRENT_BINARY_DIR}/dtnservice.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/dtnservice.pb.h
  )
endif()

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${HW_PROTO})

set(GRPC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/dtnservice.grpc.pb.cc)
set(GRPC_HDRS ${CMAKE_CURRENT_BINARY_DIR}/dtnservice.grpc.pb.hh)

add_custom_command(
  OUTPUT "${GRPC_SRCS}" "${GRPC_HDRS}"
  COMMAND protobuf::protoc
  ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}" -I "${PROTO_PATH}"
      --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN_EXECUTABLE}" "${HW_PROTO}"
  DEPENDS "${HW_PROTO}"
)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(proto_lib STATIC ${PROTO_SRCS} ${PROTO_HDRS} ${GRPC_SRCS} ${GRPC_HDRS})
target_link_libraries(proto_lib protobuf::libprotobuf gRPC::grpc++_unsecure)
