cmake_minimum_required(VERSION 3.31)
project(synesthesia LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(TT_BUILD_RELEASE "Build with -O3 -DNDEBUG" OFF)
if(TT_BUILD_RELEASE)
  message(STATUS "Build with -03 -DNDEBUG")
  add_compile_options(-O3 -DNDEBUG)
endif()


add_subdirectory(external)
add_subdirectory(assets)
add_subdirectory(src)

