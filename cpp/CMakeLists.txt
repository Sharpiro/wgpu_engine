cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wpedantic -Wextra")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(HelloWorld)

include_directories("wgpu/include")
# add_library(webgpuz SHARED IMPORTED GLOBAL)
# link_directories("wgpu/lib")
add_library(webgpu SHARED IMPORTED GLOBAL)
set_target_properties(webgpu PROPERTIES
    IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/wgpu/lib/wgpu_native.dll"
    IMPORTED_IMPLIB "${CMAKE_SOURCE_DIR}/wgpu/lib/wgpu_native.lib"
)

add_executable(temp_exe main.cpp)
# target_link_libraries(temp_exe PRIVATE wgpu_native)
