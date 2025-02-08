cmake_minimum_required(VERSION 3.10)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wpedantic -Wextra")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wpedantic -Wextra")
if (UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --gcc-toolchain=/usr/local -g")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --gcc-toolchain=/usr/local -g")
endif()
set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(blocks)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(FetchContent)

FetchContent_Declare(
  magic_enum
  GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
  GIT_TAG        v0.9.7
)
FetchContent_MakeAvailable(magic_enum)

add_executable(
    block 
    main.cpp 
    glfw_wgpu.cpp
    extension.cpp 
    shape.cpp 
)
target_include_directories(block PRIVATE wgpu/include)
target_include_directories(block PRIVATE glfw/include)
target_include_directories(block PRIVATE ${magic_enum_SOURCE_DIR}/include)
target_link_directories(block PRIVATE wgpu/lib)
target_link_directories(block PRIVATE glfw/lib-vc2022)

message(STATUS "LOG: ${magic_enum_SOURCE_DIR}")

if (WIN32)
    add_definitions(-DWINDOWS)
    target_link_libraries(
        block 
        wgpu_native.lib 
        wgpu_native.dll 
        wgpu_native.dll.lib 
        Userenv
        Ws2_32
        ntdll
        opengl32
        d3dcompiler
        glfw3
    )
else()
    add_definitions(-DLINUX)
    target_link_libraries(
        block 
        glfw3
        wgpu_native
    )
endif()
