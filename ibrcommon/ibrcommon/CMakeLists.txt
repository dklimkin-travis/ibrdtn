set(ibrcommon_sources
    appstreambuf.cpp
    Logger.cpp
    MonotonicClock.cpp
    TimeMeasurement.cpp
    data/Base64.cpp
    data/Base64Reader.cpp
    data/Base64Stream.cpp
    data/BLOB.cpp
    data/BloomFilter.cpp
    data/ConfigFile.cpp
    data/File.cpp
    data/iobuffer.cpp
    link/LinkEvent.cpp
    link/LinkManager.cpp
    link/LinkMonitor.cpp
    link/PosixLinkManager.cpp
    net/socket.cpp
    net/socketstream.cpp
    net/stopandwait.cpp
    net/vaddress.cpp
    net/vinterface.cpp
    net/vsocket.cpp
    thread/AtomicCounter.cpp
    thread/Conditional.cpp
    thread/Mutex.cpp
    thread/MutexLock.cpp
    thread/RWLock.cpp
    thread/RWMutex.cpp
    thread/Semaphore.cpp
    thread/SignalHandler.cpp
    thread/Thread.cpp
    thread/Timer.cpp
)

set(sslsupport_sources
    ssl/AES128Stream.cpp
    ssl/CipherStream.cpp
    ssl/HashStream.cpp
    ssl/HMacStream.cpp
    ssl/iostreamBIO.cpp
    ssl/MD5Stream.cpp
    ssl/RSASHA256Stream.cpp
    ssl/SHA256Stream.cpp
    ssl/TLSStream.cpp
    ssl/XORStream.cpp
    ssl/gcm/gcm.cpp
    ssl/gcm/gcm_aes.c
    ssl/gcm/gf128mul.cpp
)

# We need pthreads.
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
set(IBRCOMMON_LIBS ${CMAKE_THREAD_LIBS_INIT})

# Choose an implementation for netlink manager.
option(Netlink "Build with netlink library [default=ON]" ON)

if (WIN32)
  list(APPEND ibrcommon_sources link/Win32LinkManager.cpp)
endif()

if (HAS_NETLINK)
  include_directories("${PC_LIBNL_INCLUDE_DIRS}")
  list(APPEND ibrcommon_sources link/NetLinkManager.cpp)
  list(APPEND IBRCOMMON_LIBS ${PC_LIBNL_LIBRARIES})
endif()

if (Lowpan)
  message(STATUS "Building with lowpan")
  if(NOT HAS_NETLINK)
    message(SEND_ERROR "Netlink is required for lowpan support")
  endif()
  list(APPEND ibrcommon_sources
      net/lowpansocket.cpp
      net/lowpanstream.cpp
      link/nl_policy.c
  )
endif()

if (Openssl)
  pkg_check_modules(OPENSSL REQUIRED libssl libcrypto)
  list(APPEND ibrcommon_sources ${sslsupport_sources})
  list(APPEND IBRCOMMON_LIBS ${OPENSSL_LIBRARIES})
endif()

# Enable/disable xml
option(Xml "Build with xml support [default=OFF]" OFF)

if (Xml)
  pkg_search_module(LIBXML2 REQUIRED libxml-2.0)
  message(STATUS "Building with libxml-" ${LIBXML2_VERSION})
  list(APPEND ibrcommon_sources xml/XMLStreamReader.cpp xml/XMLStreamWriter.cpp)
  include_directories("${LIBXML2_INCLUDE_DIRS}")
  list(APPEND IBRCOMMON_LIBS ${LIBXML2_LIBRARIES})
endif()

# TODO: implement and test HAVE_SYSLOG_H
#  list(APPEND ibrcommon_sources SyslogStream.cpp)

# TODO: find out where CLOCK_BOOTTIME can come from.
# TODO: find out where MCAST_JOIN_GROUP can come from.
# TODO: find out where MCAST_LEAVE_GROUP can come from.

# Generate dummy configs, not used with CMake.
file(WRITE config.h "/* cmake is used. */")
file(WRITE ibrcommon.h "/* cmake is used. */")

add_library(ibrcommon SHARED ${ibrcommon_sources})
target_link_libraries(ibrcommon ${IBRCOMMON_LIBS})

install(TARGETS ibrcommon LIBRARY DESTINATION lib)
