# SPDX-FileCopyrightText: 2026 PhotoCarta contributors
# SPDX-License-Identifier: GPL-3.0-or-later

add_library(photocarta_catalog STATIC
    catalogitem.h
    catalogitem.cpp
    catalogmodel.h
    catalogmodel.cpp
    catalogfiltermodel.h
    catalogfiltermodel.cpp
    collectionmodel.h
    collectionmodel.cpp
    selectionmanager.h
    selectionmanager.cpp
    thememanager.h
    thememanager.cpp
    sessioncontroller.h
    sessioncontroller.cpp
    database/catalogdatabase.h
    database/catalogdatabase.cpp
    database/digikamdatabaseadapter.h
    database/digikamdatabaseadapter.cpp
    ingestion/directoryscanner.h
    ingestion/directoryscanner.cpp
    thumbnails/thumbnailcache.h
    thumbnails/thumbnailcache.cpp
    thumbnails/thumbnailimageprovider.h
    thumbnails/thumbnailimageprovider.cpp
    metadata/exifdata.h
    metadata/exifparser.h
    metadata/exifparser.cpp
    export/imageexporter.h
    export/imageexporter.cpp
    settings/settingsmanager.h
    settings/settingsmanager.cpp
)

target_include_directories(photocarta_catalog PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(photocarta_catalog PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Sql
    Qt6::Concurrent
)

set_target_properties(photocarta_catalog PROPERTIES
    AUTOMOC ON
    AUTORCC ON
)
