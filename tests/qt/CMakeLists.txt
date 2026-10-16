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

# Automated startup smoke tests for main application executable
add_test(NAME app_startup_smoke_test
    COMMAND photocarta --smoke-test -platform offscreen
)

add_test(NAME app_screenshot_smoke_test
    COMMAND photocarta --screenshot "${CMAKE_CURRENT_BINARY_DIR}/smoke_screenshot.png" -platform offscreen
)
