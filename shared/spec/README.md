# spec/: language-neutral specification

Everything a port (Rust, Go, Java, TypeScript, Python) needs. Each port vendors this
directory and `data/` into its own `shared/` (ADR-0001).

| File | Normative? | Content |
|---|---|---|
| `ALGORITHM.md` | yes | serial-day model, BS↔AD, weekday, arithmetic, diff, fiscal year |
| `FORMATTING.md` | yes | names (En/Ne), short/long/pattern formats, digit localisation |
| `PARSING.md` | yes | strict and lenient parsers, closed month-name list, error kinds |
| `PATRO.md` | yes | tithi/holiday/event data model, queries, layout guidance |
| `PARITY.md` | yes | C# golden vectors and the deliberate deviations D-01 … D-15 |
| `PORT-PLAN.md` | yes | phases and acceptance criteria every port meets |
| `BENCHMARK.md` | yes | benchmark protocol: reference machine, the 64 dates, operations B1–B8, targets |
| `vectors/csharp-tests/catalog.tsv` | yes | every test case of C# NepDate's own test suite, tagged with the phase that covers it |
| `vectors/month-boundaries.csv` | yes | first and last day of every BS month (7176 rows) |
| `vectors/patro-summary.json` | yes | patro counts every port reads: coverage, tithi, holidays, events, names |
| `vectors/parse.tsv` | yes | strict + lenient parser expectations |
| `vectors/csharp-golden/*.tsv` | parity | recorded output of C# NepDate 2.0.7 (`PARITY.md`) |
| `reference/verify_vectors.py` | reference | executable version of the specs; run it after any spec/data change |

Names in examples (`FromStr`, `Lang::Ne`) come from the Rust port. Other ports use their own
idiomatic names; the results must be the same.

## Starting a port

1. Create `nepdate-<lang>` and copy the hub's `data/` and `spec/` into `shared/`, with the hub
   commit in `shared/VERSION`. Call the hub's reusable `shared-check` and `shared-sync` workflows;
   only the regenerate command differs per port.
2. Write a small generator in the port's own tooling that turns `shared/data/` into committed
   constant tables. Never hand-copy tables, never parse JSON at runtime.
3. Implement ALGORITHM §2–8 line by line.
4. Tests read `shared/spec/vectors/` directly: pass `month-boundaries.csv`, then an exhaustive
   serial round-trip (0..=109211).
5. Implement FORMATTING and PARSING, and pass `parse.tsv` and the golden columns, skipping
   only the rows `PARITY.md` lists.
6. PATRO (the separate patro package, ADR-0004), with its counts read from
   `shared/spec/vectors/patro-summary.json`.
7. Add the port to the table in the hub `README.md`.
8. No port-local deviations. If one seems necessary, open a PR to this hub.
