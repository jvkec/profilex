cmake_minimum_required(VERSION 3.24)

project(profilex VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PROFILEX_BUILD_TESTS "Build ProfileX tests" ON)
option(PROFILEX_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)

add_library(profilex_lib
    src/cli/application.cpp
    src/cli/command_parser.cpp
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
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

target_compile_features(profilex_lib PUBLIC cxx_std_20)
target_compile_definitions(profilex_lib PUBLIC PROFILEX_VERSION="${PROJECT_VERSION}")

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

install(TARGETS profilex RUNTIME DESTINATION bin)
install(FILES README.md docs/release-install.md CHANGELOG.md DESTINATION share/doc/profilex)

add_custom_target(profilex_demo
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/demo.sh ${CMAKE_CURRENT_BINARY_DIR}/profilex
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS profilex
    COMMENT "Running the ProfileX demo workflow"
)

add_custom_target(profilex_stress
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/stress_test.sh ${CMAKE_CURRENT_BINARY_DIR}/profilex
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS profilex
    COMMENT "Running the ProfileX stress workflow"
)

add_custom_target(profilex_deep_stress
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/deep_stress_test.sh ${CMAKE_CURRENT_BINARY_DIR}/profilex
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS profilex
    COMMENT "Running the ProfileX deep stress workflow"
)

add_custom_target(profilex_corruption_demo
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/storage_corruption_demo.sh ${CMAKE_CURRENT_BINARY_DIR}/profilex
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS profilex
    COMMENT "Running the ProfileX storage corruption workflow"
)

add_custom_target(profilex_release_check
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/clean_release_validate.sh ${CMAKE_CURRENT_SOURCE_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS profilex
    COMMENT "Running the ProfileX release/install validation workflow"
)

include(CTest)
if(BUILD_TESTING AND PROFILEX_BUILD_TESTS)
    add_executable(profilex_stats_tests tests/stats_tests.cpp)
    target_link_libraries(profilex_stats_tests PRIVATE profilex_lib)
    add_test(NAME profilex_stats_tests COMMAND profilex_stats_tests)

    add_executable(profilex_storage_tests tests/storage_tests.cpp)
    target_link_libraries(profilex_storage_tests PRIVATE profilex_lib)
    add_test(NAME profilex_storage_tests COMMAND profilex_storage_tests)

    add_executable(profilex_cli_tests tests/cli_tests.cpp)
    target_link_libraries(profilex_cli_tests PRIVATE profilex_lib)
    add_test(NAME profilex_cli_tests COMMAND profilex_cli_tests)

    add_executable(profilex_runner_tests tests/runner_tests.cpp)
    target_link_libraries(profilex_runner_tests PRIVATE profilex_lib)
    add_test(NAME profilex_runner_tests COMMAND profilex_runner_tests)
endif()

set(CPACK_PACKAGE_NAME "ProfileX")
set(CPACK_PACKAGE_VENDOR "ProfileX")
set(CPACK_PACKAGE_CONTACT "profilex@example.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Local compare-first performance investigator for C++ workflows")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_FILE_NAME "profilex-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_GENERATOR "TGZ")
include(CPack)
