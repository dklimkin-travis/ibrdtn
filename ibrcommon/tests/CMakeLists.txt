set(ibrcommon_test_sources
    Main.cpp
    net/tcpclienttest.cpp
    net/tcpstreamtest.cpp
    thread/MutexTests.cpp
    thread/QueueTest.cpp
    thread/ThreadTest.cpp
    thread/TimerTest.cpp
)

if(Netlink)
  list(APPEND ibrcommon_test_sources link/netlinktest.cpp)
endif()

if(Openssl)
  list(APPEND ibrcommon_test_sources
      ssl/CipherStreamTest.cpp
      ssl/HashStreamTest.cpp
      ssl/RSASHA256StreamTest.cpp
  )
endif()


pkg_check_modules(CPPUNIT REQUIRED cppunit)

# build ibrcommon_test as a binary
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
link_directories(${CMAKE_SOURCE_DIR}/ibrcommon)
add_executable(ibrcommon_test ${ibrcommon_test_sources})
target_link_libraries(ibrcommon_test ibrcommon ${CPPUNIT_LIBRARIES})
