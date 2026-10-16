# PhotoCarta Agent Instructions

## Product and scope

- Read `README.md`, `docs/architecture.md`, and `docs/initial-release.md` first.
- PhotoCarta is an open-source digiKam-based desktop application. Prioritize a
  modern GUI and an early release on Windows, macOS, and Linux.
- Use C++, Qt 6, Qt Quick/QML, and CMake. Rust is not required.
- Reuse backend operations and media engines. Keep necessary Qt Widgets tools and
  KDE dependencies during the transition. Avoid unrelated backend rewrites.
- Support local, removable, and mounted network collections. One million items is
  a design target, not a demonstrated capacity or an initial optimization gate.
- Future feature scope and milestones are agreed with the project owner.

## Repository and upstream

- This repository currently contains project initialization and documentation;
  application source and build targets have not been imported yet.
- `F:/dev/digikam` is the reference checkout on the current developer's machine.
  Read that repository's own instructions before working there. Do not make it a
  hard-coded build dependency or modify it as a side effect of PhotoCarta work.
- Record provenance when importing source. Preserve original copyright headers,
  SPDX identifiers, licenses, and attribution. New project-owned material defaults
  to GPL-3.0-or-later; imported material retains its own terms.
- Do not commit production photo catalogs, private photos, credentials, build
  products, generated caches, or large model weights.

## Implementation conventions

- Keep SQL, scans, image decoding, and filesystem operations outside QML. Use
  C++ services and item-model adapters; respect QObject thread ownership.
- Keep long-running work off the GUI thread and bound queues and image buffers.
- Use shared UI components and design tokens. Maintain keyboard navigation,
  accessibility, display scaling, and light/dark theme behavior.
- Do not treat temporarily unavailable collections as deleted. Validate metadata
  writes and destructive operations using copied catalogs and disposable media.
- Match upstream C++ conventions in imported files; avoid unrelated reformatting.
  `.clang-format` describes new project-owned code, not an upstream cleanup.
- Repository documentation, source comments, and commit messages are in English.
  Converse with the user in their chosen language.

## GitFlow

- Read `docs/gitflow.md`. `main` is the stable line; `develop` is integration.
- Branch from `develop` for normal work. Agents use `codex/feature/<topic>`,
  `codex/bugfix/<topic>`, or `codex/chore/<topic>` as appropriate.
- Releases use `release/<version>` from `develop`; urgent fixes use
  `hotfix/<version>` from `main`. Merge both into `main` and back into `develop`.
- Use PRs and merge commits for protected branches. Do not force-push, delete,
  or commit directly to `main` or `develop`. Do not weaken rules to pass CI.
- Preserve user changes. Do not reset, discard, auto-merge, or publish a release
  without authorization in the current task. Do not infer a commit/push request
  from an unrelated implementation task.

## Validation and reporting

- Run `python scripts/check_repository.py` and
  `python -m unittest discover -s tests -v` for initialization/policy changes.
- Current CI validates repository files and GitFlow policy on three host operating
  systems. It does not compile or test the future application.
- Once application targets exist, add real configure/build/tests and UI smoke
  checks to CI. Use visual review for layout changes and focused behavioral tests
  for integration changes; avoid tests that only restate styling code.
- Report what changed, what was actually verified, and remaining limitations.
  Do not report planned platform support or scale goals as tested facts.
- Keep these instructions and project documentation aligned with agreed decisions.
