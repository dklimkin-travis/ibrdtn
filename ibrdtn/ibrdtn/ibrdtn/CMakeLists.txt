file(WRITE config.h "/* cmake is used. */")

set(ibrdtn_sources
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

set(secext_sources
    security/BundleAuthenticationBlock.cpp
    security/ExtensionSecurityBlock.cpp
    security/MutableSerializer.cpp
    security/PayloadConfidentialBlock.cpp
    security/PayloadIntegrityBlock.cpp
    security/SecurityBlock.cpp
    security/SecurityKey.cpp
    security/StrictSerializer.cpp
)

set(IBRDTN_LIBS ibrcommon)

option(DtnSec "Enable security extensions" ${Openssl})

if(DtnSec)
  if(NOT Openssl)
    message(SEND_ERROR "Bundle Security Protocol requires OpenSSL enabled")
  endif()
  message(STATUS "Bundle Security Protocol support is enabled")
  list(APPEND ibrdtn_sources ${secext_sources})
else()
  message(STATUS "Warning: Bundle Security Protocol support is disabled")
endif()

option(Compression "Enable compressed bundles" OFF)

if(Compression)
  message(STATUS "Bundle compression support is enabled")
  list(APPEND ibrdtn_sources data/CompressedPayloadBlock.cpp)
endif()

add_library(ibrdtn SHARED ${ibrdtn_sources})
target_link_libraries(ibrdtn ${IBRDTN_LIBS})

install(TARGETS ibrdtn LIBRARY DESTINATION lib)
