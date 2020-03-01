cmake_minimum_required(VERSION 3.9)

project (ibrcommon)

add_library(ibrcommon
  SHARED
    ibrcommon/appstreambuf.cpp
    ibrcommon/Logger.cpp
    ibrcommon/MonotonicClock.cpp
    ibrcommon/TimeMeasurement.cpp
    ibrcommon/data/Base64.cpp
    ibrcommon/data/Base64Reader.cpp
    ibrcommon/data/Base64Stream.cpp
    ibrcommon/data/BLOB.cpp
    ibrcommon/data/BloomFilter.cpp
    ibrcommon/data/ConfigFile.cpp
    ibrcommon/data/File.cpp
    ibrcommon/data/iobuffer.cpp
    ibrcommon/link/LinkEvent.cpp
    ibrcommon/link/LinkManager.cpp
    ibrcommon/link/LinkMonitor.cpp
    ibrcommon/link/PosixLinkManager.cpp
    ibrcommon/net/socket.cpp
    ibrcommon/net/socketstream.cpp
    ibrcommon/net/stopandwait.cpp
    ibrcommon/net/vaddress.cpp
    ibrcommon/net/vinterface.cpp
    ibrcommon/net/vsocket.cpp
    ibrcommon/thread/AtomicCounter.cpp
    ibrcommon/thread/Conditional.cpp
    ibrcommon/thread/Mutex.cpp
    ibrcommon/thread/MutexLock.cpp
    ibrcommon/thread/RWLock.cpp
    ibrcommon/thread/RWMutex.cpp
    ibrcommon/thread/Semaphore.cpp
    ibrcommon/thread/SignalHandler.cpp
    ibrcommon/thread/Thread.cpp
    ibrcommon/thread/Timer.cpp
)

target_include_directories(ibrcommon PUBLIC "${CMAKE_CURRENT_LIST_DIR}")

# We need pthreads.
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(ibrcommon Threads::Threads)

if (HAS_NETLINK)
  target_sources(ibrcommon PRIVATE ibrcommon/link/NetLinkManager.cpp)
  target_include_directories(ibrcommon PUBLIC "${PC_LIBNL_INCLUDE_DIRS}")
  target_link_libraries(ibrcommon ${PC_LIBNL_LIBRARIES})
endif()

if (Lowpan)
  message(STATUS "Building with lowpan")
  if(NOT HAS_NETLINK)
    message(SEND_ERROR "Netlink is required for lowpan support")
  endif()
  target_sources(ibrcommon
    PRIVATE
      ibrcommon/net/lowpansocket.cpp
      ibrcommon/net/lowpanstream.cpp
      ibrcommon/link/nl_policy.c
  )
endif()

if (Openssl)
  pkg_check_modules(OPENSSL REQUIRED libssl libcrypto)
  target_link_libraries(ibrcommon ${OPENSSL_LIBRARIES})
  target_sources(ibrcommon
    PRIVATE
      ibrcommon/ssl/AES128Stream.cpp
      ibrcommon/ssl/CipherStream.cpp
      ibrcommon/ssl/HashStream.cpp
      ibrcommon/ssl/HMacStream.cpp
      ibrcommon/ssl/iostreamBIO.cpp
      ibrcommon/ssl/MD5Stream.cpp
      ibrcommon/ssl/RSASHA256Stream.cpp
      ibrcommon/ssl/SHA256Stream.cpp
      ibrcommon/ssl/TLSStream.cpp
      ibrcommon/ssl/XORStream.cpp
  )
endif()

# TODO: implement and test HAVE_SYSLOG_H
#  list(APPEND ibrcommon_sources SyslogStream.cpp)

# TODO: find out where CLOCK_BOOTTIME can come from.
# TODO: find out where MCAST_JOIN_GROUP can come from.
# TODO: find out where MCAST_LEAVE_GROUP can come from.

# Generate dummy configs, not used with CMake.
file(WRITE ibrcommon/config.h "/* cmake is used. */")
file(WRITE ibrcommon/ibrcommon.h "/* cmake is used. */")

if(Tests)
  add_subdirectory("tests")
endif()

install(
  TARGETS ibrcommon
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  COMPONENT library
)
