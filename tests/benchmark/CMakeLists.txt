# SPDX-FileCopyrightText: 2026 PhotoCarta contributors
# SPDX-License-Identifier: GPL-3.0-or-later

# Scale benchmark. Built only on request so the default build and the default
# ctest run stay fast: a 100,000-item catalog takes minutes and gigabytes.
#
#   cmake -B build -DBUILD_BENCHMARK=ON ...
#   ctest --test-dir build -R benchmark --output-on-failure
#
# The ctest entry is tagged "benchmark" and is not part of the default suite
# unless PHOTOCARTA_BENCHMARK_MEDIA is set to a photo directory.

if(NOT BUILD_BENCHMARK)
    return()
endif()

add_executable(photocarta_benchmark benchmark_main.cpp)

target_include_directories(photocarta_benchmark PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(photocarta_benchmark PRIVATE
    photocarta_catalog
    Qt6::Core
    Qt6::Gui
    Qt6::Sql
)

set_target_properties(photocarta_benchmark PROPERTIES
    AUTOMOC ON
)

if(WIN32)
    target_link_libraries(photocarta_benchmark PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:Psapi>
    )
endif()

# Provide a metadata header so the report can name the exact source revision.
find_package(Git QUIET)
set(PHOTOCARTA_COMMIT "unknown")
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE PHOTOCARTA_COMMIT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
endif()
target_compile_definitions(photocarta_benchmark PRIVATE
    PHOTOCARTA_COMMIT="${PHOTOCARTA_COMMIT}"
    PHOTOCARTA_VERSION="${PROJECT_VERSION}"
)

# Registered only when the operator points it at real media, so a normal
# `ctest` never runs an unmetered multi-minute job.
if(PHOTOCARTA_BENCHMARK_MEDIA)
    add_test(NAME benchmark_scale
        COMMAND photocarta_benchmark
                --media-dir "${PHOTOCARTA_BENCHMARK_MEDIA}"
                --work-dir "${CMAKE_CURRENT_BINARY_DIR}/benchmark-work"
                --label "ctest"
    )
    set_tests_properties(benchmark_scale PROPERTIES
        LABELS benchmark
        TIMEOUT 7200
    )
endif()
