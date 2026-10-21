"""Launch a packaged PhotoCarta build to verify that it starts and exits.

Release packages are deployed with their Qt runtime on Windows and macOS and rely
on the system Qt on Linux. Building and archiving them does not prove that the
result runs, so this script extracts the produced artifact into a temporary
directory, outside the build tree, and runs the packaged executable with
``--smoke-test``.

The startup smoke test loads and closes the QML engine, so it covers installation,
startup, and a clean exit for the packaged layout. Headless offscreen startup is
tried first, with the platform's default QPA plugin as a fallback.
"""

import argparse
import inspect
import os
import platform
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

SMOKE_OFFSCREEN_ARGUMENTS = ("--smoke-test", "-platform", "offscreen")
SMOKE_DEFAULT_ARGUMENTS = ("--smoke-test",)
SMOKE_TIMEOUT_SECONDS = 120

# Archive extraction should apply the data filter where the interpreter supports
# it: the artifacts are our own build output, but the filter keeps extraction
# confined to regular files and directories.
_UNPACK_KWARGS = (
    {"filter": "data"} if "filter" in inspect.signature(shutil.unpack_archive).parameters else {}
)


def find_archives(package_dir, system=None):
    """Return the package artifact for the platform, most specific first."""
    system = (system or platform.system()).lower()
    directory = Path(package_dir)
    if system == "windows":
        return sorted(directory.glob("PhotoCarta-*-windows-*.zip"))
    if system == "darwin":
        # The ZIP contains the deployed .app bundle; mounting the DMG is not
        # needed to prove that the packaged application starts.
        return sorted(directory.glob("PhotoCarta-*-macos-*.zip"))
    return sorted(directory.glob("PhotoCarta-*-linux-*.tar.gz"))


def find_executable(root, system=None):
    """Locate the application binary inside an extracted package."""
    system = (system or platform.system()).lower()
    root = Path(root)

    if system == "windows":
        candidates = [path for path in root.rglob("photocarta.exe") if path.is_file()]
    else:
        candidates = [path for path in root.rglob("photocarta") if path.is_file()]

    if system == "darwin":
        # Prefer the bundle entry point over any loose copy.
        bundle_candidates = [path for path in candidates if "Contents/MacOS" in path.as_posix()]
        if bundle_candidates:
            candidates = bundle_candidates

    return sorted(candidates)[0] if candidates else None


def extract_archive(archive, destination, system=None):
    """Extract a ZIP or gzipped tarball artifact into destination."""
    system = (system or platform.system()).lower()
    archive = Path(archive)

    if system == "darwin" and archive.suffix == ".zip":
        # ditto restores the symlinks and permissions inside a deployed .app
        # bundle; Python's zipfile extraction does not.
        subprocess.run(["ditto", "-x", "-k", str(archive), str(destination)], check=True)
    elif archive.suffix == ".zip":
        shutil.unpack_archive(str(archive), str(destination), "zip")
    elif archive.name.endswith((".tar.gz", ".tgz")):
        shutil.unpack_archive(str(archive), str(destination), "gztar", **_UNPACK_KWARGS)
    else:
        shutil.unpack_archive(str(archive), str(destination))


def _smoke_environment(executable, force_offscreen):
    environment = os.environ.copy()
    if force_offscreen:
        environment["QT_QPA_PLATFORM"] = "offscreen"
    else:
        environment.pop("QT_QPA_PLATFORM", None)

    executable_dir = str(executable.parent)
    environment["PATH"] = executable_dir + os.pathsep + environment.get("PATH", "")

    # On Linux the package does not bundle Qt; point the loader at the Qt
    # installation that built it when the CI environment exposes one.
    qt_root = environment.get("QT_ROOT_DIR") or environment.get("QT_ROOT")
    if qt_root and platform.system().lower() != "windows":
        library_dirs = [str(Path(qt_root) / "lib")]
        environment["LD_LIBRARY_PATH"] = os.pathsep.join(
            library_dirs + [environment.get("LD_LIBRARY_PATH", "")]
        )

    return environment


