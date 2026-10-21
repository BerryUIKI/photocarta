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

# Target installation and bundle properties
if(WIN32)
    set(PHOTOCARTA_RUNTIME_INSTALL_DIR ".")
else()
    set(PHOTOCARTA_RUNTIME_INSTALL_DIR "bin")
endif()

if(APPLE)
    set_target_properties(photocarta PROPERTIES
        AUTOMOC ON
        AUTORCC ON
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_BUNDLE_NAME "PhotoCarta"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.photocarta.PhotoCarta"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        # Deployed bundles load Qt from Contents/Frameworks. Keep that load path
        # in the installed binary so @rpath references resolve without relying on
        # macdeployqt to rewrite them.
        INSTALL_RPATH "@executable_path/../Frameworks"
    )
else()
    set_target_properties(photocarta PROPERTIES
        AUTOMOC ON
        AUTORCC ON
        WIN32_EXECUTABLE OFF  # Keep console visible during prototype bring-up / CLI commands
        MACOSX_BUNDLE OFF
    )
endif()

install(TARGETS photocarta
    RUNTIME DESTINATION ${PHOTOCARTA_RUNTIME_INSTALL_DIR}
    BUNDLE DESTINATION .
)

install(FILES
    ${CMAKE_SOURCE_DIR}/LICENSE
    ${CMAKE_SOURCE_DIR}/README.md
    DESTINATION .
)
