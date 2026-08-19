cmake_minimum_required(VERSION 3.20)

project(mcpi_recompiled
    VERSION 0.0.1
    DESCRIPTION "Source reconstruction and native port of Minecraft: Pi Edition 0.1.1"
    LANGUAGES CXX
)

option(MCPI_BUILD_TESTS "Build mcpi-recompiled tests" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(mcpi_core
    src/api/Command.cpp
)

target_include_directories(mcpi_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(mcpi_core PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(mcpi_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(mcpi_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(mcpi-recompiled
    src/main.cpp
)

target_link_libraries(mcpi-recompiled PRIVATE mcpi_core)

if(MCPI_BUILD_TESTS)
    enable_testing()

    add_executable(mcpi_protocol_tests
        tests/protocol_tests.cpp
    )
    target_link_libraries(mcpi_protocol_tests PRIVATE mcpi_core)

    add_test(NAME protocol COMMAND mcpi_protocol_tests)
endif()