def _run_smoke(executable, arguments, force_offscreen):
    """Run one startup attempt. Returns the completed process or None on timeout."""
    print(f"Running: {executable} {' '.join(arguments)}")
    try:
        return subprocess.run(
            [str(executable), *arguments],
            cwd=str(executable.parent),
            env=_smoke_environment(executable, force_offscreen),
            capture_output=True,
            text=True,
            timeout=SMOKE_TIMEOUT_SECONDS,
        )
    except subprocess.TimeoutExpired:
        print(f"Error: packaged application did not exit within {SMOKE_TIMEOUT_SECONDS}s", file=sys.stderr)
        return None


def _print_package_diagnostics(executable, system):
    """Print bundle details when startup fails, to explain deployment problems."""
    print("Package diagnostics:", file=sys.stderr)
    contents_dir = executable.parent.parent if system == "darwin" else executable.parent
    frameworks = contents_dir / "Frameworks"
    print(f"  Frameworks directory present: {frameworks.is_dir()}", file=sys.stderr)
    if frameworks.is_dir():
        entries = sorted(path.name for path in frameworks.iterdir())[:12]
        print(f"  Frameworks entries: {', '.join(entries)}", file=sys.stderr)

    if system == "darwin" and shutil.which("otool"):
        completed = subprocess.run(["otool", "-l", str(executable)], capture_output=True, text=True)
        interesting = [
            line.strip()
            for line in completed.stdout.splitlines()
            if "LC_RPATH" in line or line.strip().startswith("path ")
        ]
        print("  otool rpath entries:", file=sys.stderr)
        for line in interesting[:10]:
            print(f"    {line}", file=sys.stderr)


def verify_archive(archive, system=None):
    """Extract one package and run the packaged executable. Returns True on success."""
    system = (system or platform.system()).lower()
    archive = Path(archive)
    print(f"Verifying packaged application: {archive}")

    with tempfile.TemporaryDirectory(prefix="photocarta-package-check-") as temporary:
        extracted = Path(temporary) / "extracted"
        extracted.mkdir()
        try:
            extract_archive(archive, extracted, system)
        except (OSError, shutil.ReadError, subprocess.SubprocessError) as error:
            print(f"Error: could not extract {archive}: {error}", file=sys.stderr)
            return False

        executable = find_executable(extracted, system)
        if executable is None:
            print(f"Error: no PhotoCarta executable found in {archive}", file=sys.stderr)
            return False

        # Archive extraction tools do not always restore the executable bit.
        if system != "windows":
            try:
                executable.chmod(executable.stat().st_mode | 0o111)
            except OSError as error:
                print(f"Error: could not make {executable} executable: {error}", file=sys.stderr)
                return False

        # Prefer headless startup; fall back to the platform plugin when the
        # package does not ship the offscreen plugin.
        attempts = (
            (SMOKE_OFFSCREEN_ARGUMENTS, True),
            (SMOKE_DEFAULT_ARGUMENTS, False),
        )
        last_returncode = None
        for arguments, force_offscreen in attempts:
            try:
                completed = _run_smoke(executable, arguments, force_offscreen)
            except OSError as error:
                print(f"Error: could not start packaged application: {error}", file=sys.stderr)
                return False

            if completed is None:
                continue

            if completed.stdout:
                print(completed.stdout, end="")
            if completed.stderr:
                print(completed.stderr, end="", file=sys.stderr)

            if completed.returncode == 0:
                print("Packaged application started and exited cleanly.")
                return True

            last_returncode = completed.returncode
            print(f"Notice: startup attempt exited with {last_returncode}; trying the next platform plugin.",
                  file=sys.stderr)

    print(f"Error: packaged application failed to start (last exit code {last_returncode})", file=sys.stderr)
    _print_package_diagnostics(executable, system)
    return False


def main():
    parser = argparse.ArgumentParser(description="Verify that packaged PhotoCarta artifacts start.")
    parser.add_argument("--package-dir", default="packages", help="Directory containing package artifacts")
    parser.add_argument("--platform", default=None, help="Override platform detection (windows, darwin, linux)")
    args = parser.parse_args()

    package_dir = Path(args.package_dir)
    if not package_dir.is_dir():
        print(f"Error: package directory not found: {package_dir}", file=sys.stderr)
        return 1

    archives = find_archives(package_dir, args.platform)
    if not archives:
        print(f"Error: no package artifact found in {package_dir}", file=sys.stderr)
        return 1

    for archive in archives:
        if not verify_archive(archive, args.platform):
            return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
