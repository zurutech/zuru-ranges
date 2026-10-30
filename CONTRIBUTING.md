# Contributing to zuru-ranges

Thanks for helping. This page is the policy: what is accepted, how a change is proposed,
reviewed and landed. The git mechanics of working from a fork (remotes, rebasing, worktrees)
are in [docs/git-workflow.md](docs/git-workflow.md). Who reviews is in
[MAINTAINERS.md](MAINTAINERS.md).

The rules are the same for everyone, inside ZURU or not: changes reach `main` through a pull
request from a fork, reviewed by a maintainer. A ruleset on `main` enforces it.

## What to contribute

- Conformance: a component that drifted from the current working draft, a resolved LWG issue
  not yet applied, a paper adopted into the draft. The README lists what is implemented.
- Unreal: a container or engine version the traits do not cover, or a quirk the fakes do not
  reproduce.
- Toolchains: a workaround for a compiler in the CI matrix, or a new row of it.
- Fixes, each with the test that would have caught the bug.

Open an issue first for anything that adds a non-standard component or changes a documented
deviation; the answer may be a pointer to a paper, or the reason it cannot be done with Unreal
iterators. Formatting-only changes are not accepted (`.pre-commit-config.yaml` says why), and
dependencies are not added.

## Reporting a bug

Open an issue with the compiler and standard library (versions and `-std`), the Unreal
container and build configuration if one is involved, and a reduced program: a `static_assert`
or a GoogleTest body that fails. The paragraph of the draft that the library contradicts is the
best report of all.

## Workflow

1. Fork the repository, clone the fork, add `zurutech/zuru-ranges` as `upstream`
   ([docs/git-workflow.md](docs/git-workflow.md)).
