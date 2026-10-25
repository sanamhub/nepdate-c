<!-- What this changes and why, in two or three sentences. -->

## What to look at first

-

## How it was tested

## Checks

- [ ] `cmake -P tools/ci.cmake` passes; last line:
- [ ] Tests added or updated for the change; the shared vector suites pass
- [ ] No exported name beyond `docs/API.md`; the core library needs nothing from libc
- [ ] `CHANGELOG.md` `[Unreleased]` updated, if a user would notice
