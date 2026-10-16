# PhotoCarta Local Build Instructions

This guide describes how to configure, build, run, and test PhotoCarta on Windows, macOS, and Linux.

## Prerequisites

- **CMake**: version 3.22 or higher
- **C++ Compiler**:
  - Windows: MSVC 2022 (v143) or newer
  - Linux: GCC 11+ or Clang 13+
  - macOS: Apple Clang / Xcode 14+
- **Qt 6**: version 6.5 or higher (6.7 LTS or 6.8 LTS recommended)
  - Required modules: `Core`, `Gui`, `Widgets`, `Qml`, `Quick`, `QuickControls2`, `Concurrent`, `Test`
- **Build tool**: [Ninja](https://ninja-build.org/) recommended (or Make / MSVC MSBuild)

---

## 1. Environment Setup

### Windows
If using Microsoft Visual Studio 2022:
1. Open **x64 Native Tools Command Prompt for VS 2022** (or run `vcvars64.bat`).
2. Ensure Qt 6 is in your `CMAKE_PREFIX_PATH`:
   ```cmd
   set CMAKE_PREFIX_PATH=C:\Qt\6.7.3\msvc2022_64
   ```

### Linux (Debian / Ubuntu)
Install compiler and development headers:
```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build xvfb \
    qt6-base-dev qt6-declarative-dev qml6-module-qtquick-controls \
    libgl1-mesa-dev libxkbcommon-dev
```

### macOS (Homebrew)
Install dependencies:
```bash
brew install cmake ninja qt@6
export CMAKE_PREFIX_PATH="$(brew --prefix qt@6)"
```

---

## 2. Configuring and Building

### Using CMake Presets (Recommended)

PhotoCarta provides presets in [CMakePresets.json](../CMakePresets.json):

```bash
# Configure debug build
cmake --preset default

# Build all targets (application + tests)
cmake --build --preset default
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
- Windows: `build/src/photocarta.exe`
- Linux/macOS: `build/src/photocarta`

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