2. Branch from `upstream/main`, never from your fork's `main`. One logical change per branch.
3. Build and test locally (`scripts/make.sh -W`, see [Tests](#tests)) and run `pre-commit`.
4. Open a draft pull request against `main`. When CI is green, rebase onto the current
   `upstream/main` one last time, then mark it ready for review: GitHub requests the
   maintainers' review at that moment (`.github/CODEOWNERS`), so there is no one to ping.
5. Address the review with new commits. From here on, rebase only to resolve conflicts.
6. A maintainer squashes and merges.

## Pull request title

The title becomes the subject of the commit on `main`:

```
[<tag>] <summary>
```

`<tag>` is the library directory the change lives in, which mirrors the draft's clause:
`[ranges]`, `[algorithm]`, `[iterator]`, `[unreal]` (`zr/unreal/`); for the rest `[cmake]`,
`[ci]`, `[test]`, `[docs]`. A stable name from the draft is fine when it is more precise:
`[range.concat]`.

`<summary>` starts with the paper or LWG issue the change implements, when there is one, then
says what changed, in the imperative, without a trailing period. Keep the whole line under 72
characters.

```
[ranges] P2542R8: views::concat
[ranges] LWG3715: view_interface::empty is overconstrained
[algorithm] P1223R5: ranges::find_last, find_last_if, find_last_if_not
[unreal] range_traits for TSparseArray in UE 5.3
[ci] Build clang 18 over the engine's libc++ 15
```

## Pull request description

The description becomes the body of that commit, so write it for `git log`, not for the
review: what changed and why, in a few sentences, wrapped at 72 columns. Name the wording
followed (clause, paper, issue), every deviation added and its reason, and the compiler or
standard library that needed a workaround. Leave out how you tested, lists of files,
checklists, and anything true only during the review; `Fixes #N` closes the issue. Edit the
description if the change moved during review: it is what gets merged, the branch's own commit
messages are discarded.

## Code

- The current working draft is the specification, not C++20 and not a particular standard
  library: names, template heads, constraints, `noexcept`, members and semantics, including
  the exposition-only helpers. Each header names its clause; new code cites the clause, paper
  or issue it follows (`[range.take]`, `P2846R6`, `LWG3715`).
- Deviations exist only for Unreal containers, are marked `// DEVIATION:` with the reason, and
  are listed in the README when they recur.
- Headers are self-contained (CI compiles each one alone), `#pragma once`, include what they
  use, and stay free of Unreal headers outside `zr/unreal/`. The module (`modules/zr.cppm`)
  re-exports names: a new public name is added there too.
- Everything is `constexpr` that the draft makes `constexpr`, and it is checked in
  `test/compile/constexpr_checks.cpp`.
- Match the surrounding code. There is no clang-format on purpose; `pre-commit` covers
  whitespace, YAML, CMake and shell lint. Comments say what is not obvious from the code, not
  what the code does.
- No Epic Games source anywhere, not even in the fakes: the licensing boundary in the README
  is what lets this repository be public. Unreal behaviour is mirrored in `test/fake-unreal/`
  with original code.
- The change builds warning-free on every row of the CI matrix (README, Requirements). A
  workaround for one toolchain is guarded by that toolchain's version and says which bug it
  works around.

## Tests

Every change in behaviour comes with the test that fails without it, in the same pull request.
Test the change, not its surroundings: one `static_assert` per requirement the change adds or
alters, one `TEST` per behaviour, a std container plus the Unreal containers whose quirks the
change touches, and nothing an existing test already covers. Small and readable beats
exhaustive.

- Classification (concepts, `range_traits`, iterator categories, constant evaluation): a
  `static_assert` in the matching file under `test/compile/`.
- Behaviour: GoogleTest in `test/ranges/`, `test/iterator/` or `test/algorithm/`, in the file
  named after the header (`views/take_view.h` is tested by `test/ranges/take_drop_view.cpp`).
  A new component gets a new file; the glob in `test/CMakeLists.txt` picks it up.
- A requirement of the draft: `test/ranges/conformance.cpp`, one test per requirement;
  interoperability with the std adaptors: `test/ranges/std_interop.cpp`.
- An Unreal quirk: the fake that reproduces it in `test/fake-unreal/` (documented in its
  README), and a test that passes under both iterator configurations.

Each test file is built three times (shipping and debug iterators, and debug against the UE
5.3 mocks) and every header once on its own; nothing opts in. Before asking for review run
`scripts/make.sh -W`; `-s` adds the sanitizers, `-c clang++ --libcxx` the other standard
library, `-m` the module build (README, Building the tests).

## Review

Keep the pull request a draft until CI is green. Then align the branch with the current
`upstream/main` (rebase, push with `--force-with-lease`; docs/git-workflow.md, Rebasing), so
the review starts from a branch that merges cleanly, and mark it ready: the review request
goes out by itself. Expect an answer within a few working days.

From that point on, push new commits and do not rebase, squash or amend. The reviewer follows
the progression of the branch, commit by commit, since their last pass; a force push takes
that away and dismisses the approval. The two exceptions are a conflict with `main` (rebase,
push with `--force-with-lease`, say so in a comment) and the reviewer asking for it. The
commits are squashed at merge, so their number and their messages do not matter.

Answer every comment, with the commit that addresses it or the reason not to. Resolve a
conversation once the change it asks for is pushed; the reviewer reopens it if it is not
settled. The merge is blocked while any conversation is open.

## Landing

A maintainer merges with "Squash and merge": the pull request title is the commit subject, the
description its body, the author is you. Dependent changes go in one pull request at a time:
open the next one when the previous one has landed, or open it as a draft that says which
pull request it is stacked on.

## Public email

The author of every commit on `main` must be reachable: `git blame` is how a maintainer, or
anyone else, asks about a line years later. GitHub's "Keep my email addresses private" setting
replaces your address with `<id>+<login>@users.noreply.github.com`, both on commits pushed
with it and on the squashed commit, whose author email is the one of the GitHub account that
opened the pull request. Turn the setting off at <https://github.com/settings/emails> before
opening a pull request, and commit with that same address (`git config user.email`). CI fails a
pull request whose commits carry a noreply address; the squashed commit cannot be checked
before the merge, so the account setting is the part that matters.

## License

Contributions are licensed under the Apache License 2.0, like the project (`LICENSE`,
`NOTICE`); section 5 of the license is the whole agreement, there is no CLA to sign. New files
start with the same header as the existing ones; a copyright line of your own may precede the
`SPDX-License-Identifier` line.
