set(ibrdtn_test_sources
    Main.cpp
    api/TestPlainSerializer.cpp
    data/TestBundleID.cpp
    data/TestBundleList.cpp
    data/TestBundleSet.cpp
    data/TestBundleString.cpp
    data/TestDictionary.cpp
    data/TestEID.cpp
    data/TestExtensionBlock.cpp
    data/TestSDNV.cpp
    data/TestSerializer.cpp
    data/TestTrackingBlock.cpp
    net/TestStreamConnection.cpp
    utils/TestUtils.cpp
)

if (Compression)
  list(APPEND ibrdtn_test_sources data/TestCompressedPayloadBlock.cpp)
  list(APPEND ibrdtn_test_sources ${ZLIB_LIBRARIES})
endif()

if (DtnSec)
  list(APPEND ibrdtn_test_sources
      security/PayloadConfidentialBlockTest.cpp
      security/PayloadIntegrityBlockTest.cpp
      security/TestSecurityBlock.cpp
  )
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(IBRDN_TEST_LIBRARIES ibrcommon ibrdtn ${CPPUNIT_LIBRARIES})

add_executable(ibrdtn_test ${ibrdtn_test_sources})
target_link_libraries(ibrdtn_test ${IBRDN_TEST_LIBRARIES})
