# SPDX-FileCopyrightText: 2026 PhotoCarta contributors
# SPDX-License-Identifier: GPL-3.0-or-later

if(BUILD_TESTING)
    add_subdirectory(qt)
endif()

# Opt-in: the scale benchmark takes minutes and gigabytes on a real library.
option(BUILD_BENCHMARK "Build the scale benchmark target" OFF)
add_subdirectory(benchmark)
