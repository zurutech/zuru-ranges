# Maintainers

Maintainers review and merge pull requests, triage issues and keep CI green. They are the
owners in `.github/CODEOWNERS`: GitHub requests their review when a pull request is opened or
leaves draft, and the ruleset on `main` requires their approval.

## Lead maintainer

Responsible for the project as a whole, and for any area without a maintainer of its own.

R. Aoto
aotodev@gmail.com (email), [aotodev](https://github.com/aotodev) (GitHub)

## Areas

None yet beyond the lead maintainer. An area that gains one is listed here, as in
[libc++'s Maintainers.md](https://github.com/llvm/llvm-project/blob/main/libcxx/Maintainers.md):
the area, then each maintainer's name, public email and GitHub account, and the matching
paths in `CODEOWNERS`.

## Becoming a maintainer

A regular contributor with several reviewed changes landed in an area may volunteer, or be
nominated, in a pull request that adds them here and to `CODEOWNERS`. The lead maintainer
accepts it. A maintainer who has been inactive for a year is moved to an "Inactive" list by
the same route.

## Landing a pull request

1. The title and description are what the commit on `main` will say (CONTRIBUTING.md); fix
   them before merging, not after.
2. CI is green, every conversation is resolved, the approval is on the last push.
3. "Squash and merge". The author of the squashed commit is the account that opened the pull
   request, with its account email; if that is a noreply address the author has not turned
   off "Keep my email addresses private", and the merge waits until they do.
