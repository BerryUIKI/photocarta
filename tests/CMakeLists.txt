# SPDX-FileCopyrightText: 2026 PhotoCarta contributors
# SPDX-License-Identifier: GPL-3.0-or-later

if(BUILD_TESTING)
    add_subdirectory(qt)
endif()

# tests/benchmark is added from the top-level CMakeLists.txt, not here: this
# directory is skipped entirely when BUILD_TESTING is OFF, and a benchmark run
# uses exactly that configuration.
