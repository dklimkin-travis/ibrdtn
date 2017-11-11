cmake_minimum_required(VERSION 3.2)

if(Lowpan)
  add_definitions(-D IBRCOMMON_SUPPORT_LOWPAN=1)
endif()

if(Openssl)
  add_definitions(-D IBRCOMMON_SUPPORT_SSL=1)
endif()

if(Xml)
  add_definitions(-D IBRCOMMON_SUPPORT_XML=1)
endif()

add_subdirectory("ibrdtn")
#add_subdirectory("daemon")
#add_subdirectory("tools")
