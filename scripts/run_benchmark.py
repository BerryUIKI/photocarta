"""Run the PhotoCarta scale benchmark and record the result.

The benchmark needs a real photo library: thumbnail throughput depends on the
decode cost of camera output, so a synthetic JPEG set produces a number that
does not describe real use. This script only drives the already-built binary
and appends the result to docs/benchmarks.md; it does not generate media.

Typical use:

    python scripts/run_benchmark.py --media-dir D:/photos/library

Build the target first (it is opt-in and not part of the default build):

    cmake -B build/bench -DBUILD_BENCHMARK=ON -DBUILD_TESTING=ON
    cmake --build build/bench --target photocarta_benchmark

On Windows the Qt runtime directory must be on PATH or the executable fails
with 0xC0000135; scripts\\build-msvc.bat sets that up for a plain build.
"""

import argparse
import json
import os
import platform
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_BUILD = ROOT / "build" / "bench"
REPORT = ROOT / "docs" / "benchmarks.md"


def find_binary(build_dir):
    """Locate the benchmark executable for the current platform."""
    names = ["photocarta_benchmark.exe", "photocarta_benchmark"] if os.name == "nt" else [
        "photocarta_benchmark", "photocarta_benchmark.exe"
    ]
    candidates = []
    for name in names:
        candidates.extend(
            path for path in build_dir.rglob(name)
            if path.is_file() and "CMakeFiles" not in path.parts
        )
    if not candidates:
        raise SystemExit(
            f"error: photocarta_benchmark not found under {build_dir}\n"
            "Configure with -DBUILD_BENCHMARK=ON and build that target first."
        )
    return candidates[0]


def hardware_summary():
    """Best-effort description of the measuring machine, for the record."""
    parts = [platform.machine() or "unknown", platform.system(), platform.release()]
    if os.name == "nt":
        try:
            output = subprocess.run(
                ["powershell", "-NoProfile", "-Command",
                 "(Get-CimInstance Win32_Processor).Name; "
                 "(Get-CimInstance Win32_ComputerSystem).TotalPhysicalMemory"],
                capture_output=True, text=True, timeout=30, check=False,
            ).stdout.splitlines()
            if output:
                parts.insert(0, output[0].strip())
            if len(output) > 1:
                try:
                    parts.append(f"{int(output[1].strip()) / (1024 ** 3):.0f} GB RAM")
                except ValueError:
                    pass
        except (OSError, subprocess.SubprocessError):
            pass
    else:
        try:
            with open("/proc/cpuinfo", encoding="utf-8") as handle:
                for line in handle:
                    if line.startswith("model name"):
                        parts.insert(0, line.split(":", 1)[1].strip())
                        break
            meminfo = Path("/proc/meminfo").read_text(encoding="utf-8")
            for line in meminfo.splitlines():
                if line.startswith("MemTotal:"):
                    parts.append(f"{int(line.split()[1]) / (1024 ** 2):.0f} GB RAM")
                    break
        except OSError:
            pass
    return ", ".join(parts)


def table_row(report):
    """Render the docs/benchmarks.md row for one run."""
    open_ms = report["catalogOpen"]["openAndLoadMs"]
    filter_ms = report["filter"]["p95Ms"]
    thumbs = report["thumbnails"]
    memory = report["memory"]
    verdict = report["verdict"]

    def mark(ok):
        return "pass" if ok else "**MISS**"

    stall = thumbs.get("longestEventLoopStallMs", -1)
    stall_text = f"{stall:.0f} ms" if stall >= 0 else "not measured"
    return "| {label} | {count:,} | {open} ({vo}) | {filt} ({vf}) | {tps} ({vt}) | {stall} ({vs}) | {mem} ({vm}) |".format(
        label=report.get("label") or "unlabelled",
        count=report["itemCount"],
        open=f"{open_ms:.0f} ms", vo=mark(verdict["openPass"]),
        filt=f"{filter_ms:.1f} ms", vf=mark(verdict["filterPass"]),
        tps=f"{thumbs['perSecond']:.0f}/s", vt=mark(verdict["thumbnailPass"]),
        stall=stall_text, vs=mark(verdict["stallPass"]),
        mem=f"{memory['peakRssMb']:.0f} MB", vm=mark(verdict["memoryPass"]),
    )


