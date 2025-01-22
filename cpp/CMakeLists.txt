cmake_minimum_required(VERSION 3.10)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wpedantic -Wextra")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wpedantic -Wextra")
set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(blocks)

include_directories("wgpu/include")
include_directories("magic_enum")
link_directories("wgpu/lib")
add_executable(temp_exe main.cpp)

if (WIN32)
    target_link_libraries(
        temp_exe 
        wgpu_native.lib 
        wgpu_native.dll 
        wgpu_native.dll.lib 
        Userenv
        Ws2_32
        ntdll
        opengl32
        d3dcompiler
    )
else()
    target_link_libraries(
        temp_exe 
    )
endif()
