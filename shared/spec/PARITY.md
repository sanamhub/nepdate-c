# Parity with C# NepDate, and deliberate deviations (normative, every port)

"Port" means **same behaviour**, not "same idea". Every port matches C# NepDate except for
the deviations listed here, and all ports deviate in the same way. Decided in
[ADR-0003](https://github.com/sanamhub/nepdate/blob/main/docs/adr/0003-parity-with-csharp.md).

## 1. The oracle is the C# *code*, captured as golden vectors

The C# docs (README, `llms-full.txt`, XML comments) disagree with the C# code in several
places: wrong min/max AD dates in comments (1844-04-13 vs the real 1844-04-11), metadata
claimed as 2001–2089 but actually 2077–2084, a README calendar example that doesn't match the
data, and documented APIs that don't exist. **Docs are never the oracle.**

`tools/csharp-golden/` runs the published NuGet package `NepDate 2.0.7` and writes
`spec/vectors/csharp-golden/*.tsv`:

| File | Rows | Columns |
|---|---|---|
| `dates.tsv` | 1018 dates | bs, ad, weekday, day_of_year, month_length, to_string, unicode, long_en, long_en_weekday, long_ne, long_ne_weekday, dmy_dash_nopad, fy_start, fy_end, quarter_start, quarter_end |
| `add-days.tsv` | 9162 | bs, days, result |
| `add-months.tsv` | 10180 | bs, months, clamp, overflow (`awayFromMonthEnd: true`) |
| `parse.tsv` | 42 | input, strict (`NepaliDate.TryParse`), smart (`SmartDateParser.TryParse`); for comparison only, not a test target (D-05, D-06; ports test `spec/vectors/parse.tsv`) |
| `format-pattern.tsv` | 451 | bs, pattern, result (`ToString(string)`) |

`ERR` = exception or `false` from TryParse.

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
| D-07 | `DateTime - DateTime` → `TimeSpan` | signed integer day difference | no TimeSpan type needed |
| D-08 | Fiscal year API: instance methods with `yearOffset` ints; static `GetFiscalYearQuarterStartDate(fy, month)` treats `fy` as a calendar year | a fiscal-year value: containing(date), start, end, quarter(q); quarters Q1 = Shrawan..Ashoj … Q4 = Baishakh..Ashad of the next year | clearer, no hidden offset rules. Vectors `fy_start`/`fy_end`/`quarter_*` still match |
| D-09 | `NepaliDateRange` has `Except`, `Union` (hull), `WorkingDays`, `WeekendDays`, lazy argument exceptions | a date range with month, year, contains, length and a day iterator only; no set operations, splitting or working days | YAGNI; iterate and filter for working days |
| D-10 | `BulkConvert` with PLINQ (unordered output) | not ported | the language's own map/parallel tools do this |
| D-11 | Serialization: STJ, Newtonsoft, XML, object form `{Year,Month,Day}` | one string form `"YYYY-MM-DD"` | one format |
| D-12 | `IsLeapYear()` = Gregorian leap-ness of the AD year of the date (changes mid-BS-year) | not ported; year length (364..=367) instead | the C# meaning is surprising |
| D-13 | Tithi, holidays and events are members of `NepaliDate`, in the one `NepDate` package (the author keeps it this way, 2026-10-07) | a separate package `nepdate-patro` that depends on the core (ADR-0004); same data and query results | size and release cadence; most users only convert dates |
| D-14 | `IsToday` / `IsYesterday` / `IsTomorrow` | not ported as such: compare with the port's `today(zone)` (and `±1` day); a port may add an is-today helper | YAGNI; "today" always needs an explicit zone |
| D-15 | `ToString(string)` standard formats `G`, `g`, `d`, `D`, `s` | supported identically, **plus** Nepali-language output | parity + a feature the widget needs |

Everything not listed here must match the golden vectors.

## 3. Updating

- A new deviation needs a new row here, agreed in a PR to this repository, and applies to
  every port. A port never adds a row on its own.
- Regenerating vectors against a newer NepDate NuGet version is a maintainer decision
  (`tools/README.md`).
