# Runbook: release `nepdate-c`

Owner: maintainer. Approver: the required reviewer of the `production` environment. One tag
releases the core and the patro.

## Before tagging

1. `main` is green on every CI job, including `packages`, `amalgamation` and `isolation`.
   `cmake -P tools/ci.cmake` passes locally.
2. The version is `0.1.0-beta.1` in `CMakeLists.txt`, `include/nepdate.h`,
   `patro/CMakeLists.txt`, `ports/*/vcpkg.json` and `conan/*/conanfile.py`:
   `cmake "-DVERSION=0.1.0-beta.1" -P tools/release_check.cmake` passes.
3. `CHANGELOG.md` has `## [0.1.0-beta.1] - YYYY-MM-DD`; the release notes are that section.
4. `cmake -DCHECK=ON -P tools/amalgamate.cmake` passes.
5. The side-by-side install is fresh: install this tree and `nepdate-cpp` into one prefix, build
   `tests/side_by_side`, and see both consumers print a date. No header, library, pkg-config or
   CMake package path is shared.

## Dry run

`release.yml` also runs from `workflow_dispatch` with a `version` input. That run does preflight and
the full CI, and the `release` job is skipped because there is no tag. It proves the version and the
changelog agree without publishing anything.

The approval step is reached only by a tag push. To dry-run that path, create the tag on a throwaway
branch, push it, and delete the branch and tag before the approval:

```bash
git switch -c release-dry-run
git tag -a v0.1.0-beta.1 -m "v0.1.0-beta.1"
git push origin release-dry-run v0.1.0-beta.1
# open the release run, confirm it reaches production approval, then:
git push origin :refs/tags/v0.1.0-beta.1
git switch main
git branch -D release-dry-run
```

Do not approve the dry run. The real tag is `v0.1.0-beta.1`; pushing it is the release.

## Releasing

```bash
git tag -a v0.1.0-beta.1 -m "v0.1.0-beta.1"
git push origin v0.1.0-beta.1
```

Preflight checks the tag is on `main` and every version location; CI runs; the environment waits
for the approver; the release job creates the source archive and the amalgamation archive, their
`SHA256SUMS`, a build provenance attestation for each, and the GitHub Release with the changelog
section as notes.

## After releasing

1. Download both archives and `SHA256SUMS`; check `sha256sum -c SHA256SUMS`.
2. Compute the archive hashes for the package files:
   `curl -L -o nepdate-c-0.1.0-beta.1.tar.gz https://github.com/sanamhub/nepdate-c/archive/refs/tags/v0.1.0-beta.1.tar.gz`
   then `sha512sum` for vcpkg and `sha256sum` for Conan, and put them in the two portfiles and
   `conandata.yml` in place of the placeholders.
3. Registry submissions wait until the project meets the vcpkg and Conan Center intake rules for
   age and use; until then the overlay ports and recipes in this repository are the install path.
   When submitting, from a vcpkg fork, copy `ports/nepdate-c` and `ports/nepdate-c-patro` to `ports/`, run
   `vcpkg install --classic nepdate-c nepdate-c-patro --overlay-ports=ports`, then
   `vcpkg x-add-version --all`, and open the PR.
4. From a Conan Center fork, copy `conan/nepdate-c` and `conan/nepdate-c-patro` to
   `recipes/<name>/all/`, add `config.yml` with the version, and run `conan create` for both before
   opening the PR.
5. Verify the side-by-side install from the released tree once more, then update the hub site with
   the C page, the code tab and the benchmark rows (the hub PR).

## Rolling back

Never move or delete the tag: vcpkg and Conan pin the archive hash. Fix forward with the next
version, say in the changelog which version to avoid and why, and update the package-manager entries
to the fixed version. If the release job failed after approval, delete the draft Release if one was
created; the tag stays.
