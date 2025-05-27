cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(XVM LANGUAGES CXX)

file(GLOB_RECURSE SOURCES src/*.cpp)

add_library(XVM STATIC ${SOURCES})

target_include_directories(XVM PUBLIC
  include
  include/XVM
)

project(XVM-STANDALONE LANGUAGES CXX)

file(GLOB_RECURSE STANDALONE_SOURCES standalone/*.cpp)

add_executable(XVM-STANDALONE ${SOURCES} ${STANDALONE_SOURCES})

target_include_directories(XVM-STANDALONE PUBLIC
  include
  include/XVM
)

