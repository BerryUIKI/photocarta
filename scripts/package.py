"""Cross-platform packaging and deployment script for PhotoCarta."""

import argparse
import hashlib
import os
import platform
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def get_project_version():
    """Extract PROJECT_VERSION from root CMakeLists.txt."""
    cmake_text = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    match = re.search(r"project\s*\(\s*PhotoCarta[\s\S]*?VERSION\s+([0-9A-Za-z._-]+)", cmake_text)
    if match:
        return match.group(1)
    return "0.1.0"


def get_architecture():
    """Return normalized architecture name."""
    machine = platform.machine().lower()
    if machine in ("amd64", "x86_64", "x64"):
        return "x86_64"
    if machine in ("arm64", "aarch64"):
        return "arm64"
    return machine


def find_cmake():
    """Locate cmake executable."""
    env_bin = os.environ.get("CMAKE_EXECUTABLE")
    if env_bin and Path(env_bin).is_file():
        return str(Path(env_bin))
    which = shutil.which("cmake")
    if which:
        return which
    local_cmake = ROOT / ".local" / "python-tools" / "cmake" / "data" / "bin" / ("cmake.exe" if sys.platform == "win32" else "cmake")
    if local_cmake.is_file():
        return str(local_cmake)
    return "cmake"


def find_windeployqt(qt_dir=None):
    """Locate windeployqt executable on Windows."""
    if qt_dir:
        cand = Path(qt_dir) / "bin" / "windeployqt.exe"
        if cand.is_file():
            return cand
        cand = Path(qt_dir) / "windeployqt.exe"
        if cand.is_file():
            return cand

    env_bin = os.environ.get("WINDEPLOYQT_EXECUTABLE")
    if env_bin and Path(env_bin).is_file():
        return Path(env_bin)

    search_roots = []
    if "QT_ROOT" in os.environ:
        search_roots.append(Path(os.environ["QT_ROOT"]))
    if "CMAKE_PREFIX_PATH" in os.environ:
        for prefix in os.environ["CMAKE_PREFIX_PATH"].split(os.pathsep):
            search_roots.append(Path(prefix))

    # Also search .local directory if present
    local_qt = ROOT / ".local" / "Qt"
    if local_qt.is_dir():
        search_roots.extend(local_qt.glob("*/*"))

    for search_root in search_roots:
        candidate = search_root / "bin" / "windeployqt.exe"
        if candidate.is_file():
            return candidate

    which = shutil.which("windeployqt")
    if which:
        return Path(which)

    return None


def find_macdeployqt(qt_dir=None):
    """Locate macdeployqt executable on macOS."""
    if qt_dir:
        cand = Path(qt_dir) / "bin" / "macdeployqt"
        if cand.is_file():
            return cand
        cand = Path(qt_dir) / "macdeployqt"
        if cand.is_file():
            return cand

    env_bin = os.environ.get("MACDEPLOYQT_EXECUTABLE")
    if env_bin and Path(env_bin).is_file():
        return Path(env_bin)

    search_roots = [
        Path("/opt/homebrew/opt/qt@6/bin/macdeployqt"),
        Path("/usr/local/opt/qt@6/bin/macdeployqt"),
    ]
    if "CMAKE_PREFIX_PATH" in os.environ:
        for prefix in os.environ["CMAKE_PREFIX_PATH"].split(os.pathsep):
            search_roots.append(Path(prefix) / "bin" / "macdeployqt")

    for candidate in search_roots:
        if candidate.is_file():
            return candidate

    which = shutil.which("macdeployqt")
    if which:
        return Path(which)

    return None


def calculate_sha256(file_path):
    """Compute hex SHA-256 checksum for a file."""
    hasher = hashlib.sha256()
    with open(file_path, "rb") as stream:
        for chunk in iter(lambda: stream.read(65536), b""):
            hasher.update(chunk)
    return hasher.hexdigest()


def create_archive_with_root(archive_format, output_path, staging_dir, folder_name):
    """Create a zip or gzipped tarball where contents are inside folder_name."""
    temp_wrap_dir = staging_dir.parent / f"_wrap_{folder_name}"
    if temp_wrap_dir.exists():
        shutil.rmtree(temp_wrap_dir)
    target_dir = temp_wrap_dir / folder_name
    shutil.copytree(staging_dir, target_dir)

    base_name = str(output_path)
    if base_name.endswith(".zip"):
        base_name = base_name[:-4]
    elif base_name.endswith(".tar.gz"):
        base_name = base_name[:-7]

    created = shutil.make_archive(
        base_name=base_name,
        format=archive_format,
        root_dir=temp_wrap_dir,
        base_dir=folder_name,
    )
    shutil.rmtree(temp_wrap_dir)
    return Path(created)


