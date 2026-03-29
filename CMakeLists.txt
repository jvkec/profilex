cmake_minimum_required(VERSION 3.24)

project(profilex VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PROFILEX_BUILD_TESTS "Build ProfileX tests" ON)
option(PROFILEX_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)

add_library(profilex_lib
    src/cli/application.cpp
    src/model/types.cpp
    src/report/printer.cpp
    src/runner/command_runner.cpp
    src/stats/stats_engine.cpp
    src/storage/json_codec.cpp
    src/storage/run_repository.cpp
)

target_include_directories(profilex_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(profilex_lib PUBLIC cxx_std_20)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(profilex_lib PRIVATE -Wall -Wextra -Wpedantic)
    if(PROFILEX_WARNINGS_AS_ERRORS)
        target_compile_options(profilex_lib PRIVATE -Werror)
    endif()
elseif(MSVC)
    target_compile_options(profilex_lib PRIVATE /W4)
    if(PROFILEX_WARNINGS_AS_ERRORS)
        target_compile_options(profilex_lib PRIVATE /WX)
    endif()
endif()

add_executable(profilex src/main.cpp)
target_link_libraries(profilex PRIVATE profilex_lib)

add_custom_target(profilex_demo
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/demo.sh ${CMAKE_CURRENT_BINARY_DIR}/profilex
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS profilex
    COMMENT "Running the ProfileX demo workflow"
)

include(CTest)
if(BUILD_TESTING AND PROFILEX_BUILD_TESTS)
    add_executable(profilex_stats_tests tests/stats_tests.cpp)
    target_link_libraries(profilex_stats_tests PRIVATE profilex_lib)
    add_test(NAME profilex_stats_tests COMMAND profilex_stats_tests)
endif()
