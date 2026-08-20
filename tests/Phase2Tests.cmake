if(NOT MCPI_BUILD_TESTS)
    return()
endif()

function(mcpi_add_phase2_cpp_test name source)
    if(EXISTS "${PROJECT_SOURCE_DIR}/${source}")
        add_executable("mcpi_${name}_tests" "${PROJECT_SOURCE_DIR}/${source}")
        target_link_libraries("mcpi_${name}_tests" PRIVATE mcpi_core)
        add_test(NAME "${name}" COMMAND "mcpi_${name}_tests")
    endif()
endfunction()

mcpi_add_phase2_cpp_test(camera tests/camera_tests.cpp)
mcpi_add_phase2_cpp_test(asset_source tests/asset_source_tests.cpp)
mcpi_add_phase2_cpp_test(chunk_mesh tests/chunk_mesh_tests.cpp)
mcpi_add_phase2_cpp_test(ui_audio tests/ui_audio_tests.cpp)
mcpi_add_phase2_cpp_test(phase2_parity_acceptance tests/phase2_parity_acceptance_tests.cpp)
mcpi_add_phase2_cpp_test(render_snapshot tests/render_snapshot.cpp)

if(MCPI_BUILD_CLIENT AND EXISTS "${PROJECT_SOURCE_DIR}/tests/assets_cli_smoke.py")
    add_test(
        NAME assets_cli
        COMMAND ${MCPI_PYTHON_EXECUTABLE}
                ${PROJECT_SOURCE_DIR}/tests/assets_cli_smoke.py
                $<TARGET_FILE:mcpi-recompiled>
    )
endif()

if(EXISTS "${PROJECT_SOURCE_DIR}/tests/platform_contract_tests.py")
    add_test(
        NAME platform_contract
        COMMAND ${MCPI_PYTHON_EXECUTABLE}
                ${PROJECT_SOURCE_DIR}/tests/platform_contract_tests.py
                ${PROJECT_SOURCE_DIR}
    )
endif()

if(EXISTS "${PROJECT_SOURCE_DIR}/tests/parity_report_tests.py")
    add_test(
        NAME parity_report
        COMMAND ${MCPI_PYTHON_EXECUTABLE}
                ${PROJECT_SOURCE_DIR}/tests/parity_report_tests.py
                ${PROJECT_SOURCE_DIR}/docs/parity-status.md
    )
endif()
