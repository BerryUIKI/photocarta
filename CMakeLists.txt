# SPDX-FileCopyrightText: 2026 PhotoCarta contributors
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.22)

project(PhotoCarta
    VERSION 0.1.0
    DESCRIPTION "Modern desktop photo management based on digiKam"
    HOMEPAGE_URL "https://github.com/BerryUIKI/photocarta"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable standard compiler warnings
if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Workaround for missing AGL framework in macOS SDK 15+ / Xcode 16 (QTBUG-137687)
if(APPLE AND NOT TARGET WrapOpenGL::WrapOpenGL)
    add_library(WrapOpenGL::WrapOpenGL INTERFACE IMPORTED)
    set_target_properties(WrapOpenGL::WrapOpenGL PROPERTIES
        INTERFACE_LINK_LIBRARIES "-framework OpenGL"
    )
endif()

# Find Qt 6 dependencies
find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Qml
    Quick
    QuickControls2
    Concurrent
    Test
)

# Standard testing enablement
include(CTest)

add_subdirectory(src)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
