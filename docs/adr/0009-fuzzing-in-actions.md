# ADR-0009: Fuzzing with libFuzzer in GitHub Actions

- Status: Accepted
- Date: 2026-10-25
- Deciders: Sanam Pakuwal
- Amends: ADR-0004, CI jobs

## Context

The parsers and the pattern formatter take arbitrary bytes, so they are fuzzed on every pull
request with ASan and UBSan, nightly for longer, with the corpus kept between runs. The roadmap's
OSS-Fuzz exclusion still stands: the project has no large user base.

ClusterFuzzLite is the usual GitHub Actions wrapper. It needs a `.clusterfuzzlite/` directory with a
Dockerfile and a build script, runs its own batch and continuous modes, and reports coverage in its
own format. The repository already builds with CMake and the fuzz preset, and libFuzzer prints
executions per second and coverage in its final statistics.

## Decision

The `fuzz` workflow builds the three targets of `tests/fuzz/` with the `fuzz` preset and runs each
under libFuzzer with ASan and UBSan. A pull request runs each target for 60 seconds; the nightly
schedule runs each for 10 minutes. The seed corpus is generated at configure time from
`spec/vectors/parse.tsv` and `spec/vectors/format-pattern.tsv`, so no generated corpus is
committed. `actions/cache/restore` and `actions/cache/save` keep `build/fuzz/fuzz/corpus/<target>`
between runs, keyed by target and run; a matching restore uses the newest prefix. A crashing input
stays in `build/fuzz/artifacts/<target>/` and is uploaded when a run fails, so it can be minimised
into a regression test under `tests/`.

The workflow pins every action to a commit SHA with its version in a comment and gives the job
`contents: read`, as ADR-0004 requires. The `.clusterfuzzlite/` directory is not added because the
direct libFuzzer run already provides the required build, sanitizers, time budget, statistics and
corpus persistence.

## Consequences

- The job log ends with libFuzzer's `stat::number_of_executed_units`, `exec/s` and `cov` lines.
- The corpus lives in the Actions cache, not in the tree; a cold cache starts from the vectors.
- A schedule run costs 30 target-minutes, a pull request 3.
- ClusterFuzzLite can be added later without changing the targets if batch triage is wanted.
