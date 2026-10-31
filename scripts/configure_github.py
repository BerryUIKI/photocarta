"""Plan or apply this repository's GitFlow settings using authenticated gh CLI."""

import argparse
import datetime
import json
import re
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def gh_api(endpoint, method="GET", payload=None):
    command = ["gh", "api", endpoint, "--method", method]
    if payload is not None:
        command.extend(["--input", "-"])
    result = subprocess.run(
        command, input=json.dumps(payload) if payload is not None else None,
        encoding="utf-8", capture_output=True, check=False,
    )
    if result.returncode:
        raise RuntimeError(f"GitHub API failed for {method} {endpoint}: {result.stderr.strip()}")
    return json.loads(result.stdout) if result.stdout.strip() else None


def matches(expected, actual):
    """Compare managed fields, allowing GitHub to add response-only defaults."""
    if isinstance(expected, dict):
        return isinstance(actual, dict) and all(
            key in actual and matches(value, actual[key]) for key, value in expected.items()
        )
    if isinstance(expected, list):
        if not isinstance(actual, list) or len(expected) != len(actual):
            return False
        remaining = list(actual)
        for item in expected:
            for index, other in enumerate(remaining):
                if matches(item, other):
                    remaining.pop(index)
                    break
            else:
                return False
        return True
    return expected == actual


def load_configuration():
    settings = json.loads((ROOT / ".github/repository-settings.json").read_text())
    rules = [json.loads(path.read_text()) for path in sorted((ROOT / ".github/rulesets").glob("*.json"))]
    if not rules or len({rule["name"] for rule in rules}) != len(rules):
        raise ValueError("Ruleset names must be present and unique.")
    return settings, rules


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", default="BerryUIKI/photocarta")
    parser.add_argument("--apply", action="store_true", help="Write settings; default is an offline plan.")
    args = parser.parse_args()
    if not re.fullmatch(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+", args.repo):
        parser.error("--repo must be owner/name")
    settings, rules = load_configuration()
    if not args.apply:
        print(json.dumps({"repository": args.repo, "settings": settings, "rulesets": rules}, indent=2))
        return 0

    endpoint = f"repos/{args.repo}"
    repo = gh_api(endpoint)
    if not repo.get("permissions", {}).get("admin"):
        raise RuntimeError("Repository administrator access is required.")
    main_ref = gh_api(f"{endpoint}/git/ref/heads/main")
    # Full ref listing avoids treating an authorization/network failure as an absent branch.
    branches = gh_api(f"{endpoint}/git/matching-refs/heads/")
    existing = gh_api(f"{endpoint}/rulesets?per_page=100&includes_parents=false")
    if len(existing) == 100:
        raise RuntimeError("Ruleset listing may be truncated; refusing ambiguous updates.")
    names = {rule["name"] for rule in rules}
    managed = [gh_api(f"{endpoint}/rulesets/{rule['id']}") for rule in existing if rule["name"] in names]
    if len({rule["name"] for rule in managed}) != len(managed):
        raise RuntimeError("Duplicate managed ruleset names; resolve before applying.")
    backup_dir = ROOT / ".local/github-settings"
    backup_dir.mkdir(parents=True, exist_ok=True)
    timestamp = datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%dT%H%M%S%fZ")
    backup = backup_dir / f"{timestamp}.json"
    backup.write_text(json.dumps({
        "repository": args.repo,
        "settings": {key: repo.get(key) for key in settings},
        "rulesets": managed,
        "branches": [{"ref": branch["ref"], "sha": branch["object"]["sha"]} for branch in branches],
    }, indent=2) + "\n", encoding="utf-8")
    print(f"Saved previous managed settings to {backup.relative_to(ROOT)}")
    if not any(branch["ref"] == "refs/heads/dev" for branch in branches):
        gh_api(f"{endpoint}/git/refs", "POST", {"ref": "refs/heads/dev", "sha": main_ref["object"]["sha"]})
        print("Created dev from main.")
    if not matches(settings, repo):
        gh_api(endpoint, "PATCH", settings)
        print("Updated merge settings.")
    by_name = {rule["name"]: rule for rule in managed}
    for desired in rules:
        current = by_name.get(desired["name"])
        if current and matches(desired, current):
            print(f"Already matches: {desired['name']}")
            continue
        target = f"{endpoint}/rulesets" + (f"/{current['id']}" if current else "")
        result = gh_api(target, "PUT" if current else "POST", desired)
        verified = gh_api(f"{endpoint}/rulesets/{result['id']}")
        if not matches(desired, verified):
            raise RuntimeError(f"Read-back mismatch for {desired['name']}; inspect GitHub settings.")
        print(f"Applied and verified: {desired['name']} (id={result['id']})")
    if not matches(settings, gh_api(endpoint)):
        raise RuntimeError("Repository merge settings did not match after application.")
    print("GitFlow settings verified. Unrelated rulesets were left untouched.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
