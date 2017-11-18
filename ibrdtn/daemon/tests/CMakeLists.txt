set(dtnd_test_sources
    testsuite.cpp
)

set(dtnd_unittest_sources
    unittests/BaseRouterTest.cpp
    unittests/BundleSetTest.cpp
    unittests/BundleStorageTest.cpp
    unittests/ConfigurationTest.cpp
    unittests/DaemonTest.cpp
    unittests/DatagramClTest.cpp
    unittests/DataStorageTest.cpp
    unittests/FakeDatagramService.cpp
    unittests/Main.cpp
    unittests/NativeSerializerTest.cpp
    unittests/NodeTest.cpp
)

set(TEST_LIBRARIES ibrcommon ibrdtn dtnd_lib ${CPPUNIT_LIBRARIES})

add_executable(dtnd_test ${dtnd_test_sources})
target_link_libraries(dtnd_test ${TEST_LIBRARIES})

add_executable(dtnd_unittest ${dtnd_unittest_sources})
target_link_libraries(dtnd_unittest ${TEST_LIBRARIES})
