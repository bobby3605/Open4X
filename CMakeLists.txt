cmake_minimum_required(VERSION 3.29)
set(CMAKE_CXX_COMPILER "c++")
set(CMAKE_CXX_STANDARD 23)
set(CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O3")
set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PROJECT_NAME Open4X)
set(COMPILE_FLAGS "-march=native -mavx2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPILE_FLAGS}")

project("${PROJECT_NAME}")

file(GLOB SOURCES
  src/*.hpp
  src/*.cpp
  src/Renderer/Vulkan/*.hpp
  src/Renderer/Vulkan/*.cpp
  src/Renderer/Allocator/*.hpp
  src/Renderer/Allocator/*.cpp
  src/utils/*.hpp
  src/utils/*.cpp
  )

add_executable("${PROJECT_NAME}" ${SOURCES})

add_subdirectory(external/VulkanMemoryAllocator/VulkanMemoryAllocator/)
add_subdirectory(external/fastgltf/fastgltf/)

target_include_directories("${PROJECT_NAME}" PUBLIC
  external/rapidjson/rapidjson/include
  external/stb
  external/VulkanMemoryAllocator
  external/fastgltf
)

target_link_libraries("${PROJECT_NAME}" PUBLIC vulkan glfw glslang glslang-default-resource-limits SPIRV SPIRV-Tools-opt spirv-cross-glsl spirv-cross-core GPUOpen::VulkanMemoryAllocator fastgltf)
