cmake_minimum_required(VERSION 3.20)

project(mcpi_recompiled
    VERSION 0.1.0
    DESCRIPTION "Source reconstruction and native port of Minecraft: Pi Edition 0.1.1"
    LANGUAGES CXX
)

option(MCPI_BUILD_TESTS "Build mcpi-recompiled tests" ON)
option(MCPI_BUILD_CLIENT "Build the native Phase 1 client" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

add_library(mcpi_core
    src/api/ApiDispatcher.cpp
    src/api/ApiServer.cpp
    src/api/Command.cpp
    src/game/Entity.cpp
    src/game/GameState.cpp
    src/game/Inventory.cpp
    src/game/Physics.cpp
    src/game/Player.cpp
    src/parity/ReferenceCase.cpp
    src/parity/ReferenceSuite.cpp
    src/storage/LegacyLevelStorage.cpp
    src/storage/Nbt.cpp
    src/storage/PiLevelStorage.cpp
    src/storage/StorageRouter.cpp
    src/world/BlockBehavior.cpp
    src/world/Chunk.cpp
    src/world/LightEngine.cpp
    src/world/RandomLevelSource.cpp
    src/world/World.cpp
    src/world/WorldGenerator.cpp
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

if(MCPI_BUILD_CLIENT)
    include(FetchContent)
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
    set(SDL_TESTS OFF CACHE BOOL "" FORCE)
    set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        sdl3
        URL https://github.com/libsdl-org/SDL/releases/download/release-3.4.14/SDL3-3.4.14.tar.gz
        URL_HASH SHA256=30d4aa2b3037718142b32dffd4e72f917ebb6cc5227150e7bb9c45efb2153aeb
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(sdl3)

    add_library(mcpi_client
        src/client/ClientApp.cpp
    )
    target_link_libraries(mcpi_client PUBLIC mcpi_core SDL3::SDL3-static)
    target_compile_definitions(mcpi_client PRIVATE SDL_MAIN_HANDLED)
    if(MSVC)
        target_compile_options(mcpi_client PRIVATE /W4 /permissive-)
    else()
        target_compile_options(mcpi_client PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

add_executable(mcpi-recompiled
    src/main.cpp
)

target_link_libraries(mcpi-recompiled PRIVATE mcpi_core)
if(MCPI_BUILD_CLIENT)
    target_link_libraries(mcpi-recompiled PRIVATE mcpi_client)
    target_compile_definitions(mcpi-recompiled PRIVATE MCPI_BUILD_CLIENT=1 SDL_MAIN_HANDLED)
endif()

add_executable(mcpi-parity
    src/parity/main.cpp
)
target_link_libraries(mcpi-parity PRIVATE mcpi_core)
if(MSVC)
    target_compile_options(mcpi-parity PRIVATE /W4 /permissive-)
else()
    target_compile_options(mcpi-parity PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(MCPI_BUILD_TESTS)
    enable_testing()
    find_program(MCPI_PYTHON_EXECUTABLE NAMES python python3 REQUIRED)
    find_package(Java REQUIRED COMPONENTS Development Runtime)

    add_executable(mcpi_protocol_tests tests/protocol_tests.cpp)
    target_link_libraries(mcpi_protocol_tests PRIVATE mcpi_core)

    add_executable(mcpi_api_server_tests tests/api_server_tests.cpp)
    target_link_libraries(mcpi_api_server_tests PRIVATE mcpi_core)

    add_executable(mcpi_api_dispatcher_tests tests/api_dispatcher_tests.cpp)
    target_link_libraries(mcpi_api_dispatcher_tests PRIVATE mcpi_core)

    add_executable(mcpi_world_backend_tests tests/world_backend_tests.cpp)
    target_link_libraries(mcpi_world_backend_tests PRIVATE mcpi_core)

    add_executable(mcpi_chunk_storage_tests tests/chunk_storage_tests.cpp)
    target_link_libraries(mcpi_chunk_storage_tests PRIVATE mcpi_core)

    add_executable(mcpi_world_lifecycle_tests tests/world_lifecycle_tests.cpp)
    target_link_libraries(mcpi_world_lifecycle_tests PRIVATE mcpi_core)

    add_executable(mcpi_phase1_acceptance_tests tests/phase1_acceptance_tests.cpp)
    target_link_libraries(mcpi_phase1_acceptance_tests PRIVATE mcpi_core)

    add_executable(mcpi_parity_framework_tests tests/parity_framework_tests.cpp)
    target_link_libraries(mcpi_parity_framework_tests PRIVATE mcpi_core)

    add_executable(mcpi_parity_reference_tests tests/parity_reference_tests.cpp)
    target_link_libraries(mcpi_parity_reference_tests PRIVATE mcpi_core)

    add_executable(mcpi_worldgen_parity_tests tests/worldgen_parity_tests.cpp)
    target_link_libraries(mcpi_worldgen_parity_tests PRIVATE mcpi_core)

    add_executable(mcpi_lighting_parity_tests tests/lighting_parity_tests.cpp)
    target_link_libraries(mcpi_lighting_parity_tests PRIVATE mcpi_core)

    add_executable(mcpi_block_behavior_tests tests/block_behavior_tests.cpp)
    target_link_libraries(mcpi_block_behavior_tests PRIVATE mcpi_core)

    add_executable(mcpi_storage_parity_tests tests/storage_parity_tests.cpp)
    target_link_libraries(mcpi_storage_parity_tests PRIVATE mcpi_core)

    add_executable(mcpi_inventory_tests tests/inventory_tests.cpp)
    target_link_libraries(mcpi_inventory_tests PRIVATE mcpi_core)

    add_executable(mcpi_player_physics_tests tests/player_physics_tests.cpp)
    target_link_libraries(mcpi_player_physics_tests PRIVATE mcpi_core)

    add_executable(mcpi_game_state_player_tests tests/game_state_player_tests.cpp)
    target_link_libraries(mcpi_game_state_player_tests PRIVATE mcpi_core)

    add_executable(mcpi_entity_system_tests tests/entity_system_tests.cpp)
    target_link_libraries(mcpi_entity_system_tests PRIVATE mcpi_core)

    add_executable(mcpi_python_api_host tests/support/python_api_host.cpp)
    target_link_libraries(mcpi_python_api_host PRIVATE mcpi_core)

    file(GLOB_RECURSE MCPI_JAVA_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/clients/java/src/*.java
    )
    set(MCPI_JAVA_CLASSES_DIR ${CMAKE_CURRENT_BINARY_DIR}/java-classes)
    add_custom_target(mcpi_java_client ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory ${MCPI_JAVA_CLASSES_DIR}
        COMMAND ${Java_JAVAC_EXECUTABLE}
                -encoding UTF-8
                -d ${MCPI_JAVA_CLASSES_DIR}
                ${MCPI_JAVA_SOURCES}
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/java/JavaApiSmoke.java
        SOURCES ${MCPI_JAVA_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/tests/java/JavaApiSmoke.java
        COMMENT "Compiling Minecraft Pi Java compatibility client"
        VERBATIM
    )

    add_test(NAME protocol COMMAND mcpi_protocol_tests)
    add_test(NAME api_server COMMAND mcpi_api_server_tests)
    add_test(NAME api_dispatcher COMMAND mcpi_api_dispatcher_tests)
    add_test(NAME world_backend COMMAND mcpi_world_backend_tests)
    add_test(NAME chunk_storage COMMAND mcpi_chunk_storage_tests)
    add_test(NAME world_lifecycle COMMAND mcpi_world_lifecycle_tests)
    add_test(NAME phase1_acceptance COMMAND mcpi_phase1_acceptance_tests)
    add_test(NAME parity_framework COMMAND mcpi_parity_framework_tests)
    add_test(
        NAME parity_reference
        COMMAND mcpi_parity_reference_tests
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/reference/api-basic.ref
    )
    add_test(
        NAME parity_cli
        COMMAND ${MCPI_PYTHON_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity_cli_tests.py
                ${CMAKE_CURRENT_BINARY_DIR}
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/reference/api-basic.ref
    )
    add_test(NAME worldgen_parity COMMAND mcpi_worldgen_parity_tests)
    add_test(NAME lighting_parity COMMAND mcpi_lighting_parity_tests)
    add_test(NAME block_behavior COMMAND mcpi_block_behavior_tests)
    add_test(NAME storage_parity COMMAND mcpi_storage_parity_tests)
    add_test(NAME inventory COMMAND mcpi_inventory_tests)
    add_test(NAME player_physics COMMAND mcpi_player_physics_tests)
    add_test(NAME game_state_player COMMAND mcpi_game_state_player_tests)
    add_test(NAME entity_system COMMAND mcpi_entity_system_tests)
    add_test(
        NAME python_api_smoke
        COMMAND ${MCPI_PYTHON_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/python_api_smoke.py
                $<TARGET_FILE:mcpi_python_api_host>
    )
    add_test(
        NAME main_executable_smoke
        COMMAND ${MCPI_PYTHON_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/main_executable_smoke.py
                $<TARGET_FILE:mcpi-recompiled>
    )
    add_test(
        NAME java_api_smoke
        COMMAND ${Java_JAVA_EXECUTABLE}
                -cp ${MCPI_JAVA_CLASSES_DIR}
                JavaApiSmoke
                $<TARGET_FILE:mcpi-recompiled>
    )
    add_test(
        NAME release_workflow
        COMMAND ${MCPI_PYTHON_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/release_workflow_tests.py
    )
    add_test(
        NAME reverse_engineering_docs
        COMMAND ${MCPI_PYTHON_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/reverse_engineering_docs_tests.py
    )
endif()