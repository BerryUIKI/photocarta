# SPDX-FileCopyrightText: 2026 PhotoCarta contributors
# SPDX-License-Identifier: GPL-3.0-or-later

add_subdirectory(catalog)

add_executable(photocarta
    main.cpp
    resources/photocarta.qrc
    resources/qml.qrc
)

target_include_directories(photocarta PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(photocarta PRIVATE
    photocarta_catalog
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
)

target_compile_definitions(photocarta PRIVATE
    PHOTOCARTA_VERSION="${PROJECT_VERSION}"
)

set_target_properties(photocarta PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    WIN32_EXECUTABLE OFF  # Keep console visible during prototype bring-up / CLI commands
    MACOSX_BUNDLE OFF
)

install(TARGETS photocarta
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)
