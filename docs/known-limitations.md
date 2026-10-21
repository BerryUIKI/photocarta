# Known Limitations

This document records the current compatibility, scale, accessibility, and
packaging limits of PhotoCarta together with the license notices and runtime
dependencies that apply to a distributed build. It supports the release check for
documented limits, notices, and packaging dependencies in
[Initial release scope](initial-release.md).

## Interface scope

- The initial release converts the main photo management window. Retained digiKam
  tool windows (for example the editor, batch queue manager, and camera import)
  are not integrated into the Qt Quick shell yet.
- The application is a catalog browser and organizer: it does not develop RAW
  files, edit pixels, or replace the digiKam editor.
- Ratings, favorites, color labels, titles, captions, and tags are cataloged
  metadata. PhotoCarta does not write embedded XMP/IPTC into the image files;
  metadata interchange uses companion `.xmp` sidecars.

## Catalog compatibility

- PhotoCarta creates its own SQLite schema (WAL journal, foreign keys enabled)
  for scanned folders.
- Existing digiKam `digikam4.db` catalogs can be opened and browsed. Editing
  support through the digiKam adapter is limited to ratings and tags; titles,
  captions, color labels, and favorite flags are only persisted in PhotoCarta's
  own catalog. Catalogs from untested digiKam schema versions may not open.
- Catalog writes are validated against copied catalogs only. Back up a digiKam
  catalog before opening it with PhotoCarta.
- Importing a digiKam catalog copies catalog records; it does not move or modify
  the referenced original files.

## Destructive operations

- Move to Trash uses `QFile::moveToTrash`. Files on network shares or some
  removable media may not have a system trash. Those files are left in place and
  reported; PhotoCarta never falls back to permanent deletion.
- Items from offline collections, demo fixtures, and missing files are refused,
  so absent media is never treated as deleted.
- Catalog deletion and file trashing are not one atomic transaction. If the
  process stops between the two steps, a file may remain in the trash while its
  catalog record is gone, or a record may remain after the file was moved.
- There is no undo history inside PhotoCarta. Restoring trashed files is done
  through the system trash.

## Scale and performance

- One million cataloged items is an architecture and validation target. No
  million-item benchmark has been run, and no capacity claim follows from the
  current implementation.
- The interface uses a virtualized grid and paged catalog queries, but thumbnail
  decoding and scanning are bounded by local disk and CPU behavior rather than by
  a measured budget.

## Display scaling and keyboard access

- High-DPI scaling uses Qt's `PassThrough` rounding policy, so fractional scale
  factors (125%, 150%, ...) are preserved.
- Rendering is verified with offscreen screenshots at 100% and 150%. Other
  fractional factors, mixed-DPI multi-monitor setups, and per-monitor changes at
  runtime are not verified.
- Offscreen screenshot capture runs without system fonts in CI, so text renders
  as placeholder boxes there. Layout and focus indicators are still reviewed from
  those captures, but typography must be checked on a real desktop session.
- Custom-drawn controls expose accessible roles, names, states, and keyboard
  activation, and dialogs take focus on open, respond to Escape, and return focus
  to the photo grid. This has not yet been validated with NVDA, VoiceOver, or
  Orca, so screen reader behavior is not a tested claim.
- Tab order follows QML item order: toolbar, collection pane, photo grid, details
  pane, and status bar.

## Packaging and runtime dependencies

- CMake requires Qt 6.5 or newer with the Core, Gui, Widgets, Qml, Quick,
  QuickControls2, Concurrent, Sql, and Test modules, plus a C++17 compiler.
- Release artifacts are produced by `scripts/package.py`. On Windows it runs
  `windeployqt` and writes a portable ZIP; on macOS it runs `macdeployqt` and
  writes a DMG plus a ZIP; on Linux it writes a TGZ and relies on the system Qt 6
  runtime. Each run writes SHA-256 checksums.
- The alternative CPack rules produce a ZIP on Windows, a drag-and-drop image and
  TGZ on macOS, and a TGZ and DEB on Linux. They package the install tree as-is
  and do not deploy Qt libraries or QML modules.
- Linux packages do not bundle Qt or KDE libraries, so they need the matching Qt 6
  runtime from the distribution until an AppImage or equivalent is added.
- Database storage uses Qt SQL with the SQLite driver.
- Builds, behavioral tests, and package creation run in CI on Windows, macOS, and
  Linux. The packaged artifacts are uploaded but are not yet launched as part of
  the pipeline, so installation and startup of the packaged build on each
  platform is still a manual verification step.

## License notices

- Project-owned source, QML, and documentation are licensed under
  GPL-3.0-or-later unless a file states otherwise. See [LICENSE](../LICENSE).
- The project is based on digiKam. Reused digiKam files retain their original
  copyright headers, SPDX identifiers, and GPL terms; digiKam is not relicensed.
- Qt is distributed under the LGPL-3.0/GPL-3.0/commercial licenses depending on
  the installation. Redistributors must include the applicable Qt notices and
  offer the corresponding source where required.
- Third-party icons, fonts, or libraries added later must keep their own notices;
  a dependency review is required before distributing combined builds.
