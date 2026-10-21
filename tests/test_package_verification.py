"""Behavior checks for packaged-artifact startup verification."""

import shutil
import tempfile
import unittest
from pathlib import Path

from scripts.verify_package import extract_archive, find_archives, find_executable


class PackageVerificationTests(unittest.TestCase):
    def test_find_archives_selects_the_platform_artifact(self):
        with tempfile.TemporaryDirectory() as temporary:
            package_dir = Path(temporary)
            names = [
                "PhotoCarta-0.1.0-windows-x86_64.zip",
                "PhotoCarta-0.1.0-macos-arm64.zip",
                "PhotoCarta-0.1.0-macos-arm64.dmg",
                "PhotoCarta-0.1.0-linux-x86_64.tar.gz",
                "SHA256SUMS-windows-x86_64.txt",
            ]
            for name in names:
                (package_dir / name).touch()

            self.assertEqual(
                [path.name for path in find_archives(package_dir, "windows")],
                ["PhotoCarta-0.1.0-windows-x86_64.zip"],
            )
            self.assertEqual(
                [path.name for path in find_archives(package_dir, "darwin")],
                ["PhotoCarta-0.1.0-macos-arm64.zip"],
            )
            self.assertEqual(
                [path.name for path in find_archives(package_dir, "linux")],
                ["PhotoCarta-0.1.0-linux-x86_64.tar.gz"],
            )

    def test_find_archives_returns_empty_without_a_matching_package(self):
        with tempfile.TemporaryDirectory() as temporary:
            self.assertEqual(find_archives(temporary, "windows"), [])

    def test_extracted_windows_package_is_located(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            executable = root / "PhotoCarta-0.1.0-windows-x86_64" / "photocarta.exe"
            executable.parent.mkdir(parents=True)
            executable.touch()
            (executable.parent / "Qt6Core.dll").touch()

            self.assertEqual(find_executable(root, "windows"), executable)

    def test_extracted_macos_bundle_is_located(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            executable = (
                root / "PhotoCarta-0.1.0-macos-arm64" / "PhotoCarta.app" / "Contents" / "MacOS" / "photocarta"
            )
            executable.parent.mkdir(parents=True)
            executable.touch()

            self.assertEqual(find_executable(root, "darwin"), executable)

    def test_extracted_linux_package_is_located(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            executable = root / "PhotoCarta-0.1.0-linux-x86_64" / "bin" / "photocarta"
            executable.parent.mkdir(parents=True)
            executable.touch()

            self.assertEqual(find_executable(root, "linux"), executable)

    def test_missing_executable_is_reported(self):
        with tempfile.TemporaryDirectory() as temporary:
            self.assertIsNone(find_executable(temporary, "linux"))

    def test_zip_package_round_trips(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = root / "source" / "PhotoCarta-0.1.0-windows-x86_64"
            source.mkdir(parents=True)
            (source / "photocarta.exe").touch()
            archive = root / "package.zip"
            shutil.make_archive(str(archive.with_suffix("")), "zip", root / "source")

            extracted = root / "extracted"
            extract_archive(archive, extracted)
            self.assertTrue((extracted / "PhotoCarta-0.1.0-windows-x86_64" / "photocarta.exe").is_file())

    def test_tarball_package_round_trips(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = root / "source" / "PhotoCarta-0.1.0-linux-x86_64"
            (source / "bin").mkdir(parents=True)
            (source / "bin" / "photocarta").touch()
            archive = Path(shutil.make_archive(str(root / "package"), "gztar", root / "source"))

            extracted = root / "extracted"
            extract_archive(archive, extracted)
            self.assertTrue((extracted / "PhotoCarta-0.1.0-linux-x86_64" / "bin" / "photocarta").is_file())


if __name__ == "__main__":
    unittest.main()
