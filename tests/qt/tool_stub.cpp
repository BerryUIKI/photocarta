// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

// Minimal stand-in for an external editor. The external-tool tests launch this
// binary and need to observe the arguments it received, so it writes its first
// argument to the file named by PHOTOCARTA_TOOL_STUB_OUTPUT and exits.
//
// Deliberately dependency-free: the test process starts it detached, and it
// must run wherever the test executable runs.

#include <cstdlib>
#include <fstream>

int main(int argc, char *argv[])
{
    const char *outputPath = std::getenv("PHOTOCARTA_TOOL_STUB_OUTPUT");
    if (outputPath == nullptr)
    {
        return 0;
    }

    std::ofstream output(outputPath, std::ios::out | std::ios::trunc);
    if (!output)
    {
        return 2;
    }

    if (argc > 1)
    {
        output << argv[1];
    }
    return output.good() ? 0 : 3;
}
