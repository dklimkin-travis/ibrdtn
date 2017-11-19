file(WRITE config.h "/* cmake is used. */")

set(ALL_TOOLS
  dtnconvert
  dtnping
  dtnrecv
  dtnsend
  dtnstream
  dtntracepath
  dtntrigger
)

set(TOOL_LIBRARIES ibrcommon ibrdtn)

if (LibArchive)
  list(APPEND ${ALL_TOOLS} dtninbox dtnoutbox)
    set(dtninbox_EXTRA io/FileHash.cpp io/ObservedFile.cpp io/TarUtils.cpp)
    set(dtnoutbox_EXTRA io/FileHash.cpp io/ObservedFile.cpp io/TarUtils.cpp)
  if (TFFS)
    list(APPEND dtninbox_EXTRA io/FATFile.cpp io/FatImageReader.cpp)
    list(APPEND dtnoutbox_EXTRA io/FATFile.cpp io/FatImageReader.cpp)
  endif()
endif()

if (Tunnel)
  list(APPEND ${ALL_TOOLS} dtntunnel)
endif()

set(dtnstream_EXTRA
  streaming/BundleStream.cpp
  streaming/BundleStreamBuf.cpp
  streaming/StreamBundle.cpp
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

foreach(TOOL ${ALL_TOOLS})
  add_executable(${TOOL} "${TOOL}.cpp" "${${TOOL}_EXTRA}")
  target_link_libraries(${TOOL} ${TOOL_LIBRARIES})
endforeach()
