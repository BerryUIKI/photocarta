"""Validate pull-request routes without executing untrusted event content."""

import json
import os
import re
from pathlib import Path


WORK_TYPES = {"feature", "bugfix", "fix", "chore", "docs", "ci", "refactor", "test", "perf"}


def branch_kind(branch):
    if branch.startswith("codex/"):
        branch = branch.removeprefix("codex/")
    match = re.fullmatch(r"([a-z]+)/([A-Za-z0-9][A-Za-z0-9._/-]*)", branch)
    return match.group(1) if match else None


def validate_route(base, head, same_repository=True):
    """Return an error message, or None for an allowed route."""
    kind = branch_kind(head)
    stabilization = kind in {"release", "hotfix"} and head.startswith(("release/", "hotfix/"))
    if base == "main":
        if same_repository and stabilization:
            return None
        return "main accepts same-repository release/* or hotfix/* PRs only."
    if base == "develop":
        if kind in WORK_TYPES or kind == "dependabot":
            return None
        if same_repository and (head == "main" or stabilization):
            return None
        return "develop accepts work branches, or same-repository release/hotfix/main back-merges."
    if branch_kind(base) in {"release", "hotfix"}:
        if kind in {"bugfix", "fix", "chore", "docs", "ci", "test"}:
            return None
        return "Stabilization branches accept focused fixes, documentation, CI, and tests."
    return "Choose develop, main, release/*, or hotfix/* as the PR target."


def main():
    event_path = os.environ.get("GITHUB_EVENT_PATH")
    if not event_path:
        print("No GitHub event provided; PR routing is checked in CI and unit tests.")
        return 0
    event = json.loads(Path(event_path).read_text(encoding="utf-8"))
    pr = event.get("pull_request")
    if not pr:
        print("Not a pull-request event; no route to validate.")
        return 0
    same_repository = pr["head"]["repo"] is not None and (
        pr["head"]["repo"]["full_name"] == pr["base"]["repo"]["full_name"]
    )
    error = validate_route(pr["base"]["ref"], pr["head"]["ref"], same_repository)
    if error:
        print(error)
        return 1
    print("GitFlow PR route is valid.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
