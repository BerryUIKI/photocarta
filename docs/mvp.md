# Minimum Viable Product

Status: agreed 2026-10-22. This document defines what "MVP" means for PhotoCarta.
[Initial release scope](initial-release.md) remains the transition scope, and
[Known limitations](known-limitations.md) records what is not verified today.

## Goal

A photographer can replace ad-hoc folder browsing with PhotoCarta for one local
library: find photos, review and cull them, organize metadata, and perform common
file operations on Windows, macOS, and Linux — without installing digiKam.

## Audience

- Individual photographers and enthusiasts with one local library of up to
  100,000 items on an internal or directly attached disk.
- digiKam is not required. Opening an existing digiKam catalog remains a
  compatibility feature, not the product's core promise, and the MVP makes no
  round-trip guarantee for catalogs edited by both applications.

## Platform and distribution

- Windows x86_64, macOS arm64, and Linux x86_64 builds.
- A packaged, self-contained application per platform. The Linux package relies
  on a compatible system Qt 6 runtime.
- Code signing, macOS notarization, auto-update, and installers beyond the
  portable packages are not MVP requirements; unsigned-package warnings are
  documented instead.

## Must-have journeys

| Journey | Acceptance |
| --- | --- |
| Add and reopen a library | Select one or more folders, scan them recursively into a catalog, restart the application, and find the same items and metadata. |
| Browse and preview | Virtualized thumbnail grid, full preview with zoom, rotation and fullscreen, side-by-side compare, and a histogram. |
| Find photos | Text search, sorting, and filtering by tag, rating, color label, and collection. |
| Organize metadata | Ratings, favorites, color labels, tags, title, and caption can be set for one photo or a whole selection and persist across restarts. XMP sidecar export and import remain available. |
| File operations | Batch rename with preview, export with format conversion and resizing, move to trash with confirmation, reveal in file manager, and copy paths. |
| Editor handoff | Open one selected photo in an external editor; refuse and explain unsupported selections instead of failing silently. |
| Interface quality | Dark and light themes, complete keyboard operation with a visible focus indicator, accessible roles and names on custom controls, and 100–150% display scaling. |
| Safety | Offline collections and missing files are never treated as deleted; destructive operations require confirmation; there is no permanent-deletion fallback. |

## Quality bar

- Every catalog read and write used by the journeys is covered by automated Qt
  tests, and catalog changes are validated against copied catalogs and disposable
  media.
- CI builds and runs the behavioral tests on Windows, macOS, and Linux, and
  verifies that each packaged build starts.
- A manual release pass completes the journeys above on every platform with a
  disposable library and a copied catalog.
- Limitations, packaging dependencies, and license notices are current, and no
  known data-loss defect is open.

## Scale bar: 100,000 items

The MVP claim requires a recorded measurement, not an architectural assertion.
Provisional targets on the reference machine (4 CPU cores, SSD, 16 GB RAM, local
disk, JPEG library):

| Measurement | Target |
| --- | --- |
| Open a 100,000-item catalog to an interactive grid | 3 seconds or less |
| Search or filter update | 400 ms or less at the 95th percentile |
| Thumbnail generation | 40 per second or more |
| Longest UI-thread stall | 100 ms or less |
| Resident memory with catalog and thumbnails | 1.5 GB or less |

Method:

- A benchmark script generates a synthetic 100,000-item catalog and drives the
  measured operations.
- A benchmark mode in the application, or a dedicated benchmark test, records
  catalog open, filter, and thumbnail timings.
- Results are recorded with the exact hardware and software versions in
  `docs/benchmarks.md`.
- Until a run is recorded, no capacity claim is made. The one-million-item
  architecture target in [Storage and scale](storage-and-scale.md) stays
  unmeasured and is not part of the MVP.

## Explicitly out of scope

Camera import, the batch queue manager, in-process digiKam tools, RAW
development, pixel editing, face recognition, cloud synchronization, video
cataloging, translations, auto-update, code signing and notarization, an undo
history, a plugin API, and million-item optimization.

## Gaps to close before the MVP

1. **Scale validation** — build the benchmark harness, record the first
   100,000-item run, and tune paged queries or thumbnail throughput if a target
   is missed. This is the largest remaining item.
2. **Preferences** — an in-app dialog for theme and external editor path. Today
   the theme is a toolbar toggle and the editor is configured through
   `PHOTOCARTA_EXTERNAL_EDITOR`. Decide whether this is required for the MVP or
   deferred; it is not part of the acceptance checklist below.
3. **Acceptance run** — execute the journeys on all three platforms with
   disposable media and a copied catalog, and record the results.

## Definition of done

- Every must-have journey passes on Windows, macOS, and Linux.
- The 100,000-item benchmark is recorded and within target.
- CI is green on all three platforms and verifies packaged startup.
- Limitations and packaging documentation are current.
- The MVP release is tagged on `main` through the GitFlow release path.