def main():
    parser = argparse.ArgumentParser(description="Package PhotoCarta release binaries.")
    parser.add_argument("--build-dir", default="build", help="Path to CMake build directory")
    parser.add_argument("--output-dir", default="packages", help="Destination directory for packages")
    parser.add_argument("--qt-dir", default=None, help="Path to Qt installation or bin directory")
    parser.add_argument("--version", default=None, help="Package version string (defaults to CMakeLists.txt)")
    parser.add_argument("--skip-deploy", action="store_true", help="Skip running windeployqt/macdeployqt")
    args = parser.parse_args()

    version = args.version or get_project_version()
    arch = get_architecture()
    current_os = platform.system().lower()

    build_dir = (ROOT / args.build_dir).resolve()
    output_dir = (ROOT / args.output_dir).resolve()
    staging_dir = output_dir / f"staging_{current_os}_{arch}"

    if not build_dir.is_dir():
        print(f"Error: Build directory not found: {build_dir}", file=sys.stderr)
        return 1

    output_dir.mkdir(parents=True, exist_ok=True)
    if staging_dir.exists():
        shutil.rmtree(staging_dir)
    staging_dir.mkdir(parents=True, exist_ok=True)

    print(f"=== Packaging PhotoCarta v{version} ({current_os} {arch}) ===")
    print(f"Build directory:   {build_dir}")
    print(f"Staging directory: {staging_dir}")
    print(f"Output directory:  {output_dir}")

    # Step 1: Install target into staging directory
    print("\n--- Step 1: Installing CMake targets ---")
    install_cmd = [
        find_cmake(),
        "--install",
        str(build_dir),
        "--prefix",
        str(staging_dir),
        "--config",
        "Release",
    ]
    subprocess.run(install_cmd, check=True)

    packages_created = []

    # Step 2: Platform-specific deployment and packaging
    if current_os == "windows":
        print("\n--- Step 2: Running windeployqt ---")
        executable = staging_dir / "photocarta.exe"
        if not executable.is_file():
            # If installed under bin/
            bin_executable = staging_dir / "bin" / "photocarta.exe"
            if bin_executable.is_file():
                executable = bin_executable

        if not executable.is_file():
            print(f"Error: Executable not found in staging directory: {staging_dir}", file=sys.stderr)
            return 1

        windeployqt = find_windeployqt(args.qt_dir)
        if windeployqt and not args.skip_deploy:
            print(f"Found windeployqt: {windeployqt}")
            deploy_cmd = [
                str(windeployqt),
                "--release",
                "--qmldir",
                str(ROOT / "src" / "qml"),
                "--compiler-runtime",
                "--no-translations",
                str(executable),
            ]
            print(f"Running: {' '.join(deploy_cmd)}")
            env = os.environ.copy()
            env["PATH"] = str(windeployqt.parent) + os.pathsep + env.get("PATH", "")
            subprocess.run(deploy_cmd, check=True, env=env)
        else:
            print("Notice: windeployqt not found or deployment skipped.")

        print("\n--- Step 3: Creating Windows portable zip package ---")
        package_base = f"PhotoCarta-{version}-windows-{arch}"
        zip_path = output_dir / f"{package_base}.zip"
        archive_file = create_archive_with_root("zip", zip_path, staging_dir, package_base)
        packages_created.append(archive_file)

    elif current_os == "darwin":
        print("\n--- Step 2: Running macdeployqt ---")
        bundle_path = staging_dir / "PhotoCarta.app"
        if not bundle_path.is_dir():
            alt_bundle = staging_dir / "photocarta.app"
            if alt_bundle.is_dir():
                bundle_path = alt_bundle

        package_base = f"PhotoCarta-{version}-macos-{arch}"

        macdeployqt = find_macdeployqt(args.qt_dir)
        if bundle_path.is_dir() and macdeployqt and not args.skip_deploy:
            print(f"Found macdeployqt: {macdeployqt}")
            deploy_cmd = [
                str(macdeployqt),
                str(bundle_path),
                f"-qmldir={ROOT / 'src' / 'qml'}",
                "-dmg",
            ]
            print(f"Running: {' '.join(deploy_cmd)}")
            subprocess.run(deploy_cmd, check=True)

            # macdeployqt creates PhotoCarta.dmg alongside the app
            dmg_candidate = bundle_path.parent / f"{bundle_path.stem}.dmg"
            if dmg_candidate.is_file():
                final_dmg = output_dir / f"{package_base}.dmg"
                shutil.move(dmg_candidate, final_dmg)
                packages_created.append(final_dmg)

        # Also create zip archive of the staging directory
        print("\n--- Step 3: Creating macOS zip package ---")
        zip_path = output_dir / f"{package_base}.zip"
        archive_file = create_archive_with_root("zip", zip_path, staging_dir, package_base)
        packages_created.append(archive_file)

    else:
        print("\n--- Step 2: Creating Linux tarball package ---")
        package_base = f"PhotoCarta-{version}-linux-{arch}"
        tar_path = output_dir / f"{package_base}.tar.gz"
        archive_file = create_archive_with_root("gztar", tar_path, staging_dir, package_base)
        packages_created.append(archive_file)

    # Step 3: Generate SHA-256 checksums
    print("\n--- Checksums (SHA-256) ---")
    checksum_lines = []
    for pkg in packages_created:
        sha = calculate_sha256(pkg)
        line = f"{sha}  {pkg.name}"
        checksum_lines.append(line)
        print(line)

    sums_path = output_dir / f"SHA256SUMS-{current_os}-{arch}.txt"
    sums_path.write_text("\n".join(checksum_lines) + "\n", encoding="utf-8")
    print(f"\nChecksums written to {sums_path}")

    # Clean up staging directory to leave only package artifacts
    if staging_dir.exists():
        shutil.rmtree(staging_dir)

    print("\nPackaging completed successfully!")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
