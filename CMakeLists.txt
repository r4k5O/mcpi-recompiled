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

find_package(Threads REQUIRED)

add_library(mcpi_core
    src/api/ApiDispatcher.cpp
    src/api/ApiServer.cpp
    src/api/Command.cpp
)

target_include_directories(mcpi_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(mcpi_core PUBLIC cxx_std_20)
target_link_libraries(mcpi_core PUBLIC Threads::Threads)

if(WIN32)
    target_link_libraries(mcpi_core PUBLIC ws2_32)
endif()

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
    find_program(MCPI_PYTHON_EXECUTABLE NAMES python python3 REQUIRED)

    add_executable(mcpi_protocol_tests
        tests/protocol_tests.cpp
    )
    target_link_libraries(mcpi_protocol_tests PRIVATE mcpi_core)

    add_executable(mcpi_api_server_tests
        tests/api_server_tests.cpp
    )
    target_link_libraries(mcpi_api_server_tests PRIVATE mcpi_core)

    add_executable(mcpi_api_dispatcher_tests
        tests/api_dispatcher_tests.cpp
    )
    target_link_libraries(mcpi_api_dispatcher_tests PRIVATE mcpi_core)

    add_executable(mcpi_world_backend_tests
        tests/world_backend_tests.cpp
    )
    target_link_libraries(mcpi_world_backend_tests PRIVATE mcpi_core)

    add_executable(mcpi_python_api_host
        tests/support/python_api_host.cpp
    )
    target_link_libraries(mcpi_python_api_host PRIVATE mcpi_core)

    add_test(NAME protocol COMMAND mcpi_protocol_tests)
    add_test(NAME api_server COMMAND mcpi_api_server_tests)
    add_test(NAME api_dispatcher COMMAND mcpi_api_dispatcher_tests)
    add_test(NAME world_backend COMMAND mcpi_world_backend_tests)
    add_test(
        NAME python_api_smoke
        COMMAND ${MCPI_PYTHON_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/python_api_smoke.py
                $<TARGET_FILE:mcpi_python_api_host>
    )
endif()
