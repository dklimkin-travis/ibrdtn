add_executable(ibrcommon_test
    Main.cpp
    net/tcpclienttest.cpp
    net/tcpstreamtest.cpp
    thread/MutexTests.cpp
    thread/QueueTest.cpp
    thread/ThreadTest.cpp
    thread/TimerTest.cpp
)
target_link_libraries(ibrcommon_test ibrcommon ${CPPUNIT_LIBRARIES})

pkg_check_modules(CPPUNIT REQUIRED cppunit)
target_include_directories(ibrcommon_test PRIVATE ${CMAKE_CURRENT_LIST_DIR})

if(Netlink)
  target_sources(ibrcommon_test PRIVATE link/netlinktest.cpp)
endif()

if(Openssl)
  target_sources(ibrcommon_test
    PRIVATE
      ssl/CipherStreamTest.cpp
      ssl/HashStreamTest.cpp
      ssl/RSASHA256StreamTest.cpp
  )
endif()

add_test(NAME ibrcommon.Test COMMAND ibrcommon_test)
