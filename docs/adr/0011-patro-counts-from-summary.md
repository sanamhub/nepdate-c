# ADR-0011: The patro counts come from the hub's generated summary

- Status: Accepted
- Date: 2026-10-31
- Deciders: Sanam Pakuwal
- Amends: ADR-0001, the sentence that the generator reads `shared/data/` and checks it

## Context

`tools/codegen` carried the patro counts as C constants: 2793 covered days, 2190 tithi days, 294
holidays, 1671 event days, 3032 events, 753 distinct names and 59800 name bytes, plus the coverage
years and the first serial. They were checks, not parameters: a data change that renamed one event
made the generator refuse the new data, and every port had to be edited by hand before a sync pull
request could pass. The counts are a property of the data, so the hub computes them.

## Decision

- The hub writes `spec/vectors/patro-summary.json` from `data/patro/` with `tools/make_vectors.py`.
  It holds the coverage, the tithi range and counts, the holiday, event-day and event counts, the
  distinct name counts and the name-byte total.
- The generator reads that file from `shared/spec/vectors/`, validates the parsed data against every
  value, and writes the values into `src/generated/patro_tables.h`. The library and the tests read
  the generated macros and never a count of their own.
- A missing summary fails the generator, because a `shared/` older than the summary cannot say
  which counts the data should have.
- The generator still reads `shared/data/`; the summary is a second, derived input, not a
  replacement for the data.

## Consequences

- A data update changes the generated header and nothing in the source. The generated header for
  the same data stays byte-identical.
- A summary that disagrees with the data fails the build: that means the hub's data and its summary
  are out of step and must be regenerated.
- A port whose `shared/` predates the summary must resync before its generator runs.
