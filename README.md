# PhotoCarta

PhotoCarta is an open-source desktop photo management project based on digiKam,
with a redesigned user interface. PhotoCarta is the current working name.

The first release focuses on the GUI: improve the layout, navigation, visual
consistency, and everyday photo browsing experience while reusing the existing
backend. Release early, then improve functionality incrementally.

## Project decisions

- Support Windows, macOS, and Linux from the first phase.
- Keep C++ and Qt 6; use Qt Quick/QML for the new main interface.
- Reuse digiKam's database, media engines, and established operations where practical.
- Support local disks, removable drives, and mounted network collections.
- Treat one million cataloged items as an architecture and validation target,
  not an already demonstrated performance claim.
- Keep the initial release focused on GUI changes. Later feature priorities and
  milestones will be agreed with the project owner.

## Documentation

- [Architecture and reuse strategy](docs/architecture.md)
- [Storage choices and scale](docs/storage-and-scale.md)
- [Initial release scope](docs/initial-release.md)
- [Known limitations](docs/known-limitations.md)
- [GitFlow and repository protection](docs/gitflow.md)
- [Local build instructions](docs/build-instructions.md)
- [Contributing](CONTRIBUTING.md)

## Current status

The Qt Quick GUI is implemented with CMake, C++ presentation models, and QML assets.
It delivers a photo-focused desktop interface with dark and light themes, responsive collection and
metadata sidebars, a virtualized thumbnail grid, a large-image preview mode, a side-by-side compare
view, and procedural SVG demo fixtures.

Keyboard navigation covers the grid and every custom-drawn control: controls are reachable with Tab,
show a visible focus ring, activate with Enter or Space, and expose accessible roles, names, and
states. The photo grid is an accessible list, and modal dialogs take focus on open, respond to
Escape, and return focus to the grid. Fractional display scaling is preserved through Qt's
`PassThrough` high-DPI policy and rendering is checked with offscreen screenshots at 150%.

Metadata editing is available in the details pane: ratings, favorites, color labels, titles,
captions, and tags are written to the local SQLite catalog for a single photo or the whole selection,
the sidebar tag list is derived from the catalog, and the grid can be filtered by search text, tag,
rating, color label, or category.

The toolbar and the photo context menu can hand the selected photo to an external editor: an
installed ShowFoto or digiKam, or the executable named by `PHOTOCARTA_EXTERNAL_EDITOR`. Unsupported
handoffs, such as demo, offline, or multi-photo selections, are reported rather than ignored.

The catalog supports recursive folder scanning into SQLite, batch rename with a preview, image export
with format conversion and resizing, XMP sidecar read/write, and move-to-trash. Existing digiKam
catalogs can be opened and browsed, with rating and tag edits routed through the digiKam adapter.
Offline collections are represented and never treated as deleted.

The presentation interface is decoupled from backend engines. Remaining integration work, including
the retained digiKam tool windows, is tracked in [Known limitations](docs/known-limitations.md).

The reference checkout is `F:/dev/digikam`, inspected at commit
`c60c45a8aa30a6a135982cfa4e46cbcaff0bc2c8` (version declared as 9.2.0).
This machine-specific path is a development reference, not a release dependency.

## License

Project-owned material is licensed under **GPL-3.0-or-later**, unless a file states
otherwise; see [LICENSE](LICENSE). Reused digiKam and third-party files retain
their original license headers, notices, and attributions. This does not relicense
digiKam or replace a dependency review before distributing combined builds.

## Repository checks

```sh
python scripts/check_repository.py
python -m unittest discover -s tests -v
```

GitHub Actions runs repository checks on Windows, macOS, and Linux, and builds the
application with the focused Qt test suite and startup/screenshot smoke checks on
each platform. Normal PRs target `develop`; `main` is reserved for release and
hotfix integration.
