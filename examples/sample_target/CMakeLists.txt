cmake_minimum_required(VERSION 3.24)

project(profilex_sample_target LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(sample_bench src/main.cpp)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(sample_bench PRIVATE -Wall -Wextra -Wpedantic)
elseif(MSVC)
    target_compile_options(sample_bench PRIVATE /W4)
endif()
