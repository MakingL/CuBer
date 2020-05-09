cmake_minimum_required(VERSION 2.6)

project(CuBer)

set(CMAKE_CXX_STANDARD 11)

set(CXX_FLAGS
        -g
        # -DVALGRIND
        -DCHECK_PTHREAD_RETURN_VALUE
        -D_FILE_OFFSET_BITS=64
        -Wall
        -Wextra
        -Werror
        # -Wconversion
        -Wno-unused-parameter
        -Wold-style-cast
        -Woverloaded-virtual
        -Wpointer-arith
        # -Wshadow
        -Wwrite-strings
        # -march=native
        # -MMD
        -std=c++11
        -rdynamic
        )
if(CMAKE_BUILD_BITS EQUAL 32)
    list(APPEND CXX_FLAGS "-m32")
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    list(APPEND CXX_FLAGS "-Wno-null-dereference")
    list(APPEND CXX_FLAGS "-Wno-sign-conversion")
    list(APPEND CXX_FLAGS "-Wno-unused-local-typedef")
    list(APPEND CXX_FLAGS "-Wthread-safety")
    list(REMOVE_ITEM CXX_FLAGS "-rdynamic")
endif()
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

#include_directories(${Boost_INCLUDE_DIRS})

include_directories(${PROJECT_SOURCE_DIR}) # 头文件

set(INC_DIR include)
include_directories(${INC_DIR})

add_subdirectory(lib/yaml-cpp)
add_subdirectory(base)
add_subdirectory(net)
add_subdirectory(http)
add_subdirectory(config)

add_executable(CuBer main.cpp)

# 添加链接库
target_link_libraries(CuBer cuber_base)
target_link_libraries(CuBer cuber_net)
target_link_libraries(CuBer cuber_http)
target_link_libraries(CuBer cuber_config)
