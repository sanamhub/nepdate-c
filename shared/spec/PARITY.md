# Parity with C# NepDate, and deliberate deviations (normative, every port)

"Port" means **same behaviour**, not "same idea". Every port matches C# NepDate except for
the deviations listed here, and all ports deviate in the same way. Decided in
[ADR-0003](https://github.com/sanamhub/nepdate/blob/main/docs/adr/0003-parity-with-csharp.md).

## 1. The oracle is the C# *code*, captured as golden vectors

The C# docs (README, `llms-full.txt`, XML comments) disagree with the C# code in several
places: wrong min/max AD dates in comments (1844-04-13 vs the real 1844-04-11), metadata
claimed as 2001 to 2089 but actually 2077 to 2084, a README calendar example that doesn't match the
data, and documented APIs that don't exist. **Docs are never the oracle.**

`tools/csharp-golden/` runs the published NuGet package `NepDate 2.0.7` and writes
`spec/vectors/csharp-golden/*.tsv`:

| File | Rows | Columns |
|---|---|---|
| `dates.tsv` | 1018 dates | bs, ad, weekday, day_of_year, month_length, to_string, unicode, long_en, long_en_weekday, long_ne, long_ne_weekday, dmy_dash_nopad, fy_start, fy_end, quarter_start, quarter_end |
| `add-days.tsv` | 9162 | bs, days, result |
| `add-months.tsv` | 10180 | bs, months, clamp, overflow (`awayFromMonthEnd: true`) |
| `parse.tsv` | 42 | input, strict (`NepaliDate.TryParse`), smart (`SmartDateParser.TryParse`); for comparison only, not a test target (D-05, D-06; ports test `spec/vectors/parse.tsv`) |
| `format-pattern.tsv` | 452 | bs, pattern, result (`ToString(string)`) |
| `day-diff.tsv` | 16 | a, b, days_until (`b - a`, D-07), operator_minus (`a - b`) |
| `ymd-int.tsv` | 14 | kind, input, ymd, result; the `YYYYMMDD` integer both ways (D-11) |
| `bulk.tsv` | 6 | operation, input, output; the output is in input order (NC04-NC08) |
| `serialization.tsv` | 11 | kind, input, json, result; the string hook round trip (D-01, D-11) |
| `format-matrix.tsv` | 102 | kind, bs, order, separator, pad, lang, format, capacity, result (NF02, NF07; D-15) |
| `ranges.tsv` | 54 | start, end, operation, argument, result (NR01-NR04, NR06, NR15-NR21, NR27, NR30; D-01, D-16) |
| `range-factories.tsv` | 27 | factory, argument, result (NR07-NR11; D-01) |
| `range-splits.tsv` | 9 | start, end, operation, result (NR22, NR23; D-16) |
| `range-days.tsv` | 18 | start, end, operation, argument, result (NR24-NR27; D-16) |
| `range-format.tsv` | 18 | start, end, kind, order, separator, lang, result (NR28, NR29; D-01, D-15) |

`year-length.tsv` has 299 rows and is the one reference vector in this set: C# has no year-length
API (D-12), so it lives at `spec/vectors/year-length.tsv` and is summed from the month lengths in
`data/` by `tools/make_vectors.py`, not recorded from C#.

`ERR` = exception or `false` from TryParse, or the port's refusal of an empty range. A line
starting with `#` in a new vector file is a comment; the readers skip it. Fields are tab-separated;
`serialization.tsv` holds literal JSON text, quotes included, so a reader must not use a
quote-aware TSV mode.

The new files carry a `note` column. A row whose note is a D number holds the port's expected
value, not the raw C# value; a port that compares raw C# output skips that row by the note, the
same way the existing readers skip the D-04 pattern rows. `year-length.tsv` has no note column
because the whole file is the D-12 reference vector.

Every port's test suite loads these files and matches every row, **except** rows affected by
a deviation below. Those rows are skipped by an explicit filter with a comment naming the
D-ID.

## 2. Deliberate deviations

Port behaviour is described in neutral terms. Each port's API doc gives its own names.

| ID | C# behaviour | Port behaviour | Why |
|---|---|---|---|
| D-01 | `default(NepaliDate)` is `0000/00/00`, an invalid value that throws when used | an invalid date can't be constructed. Where the language forces a zero value (Go), that value means "no date": it reports itself as zero, every operation on it that returns an error returns the out-of-range error, and every other operation returns the zero value of its result type (`""`, `0`, `false`, an empty info), never a made-up date | make invalid states unrepresentable, as far as the language allows |
| D-02 | `AddMonths(double)` accepts fractions (×30.4167 days) | add months takes an integer and an explicit overflow mode (clamp, or overflow into the next month) | fractional months are ambiguous; use add days |
| D-03 | `AddDays(double)` floors fractions | add days takes an integer | same |
| D-04 | `MMM` abbreviations: Ashad and Ashoj both become `Ash` | `Asa` (Ashad) and `Aso` (Ashoj); other abbreviations unchanged | fix the ambiguity |
| D-05 | Strict parse rejects Devanagari digits; no length limit per number (int overflow possible) | strict parse accepts ASCII **and** Devanagari digits (mixing allowed); max 4 digits per group | converters need it; still deterministic; no overflow |
| D-06 | `SmartDateParser`: fuzzy substring month matching, 6-permutation guessing, `yy`→`20yy`, extra groups ignored | lenient parse: explicit rules + closed month-name list in `PARSING.md`; vectors `vectors/parse.tsv`; no guessing; 2-digit years → `Ambiguous` | determinism and cross-language reproducibility |
| D-07 | `DateTime - DateTime` → `TimeSpan` | signed integer day difference. Where the language overloads `-`, the operator keeps the C# direction, `this - other`, positive when the left date is later. The named method `days_until` (and its translations) uses the opposite direction, `other - this`, the direction of `ALGORITHM.md` `days_between(a, b)`; the two names are not interchangeable | no TimeSpan type needed; the operator and the named method answer different questions |
| D-08 | Fiscal year API: instance methods with `yearOffset` ints; static `GetFiscalYearQuarterStartDate(fy, month)` treats `fy` as a calendar year | a fiscal-year value: containing(date), start, end, quarter(q); quarters Q1 = Shrawan..Ashoj … Q4 = Baishakh..Ashad of the next year | clearer, no hidden offset rules. Vectors `fy_start`/`fy_end`/`quarter_*` still match |
| D-11 | Serialization: STJ string and object, Newtonsoft string and object, XML, and a `TypeConverter` for string, `YYYYMMDD` integer and `DateTime` | the canonical text form is `YYYY-MM-DD`. Every port converts the `YYYYMMDD` integer to and from a date. Serialization uses the language's standard hook only: Rust `serde` behind a feature; Go `encoding.TextMarshaler`/`TextUnmarshaler` and `json.Marshaler`; Swift `Codable`; Kotlin `kotlinx.serialization` `KSerializer` in an optional artifact; TypeScript `toJSON` plus a `fromJSON` or reviver helper; Python `to_json`/`from_json` plus `__reduce__` for pickle; C++ `operator<<`/`std::formatter` and a from-string parser; C a format and parse pair; PHP `JsonSerializable` and `__toString`; Dart `toJson` and a `fromJson` factory. No object form, XML, Newtonsoft or `TypeConverter` type | the standard hook is what each ecosystem uses; one text form keeps interchange exact; the integer form is small and already part of the C# contract |
| D-12 | `IsLeapYear()` = Gregorian leap-ness of the AD year of the date (changes mid-BS-year) | not ported; a year length (364 to 367) instead, in every port | the C# meaning is surprising |
| D-13 | Tithi, holidays and events are members of `NepaliDate`, in the one `NepDate` package (the author keeps it this way, 2026-10-07) | a separate package `nepdate-patro` that depends on the core (ADR-0004); same data and query results | size and release cadence; most users only convert dates |
| D-15 | `ToString(string)` standard formats `G`, `g`, `d`, `D`, `s` | supported identically, **plus** Nepali-language output | parity + a feature the widget needs |
| D-16 | `SplitByMonth` and `SplitByFiscalQuarter` advance to the month or quarter after the last piece, the enumerator, `WorkingDays`, `WeekendDays` and `DatesWithInterval` advance one step after the last date, and `IsAdjacentTo` computes the day after a range end, so a range that reaches 2199-12-30 throws `ArgumentOutOfRangeException`; `Intersect` throws for disjoint ranges when the receiver starts at 1901-01-01 | a valid range never throws because it touches the data end: both splits return every piece, the last ending at the range end; the enumerator and the three day helpers return every date up to and including 2199-12-30; `IsAdjacentTo` returns `false` when the other range would have to start after 2199-12-30, and in the mirror case before 1901-01-01; `Intersect` of disjoint ranges returns the port's "no range" value, also when the receiver starts at 1901-01-01 | C# advances one step past the last item, which is an implementation artifact, not a rule; throwing on a valid range is never what a caller wants |

Everything not listed here must match the golden vectors.

## 3. Updating

- A new deviation needs a new row here, agreed in a PR to this repository, and applies to
  every port. A port never adds a row on its own.
- Regenerating vectors against a newer NepDate NuGet version is a maintainer decision
  (`tools/README.md`).
