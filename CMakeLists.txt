cmake_minimum_required(VERSION 3.10)

project(Open4X)

file(GLOB SOURCES
  src/*.hpp
  src/*.cpp
  )

add_executable(Open4X ${SOURCES})

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(COMPILE_FLAGS "-O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPILE_FLAGS}")

set(LIBRARY_FLAGS "-lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LIBRARY_FLAGS}")
