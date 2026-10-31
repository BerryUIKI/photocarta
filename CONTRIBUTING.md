# Contributing to PhotoCarta

Start with the [initial release scope](docs/initial-release.md) and
[architecture](docs/architecture.md). The current priority is the new GUI while
retaining established digiKam behavior.

## Development workflow

Use [GitFlow](docs/gitflow.md). Normal changes target `dev`, not `main`.

```sh
git clone https://github.com/BerryUIKI/photocarta.git
cd photocarta
git switch dev
git switch -c feature/your-topic
```

Make focused commits with clear English messages. Suggested prefixes are `feat:`,
`fix:`, `docs:`, `chore:`, `ci:`, `refactor:`, and `test:`. Use merge commits when
merging PRs so release and hotfix ancestry is preserved.

## Current checks

Python 3.12 or newer is sufficient for the repository checks; no extra packages
are required:

```sh
python scripts/check_repository.py
python -m unittest discover -s tests -v
git diff --check
```

The application and behavioral tests are built with CMake and Qt 6. See
[Local build instructions](docs/build-instructions.md) for platform prerequisites:

```sh
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

CI validates repository policy, GitFlow rules, and native builds on Windows,
macOS, and Linux, executing the full Qt behavioral test suite and startup smoke tests.

For UI changes, include before/after images where useful and describe keyboard,
display-scaling, and theme validation. State which platforms were tested. Use
copied catalogs and disposable photo fixtures for write operations.

## Licensing and upstream code

Unless a file states otherwise, project-owned contributions are provided under
GPL-3.0-or-later; see [LICENSE](LICENSE). No copyright assignment is required.
Preserve upstream notices and licenses when reusing code and record its origin.
Do not add third-party assets, weights, or code without recording their licenses.
Before distributing combined builds, check the actual included dependencies and
preserve their required notices and source availability obligations.

## Pull requests

Explain the user-facing result, relevant scope, verification, and limitations.
Resolve review discussions and wait for `Required checks`. The single-maintainer
bootstrap policy requires a PR and CI but does not require an independent approval;
review is welcome. Team review requirements can be raised when another maintainer
is available.