def append_report(report, row, hardware, binary_version):
    """Append the run to docs/benchmarks.md under the current month."""
    from datetime import date

    entry = [
        "",
        f"### {date.today().isoformat()} — {report.get('label') or 'unlabelled'}",
        "",
        f"- Machine: {hardware}",
        f"- Qt: {report['environment']['qtVersion']}, "
        f"build: {report['environment']['buildType']}, "
        f"OS: {report['environment']['os']}",
        f"- Commit: {binary_version}, catalog items: {report['itemCount']:,}",
        f"- Ingest: {report['ingest']['scanMs']:.0f} ms "
        f"({report['ingest']['itemsPerSecond']:.0f} items/s)",
        f"- Filter samples: {report['filter']['samples']}, "
        f"median {report['filter']['medianMs']:.1f} ms, "
        f"p95 {report['filter']['p95Ms']:.1f} ms, "
        f"max {report['filter']['maxMs']:.1f} ms",
        f"- Thumbnails: {report['thumbnails']['generated']} generated, "
        f"longest event-loop stall {report['thumbnails']['longestEventLoopStallMs']:.0f} ms",
        "",
        row,
        "",
    ]
    with REPORT.open("a", encoding="utf-8") as handle:
        handle.write("\n".join(entry))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--media-dir", required=True, type=Path,
                        help="Directory of real photos to scan. Read only; nothing is modified.")
    parser.add_argument("--build-dir", type=Path, default=DEFAULT_BUILD,
                        help=f"Build directory containing the benchmark target (default: {DEFAULT_BUILD})")
    parser.add_argument("--label", default="",
                        help="Label for the run, e.g. 'Windows 11, cold cache, SSD'")
    parser.add_argument("--thumbnails", type=int, default=500,
                        help="Thumbnails to generate for the throughput measurement (default: 500)")
    parser.add_argument("--work-dir", type=Path,
                        help="Where to put the benchmark catalog and thumbnail cache. "
                             "Defaults to a temporary directory that is removed afterwards.")
    parser.add_argument("--keep-work-dir", action="store_true",
                        help="Keep the benchmark catalog for inspection (implies a stable work dir).")
    parser.add_argument("--record", action="store_true",
                        help="Append the result to docs/benchmarks.md.")
    parser.add_argument("--json", type=Path, help="Also write the raw JSON report here.")
    args = parser.parse_args()

    media = args.media_dir.expanduser().resolve()
    if not media.is_dir():
        raise SystemExit(f"error: --media-dir is not a directory: {media}")

    binary = find_binary(args.build_dir)

    if args.work_dir:
        work = args.work_dir.expanduser().resolve()
        work.mkdir(parents=True, exist_ok=True)
        cleanup = False
    elif args.keep_work_dir:
        work = (ROOT / "build" / "benchmark-work").resolve()
        work.mkdir(parents=True, exist_ok=True)
        cleanup = False
    else:
        work = Path(tempfile.mkdtemp(prefix="photocarta-benchmark-"))
        cleanup = True

    command = [
        str(binary),
        "--media-dir", str(media),
        "--work-dir", str(work),
        "--thumbnails", str(args.thumbnails),
    ]
    if args.label:
        command += ["--label", args.label]

    print(f"Running: {' '.join(command)}")
    try:
        result = subprocess.run(command, capture_output=True, text=True, check=False)
    finally:
        pass

    if result.returncode != 0:
        sys.stderr.write(result.stderr)
        raise SystemExit(f"error: benchmark exited with {result.returncode}")

    # The binary writes JSON on stdout and its human summary on stderr.
    try:
        report = json.loads(result.stdout)
    except json.JSONDecodeError as error:
        sys.stderr.write(result.stderr)
        raise SystemExit(f"error: benchmark did not return JSON: {error}")

    row = table_row(report)
    print(result.stderr.strip())
    print()
    print(row)

    if args.json:
        args.json.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        print(f"\nJSON report written to {args.json}")

    if args.record:
        version = subprocess.run(
            ["git", "rev-parse", "--short", "HEAD"], cwd=ROOT,
            capture_output=True, text=True, check=False,
        ).stdout.strip() or "unknown"
        append_report(report, row, hardware_summary(), version)
        print(f"Result appended to {REPORT.relative_to(ROOT)}")

    if cleanup:
        shutil.rmtree(work, ignore_errors=True)

    return 0 if report["verdict"]["allPass"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
