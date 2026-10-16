"""Dependency-free checks for the current documentation and policy repository."""

import json
import re
import subprocess
from pathlib import Path
from urllib.parse import unquote


ROOT = Path(__file__).resolve().parents[1]


def main():
    errors = []
    required = [
        "README.md", "AGENTS.md", "CONTRIBUTING.md", "LICENSE", ".gitignore",
        ".gitattributes", ".editorconfig", ".clang-format", "docs/gitflow.md",
        ".github/CODEOWNERS", ".github/workflows/ci.yml",
    ]
    for name in required:
        if not (ROOT / name).is_file():
            errors.append(f"Missing file: {name}")
    result = subprocess.run(
        ["git", "ls-files", "--cached", "--others", "--exclude-standard", "-z"],
        cwd=ROOT, capture_output=True, check=True,
    )
    names = sorted(set(result.stdout.decode("utf-8").strip("\0").split("\0")))
    text_suffixes = {".md", ".py", ".json", ".yml", ".yaml", ".toml"}
    for name in filter(None, names):
        path = ROOT / name
        if not path.is_file() or (path.suffix not in text_suffixes and name not in required):
            continue
        try:
            content = path.read_text(encoding="utf-8")
        except UnicodeError:
            errors.append(f"Not UTF-8: {name}")
            continue
        if not content.endswith("\n"):
            errors.append(f"Missing final newline: {name}")
        if path.name != "LICENSE":
            for number, line in enumerate(content.splitlines(), 1):
                if line.rstrip() != line:
                    errors.append(f"Trailing whitespace: {name}:{number}")
        if path.suffix == ".json":
            try:
                json.loads(content)
            except json.JSONDecodeError as error:
                errors.append(f"Invalid JSON: {name}: {error}")
        if path.suffix == ".md":
            for link in re.findall(r"\]\(([^)]+)\)", content):
                if re.match(r"^[a-zA-Z][a-zA-Z0-9+.-]*:", link) or link.startswith("#"):
                    continue
                target = unquote(link.split("#", 1)[0])
                if target and not (path.parent / target).exists():
                    errors.append(f"Broken local link: {name}: {link}")
    for error in errors:
        print(error)
    if errors:
        return 1
    print(f"Repository checks passed ({len(names)} tracked or new files inspected).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
