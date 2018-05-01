file(WRITE config.h "/* cmake is used. */")
file(WRITE ibrdtn.h "/* cmake is used. */")

add_library(ibrdtn SHARED
    api/Client.cpp
    api/PlainSerializer.cpp
    data/AdministrativeBlock.cpp
    data/AgeBlock.cpp
    data/Block.cpp
    data/BundleBuilder.cpp
    data/Bundle.cpp
    data/BundleFragment.cpp
    data/BundleID.cpp
    data/BundleList.cpp
    data/BundleMerger.cpp
    data/BundleSet.cpp
    data/BundleSetImpl.cpp
    data/BundleString.cpp
    data/CustodySignalBlock.cpp
    data/Dictionary.cpp
    data/DTNTime.cpp
    data/EID.cpp
    data/Endianess.cpp
    data/ExtensionBlock.cpp
    data/MemoryBundleSet.cpp
    data/MetaBundle.cpp
    data/PayloadBlock.cpp
    data/PrimaryBlock.cpp
    data/SchedulingBlock.cpp
    data/ScopeControlHopLimitBlock.cpp
    data/SDNV.cpp
    data/Serializer.cpp
    data/StatusReportBlock.cpp
    data/StreamBlock.cpp
    data/TrackingBlock.cpp
    streams/StreamBuffer.cpp
    streams/StreamConnection.cpp
    streams/StreamContactHeader.cpp
    streams/StreamDataSegment.cpp
    utils/Clock.cpp
    utils/Random.cpp
    utils/Utils.cpp
)

if(DtnSec)
  message(STATUS "Bundle Security Protocol support enabled")
  target_sources(ibrdtn
    PRIVATE
      security/BundleAuthenticationBlock.cpp
      security/ExtensionSecurityBlock.cpp
      security/MutableSerializer.cpp
      security/PayloadConfidentialBlock.cpp
      security/PayloadIntegrityBlock.cpp
      security/SecurityBlock.cpp
      security/SecurityKey.cpp
      security/StrictSerializer.cpp
  )
else()
  message(STATUS "Warning: Bundle Security Protocol support disabled")
endif()

if(Compression)
  target_sources(ibrdtn PRIVATE data/CompressedPayloadBlock.cpp)
  target_link_libraries(ibrdtn ${ZLIB_LIBRARIES})
endif()

target_link_libraries(ibrdtn ibrcommon)
target_include_directories(ibrdtn PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

install(TARGETS ibrdtn LIBRARY DESTINATION lib)
