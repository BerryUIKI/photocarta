# SPDX-FileCopyrightText: 2026 PhotoCarta contributors
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.22)

project(PhotoCarta
    VERSION 0.3.0
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
    Sql
    Test
)

# Standard testing enablement
include(CTest)

# Opt-in: the scale benchmark takes minutes and gigabytes on a real library.
# Declared before the tests subdirectory so it can be built with
# -DBUILD_TESTING=OFF, which is the configuration a benchmark run wants.
option(BUILD_BENCHMARK "Build the scale benchmark target" OFF)

add_subdirectory(src)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

# Not under tests/, because tests/ is skipped when BUILD_TESTING is OFF.
add_subdirectory(tests/benchmark)

# CPack packaging configuration
include(cmake/Packaging.cmake)
