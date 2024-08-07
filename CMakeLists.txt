cmake_minimum_required(VERSION 3.10.1)

project(infra)

set(CMAKE_CXX_STANDARD 14)

option(SHARED_LIB "shared lib" ON)

#set(CMAKE_VERBOSE_MAKEFILE ON CACHE INTERNAL "" FORCE)

include(${PROJECT_SOURCE_DIR}/Toolchains.cmake)

if(WIN32)
    if(MSVC)
        #禁止编译警告：warning C4819：该文件包含不能在当前代码页(936)中表示的字符。请将该文件保存为 Unicode 格式以防止数据丢失
        add_compile_options(/wd4819)
        add_compile_options(/wd4200)  #禁止C4200警告
        add_compile_definitions(_WINSOCK_DEPRECATED_NO_WARNINGS)
        add_compile_definitions(_CRT_SECURE_NO_WARNINGS)  # 禁止C4996警告
    endif()
else()
    # 消除GCC 7.x 的一个 abi 编译 note
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-psabi")
endif()

remove_definitions(-DLOG_LIB_MODULE)
add_definitions(-DLOG_LIB_MODULE="infra")

file(GLOB_RECURSE INFRA_SOURCE_FILES src/*.cpp)
file(GLOB_RECURSE INFRA_SOURCE_HEADER_FILES src/*.h include/*.h)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

if (SHARED_LIB)
    add_library(infra SHARED ${INFRA_SOURCE_FILES} ${INFRA_SOURCE_HEADER_FILES})
else()
    add_library(infra STATIC ${INFRA_SOURCE_FILES} ${INFRA_SOURCE_HEADER_FILES})
endif()

SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib/${ToolPlatform})
