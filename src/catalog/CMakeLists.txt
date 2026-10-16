# SPDX-FileCopyrightText: 2026 PhotoCarta contributors
# SPDX-License-Identifier: GPL-3.0-or-later

add_library(photocarta_catalog STATIC
    catalogitem.h
    catalogitem.cpp
    catalogmodel.h
    catalogmodel.cpp
    catalogfiltermodel.h
    catalogfiltermodel.cpp
    selectionmanager.h
    selectionmanager.cpp
    thememanager.h
    thememanager.cpp
    sessioncontroller.h
    sessioncontroller.cpp
)

target_include_directories(photocarta_catalog PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(photocarta_catalog PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
)

set_target_properties(photocarta_catalog PROPERTIES
    AUTOMOC ON
    AUTORCC ON
)
