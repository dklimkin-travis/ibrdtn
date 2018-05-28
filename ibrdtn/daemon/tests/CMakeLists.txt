set(TEST_LIBRARIES ibrcommon ibrdtn dtnd_lib ${CPPUNIT_LIBRARIES})

add_executable(dtnd_test testsuite.cpp)
target_link_libraries(dtnd_test ${TEST_LIBRARIES})
add_test(NAME dtnd_test.Testsuite COMMAND dtnd_test)

add_executable(dtnd_unittest
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
target_link_libraries(dtnd_unittest ${TEST_LIBRARIES})
add_test(NAME dtnd_test.Unittest COMMAND dtnd_unittest)
