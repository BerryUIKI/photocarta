# Scale Benchmarks

Measured performance of the catalog, filter, and thumbnail pipeline against the
targets recorded in [Minimum viable product](mvp.md). Nothing in this file is an
estimate or an architectural promise: every row is one recorded run on the
machine named in the entry.

Until a run exists for a scale and platform, no capacity claim is made for it.
One million items remains an architecture target and is not measured here.

## Targets

From [Minimum viable product](mvp.md), measured on the reference machine:

| Measurement | Target |
| --- | --- |
| Open a 100,000-item catalog to an interactive grid | 3 seconds or less |
| Search or filter update | 400 ms or less at the 95th percentile |
| Thumbnail generation | 40 per second or more |
| Longest UI-thread stall | 100 ms or less |
| Resident memory with catalog and thumbnails | 1.5 GB or less |

## Method

`tests/benchmark/benchmark_main.cpp` measures these against a real photo
directory. The catalog is built by scanning that directory through the same
`DirectoryScanner` and `CatalogDatabase` path the application uses, so ingestion
and query behavior are measured on the production code, not a synthetic writer.

What each number means:

- **Open** — close and reopen the catalog, load every row into `CatalogModel`,
  and attach `CatalogFilterModel`. This is the startup path `main.cpp` takes for
  a saved catalog. It is currently a synchronous full load.
- **Filter p95** — 21 filter changes (free-text search, rating, tag) over the
  loaded catalog, each timed through a full `rowCount()` traversal of the proxy.
- **Thumbnails/s** — thumbnails generated into an empty cache, so every call
  decodes. Reported as generated images per second.
- **Longest stall** — the longest gap between 5 ms event-loop timer ticks while
  thumbnails generate. A run whose probe fires fewer than 10 times does not
  report a stall measurement, because too few samples overstate real blocking.
- **Peak RSS** — process peak working set after the thumbnail run.

### Running it

The target is opt-in and is not part of the default build or the default
`ctest` suite, because a 100,000-item run takes minutes and gigabytes.

```bash
cmake -B build/bench -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DBUILD_BENCHMARK=ON
cmake --build build/bench --target photocarta_benchmark
```

On Windows, `scripts\build-msvc.bat` sets up the MSVC and Qt environment; pass
the same `-DBUILD_BENCHMARK=ON` when configuring a benchmark build there.

Then measure a library and optionally append the result to this file:

```bash
python scripts/run_benchmark.py --media-dir D:/photos/library --label "Windows 11, SSD, cold cache" --record
```

The benchmark only reads the media directory. The catalog and thumbnail cache
go into a temporary directory that is removed afterwards, or into `--work-dir`
when you want to inspect them.

A run exits non-zero when a target is missed, but the script still records the
row: a recorded miss is the point of the exercise, and it gives the next
optimization a baseline to compare against.

## Results

Each entry records the exact machine, Qt version, build type, and commit.
Values in **bold** missed their target.

| Run | Items | Open | Filter p95 | Thumbnails | Longest stall | Peak RSS |
| --- | --- | --- | --- | --- | --- | --- |

_No run recorded yet._ The benchmark harness was added in this change; the first
100,000-item measurement on real media has not been performed.
