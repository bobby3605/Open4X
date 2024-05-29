cmake_minimum_required(VERSION 3.29)
set(CMAKE_CXX_COMPILER "c++")
set(CMAKE_CXX_STANDARD 23)
set(CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O3")
set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PROJECT_NAME Open4X)
set(COMPILE_FLAGS "")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPILE_FLAGS}")

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
  src/Renderer/Allocator/*.hpp
  src/Renderer/Allocator/*.cpp
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

set(LIBRARY_FLAGS "-lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi -ltbb")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LIBRARY_FLAGS}")
