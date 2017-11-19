set(PROTOTOOL_PROTOS dtnping.proto)

protobuf_generate_cpp(TOOL_PROTOS_SRCS TOOL_PROTOS_HDRS ${PROTOTOOL_PROTOS})

add_library(prototool_lib STATIC ${TOOL_PROTOS_SRCS})
target_link_libraries(prototool_lib protobuf::libprotobuf)
