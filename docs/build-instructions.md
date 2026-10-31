# PhotoCarta Local Build Instructions

This guide describes how to configure, build, run, and test PhotoCarta on Windows, macOS, and Linux.

## Prerequisites

- **CMake**: version 3.22 or higher
- **C++ Compiler**:
  - Windows: MSVC 2022 (v143) or newer
  - Linux: GCC 11+ or Clang 13+
  - macOS: Apple Clang / Xcode 14+
- **Qt 6**: version 6.5 or higher (Qt 6.8 LTS recommended)
  - Required components: `Core`, `Gui`, `Widgets`, `Qml`, `Quick`, `Concurrent`, `Test`
- **Build tool**: [Ninja](https://ninja-build.org/) recommended (or Make / MSVC MSBuild)

---

## 1. Environment Setup

### Windows
If using Microsoft Visual Studio 2022:
1. Open **x64 Native Tools Command Prompt for VS 2022** (or run `vcvars64.bat`).
2. Ensure Qt 6 is in your `CMAKE_PREFIX_PATH`:
   ```cmd
   set CMAKE_PREFIX_PATH=C:\Qt\6.8.3\msvc2022_64
   ```

### Linux (Debian / Ubuntu)
Install compiler, development headers, and offscreen test dependencies:
```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build xvfb \
    qt6-base-dev qt6-declarative-dev \
    libgl1-mesa-dev libxkbcommon-dev libxkbcommon-x11-0 \
    libegl1-mesa-dev libfontconfig1-dev libfreetype6-dev \
    libx11-xcb1 libxrender1 libxi6 libxcursor1 libvulkan-dev
```

### macOS (Homebrew)
Install dependencies:
```bash
brew install cmake ninja qt@6
export CMAKE_PREFIX_PATH="$(brew --prefix qt@6)"
```

---

## 2. Configuring and Building

### Windows: one-command build with MSVC and a system Qt

`scripts/build-msvc.bat` wraps the whole loop: it enters the MSVC environment,
puts Qt's `bin` on `PATH`, then configures, builds, and runs `ctest`.

```cmd
scripts\build-msvc.bat
```

Pass `configure`, `build`, or `test` to run a single step. The script expects the
machine-wide Qt SDK at `D:\Qt\6.8.3\msvc2022_64`; edit `VSROOT` or `QTDIR` at the
top for a different layout.

Qt is installed once per machine rather than per project, and is deliberately not
added to `PATH`. The checkout only carries a git-ignored `CMakeUserPresets.json`
whose `dev-local` preset points at that SDK:

```cmd
python -m aqt install-qt windows desktop 6.8.3 win64_msvc2022_64 -O D:/Qt
```

`CMakeUserPresets.json` is per-developer and not committed, so each machine can
point at its own Qt. CI installs the same 6.8.3 kit with
`jurplel/install-qt-action`, so a local build matches CI.

With the Ninja generator, Qt's runtime DLLs are **not** on `PATH` automatically
(the Visual Studio generator does that for you). Launching a test or the
application without `D:\Qt\6.8.3\msvc2022_64\bin` on `PATH` fails with
`0xC0000135` / "Qt6Core.dll was not found". The script handles this; a manual
`cmake` invocation does not.

### Using CMake Presets (Recommended)

PhotoCarta provides presets in [CMakePresets.json](../CMakePresets.json):

```bash
# Configure debug build
cmake --preset default

# Build all targets (application + tests)
cmake --build --preset default
```

On Windows, use the `dev-local` preset instead; it inherits `default` and adds the
Qt path:

```cmd
cmake --preset dev-local -G Ninja
cmake --build --preset dev-local
ctest --preset dev-local
```

### Manual Configuration

```bash
# Configure with Ninja generator
cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=ON

# Compile the GUI application and test suites
cmake --build build --config Release
```

The resulting executable is located at:
- Windows: `build/src/photocarta.exe` (`build/default/src/photocarta.exe` with presets)
- Linux/macOS: `build/src/photocarta`

### Windows: font rendering under the offscreen plugin

`-platform offscreen` on Windows does not get system fonts, so an offscreen
screenshot renders every glyph as a box. The layout and geometry are still valid
for review, but capture with the default plugin instead when checking typography:

```cmd
build\default\src\photocarta.exe --screenshot review.png --theme light
```

On headless Linux, `offscreen` and `xvfb-run` do render text and remain the
supported path.

---

## 3. Running the Application

### Normal Interactive Launch
```bash
./build/src/photocarta
```
This launches the Qt Quick desktop interface loaded with procedural SVG demonstration photo fixtures.

### Opening Local Photos Directly
```bash
./build/src/photocarta /path/to/photo1.jpg /path/to/photo2.png
```
Or use the **Open Photos...** toolbar button (`Ctrl+O`) to select files via the native file dialog.

### Running with Light Theme
```bash
./build/src/photocarta --theme light
```

### Launching Directly into Single-Image Preview
```bash
./build/src/photocarta --preview
```

---

## 4. Visual Review & Automated Screenshot Capture

To render the UI and capture a pixel-accurate PNG screenshot for visual review without interactive clicking:

```bash
./build/src/photocarta --screenshot screenshot_review.png
```

To capture in light theme:
```bash
./build/src/photocarta --theme light --screenshot screenshot_light.png
```

To capture single-photo preview mode:
```bash
./build/src/photocarta --preview --screenshot screenshot_preview.png
```

On headless Linux environments (e.g. CI or server), wrap with `xvfb-run` or use offscreen QPA:
```bash
QT_QPA_PLATFORM=offscreen ./build/src/photocarta --screenshot screenshot.png
```

---

## 5. Running the Test Suite

Run all behavioral Qt unit tests and automated startup smoke checks with CTest:

```bash
# Run all tests
ctest --test-dir build --output-on-failure

# Or on headless Linux
xvfb-run --auto-servernum ctest --test-dir build --output-on-failure
```

### Test Targets Included
1. `test_catalog_model`: Tests catalog item data roles, procedural fixtures, local file inspection, and formatted byte sizes.
2. `test_filter_model`: Tests live search queries (filename, camera, tag), category filters (All, Demo, Imported, Favorites), rating filters, and multi-attribute sorting.
3. `test_selection_manager`: Tests single selection, multi-selection toggles, boundary clamping, and arrow navigation.
4. `test_theme_and_session`: Tests dark/light theme switching, design token resolution, session pane toggles, and error notifications.
5. `app_startup_smoke_test`: Automated smoke test verifying that the `photocarta` executable starts, initializes the QML engine and presentation controllers, and exits cleanly.
6. `app_screenshot_smoke_test`: Automated capture test verifying window grabbing to PNG.

---

## 6. Repository Integrity Checks

Verify repository policy, file formatting, and GitFlow rules:

```bash
python scripts/check_repository.py
python -m unittest discover -s tests -v
```

---

## 7. Packaging Standalone Binaries

PhotoCarta provides automated cross-platform packaging for Windows, macOS, and Linux:

```bash
# Package portable distribution archives and compute SHA-256 checksums
python scripts/package.py --build-dir build --output-dir packages
```

Output packages by platform:
- **Windows**: `packages/PhotoCarta-<version>-windows-x86_64.zip` (self-contained portable bundle deployed with Qt runtime DLLs and QML plugins via `windeployqt`).
- **macOS**: `packages/PhotoCarta-<version>-macos-<arch>.dmg` (drag-and-drop installer disk image) and `.zip` bundle deployed via `macdeployqt`.
- **Linux**: `packages/PhotoCarta-<version>-linux-x86_64.tar.gz` (portable tarball that uses the system Qt 6 runtime).

Verify that a packaged artifact starts outside the build tree:

```bash
python scripts/verify_package.py --package-dir packages
```

The script extracts the platform archive into a temporary directory and runs the packaged
executable with `--smoke-test`, loading the QML engine and exiting cleanly. Headless offscreen
startup is tried first and the platform's default QPA plugin is used as a fallback. CI performs
the same check after packaging on all three platforms.

You can also use standard CPack from the build directory:
```bash
cpack --config build/CPackConfig.cmake -B packages
```
