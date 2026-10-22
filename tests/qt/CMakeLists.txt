# SPDX-FileCopyrightText: 2026 PhotoCarta contributors
# SPDX-License-Identifier: GPL-3.0-or-later

macro(add_photocarta_test test_name)
    add_executable(${test_name}
        ${test_name}.cpp
        ../../src/resources/photocarta.qrc
    )

    target_include_directories(${test_name} PRIVATE
        ${CMAKE_SOURCE_DIR}/src
    )

    target_compile_definitions(${test_name} PRIVATE
        PHOTOCARTA_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
    )

    target_link_libraries(${test_name} PRIVATE
        photocarta_catalog
        Qt6::Core
        Qt6::Gui
        Qt6::Test
    )

    set_target_properties(${test_name} PROPERTIES
        AUTOMOC ON
        AUTORCC ON
    )

    add_test(NAME ${test_name} COMMAND ${test_name} -platform offscreen)
endmacro()

add_photocarta_test(test_catalog_model)
add_photocarta_test(test_filter_model)
add_photocarta_test(test_selection_manager)
add_photocarta_test(test_theme_and_session)
add_photocarta_test(test_database)
add_photocarta_test(test_directory_scanner)
add_photocarta_test(test_thumbnail_cache)
add_photocarta_test(test_exif_parser)
add_photocarta_test(test_collection_model)
add_photocarta_test(test_image_exporter)
add_photocarta_test(test_settings_manager)
add_photocarta_test(test_compare_mode)
add_photocarta_test(test_histogram_calculator)
add_photocarta_test(test_metadata_editing)
add_photocarta_test(test_xmp_sidecar)
add_photocarta_test(test_batch_renamer)
add_photocarta_test(test_trash)
add_photocarta_test(test_accessibility)

# External editor handoff: the test launches this stub and inspects the
# arguments it receives, so both binaries must land in the same directory.
add_executable(photocarta_tool_stub tool_stub.cpp)
set_target_properties(photocarta_tool_stub PROPERTIES
    AUTOMOC OFF
    AUTORCC OFF
)

add_photocarta_test(test_external_tools)
add_dependencies(test_external_tools photocarta_tool_stub)

# Automated startup smoke tests for main application executable
add_test(NAME app_startup_smoke_test
    COMMAND photocarta --smoke-test -platform offscreen
)

add_test(NAME app_screenshot_smoke_test
    COMMAND photocarta --screenshot "${CMAKE_CURRENT_BINARY_DIR}/smoke_screenshot.png" -platform offscreen
)

# Render the same interface with a fractional display scale to catch layouts
# that break on 150% Windows/HiDPI scaling.
add_test(NAME app_screenshot_scaled_smoke_test
    COMMAND ${CMAKE_COMMAND} -E env QT_SCALE_FACTOR=1.5
            $<TARGET_FILE:photocarta>
            --screenshot "${CMAKE_CURRENT_BINARY_DIR}/smoke_screenshot_150pct.png"
            -platform offscreen
)

# Behavioral keyboard test: loads QML components into an offscreen window and
# exercises Tab focus, Space activation, Delete, and the focus ring. Unlike
# test_accessibility, this needs the Qt Quick runtime.
add_executable(test_qml_keyboard
    test_qml_keyboard.cpp
    ../../src/resources/qml.qrc
)

target_include_directories(test_qml_keyboard PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(test_qml_keyboard PRIVATE
    photocarta_catalog
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Test
)

set_target_properties(test_qml_keyboard PROPERTIES
    AUTOMOC ON
    AUTORCC ON
)

add_test(NAME test_qml_keyboard COMMAND test_qml_keyboard -platform offscreen)
