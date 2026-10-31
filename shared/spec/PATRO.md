# Patro: spec (v1)

The patro (पात्रो, the Nepali almanac): tithi (lunar day), gazetted public holidays and events per
BS day. Every port ships it in a separate package that depends on the core package; package names
per language are in ADR-0004. Source:
`data/patro/<year>.json` (ADR-0002). The covered years and the first and last day of every kind are
in `spec/vectors/patro-summary.json`, derived from the data. Days without data are absent from the
files.

## 1. JSON shape (input)

```json
{ "schema": 1, "source": {...}, "year": 2081,
  "days": {
    "2081-01-01": {
      "tithi":   { "np": "पञ्चमी", "en": "Panchami" },
      "holiday": true,
      "events":  [ { "np": "नयाँ वर्ष", "en": "New Year" }, ... ]
    } } }
```

All three fields are optional per day. `holiday` absent = `false`. Event order is significant
(display order).

## 2. Query semantics (every port)

```
info(date) -> CalendarInfo {
    tithi:     Option<{np, en}>
    is_public_holiday: bool
    events:    ordered list of {np, en} (possibly empty)
}
```

- `info` never fails. Dates without data return `{None, false, []}`.
- `coverage()` returns the smallest BS range containing every day that has any metadata, the
  `first_day` to `last_day` range in `spec/vectors/patro-summary.json`. The widget uses it to
  decide whether to show "no data for this year".
- `public_holidays()` returns an ascending iterator of all holiday dates.
- `name(lang)` on tithi/event returns `np` for `Lang::Ne` and `en` for `Lang::En`.

## 3. Compiled layout (every port)

The tables live only in the patro package; the core package has none of them and no reference to
the patro package. Ports compile `data/patro/` into constant tables. JSON is never read at runtime. The layout is
each port's choice, but these facts should shape it:

- The counts are in `spec/vectors/patro-summary.json`, written by `tools/make_vectors.py` from
  `data/patro/`. A port's generator reads that file and never pins a count of its own, so a data
  update changes the generated tables and nothing else. The generator still checks the data
  against the summary, and fails on a disagreement.
- Tithi data is **contiguous**: one entry for every day from `tithi_first_day` to `tithi_last_day`
  (`tithi_days`). Store the first serial and one small index per day into a table of
  `tithi_names`, so a lookup is one subtraction and one array read. The generator must fail if a
  gap appears inside the tithi range.
- Holidays and event days are sparse (`holidays`, `event_days`, `events`). Store sorted serials
  and binary search them (ALGORITHM §4), at most 11 comparisons.
- Event and tithi names repeat. Deduplicate the (np, en) pairs; `names` counts them and
  `name_bytes` is the size of the name pool the C and C++ ports build.
- Some tithi days (all in BS 2078 to 2081) have a name whose `np` ends in `*` and
  whose `en` ends in ` (K)`, for example `तृतिया*` / `Tritiya (K)`. Upstream does not say what the
  marker means, and it does not match skipped or repeated tithis in the data (U-09). Ports show
  the name exactly as in the data: no stripping, no tooltip, no meaning of their own.
- Events of one day keep file order.
- Alternative allowed: dense per-day tables over the covered days (one small index per day for
  holidays and event slices) give O(1) lookups with no binary search. A port may use them when its
  benchmark shows they are faster at an acceptable size (the C++ plan does, about 82 KB in total).

The Rust layout is in `nepdate-rust` as a worked example.

## 4. Test anchors (from the C# test suite, confirmed against data)

| Date | Tithi (en) | Holiday | Events (en, in order) |
|---|---|---|---|
| 2081-01-01 | Panchami | yes | New Year, Mesh Sankranti, Bisket Jatra |
| 2081-01-02 | Shashthi | no | (none) |
| 2081-02-15 | Panchami | yes | Republic Day |
| 2080-11-01 | Chaturthi | no | Mangala Chauthi Brata, People's War Day, Tilakund Chauthi, Kumbha Sankranti, World Radio Day |
| 2081-04-15 | Dashami | no | Kheer Khane Din, World Day Against Human Trafficking, International Friendship Day |
| 2070-06-15, 2090-01-01 | none | no | none |

Values read from `data/patro/*.json` on 2026-09-24.
