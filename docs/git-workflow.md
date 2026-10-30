# Working from a fork

The setup used to contribute to zuru-ranges, for anyone who has not worked on a project they
cannot push to. The policy (what a pull request must contain, how review goes) is in
[CONTRIBUTING.md](../CONTRIBUTING.md); this page is only the git side of it.

## Two remotes

You cannot push to `zurutech/zuru-ranges`. You push to your own copy of it on GitHub, your
fork, and open a pull request from there. So every clone talks to two repositories:

| Remote | Repository | Used for |
|---|---|---|
| `upstream` | `zurutech/zuru-ranges` | fetching: the `main` everything is based on |
| `origin` | `<you>/zuru-ranges`, your fork | pushing: your branches, the source of your pull requests |

`main` is never yours to change: it mirrors `upstream/main`, and your work lives on branches
started from it.

## One-time setup

Fork the repository on GitHub (the Fork button on <https://github.com/zurutech/zuru-ranges>),
then:

```sh
git clone git@github.com:<you>/zuru-ranges.git
cd zuru-ranges
git remote add upstream git@github.com:zurutech/zuru-ranges.git
git remote set-url --push upstream no_push        # fetch from upstream, never push to it
git fetch upstream
git branch --set-upstream-to=upstream/main main   # main follows upstream, not the fork
```

`gh repo fork zurutech/zuru-ranges --clone` does the fork, the clone and the `upstream` remote
in one go; run the last three lines after it.

Then the configuration that makes `git push` and `git rebase` do the right thing without
arguments (`--global` applies it to every repository):

```sh
git config remote.pushDefault origin   # `git push` goes to the fork ...
git config push.default current        # ... to the branch of the same name
git config fetch.prune true            # forget branches deleted on GitHub
git config rerere.enabled true         # remember conflict resolutions (see Rebasing)
git config rerere.autoupdate true
```

`git remote -v` now shows `upstream` with the push URL `no_push`: a `git push upstream` fails
instead of landing somewhere it should not. The commit email must be public; see
"Public email" in CONTRIBUTING.md.

## Keeping main a mirror

```sh
git switch main
git pull --ff-only     # from upstream/main
git push               # to origin/main, so the fork's main does not fall behind (optional)
```

`--ff-only` refuses rather than merge: if it fails, `main` has commits of its own, which it
should never have (see "Recovering" below).

## Starting a change

```sh
git fetch upstream
git switch -c concat-reserve-hint upstream/main
```

The branch starts from today's `upstream/main`, not from your fork's possibly stale `main`,
and it tracks `upstream/main`: `git status` says how far ahead of upstream you are, and
`git rebase` with no argument rebases onto it. `git branch -vv` shows the tracking.

## Pushing and opening the pull request

```sh
git push                      # creates origin/concat-reserve-hint
gh pr create --draft --web    # or the "Compare & pull request" banner GitHub shows
```

Do not use `git push -u`: it would make the branch track the fork instead of upstream.
Once the branch exists on the fork, `git log @{push}..` lists the commits not yet pushed to
it, and `git log @{upstream}..` the ones not yet in upstream's `main`.

## Rebasing

`main` moves while you work. Put your commits back on top of it while the pull request is a
draft, one last time right before marking it ready for review, and after that only to resolve
a conflict or when the reviewer asks (CONTRIBUTING.md, Review):

```sh
git fetch upstream
git rebase                    # onto upstream/main
git push --force-with-lease   # a rewritten branch needs a forced push
```

`--force-with-lease` refuses if `origin` has commits you have not seen, which a plain `--force`
would delete. Rebasing is preferred to merging `main` into the branch: the branch stays a
clean series on top of `main`, and the pull request shows only your change.

When a commit conflicts, git stops: `git status` lists the files, edit them, `git add` each,
`git rebase --continue`; `git rebase --abort` returns to where you started. Rebasing again
later would replay the same conflicts, which is what `rerere` (reuse recorded resolution)
avoids: it records how you resolved each one and applies the same resolution next time, and
with `rerere.autoupdate` stages the result. `git rerere forget <file>` drops a recorded
resolution that was wrong.

## Worktrees

A C++ build belongs to one checkout, and switching branches in it invalidates the build. A
worktree is a second checkout of the same repository in another directory, with its own branch
and its own `build/`:

```sh
git worktree add -b fold-right ../zuru-ranges-fold-right upstream/main   # a new branch
git worktree add ../zuru-ranges-review concat-reserve-hint               # an existing one
git worktree list
git worktree remove ../zuru-ranges-fold-right    # when the pull request has landed
git branch -d fold-right
```

One worktree per pull request keeps each build warm. A branch can be checked out in one
worktree at a time; `build/` is ignored by git in each, and `scripts/make.sh -B <dir>` names
another build directory if you need several per worktree.

## Looking at someone else's pull request

```sh
gh pr checkout 42    # the branch behind pull request 42, from a fork or not
```

## Recovering

- Committed on `main`: `git switch -c <topic>` takes the commits to a new branch, then
  `git switch main && git reset --hard upstream/main`.
- Branched from a stale or dirty `main`: `git rebase upstream/main`; if `main` had commits of
  its own, `git rebase --onto upstream/main origin/main`.
- `git pull` made a merge commit: `git rebase upstream/main` drops it.
- Pushed to upstream: impossible, the push URL is `no_push`.
- Lost commits after a rebase: `git reflog` lists where the branch was; `git reset --hard
  <hash>` goes back.
