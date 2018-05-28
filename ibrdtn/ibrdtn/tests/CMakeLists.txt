add_executable(ibrdtn_test
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
  target_sources(ibrdtn_test PRIVATE data/TestCompressedPayloadBlock.cpp)
endif()

if (DtnSec)
  target_sources(ibrdtn_test
    PRIVATE
      security/PayloadConfidentialBlockTest.cpp
      security/PayloadIntegrityBlockTest.cpp
      security/TestSecurityBlock.cpp
  )
endif()

target_include_directories(ibrdtn_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ibrdtn_test ibrcommon ibrdtn ${CPPUNIT_LIBRARIES})

add_test(NAME ibrdtn.Test COMMAND ibrdtn_test)
