## How to contribute
We welcome anyone who wants to contribute to this project, and we thank you for all your contributions.

To keep reviews manageable, pull requests (PRs) are accepted for review only if they follow the process below. **PRs that do not follow these guidelines may be closed without review.**

### Contribute as a Community
- You can answer questions in [the issue tracker](https://github.com/fxsound2/fxsound-app/issues), or in [the fxsound forum](https://forum.fxsound.com/).
- You can report bugs and request features by [filing an issue](https://github.com/fxsound2/fxsound-app/issues/new). Issues are the starting point for every code contribution.

### Contribute as a Developer

#### 1. Every contribution must start from a GitHub issue
- Contributions are accepted **only for issues filed in [GitHub issues](https://github.com/fxsound2/fxsound-app/issues)**.
- If you want to fix a bug or add a feature that has no issue yet, file an issue first and wait for it to be triaged before you start working on it.
- PRs that are not linked to an issue will not be reviewed.

#### 2. Work only on unassigned issues
- Pick an issue that is **not assigned** to anyone.
- Comment on the issue to say you would like to work on it, so that others know and work is not duplicated.
- Do not open PRs for issues that are already assigned to someone else.

Some good places to start (choose unassigned issues only):
- [Issues labelled 'Good First Issue'](https://github.com/fxsound2/fxsound-app/issues?q=is%3Aopen+is%3Aissue+no%3Aassignee+label%3A%22good+first+issue%22): small, well-scoped issues for first-time contributors.
- [Issues labelled 'Help Wanted'](https://github.com/fxsound2/fxsound-app/issues?q=is%3Aopen+is%3Aissue+no%3Aassignee+label%3A%22help+wanted%22): issues where the maintainers would welcome outside help.
- [Issues labelled 'Bug'](https://github.com/fxsound2/fxsound-app/issues?q=is%3Aopen+is%3Aissue+no%3Aassignee+label%3Abug): fixes that have an immediate impact.
- [Issues labelled 'Enhancement'](https://github.com/fxsound2/fxsound-app/issues?q=is%3Aopen+is%3Aissue+no%3Aassignee+label%3Aenhancement): these need discussion on the issue before any code is written.

#### 3. Reviews follow issue priority and milestone
- PRs are reviewed and merged **based on the priority and milestone of the linked issue**, not in the order they are submitted.
- PRs for high-priority issues, or issues in the current milestone, are reviewed first. PRs for lower-priority issues may wait until their milestone is taken up.

#### 4. Submitting a pull request
1. Fork the repository and create a temporary branch from the appropriate `develop/<platform>` branch (see [Branch Naming Convention](#branch-naming-convention)).
2. Make your changes, following the [Commit Messages](#commit-messages) and [Coding Style](#coding-style) guidelines.
3. Open a PR against the same `develop/<platform>` branch you branched from. Do not open PRs against `main` or `release/<platform>`.
4. Link the issue in the PR description (for example, `Fixes #123`), and describe what you changed and how you tested it.
5. Keep each PR focused on a single issue. Unrelated changes, large reformatting, or bulk refactors in the same PR will delay or block the review.
6. You are responsible for the code you submit, including code written with AI assistance. Make sure you understand, build and test every change before opening the PR.

## Branch Naming Convention

### Regular Git Branches
Regular branches are long-lived branches that are permanently available in the repository.

| Branch | Purpose |
|---|---|
| `main` | The stable production branch. Changes reach `main` only through a release branch, after review and testing. |
| `develop/<platform>` | The development branch for a platform, for example `develop/windows`. All feature and bugfix PRs target this branch. |
| `release/<platform>` | The release branch for a platform, for example `release/windows`. Used to stabilise and test a release before it is merged into `main`. |

### Temporary Git Branches
Temporary branches are short-lived and are deleted after their PR is merged. Create them from the `develop/<platform>` branch you plan to merge into.

| Branch | Purpose | Example |
|---|---|---|
| `bugfix/<short-bug-title>` | Fixing a bug reported in an issue | `bugfix/preset-not-saved` |
| `feature/<short-feature-title>` | Adding, changing or removing a feature requested in an issue | `feature/output-device-hotkey` |

Guidelines for temporary branch names:
- Use lowercase words separated by hyphens (`-`).
- Keep the title short but descriptive; avoid names made of numbers only.

## Commit Messages

Every commit message **must include the GitHub issue id** in the form `[#<issue id>]`.

We also follow [conventional commits syntax](https://www.conventionalcommits.org/):
- **fix:** a commit that patches a bug.
- **feat:** a commit that introduces a new feature.
- Other types such as `build:`, `chore:`, `ci:`, `docs:`, `style:`, `refactor:`, `perf:` and `test:` are allowed.
- A commit that introduces a breaking change appends `!` after the type, or has a `BREAKING CHANGE:` footer.

Commit messages are written in lowercase.

Examples:
- ✅ `fix: [#123] preset is not saved after renaming`
- ✅ `feat: [#456] add hotkey to switch output device`
- ❌ `fix: preset is not saved after renaming` (no issue id)
- ❌ `Fix: [#123] Preset is not saved after renaming` (not lowercase)

## Coding Style
Please follow the coding style of the module you are contributing to (FxSound_App, audiopassthru, DfxDsp). FxSound_App, which is developed in C++, follows the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html).

Changes to `audiopassthru` and `DfxDsp` affect system audio and real-time audio processing, so they get closer review. Explain the reason for such changes clearly in the issue and the PR.
