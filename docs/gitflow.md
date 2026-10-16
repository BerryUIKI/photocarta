# GitFlow and Repository Protection

## Branch roles

| Branch | Starts from | PR destination | Purpose |
| --- | --- | --- | --- |
| `main` | Existing stable history | Not a work branch | Stable/released history; remains the default branch. |
| `develop` | `main` during bootstrap | Release branch when ready | Day-to-day integration. |
| `feature/<topic>` | `develop` | `develop` | Product work. |
| `bugfix/<topic>` | `develop` | `develop` or active stabilization branch | Non-emergency fixes. |
| `chore/<topic>`, `docs/<topic>`, `ci/<topic>` | `develop` | `develop` | Maintenance. |
| `release/<version>` | `develop` | `main`, then `develop` | Stabilization and release preparation. |
| `hotfix/<version>` | `main` | `main`, then `develop` | Urgent stable-line fixes. |

Agents use `codex/feature/<topic>`, `codex/bugfix/<topic>`, or
`codex/chore/<topic>`; the CI route policy recognizes the prefix. It also accepts
focused `fix/`, `refactor/`, `test/`, `perf/`, and Dependabot work into `develop`.

## Normal work

```sh
git fetch origin
git switch develop
git pull --ff-only origin develop
git switch -c feature/photo-grid
# Implement and validate the change.
git add <specific-paths>
git commit -m "feat: redesign the photo grid"
git push -u origin feature/photo-grid
gh pr create --base develop --title "Redesign the photo grid" --body-file pr-body.md
```

Use a merge commit for the PR. Avoid rebasing shared branches. The default branch
is `main`, so explicitly select `develop` when opening normal contribution PRs.

## Releases and hotfixes

Create `release/<version>` from current `develop`. Limit changes during
stabilization to fixes and release preparation. Open a same-repository PR into
`main` and merge with a merge commit after checks pass. Fetch the resulting
`main`, then merge `origin/main` into the still-existing release branch, push it,
and PR that branch back into `develop`. This carries the stable merge ancestry
back as well as any release fixes. A `main` -> `develop` back-merge PR is also
accepted. Do not squash either integration.

For an urgent fix, create `hotfix/<version>` from `main`, PR into `main`, then
carry the resulting `main` ancestry back to `develop` in the same way. Apply a
relevant fix to any active release branch through a focused fix PR if needed.

Tag the approved release commit on `main` using an annotated `v<version>` tag.
Creating a public release and uploading packages are separate release actions;
project initialization does not publish an application release. Delete temporary
branches only after their fixes have reached every required target. Automatic
head-branch deletion is disabled to preserve release/hotfix back-merge sources.

## Enforced GitHub rules

Version-controlled payloads live in `.github/rulesets/` and
`.github/repository-settings.json`. Files alone do not activate GitHub protection;
the administrator must apply them and verify the server response.

| Policy | `main` | `develop` | `release/*`, `hotfix/*` |
| --- | --- | --- | --- |
| PR required | Yes | Yes | No; stabilization commits allowed |
| Required CI | `Required checks` | `Required checks` | Runs on pushes; required when entering stable/integration |
| Up-to-date PR required | Yes | Yes | Not applicable |
| Force pushes | Blocked | Blocked | Blocked |
| Branch deletion | Blocked | Blocked | Allowed after completion |
| Merge method | Merge commit | Merge commit | Repository merge-commit policy |
| Unresolved review discussions | Block merge | Block merge | Not applicable |
| Automatic bypass | None | None | None |

There is currently one maintainer. Required independent approvals are set to zero
so the maintainer can merge their own checked PRs. CODEOWNERS routes review but
does not make approval mandatory. Increase the approval count in both payloads
when another maintainer is available, then reapply the configuration.

The CI route check accepts only same-repository release/hotfix branches into
`main`. Normal work targets `develop`. This is CI-enforced workflow policy, not
a server-side assertion that a branch was originally created from a particular
commit. Maintainers must still verify release contents and ancestry.

## Applying and verifying configuration

Requirements: Python 3.12+, authenticated GitHub CLI, and repository administrator
access. Run from this checkout:

```sh
python scripts/configure_github.py
python scripts/configure_github.py --apply
```

The first command prints an offline plan. The second saves the previous managed
settings under ignored `.local/github-settings/`, creates `develop` from `main`
only if absent, configures merge methods, and creates or updates the three named
rulesets. It reads settings back to verify them and preserves unrelated rulesets.
Reapplying matching settings does not recreate rulesets or reset branches.

Inspect the active result:

```sh
gh api repos/BerryUIKI/photocarta/rulesets
gh api repos/BerryUIKI/photocarta/rules/branches/main
gh api repos/BerryUIKI/photocarta/rules/branches/develop
```

If a write fails partway through, stop, inspect the error and saved snapshot, and
reapply after resolving the cause. The operation is not a transaction. Do not
automatically disable protections to recover from a failing check.

Bootstrap changes are prepared on `codex/chore/project-initialization` and submitted
to `develop`. Promote the agreed baseline into `main` through a release preparation
PR when appropriate. Current CI checks repository files and policy on Linux,
Windows, and macOS; native application builds are added when application source
and build targets exist.

## References

- [GitHub ruleset API](https://docs.github.com/en/rest/repos/rules)
- [Available rules](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/managing-rulesets/available-rules-for-rulesets)
