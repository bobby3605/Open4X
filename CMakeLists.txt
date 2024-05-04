cmake_minimum_required(VERSION 3.10)

set(PROJECT_NAME Open4X)

project("${PROJECT_NAME}")

file(GLOB SOURCES
  src/*.hpp
  src/*.cpp
#  src/Vulkan/*.hpp
#  src/Vulkan/*.cpp
  src/glTF/*.hpp
  src/glTF/*.cpp
  src/Renderer/Vulkan/*.hpp
  src/Renderer/Vulkan/*.cpp
  )

add_executable("${PROJECT_NAME}" ${SOURCES})

set(ALLOW_EXTERNAL_SPIRV_TOOLS ON)
add_subdirectory(external/glslang/glslang/)
add_subdirectory(external/SPIRV-Cross/SPIRV-Cross/)
add_subdirectory(external/VulkanMemoryAllocator/VulkanMemoryAllocator/)
add_subdirectory(external/fastgltf/fastgltf/)

target_include_directories("${PROJECT_NAME}" PUBLIC
  external/rapidjson/rapidjson/include
  external/stb
  external/glslang/glslang/glslang
  external/glslang/glslang/SPIRV
  external/SPIRV-Cross/
  external/VulkanMemoryAllocator
  external/fastgltf
)

# glslang libraries
# TODO
# static build, don't use system libraries
# include static spirv-tools
target_link_libraries("${PROJECT_NAME}" PUBLIC glslang glslang-default-resource-limits SPIRV spirv-cross-cpp GPUOpen::VulkanMemoryAllocator fastgltf)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_CXX_FLAGS "-std=c++17 ")

set(COMPILE_FLAGS "-g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPILE_FLAGS}")

set(LIBRARY_FLAGS "-lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi -ltbb")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LIBRARY_FLAGS}")
