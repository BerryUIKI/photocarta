# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

`AGENTS.md` is the authoritative project instruction file (product scope, upstream
reference, GitFlow, validation rules). Read it first; this file does not repeat it.

## Build and test

Qt 6.5+ (6.8.3 in CI), CMake 3.22+, and Ninja. On Windows set `CMAKE_PREFIX_PATH` to the
Qt MSVC kit. The `build/` tree is not committed and may not exist locally.

```bash
cmake --preset default && cmake --build --preset default
```

Equivalent manual invocation (what CI runs):

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON && cmake --build build --config Release
```

```bash
ctest --test-dir build --output-on-failure
```

Single test: `ctest --test-dir build -R test_trash --output-on-failure`, or run the binary
directly for QTest's own filtering: `./build/tests/qt/test_trash testOpenAndCloseMemoryDatabase`.
Qt tests are registered with `-platform offscreen`; on headless Linux wrap ctest in `xvfb-run`.

```bash
python scripts/check_repository.py && python -m unittest discover -s tests -v && git diff --check
```

These Python checks are dependency-free and pass with the system `python`; they run on every
platform in CI. `check_repository.py` enforces final newlines, no trailing whitespace, valid
JSON, and no broken relative Markdown links, so a new `docs/` link must resolve.

Application CLI, useful for visual and startup verification without a build:

```bash
./build/src/photocarta --screenshot review.png --width 1400 --height 900
```

Other flags: `--theme light`, `--preview`, `--smoke-test`, `-d <catalog.db>`, positional image
files or `.db` paths. Screenshot and smoke-test modes exit on their own.

## Architecture

Layered by design: QML never touches SQL, filesystem, or image decoding. All persistence,
scanning, and file operations live in `photocarta_catalog` (a static lib from `src/catalog/`);
`src/main.cpp` and `src/qml/` form the presentation shell.

`main.cpp` constructs the object graph by hand — no DI container, no service locator — and
publishes each service as a root-context property (`catalogModel`, `filterModel`,
`collectionModel`, `selectionManager`, `themeManager`, `sessionController`, `settingsManager`).
QML refers to these names directly. Adding a top-level service means adding it in `main.cpp`.

### Model chain

`CatalogDatabase` → `CatalogModel` (QAbstractListModel, flat `QList<CatalogItem>`) →
`CatalogFilterModel` (search, tag/rating/color/category filters, sorting) →
`SelectionManager` (wraps the proxy, owns current/multi-selection and keyboard navigation).

Row indices crossing the QML boundary are **proxy** rows. Anything that reaches the database
must first go through `SessionController::sourceRowForProxy()`. Metadata writes are therefore
addressed by `proxyIndex` in QML and by `filePath` in `CatalogDatabase`.

### SessionController is the QML facade

It owns almost every user action: metadata edits (single and whole-selection), scanning,
collection switching, preview/compare/histogram state, export, batch rename, trash, XMP sync,
and external-editor handoff. It also owns its sub-services (`DirectoryScanner`, `ImageExporter`,
`HistogramCalculator`, `BatchRenamer`, `TrashService`, `ExternalToolService`). New user-facing
commands belong here rather than in `CatalogModel`. It carries ~50 `Q_PROPERTY` entries, so
read the header before adding state — most of it already exists.

### Persistence

`CatalogDatabase` is the single database owner and detects its schema on open:
`DatabaseSchemaType::PhotoCarta` (its own SQLite schema, WAL, foreign keys) or
`DatabaseSchemaType::Digikam` (upstream `digikam4.db`). For digiKam catalogs, reads route
through `DigikamDatabaseAdapter`, which maps `AlbumRoots`/`Albums`/`Images`/`ImageMetadata`/`Tags`
into `CollectionRecord` and `CatalogItem`, and only ratings and tags are written back — see
`docs/known-limitations.md` for the exact write boundary.

Catalog writes are batched per transaction (`updateItemsRating`, `addTagsToItems`,
`removeItems`) so a whole-selection edit is one commit and one `itemsChanged()` signal. Tag
lookups use an in-memory cache (`loadTagCache`); tag writes must keep it coherent.

Two settings layers exist: `SettingsManager` (QSettings, UI preferences and last catalog path)
and the catalog itself (photo metadata). UI-only state belongs to `SettingsManager`;
`main.cpp` wires bidirectional connections so both sides stay in sync.

### Thumbnails

`ThumbnailImageProvider` is registered as the `image` provider named `thumbnail`, so QML uses
`image://thumbnail/<url-encoded path>`. It is asynchronous (`QQuickAsyncImageProvider` +
`ThumbnailImageResponse`) and cancellable — keep decoding off the GUI thread. `ThumbnailCache`
is a process singleton with a `QCache` memory tier and a disk tier.

### UI

`Theme.qml` is a QML singleton proxying `ThemeManager`; components read design tokens
(`Theme.spacingMd`, `Theme.radiusLg`, `Theme.fontBase`, `Theme.accent`) rather than literals.
`main.cpp` forces the `Basic` Quick Controls style, so all control styling is explicit and must
honor both themes.

`main.qml` is a three-pane layout (collections / center canvas / metadata) plus modal dialogs.
The center canvas swaps `ThumbnailGridView`, `ImagePreviewView`, and `PhotoCompareView` on
`sessionController` mode flags. Custom-drawn controls need an explicit `FocusRing`, accessible
role/name/state, and Enter/Space activation — `test_accessibility` and `test_qml_keyboard`
cover this contract and will fail if a new control skips it. Fullscreen hides the toolbar,
status bar, and sidebars.

Keyboard/accessibility and 150% scaling are enforced by tests
(`app_screenshot_scaled_smoke_test` sets `QT_SCALE_FACTOR=1.5`), not just reviewed by eye.

## Conventions specific to this repo

- New source files need an SPDX header pair (`SPDX-FileCopyrightText: 2026 PhotoCarta contributors`,
  `SPDX-License-Identifier: GPL-3.0-or-later`) and must be added explicitly to the target in
  `src/catalog/CMakeLists.txt` or `tests/qt/CMakeLists.txt`; there is no globbing.
- Register a new test with `add_photocarta_test(<name>)` in `tests/qt/CMakeLists.txt` — the macro
  expects a matching `<name>.cpp` and passes `PHOTOCARTA_SOURCE_DIR` for fixture paths.
- Code lives in `namespace PhotoCarta`; behavior is exposed to QML via `Q_INVOKABLE` and
  `Q_PROPERTY`, never via string-based `QMetaObject::invokeMethod`.
- Destructive operations must refuse demo fixtures, offline collections, and missing files.
  `TrashService` and `ExternalToolService` both do this; follow that pattern.
- `.clang-format` applies to project-owned code only. Imported digiKam files keep upstream style.

## Before considering work done

Run ctest when a toolchain exists; otherwise state that the native build was not run rather than
implying it passed. CI (`.github/workflows/ci.yml`) builds natively, packages, and verifies the
packaged binary starts on all three platforms; reporting untested platform support as verified
is explicitly disallowed by `AGENTS.md`.
