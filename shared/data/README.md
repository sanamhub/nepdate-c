# data/: canonical calendar data

Single source of truth for every NepDate port (ADR-0001, ADR-0002). Ports vendor it into
`shared/data/` and read it only at dev time with their own generator. No package parses these
files at runtime.

| File | Content |
|---|---|
| `calendar/bs-calendar.json` | `epoch` (1901-01-01 BS = 1844-04-11 AD) + `month_lengths` for BS 1901–2199 (12 values each, 29–32) |
| `patro/<year>.json` | per-day tithi, public-holiday flag and events (BS 2077–2084) |
| `UPSTREAM` | the NepDate commit last imported, whether or not data changed |
| `NOTICE` | MIT notice of the upstream NepDate project, rendered from `tools/NOTICE.template` by the importer |

Rules:
- Data comes from C# NepDate through the weekly sync PR. Hand edits only when upstream can't
  take a fix in time, with a source for every change. Procedure, sources and one-time setup:
  [`docs/runbooks/data-update.md`](https://github.com/sanamhub/nepdate/blob/main/docs/runbooks/data-update.md).
- After any change: `py tools/make_vectors.py`, then `py spec/reference/verify_vectors.py`.
  Month-length changes also need a maintainer decision about the C# golden vectors.
- Accuracy: `data/` matches C# NepDate exactly (every day, both directions, every metadata day;
  `tools/compare_exhaustive.py`). Whether NepDate's own data matches the official calendar is a
  separate question. Month lengths for years beyond the officially published calendar are
  projections inherited from NepDate and may change.

## Known data questions (to raise upstream)

| Years | Finding | Likely cause |
|---|---|---|
| Metadata provenance (`data/patro/`) | Upstream's generator projects are git-ignored, so the source and licence of the tithi, holiday and event data aren't published | confirm the source and terms before any port ships `nepdate-patro` in a public release; if redistribution isn't allowed, rebuild from primary sources (the government holiday notice and the Panchanga Samiti panchang). See `docs/upstream/proposals.md`, U-04 |
| Metadata range | Upstream's changelog and XML docs say metadata covers BS 2001–2089; the data covers 2077-01-01 to 2084-08-20 (the 2001–2089 figure is the range of its offset table) | documentation error upstream (U-05) |
| Tithi `*` / "(K)" | 152 tithi days in BS 2078–2081 have names ending in `*` (np) and " (K)" (en). Nothing upstream defines the mark, and it does not match skipped or repeated tithis in the data | from upstream's unpublished data source (U-04). Ask what it means (U-09); ports show the name unchanged |
| BS 2087–2090 | 2087 has **367** days and 2090 has **364**; every other year has 365 or 366. BS New Year 2088 falls on AD 2031-04-16, two days later than its neighbours (Apr 14), and 2089–2090 on Apr 15 | probably one day too many in the 2087 projection, taken back in 2090. If so, conversions from about BS 2087-12 to 2090-12 are off by one or two days. Not verified against an official calendar (not yet published for those years) |

Checks behind this table (`py tools/data_sanity.py`; `data/known-questions.json` lists the same
entries for the script): the BS year in each "नयाँ वर्ष" event on 1 Baishakh, year lengths (364: 1 year, 365: 221, 366: 76, 367: 1) and the AD date of
every BS New Year compared with the median of its ten neighbours (only 2088 is off by 2 days).
Known correct anchors all match: 2000-01-01 = 1943-04-14, 2065-02-15 = 2008-05-28,
2072-06-03 = 2015-09-20, 2077-01-01 = 2020-04-13, 2080-01-01 = 2023-04-14,
2081-01-01 = 2024-04-13, 2082-01-01 = 2025-04-14, 2083-01-01 = 2026-04-14.

Schema gap: holidays for some people only (Kathmandu Valley, women employees, one community) are
marked only in the event name, and holidays the government announces late are missing until the
next sync. Proposed fix: schema 2 with `scope` and `tbd` fields (`docs/plan/00-ROADMAP.md`).
