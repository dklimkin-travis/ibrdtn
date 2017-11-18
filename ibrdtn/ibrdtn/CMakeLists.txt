cmake_minimum_required(VERSION 3.2)

project (ibrdtn)

add_subdirectory("ibrdtn")

if(Tests)
  add_subdirectory("tests")
endif()
