# The NepDate API: canonical inventory (v1)

This file lists the public API of the C# NepDate package and what every port must offer. It is
the inventory the ports work from when they close the gaps in `docs/API.md`. It is not a second
behaviour spec: `ALGORITHM.md`, `FORMATTING.md`, `PARSING.md` and `PATRO.md` stay normative.

The oracle is the C# code in `src/NepDate` at `origin/main`, not the C# README or the XML
comments. `PARITY.md` explains why. Where a comment disagrees with the code, this file follows
the code and names the disagreement.

The twenty decisions at the end of this file were made on 2026-11-02. The names in the tables are final for
this round. PHP and Dart are the next two ports; their columns are the same concepts in PSR-12
and Effective Dart style, not a separate API. Ruby, Java and Zig join for the 1.0 release; their columns are the same concepts in Ruby, Java and Zig style, not a separate API.

## How to read this file

- A row is one public type or member of the C# package. Constructors, properties, methods,
  operators, enum values as a group, and extension methods all count.
- The D numbers in this file refer to `PARITY.md`. A row with a D number is a deliberate,
  agreed deviation, not a gap. D-09, D-10 and D-14 were removed on 2026-11-02; their APIs are
  back, so the numbering has gaps.
- The `Vectors` column names the golden file and columns that cover the row. Every row has a
  vector, a clock note, or a decision. `clock; no vector` means the value comes from the machine
  clock, `not ported` means a decision removes the row from the port surface, and `no vector`
  means the row is a language mechanism with no cross-language answer. `reference` marks a file
  that is derived from `data/` rather than recorded from C#.
- Vectors are in `spec/vectors`. The C# files under `csharp-golden/` have 20,360 rows for dates,
  add-days and add-months, plus `format-pattern.tsv` (452), `parse.tsv` (42), `day-diff.tsv`
  (16), `ymd-int.tsv` (14), `bulk.tsv` (6), `serialization.tsv` (11), `format-matrix.tsv` (102),
  `ranges.tsv` (54), `range-factories.tsv` (27), `range-splits.tsv` (9), `range-days.tsv` (18)
  and `range-format.tsv` (18). `year-length.tsv` (299 years), `parse.tsv` (69),
  `month-boundaries.csv` (7176 data rows) and `patro-summary.json` are reference files. `ERR` in
  a vector means the C# call threw or returned false, or that a port refuses an empty range.
- A naming table gives the final name and signature for each port. Where a port already had a
  fitting name it is kept. A name may still be adjusted to a language's local style, but the
  concept, the arguments and the result do not change.
- `PHP` and `Dart` follow the same rules. PHP uses immutable value objects, `DateTimeImmutable`
  interop, exceptions plus a `try*` form, and PSR-1 and PSR-12 names. Dart uses factory
  constructors, `DateTime` interop, `Comparable`, and `null` only where the language has no
  exception path.

## Groups and counts

| Group | Prefix | Rows |
|---|---|---:|
| Dates, identity and comparison | ND | 34 |
| Conversion and serialization | NC | 16 |
| Arithmetic | NA | 5 |
| Formatting | NF | 10 |
| Parsing | NP | 9 |
| Ranges | NR | 30 |
| Fiscal year | NY | 13 |
| Patro | NT | 7 |
| **Total** | | **124** |

The decisions changed the port surface, not the C# inventory. The count stays 124.


## 1. Dates, identity and comparison

| ID | C# signature | What the code does | Vectors |
|---|---|---|---|
| ND01 | `NepaliDate(int yearBs, int monthBs, int dayBs)` | Builds a BS date and validates the year, then the month, then the day. | dates.tsv; month-boundaries.csv |
| ND02 | `NepaliDate(string rawNepaliDate)` | Parses three numeric groups in year, month, day order with the accepted separators. | parse.tsv (strict) |
| ND03 | `NepaliDate(string rawNepaliDate, bool autoAdjust, bool monthInMiddle = true)` | Parses, then applies the three heuristics: an oversized day swaps with the year, `monthInMiddle: false` swaps month and day, a month over 12 and a day under 13 swap, a year under 1000 gets 2000 added. | not ported (D-05, D-06); no vector |
| ND04 | `NepaliDate(DateTime engDate)` | Converts the date part of a Gregorian `DateTime`; rejects dates outside the supported AD span. | dates.tsv (ad, reverse) |
| ND05 | `bool IsDefault { get; }` | True only when year, month and day are all zero. | language zero value; no vector |
| ND06 | `int Year { get; }` | BS year, 1901 to 2199. | dates.tsv (bs) |
| ND07 | `int Month { get; }` | Month number, 1 to 12. | dates.tsv (bs) |
| ND08 | `int Day { get; }` | Day of month, 1 to the month length. | dates.tsv (bs) |
| ND09 | `DateTime EnglishDate { get; }` | Gregorian `DateTime` at midnight; throws on a default value. | dates.tsv (ad) |
| ND10 | `DayOfWeek DayOfWeek { get; }` | Weekday of the Gregorian date, Sunday is 0. | dates.tsv (weekday) |
| ND11 | `int DayOfYear { get; }` | 1-based ordinal in the BS year, not the Gregorian year. | dates.tsv (day_of_year) |
| ND12 | `int MonthEndDay { get; }` | Last valid day number of this BS month, 29 to 32. | dates.tsv (month_length); month-boundaries.csv |
| ND13 | `NepaliMonths MonthName { get; }` | Month enum cast from the numeric month. | dates.tsv (bs month) |
| ND14 | `static NepaliDate Today { get; }` | Today in the machine's local time zone. The port form takes the zone or offset. | clock; no vector |
| ND15 | `static NepaliDate Now { get; }` | Same date as `Today`; the time of day is discarded. | same as ND14; no vector |
| ND16 | `static readonly NepaliDate MinValue` | 1901-01-01 BS, AD 1844-04-11. | dates.tsv boundary row |
| ND17 | `static readonly NepaliDate MaxValue` | 2199-12-30 BS, AD 2143-04-15. | dates.tsv boundary row |
| ND18 | `NepaliDate MonthEndDate()` | This year and month on the last day. | month-boundaries.csv |
| ND19 | `void Deconstruct(out int year, out int month, out int day)` | Tuple deconstruction of the three components. | language destructuring; no vector |
| ND20 | `bool IsLeapYear()` | Gregorian leap year of `EnglishDate.Year`, not a BS year property. `D-12` replaces it with the BS year length, 364 to 367. | year-length.tsv (reference; D-12) |
| ND21 | `bool IsToday()` | `today(zone) == this`; the zone is explicit. | clock; no vector |
| ND22 | `bool IsYesterday()` | `today(zone) - 1 day == this`. | clock; no vector |
| ND23 | `bool IsTomorrow()` | `today(zone) + 1 day == this`. | clock; no vector |
| ND24 | `int CompareTo(NepaliDate obj)` | Compares the packed `YYYYMMDD` integer. | dates.tsv (row order) |
| ND25 | `int CompareTo(object obj)` | Null sorts before any date; another type throws `ArgumentException`. | not ported; no vector |
| ND26 | `bool Equals(NepaliDate other)` | Compares the packed integer. | dates.tsv (same row) |
| ND27 | `override bool Equals(object obj)` | True only for a `NepaliDate` with the same packed integer. | language equality; no vector |
| ND28 | `override int GetHashCode()` | Returns the packed `YYYYMMDD` integer. | language hash; no vector |
| ND29 | `static bool operator ==(NepaliDate, NepaliDate)` | Packed integer equality. | dates.tsv (row order) |
| ND30 | `static bool operator !=(NepaliDate, NepaliDate)` | Packed integer inequality. | dates.tsv (row order) |
| ND31 | `static bool operator <(NepaliDate, NepaliDate)` | Packed integer order. | dates.tsv (row order) |
| ND32 | `static bool operator <=(NepaliDate, NepaliDate)` | Packed integer order. | dates.tsv (row order) |
| ND33 | `static bool operator >(NepaliDate, NepaliDate)` | Packed integer order. | dates.tsv (row order) |
| ND34 | `static bool operator >=(NepaliDate, NepaliDate)` | Packed integer order. | dates.tsv (row order) |

**Required behaviour.** A valid date is 1901-01-01 through 2199-12-30 BS, which is 1844-04-11
through 2143-04-15 AD. Validation reports the first failure in the order year, month, day. The
month length comes from the BS table, not a fixed 30 or 31. `DayOfYear` counts from Baishakh 1
and reaches 365 or 366 on the last day of a normal year. `MinValue` is 1901-01-01 and `MaxValue`
is the last day of Chaitra 2199. These facts are covered by `dates.tsv` columns `ad`, `weekday`,
`day_of_year` and `month_length`, and by the 7176 month boundaries in `month-boundaries.csv`.
Boundary rows: 1901-01-01 is AD 1844-04-11, and 2199-12-30 is AD 2143-04-15.

`IsLeapYear` follows the Gregorian year, so 2080-01-01 (April 2023) is false while 2080-11-01
(January 2024) is true. `D-12` replaces it with the length of the BS year, 364 to 367. Every port
exposes that length as `ND20`; it sums the twelve month lengths. C# has no year-length API, so
`year-length.tsv` is a reference vector derived from `data/`, not recorded from C#.

`Today` reads the machine clock and takes a zone or a fixed offset. `Now` has the same date as
`Today`, because a date has no time of day, so `ND15` maps to the same `today(zone)` call and no
port has a separate `now`. `IsToday`, `IsYesterday` and `IsTomorrow` take the zone explicitly;
they are not relative to a hidden local zone.

`IsDefault` exists because a C# struct always has a zero value (`D-01`). Ports that can make
invalid dates unrepresentable do not need it.

Comparison and equality use the language's own protocol. The packed `YYYYMMDD` integer is the C#
implementation detail, not part of the interchange format. `dates.tsv` row order is the vector for
ordering; hash values are language-specific and have no cross-language vector.

**Naming.**

| ID | C# | Rust | Go | TypeScript | Python | Swift | Kotlin | C++ | C | PHP | Dart | Java | Ruby | Zig |
|---|---|---|---|---|---|---|---|---|---|---|---|------|
| ND01 | `NepaliDate(int yearBs, int monthBs, int dayBs)` | `NepaliDate::new(y, m, d) -> Result` | `New(y int, m Month, d int) (Date, error)` | `NepaliDate.of(y, m, d)` | `NepaliDate(y, m, d)` | `NepaliDate(year:month:day:) throws` | `NepaliDate.of(y, m, d)` | `make_date(y, month, d) -> result<date>` | `nepdate_from_bs(y, m, d, &out)` | `NepaliDate::fromBs(int $year, int $month, int $day): self` | `NepaliDate.fromBs(int year, int month, int day)` | `NepaliDate.of(y, m, d)` | `.from_bs(y, m, d)` | `Date.fromBs(y, m, d) !Date` |
| ND02 | `NepaliDate(string rawNepaliDate)` | `str::parse::<NepaliDate>()` (`FromStr`) | `Parse(s string) (Date, error)` | `NepaliDate.parse(s)` | `NepaliDate.parse(s)` | `NepaliDate.parse(_:) throws` | `NepaliDate.parse(text)` | `parse(std::string_view) -> result<date>` | `nepdate_parse(text, len, &out)` | `NepaliDate::parse(string $text): self` | `NepaliDate.parse(String text)` | `NepaliDate.parse(text)` | `.parse(s)` | `Date.parse(text) !Date` |
| ND03 | `NepaliDate(string rawNepaliDate, bool autoAdjust, bool monthInMiddle = true)` | not ported (D-05, D-06) | not ported (D-05, D-06) | not ported (D-05, D-06) | not ported (D-05, D-06) | not ported (D-05, D-06) | not ported (D-05, D-06) | not ported (D-05, D-06) | not ported (D-05, D-06) | not ported (D-05, D-06) | not ported (D-05, D-06) | not ported (D-05, D-06) | not ported (D-05, D-06) | not ported (strict and lenient parsers) |
| ND04 | `NepaliDate(DateTime engDate)` | `NepaliDate::from_gregorian(y, m, d)` | `FromGregorian(y int, m time.Month, d int)` | `NepaliDate.fromGregorian({year, month, day})` | `NepaliDate.from_date(date)` | `NepaliDate(gregorianYear:month:day:)` | `NepaliDate.ofGregorian(y, m, d)` | `from_gregorian(y, m, d)` | `nepdate_from_ad(y, m, d, &out)` | `NepaliDate::fromGregorian(DateTimeInterface $date): self` | `NepaliDate.fromGregorian(DateTime date)` | `NepaliDate.ofGregorian(y, m, d)`, `NepaliDate.of(LocalDate)` | `.from_date(::Date)`, `.from_time(::Time)`, `.from_gregorian(y, m, d)` | `Date.fromGregorian(y, m, d) !Date` |
| ND05 | `bool IsDefault { get; }` | not ported (D-01) | `IsZero() bool` | not ported (D-01) | not ported (D-01) | not ported (D-01) | not ported (D-01) | not ported (D-01) | `zero value is invalid (D-01)` | not ported (D-01) | not ported (D-01) | not ported (D-01) | not ported (D-01) | `Date.isDefault()`, `Date.isValid()` |
| ND06 | `int Year { get; }` | `year() -> u16` | `Year() int` | `year: number` | `year: int` | `year: Int` | `year: Int` | `year() -> int` | `nepdate_to_bs(date, &out).year` | `year(): int` | `int get year` | `year()` | `#year` | `Date.year()` |
| ND07 | `int Month { get; }` | `month() -> Month`, `month_number() -> u8` | `Month() Month` | `month: number` | `month: int` | `month: Int`, `bsMonth: Month` | `monthValue: Int`, `month: Month` | `month() -> month` | `nepdate_month_of(date, &out)` | `month(): int`, `monthEnum(): Month` | `int get month`, `Month get monthEnum` | `monthNumber()`, `month()` | `#month`, `#month_enum` | `Date.month()`, `Date.monthNumber()` |
| ND08 | `int Day { get; }` | `day() -> u8` | `Day() int` | `day: number` | `day: int` | `day: Int` | `dayOfMonth: Int` | `day() -> int` | `nepdate_to_bs(date, &out).day` | `day(): int` | `int get day` | `day()` | `#day` | `Date.day()` |
| ND09 | `DateTime EnglishDate { get; }` | `to_gregorian() -> GregorianDate` | `Gregorian() (int, time.Month, int)` | `toGregorian(): GregorianDate` | `to_date() -> datetime.date` | `gregorian: (year: Int, month: Int, day: Int)` | `gregorianYear`, `gregorianMonth`, `gregorianDay` | `to_year_month_day()`, `to_sys_days()` | `nepdate_to_ad(date, &out)` | `toDateTime(): DateTimeImmutable` | `DateTime toDateTime()` | `toLocalDate()`, `gregorianYear()`, `gregorianMonth()`, `gregorianDay()` | `#to_date`, `#to_datetime`, `#to_time` | `Date.englishDate()` |
| ND10 | `DayOfWeek DayOfWeek { get; }` | `weekday() -> Weekday` | `Weekday() time.Weekday` | `weekday: number` | `weekday: Weekday` | `weekday: Weekday` | `dayOfWeek: Weekday` | `weekday() -> std::chrono::weekday` | `nepdate_weekday_of(date, &out)` | `dayOfWeek(): Weekday` | `Weekday get weekday` | `weekday() -> Weekday` | `#weekday` | `Date.dayOfWeek()` |
| ND11 | `int DayOfYear { get; }` | `day_of_year() -> u16` | `YearDay() int` | `dayOfYear: number` | `day_of_year: int` | `dayOfYear: Int` | `dayOfYear: Int` | `day_of_year() -> int` | `nepdate_day_of_year(date, &out)` | `dayOfYear(): int` | `int get dayOfYear` | `dayOfYear()` | `#day_of_year` | `Date.dayOfYear()` |
| ND12 | `int MonthEndDay { get; }` | `month_length() -> u8` | `MonthLength() int` | `monthLength: number` | `month_length: int` | `monthLength: Int` | `lengthOfMonth(): Int` | `month_length() -> int` | `nepdate_month_length(date, &out)` | `monthLength(): int` | `int get monthLength` | `monthLength()` | `#month_length` | `Date.monthLength()` |
| ND13 | `NepaliMonths MonthName { get; }` | `month() -> Month` | `Month() Month` | `monthName(m)` | `month_enum: Month` | `bsMonth: Month` | `month: Month` | `month() -> month` | `nepdate_month_of(date, &out)` | `monthEnum(): Month` | `Month get monthEnum` | `month() -> Month` | `#month_enum` | `Date.month()` |
| ND14 | `static NepaliDate Today { get; }` | `today()`, `today_in(zone)` | `Today(loc *time.Location) (Date, error)` | `NepaliDate.today(zone: string = 'Asia/Kathmandu')` | `NepaliDate.today(tz: Optional[datetime.tzinfo] = None)` | `NepaliDate.today(in timeZone: TimeZone? = nil)` | `nepaliToday(zone: ZoneId = KATHMANDU)` | `today(offset = nepal_utc_offset)` | `nepdate_today_nepal(unix)`, `nepdate_today(offset, unix, &out)` | `NepaliDate::today(DateTimeZone $zone): self` | `NepaliDate.today({String timeZone = 'Asia/Kathmandu'})` | `today(ZoneId)`, `today(Clock)` | `.today(zone:, clock:)` | `Date.today(offset, unix)`, `Date.todayNepal(unix)`, `Date.todayIn(zone, unix)` |
| ND15 | `static NepaliDate Now { get; }` | same as ND14 | same as ND14 | same as ND14 | same as ND14 | same as ND14 | same as ND14 | same as ND14 | same as ND14 | same as ND14 | same as ND14 | same as ND14 | same as ND14 | `Date.now(offset, unix)` |
| ND16 | `static readonly NepaliDate MinValue` | `MIN` | `Min` | `NepaliDate.MIN` | `MIN` | `NepaliDate.min` | `NepaliDate.MIN` | `date::min()` | `NEPDATE_MIN` | `NepaliDate::min(): self` | `NepaliDate.min` | `MIN` | `Date::MIN` | `Date.min` |
| ND17 | `static readonly NepaliDate MaxValue` | `MAX` | `Max` | `NepaliDate.MAX` | `MAX` | `NepaliDate.max` | `NepaliDate.MAX` | `date::max()` | `NEPDATE_MAX` | `NepaliDate::max(): self` | `NepaliDate.max` | `MAX` | `Date::MAX` | `Date.max` |
| ND18 | `NepaliDate MonthEndDate()` | `last_day_of_month()` | `LastOfMonth()` | `lastOfMonth()` | `last_of_month()` | `lastDayOfMonth` | `lastDayOfMonth()` | `last_of_month()` | `nepdate_last_day_of_month(date, &out)` | `lastDayOfMonth(): self` | `NepaliDate lastDayOfMonth` | `lastDayOfMonth()` | `#last_day_of_month` | `Date.monthEndDate()`, `Date.lastDayOfMonth()` |
| ND19 | `void Deconstruct(out int year, out int month, out int day)` | `accessors; no tuple` | `accessors; no tuple` | `const { year, month, day } = d` | `y, m, d = d.year, d.month, d.day` | `(d.year, d.month, d.day)` | `component1()`, `component2()`, `component3()` | `to_tuple()` with structured binding | `nepdate_to_bs(date, &out)` | `toArray(): array` | `toRecord(): (int, int, int)` | `year()`, `monthNumber()`, `day()` | `#to_a`, `#to_h`, `#deconstruct` | `Date.toRecord()` |
| ND20 | `bool IsLeapYear()` | `year_length() -> u16` | `YearLength() int` | `yearLength: number` | `year_length: int` | `yearLength: Int` | `yearLength(): Int` | `year_length() -> int` | `nepdate_year_length(date, &out)` | `yearLength(): int` | `int get yearLength` | `yearLength()` | `#year_length` | `Date.yearLength()` (D-12) |
| ND21 | `bool IsToday()` | `is_today(zone)` | `IsToday(loc *time.Location) bool` | `isToday(zone: string): boolean` | `is_today(tz: datetime.tzinfo) -> bool` | `isToday(in: TimeZone) -> Bool` | `isToday(zone: ZoneId): Boolean` | `is_today(offset) -> bool` | `nepdate_is_today(date, offset, unix)` | `isToday(DateTimeZone $zone): bool` | `bool isToday(String timeZone)` | `isToday(ZoneId)`, `isToday(Clock)` | `#today?(zone:, clock:)` | `Date.isToday(offset, unix)` |
| ND22 | `bool IsYesterday()` | `is_yesterday(zone)` | `IsYesterday(loc *time.Location) bool` | `isYesterday(zone: string): boolean` | `is_yesterday(tz: datetime.tzinfo) -> bool` | `isYesterday(in: TimeZone) -> Bool` | `isYesterday(zone: ZoneId): Boolean` | `is_yesterday(offset) -> bool` | `nepdate_is_yesterday(date, offset, unix)` | `isYesterday(DateTimeZone $zone): bool` | `bool isYesterday(String timeZone)` | `isYesterday(ZoneId)`, `isYesterday(Clock)` | `#yesterday?(zone:, clock:)` | `Date.isYesterday(offset, unix)` |
| ND23 | `bool IsTomorrow()` | `is_tomorrow(zone)` | `IsTomorrow(loc *time.Location) bool` | `isTomorrow(zone: string): boolean` | `is_tomorrow(tz: datetime.tzinfo) -> bool` | `isTomorrow(in: TimeZone) -> Bool` | `isTomorrow(zone: ZoneId): Boolean` | `is_tomorrow(offset) -> bool` | `nepdate_is_tomorrow(date, offset, unix)` | `isTomorrow(DateTimeZone $zone): bool` | `bool isTomorrow(String timeZone)` | `isTomorrow(ZoneId)`, `isTomorrow(Clock)` | `#tomorrow?(zone:, clock:)` | `Date.isTomorrow(offset, unix)` |
| ND24 | `int CompareTo(NepaliDate obj)` | `Ord::cmp` | `Compare(other Date) int` | `NepaliDate.compare(a, b)` | ordering dunders | `Comparable` | `Comparable` | `operator<=>` | `nepdate_compare(a, b, &out)` | `compareTo(NepaliDate $other): int` | `int compareTo(NepaliDate other)` | `Comparable.compareTo` | `#compare_to`, `<=>` | `Date.compare(other)` |
| ND25 | `int CompareTo(object obj)` | not ported (typed comparison) | not ported (typed comparison) | not ported (typed comparison) | not ported (typed comparison) | not ported (typed comparison) | not ported (typed comparison) | not ported (typed comparison) | not ported (typed comparison) | not ported (typed comparison) | not ported (typed comparison) | not ported (typed comparison) | not ported (typed comparison) | not ported (typed comparison) |
| ND26 | `bool Equals(NepaliDate other)` | `PartialEq` | `==` | `equals(other)` | `__eq__` | `==` | `equals(Any?)` | `operator==` | `nepdate_compare(a, b, &out) == 0` | `equals(NepaliDate $other): bool` | `operator ==` | `equals(Object)` | `==`, `#eql?` | `Date.eql(other)` |
| ND27 | `override bool Equals(object obj)` | `PartialEq` | `==` | `equals(other)` | `__eq__(object)` | `==` | `equals(Any?)` | `operator==` | `nepdate_compare == 0` | `equals(NepaliDate $other)` | `operator ==` | `equals(Object)` | `==` | `Date.eql(other)` |
| ND28 | `override int GetHashCode()` | `Hash` | `comparable; no method` | `no hash; serial string is a key` | `__hash__` | `Hashable` | `hashCode()` | `std::hash` | `no hash` | `no hash` | `hashCode` | `hashCode()` | `#hash` | `Date.hash()` |
| ND29 | `static bool operator ==(NepaliDate, NepaliDate)` | `PartialEq::eq` | `==` | `equals` | `__eq__` | `==` | `==` | `operator==` | `nepdate_compare == 0` | `equals` | `==` | `compareTo() == 0` | `==` | `Date.eql(other)` |
| ND30 | `static bool operator !=(NepaliDate, NepaliDate)` | `PartialEq::ne` | `!=` | `!equals` | `__ne__` | `!=` | `!=` | `operator!=` | `nepdate_compare != 0` | `!equals` | `!=` | `compareTo() != 0` | `!=` | `!Date.eql(other)` |
| ND31 | `static bool operator <(NepaliDate, NepaliDate)` | `Ord::cmp` | `Compare` | `compare(a, b) < 0` | `__lt__` | `<` | `<` | `operator<` | `nepdate_compare < 0` | `compareTo(...) < 0` | `compareTo(...) < 0` | `compareTo() < 0` | `<` | `Date.lessThan(other)` |
| ND32 | `static bool operator <=(NepaliDate, NepaliDate)` | `Ord::cmp` | `Compare` | `compare(a, b) <= 0` | `__le__` | `<=` | `<=` | `operator<=` | `nepdate_compare <= 0` | `compareTo(...) <= 0` | `compareTo(...) <= 0` | `compareTo() <= 0` | `<=` | `Date.lessThanOrEqual(other)` |
| ND33 | `static bool operator >(NepaliDate, NepaliDate)` | `Ord::cmp` | `Compare` | `compare(a, b) > 0` | `__gt__` | `>` | `>` | `operator>` | `nepdate_compare > 0` | `compareTo(...) > 0` | `compareTo(...) > 0` | `compareTo() > 0` | `>` | `Date.greaterThan(other)` |
| ND34 | `static bool operator >=(NepaliDate, NepaliDate)` | `Ord::cmp` | `Compare` | `compare(a, b) >= 0` | `__ge__` | `>=` | `>=` | `operator>=` | `nepdate_compare >= 0` | `compareTo(...) >= 0` | `compareTo(...) >= 0` | `compareTo() >= 0` | `>=` | `Date.greaterThanOrEqual(other)` |
|------|
## 2. Conversion and serialization

| ID | C# signature | What the code does | Vectors |
|---|---|---|---|
| NC01 | `static NepaliDate DateTimeExtensions.ToNepaliDate(this DateTime englishDate)` | Calls the `DateTime` constructor. | dates.tsv (ad, reverse) |
| NC02 | `static NepaliDate SmartParsingExtensions.ToNepaliDate(this string input)` | Calls `SmartDateParser.Parse`. | parse.tsv (lenient) |
| NC03 | `static bool SmartParsingExtensions.TryToNepaliDate(this string input, out NepaliDate result)` | Calls `SmartDateParser.TryParse`; false leaves no result. | parse.tsv (lenient, ERR) |
| NC04 | `static IEnumerable<NepaliDate> NepaliDate.BulkConvert.ToNepaliDates(IEnumerable<DateTime> engDates, bool useParallel = true)` | Maps every date. The port form keeps input order, unlike the C# PLINQ path above 500 items. | bulk.tsv |
| NC05 | `static IEnumerable<DateTime> ...ToEnglishDates(IEnumerable<string> nepDates, bool useParallel = true)` | Parses each string and returns its English date, in input order. | bulk.tsv |
| NC06 | `static IEnumerable<DateTime> ...ToEnglishDates(IEnumerable<NepaliDate> nepDates, bool useParallel = true)` | Returns `EnglishDate` for each date, in input order. | bulk.tsv |
| NC07 | `static IEnumerable<NepaliDate> ...BatchProcessToNepaliDates(IEnumerable<DateTime> engDates, int batchSize = 1000)` | Converts in batches and accumulates a list. The port's single batch function is the same operation. | bulk.tsv |
| NC08 | `static IEnumerable<DateTime> ...BatchProcessToEnglishDates(IEnumerable<NepaliDate> nepDates, int batchSize = 1000)` | Converts in batches and accumulates a list. The port's single batch function is the same operation. | bulk.tsv |
| NC09 | `sealed class NepaliDateTypeConverter : TypeConverter` | Converts to and from string, `YYYYMMDD` integer and `DateTime`; `ConvertTo` string is `YYYY-MM-DD`. The type is not ported; its integer form is. | ymd-int.tsv (D-11) |
| NC10 | `class SystemTextJsonConverters.NepaliDateJsonConverter : JsonConverter<NepaliDate>` | Reads a string or an object and writes `YYYY-MM-DD`; null becomes the default value. The port form is the language's native string hook only. | serialization.tsv (D-11); dates.tsv (to_string) |
| NC11 | `class SystemTextJsonConverters.NepaliDateObjectJsonConverter` | Reads an object and writes `{ "Year", "Month", "Day" }`. | not ported (D-11); no vector |
| NC12 | `class NewtonsoftJsonConverters.NepaliDateJsonConverter : JsonConverter<NepaliDate>` | The Newtonsoft version of NC10. | not ported (D-11); no vector |
| NC13 | `class NewtonsoftJsonConverters.NepaliDateObjectJsonConverter` | The Newtonsoft version of NC11. | not ported (D-11); no vector |
| NC14 | `class NepaliDateXmlSerializer : IXmlSerializable` | Writes `YYYY-MM-DD` text and reads it back. | not ported (D-11); no vector |
| NC15 | `static JsonSerializerOptions SerializationExtensions.ConfigureForNepaliDate(this JsonSerializerOptions options, bool useObjectFormat = false)` | Registers NC10 or NC11. | not ported (D-11); no vector |
| NC16 | `static JsonSerializerSettings SerializationExtensions.ConfigureForNepaliDate(this JsonSerializerSettings settings, bool useObjectFormat = false)` | Registers NC12 or NC13. | not ported (D-11); no vector |

**Required behaviour.** C# accepts only `DateTime`, not `DateOnly`. A port exposes its platform's
Gregorian date type (Go `time.Time`, Python `datetime.date`, Swift `Date` plus a zone, Kotlin
`java.time.LocalDate`, C++ `<chrono>`, TypeScript `Date` or a structural `{year, month, day}`,
Rust `GregorianDate` plus the optional `chrono` and `jiff` conversions, C `struct tm` and Unix
seconds, PHP `DateTimeImmutable`, Dart `DateTime`). The date part is what converts; a time of day
is discarded.

Batch conversion is now required in every port. The output is ordered, unlike the C# PLINQ path,
and there is one function each way, not the five C# entry points. The input is the language's
slice, iterator or array; C takes caller-provided buffers and a count. The C# `useParallel` and
`batchSize` arguments do not cross into the port API.

The canonical text form is `YYYY-MM-DD`. `D-11` drops the object form, the Newtonsoft and XML
adapters and the `TypeConverter` type, and uses only the language's native hook. The integer form
`YYYYMMDD` is still part of the contract: every port converts it to and from a date (`NC09`).
The string and `DateTime` parts of the C# `TypeConverter` are covered by `NC01` and `NF01`.

**Naming.**

| ID | C# | Rust | Go | TypeScript | Python | Swift | Kotlin | C++ | C | PHP | Dart | Java | Ruby | Zig |
|---|---|---|---|---|---|---|---|---|---|---|---|------|
| NC01 | `static NepaliDate DateTimeExtensions.ToNepaliDate(this DateTime englishDate)` | `TryFrom<GregorianDate>` | `FromTime(t time.Time)` | `fromGregorian`, `fromJSDate` | `from_date(d)` | `init(_ date: Date, in:)` | `LocalDate.toNepaliDate()` | `from_gregorian`, `from_sys_days` | `nepdate_from_ad`, `nepdate_from_unix` | `NepaliDate::fromGregorian(DateTimeInterface $date): self` | `NepaliDate.fromGregorian(DateTime date)` | `of(LocalDate)`, `ofGregorian(y, m, d)` | `.from_date`, `.from_time`, `.from_gregorian` | `Date.fromGregorian(y, m, d)` |
| NC02 | `static NepaliDate SmartParsingExtensions.ToNepaliDate(this string input)` | `parse_lenient` | `ParseLenient` | `parseLenient` | `parse_lenient` | `parseLenient` | `parseLenient` | `parse_lenient` | `nepdate_parse_lenient` | `NepaliDate::parseLenient(string $text): self` | `NepaliDate.parseLenient(String text)` | `parseLenient(text)` | `.parse_lenient` | `Date.parseLenient(text)` |
| NC03 | `static bool SmartParsingExtensions.TryToNepaliDate(this string input, out NepaliDate result)` | `try_parse_lenient(s) -> Result` | `TryParseLenient(s string) (Date, bool)` | `tryParseLenient(s)` | `try_parse_lenient(s)` | `init?(lenient:)` | `parseLenientOrNull(text)` | `try_parse_lenient(string_view) -> std::optional<date>` | `nepdate_parse_lenient` status | `NepaliDate::tryParseLenient(string $text): ?self` | `NepaliDate? tryParseLenient(String text)` | `tryParseLenient(text) -> Optional` | `.try_parse_lenient` | `Date.tryParseLenient(text)` |
| NC04 | `static IEnumerable<NepaliDate> NepaliDate.BulkConvert.ToNepaliDates(IEnumerable<DateTime> engDates, bool useParallel = true)` | `to_nepali_dates(iter) -> Result<Vec<NepaliDate>>` | `ToNepaliDates(dates []time.Time) ([]Date, error)` | `toNepaliDates(dates: Iterable<DateInput>): NepaliDate[]` | `to_nepali_dates(dates: Iterable[date]) -> list[NepaliDate]` | `NepaliDate.fromGregorian(_:in:)` on a sequence | `toNepaliDates(dates: Iterable<LocalDate>): List<NepaliDate>` | `to_nepali_dates(std::span<const sys_days>) -> std::vector<date>` | `nepdate_bulk_to_bs(ad, n, out, out_len, &written)` | `NepaliDate::fromGregorianBatch(iterable $dates): array` | `NepaliDate.fromGregorianAll(Iterable<DateTime>)` | `toNepaliDates(Iterable<LocalDate>)` | `.from_gregorian_batch(enum)` | `toNepaliDates(dates, out)` |
| NC05 | `static IEnumerable<DateTime> ...ToEnglishDates(IEnumerable<string> nepDates, bool useParallel = true)` | `to_english_dates(strings) -> Result<Vec<...>>` | `ToEnglishDatesFromStrings([]string) ([]time.Time, error)` | `toEnglishDates(strings: Iterable<string>): Date[]` | `to_english_dates(strings: Iterable[str]) -> list[date]` | `NepaliDate.gregorianDates(from:in:)` on strings | `toEnglishDates(strings: Iterable<String>): List<LocalDate>` | `to_english_dates(std::span<const std::string>) -> result<vector<sys_days>>` | `nepdate_bulk_parse_to_ad(texts, n, out, out_len, &written)` | `NepaliDate::parseBatch(iterable $texts): array` | `NepaliDate.parseAll(Iterable<String>)` | `parseToEnglishDates(Iterable<String>)` | `.parse_batch(enum)` | `parseAll(texts, out)` |
| NC06 | `static IEnumerable<DateTime> ...ToEnglishDates(IEnumerable<NepaliDate> nepDates, bool useParallel = true)` | `to_english_dates(dates) -> Result<Vec<...>>` | `ToEnglishDates(dates []Date) ([]time.Time, error)` | `toEnglishDates(dates: Iterable<NepaliDate>): Date[]` | `to_english_dates(dates: Iterable[NepaliDate]) -> list[date]` | `NepaliDate.gregorianDates(from:in:)` on a sequence | `toEnglishDates(dates: Iterable<NepaliDate>): List<LocalDate>` | `to_english_dates(std::span<const date>) -> std::vector<sys_days>` | `nepdate_bulk_to_ad(dates, n, out, out_len, &written)` | `NepaliDate::toGregorianBatch(iterable $dates): array` | `NepaliDate.toGregorianAll(Iterable<NepaliDate>)` | `toEnglishDates(Iterable<NepaliDate>)` | `.to_gregorian_batch(enum)` | `toEnglishDates(dates, out)` |
| NC07 | `static IEnumerable<NepaliDate> ...BatchProcessToNepaliDates(IEnumerable<DateTime> engDates, int batchSize = 1000)` | same as NC04 | same as NC04 | same as NC04 | same as NC04 | same as NC04 | same as NC04 | same as NC04 | same as NC04 | same as NC04 | same as NC04 | same as NC04 | same as NC04 | `batchToNepaliDates(dates, out)` |
| NC08 | `static IEnumerable<DateTime> ...BatchProcessToEnglishDates(IEnumerable<NepaliDate> nepDates, int batchSize = 1000)` | same as NC06 | same as NC06 | same as NC06 | same as NC06 | same as NC06 | same as NC06 | same as NC06 | same as NC06 | same as NC06 | same as NC06 | same as NC06 | same as NC06 | `batchToEnglishDates(dates, out)` |
| NC09 | `sealed class NepaliDateTypeConverter : TypeConverter` | `from_ymd_int(i32) -> Result`, `to_ymd_int() -> i32` | `FromYMD(n int) (Date, error)`, `YMD() int` | `NepaliDate.fromYmd(n)`, `toYmd()` | `from_ymd_int(n)`, `to_ymd_int()` | `init?(ymd:)`, `ymd` | `fromYmd(Int)`, `toYmd()` | `from_ymd_int(int) -> result<date>`, `to_ymd_int()` | `nepdate_from_ymd_int(n, &out)`, `nepdate_to_ymd_int(date, &out)` | `fromYmd(int)`, `toYmd()` | `fromYmd(int)`, `int get ymd` | `ofYmd(int)`, `toYmd()` | `.from_ymd(int)`, `#to_ymd` | `Date.fromYmd(n)`, `Date.ymd()` |
| NC10 | `class SystemTextJsonConverters.NepaliDateJsonConverter : JsonConverter<NepaliDate>` | `serde` string behind the `serde` feature | `MarshalText`/`UnmarshalText`, `MarshalJSON`/`UnmarshalJSON` | `toJSON()`, `fromJSON()` or a reviver | `to_json()`/`from_json()`, `__reduce__` | `Codable` string | `NepaliDateSerializer` in an optional artifact | `operator<<`, `std::formatter`, `parse` | `nepdate_format_iso`, `nepdate_parse` | `JsonSerializable`, `__toString` | `toJson()`, `fromJson()` | `Serializable` (serialization proxy) | `#to_json`, `.from_json` | `Date.toJsonAlloc(allocator)`, `Date.fromJson(text)` |
| NC11 | `class SystemTextJsonConverters.NepaliDateObjectJsonConverter` | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) |
| NC12 | `class NewtonsoftJsonConverters.NepaliDateJsonConverter : JsonConverter<NepaliDate>` | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) |
| NC13 | `class NewtonsoftJsonConverters.NepaliDateObjectJsonConverter` | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) |
| NC14 | `class NepaliDateXmlSerializer : IXmlSerializable` | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) |
| NC15 | `static JsonSerializerOptions SerializationExtensions.ConfigureForNepaliDate(this JsonSerializerOptions options, bool useObjectFormat = false)` | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) |
| NC16 | `static JsonSerializerSettings SerializationExtensions.ConfigureForNepaliDate(this JsonSerializerSettings settings, bool useObjectFormat = false)` | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) | not ported (D-11) |
|------|
## 3. Arithmetic

| ID | C# signature | What the code does | Vectors |
|---|---|---|---|
| NA01 | `NepaliDate AddMonths(double months, bool awayFromMonthEnd = false)` | Negative values delegate to subtraction. A fractional value is rounded away from zero and converted to days at 30.4167 days per month. A whole value moves in month space and clamps or spills at the target month end. | add-months.tsv |
| NA02 | `NepaliDate AddDays(double days)` | Converts to `DateTime`, calls `DateTime.AddDays`, converts back; only the date part survives. | add-days.tsv |
| NA03 | `NepaliDate AddYears(int years, bool awayFromMonthEnd = false)` | `AddMonths(years * 12.0, awayFromMonthEnd)`. | add-months.tsv (years x 12) |
| NA04 | `TimeSpan Subtract(NepaliDate nepDateTo)` | C# returns `this - nepDateTo`; the port's named method returns `other - this`. | day-diff.tsv (days_until; D-07) |
| NA05 | `static TimeSpan operator -(NepaliDate d1, NepaliDate d2)` | English date difference; positive when `d1` is later. Where the language overloads `-`, the operator keeps this C# direction. | day-diff.tsv (operator_minus; D-07) |

**Required behaviour.** Whole month arithmetic is the required model. `D-02` removes fractional
months and makes the overflow choice explicit: clamp to the last day of the target month, or spill
the excess into the following month. `D-03` removes fractional days. The vectors
`add-months.tsv` (10180 rows) and `add-days.tsv` (9162 rows) cover both choices and the range
boundaries. Examples from `ALGORITHM.md`: 2081-04-32 plus two months is 2081-06-30 with clamp and
2081-07-02 with spill; 2080-01-01 to 2081-04-15 is 473 days.

`D-07` replaces `TimeSpan` with a signed integer day count, and the sign now has two names. Where
the language overloads `-`, the operator keeps the C# direction, `this - other`, positive when
the left date is later. The named method `days_until` (and `daysUntil`, `DaysUntil`) keeps the
opposite direction, `other - this`, the direction of `ALGORITHM.md` `days_between(a, b)`. A port
must not use the two names interchangeably, and its docs must state both. `day-diff.tsv` records
both directions for the same pair.

`AddYears` is `AddMonths(years * 12)` and uses the same overflow mode. `AddMonths` with a negative
value is the subtraction path; it is not a separate API.

**Naming.**

| ID | C# | Rust | Go | TypeScript | Python | Swift | Kotlin | C++ | C | PHP | Dart | Java | Ruby | Zig |
|---|---|---|---|---|---|---|---|---|---|---|---|------|
| NA01 | `NepaliDate AddMonths(double months, bool awayFromMonthEnd = false)` | `checked_add_months(i32, MonthOverflow)` | `AddMonths(n int, o Overflow)` | `addMonths(n, overflow?)` | `add_months(n, overflow=CLAMP)` | `adding(months:overflow:)` | `plusMonths(months, overflow)` | `add_months(int, overflow)` | `nepdate_add_months(date, n, overflow, &out)` | `addMonths(int $months, MonthOverflow $overflow = MonthOverflow::Clamp): self` | `NepaliDate addMonths(int months, {MonthOverflow overflow = MonthOverflow.clamp})` | `plusMonths(long, MonthOverflow)` | `#add_months(int, overflow: :clamp)` | `Date.addMonths(n, overflow)` |
| NA02 | `NepaliDate AddDays(double days)` | `checked_add_days(i32)` | `AddDays(n int)` | `addDays(n)` | `add_days(n)` | `adding(days:)` | `plusDays(days)` | `add_days(int)` | `nepdate_add_days(date, n, &out)` | `addDays(int $days): self` | `NepaliDate addDays(int days)` | `plusDays(long)` | `#add_days(int)` | `Date.addDays(n)` |
| NA03 | `NepaliDate AddYears(int years, bool awayFromMonthEnd = false)` | `checked_add_years(i32, MonthOverflow)` | `AddYears(n int, o Overflow)` | `addYears(n, overflow?)` | `add_years(n, overflow=CLAMP)` | `adding(years:overflow:)` | `plusYears(years, overflow)` | `add_years(int, overflow)` | `nepdate_add_years(date, n, overflow, &out)` | `addYears(int $years, MonthOverflow $overflow = MonthOverflow::Clamp): self` | `NepaliDate addYears(int years, {MonthOverflow overflow = MonthOverflow.clamp})` | `plusYears(long, MonthOverflow)` | `#add_years(int, overflow: :clamp)` | `Date.addYears(n, overflow)` |
| NA04 | `TimeSpan Subtract(NepaliDate nepDateTo)` | `days_until(other) -> i32`, `diff(other)` | `DaysUntil(other Date) int`, `Diff` | `daysUntil(other)`, `diff` | `days_until(other)`, `__sub__` | `days(until:)`, `diff(to:)` | `daysUntil(other)`, `until` | `days_until(other)`, `diff` | `nepdate_diff_days(a, b, &out)`, `nepdate_diff` | `daysUntil(NepaliDate $other): int`, `diff` | `int daysUntil(NepaliDate other)`, `diff` | `daysUntil(other)`, `daysSince(other)`, `until(other) -> Period` | `#days_until(other)`, `#diff(other)` | `Date.daysUntil(other)` |
| NA05 | `static TimeSpan operator -(NepaliDate d1, NepaliDate d2)` | `impl Sub for NepaliDate -> DateDiff` (`this - other`) | `no operator; use DaysUntil` | `no operator; use daysUntil` | `date - date -> datetime.timedelta` (`this - other`) | `static func - (NepaliDate, NepaliDate) -> DateDiff` | `operator fun minus(other): DateDiff` | `operator-(date, date) -> date_diff` | `no operator; use nepdate_diff_days` | `no operator; use daysUntil` | `DateDiff operator -(NepaliDate other)` | no operator; use `daysSince` | `#-` (`self - other`) | `Date.minus(other)` |
|------|
## 4. Formatting

| ID | C# signature | What the code does | Vectors |
|---|---|---|---|
| NF01 | `override string ToString()` | `YYYY/MM/DD`, zero padded, ASCII digits, exactly 10 characters. | dates.tsv (to_string) |
| NF02 | `string ToString(DateFormats dateFormat, Separators separator = Separators.ForwardSlash, bool leadingZeros = true)` | Writes the three fields in the requested order with the requested separator and optional padding. | format-matrix.tsv (short); dates.tsv (dmy_dash_nopad) |
| NF03 | `string ToLongDateString(bool leadingZeros = true, bool displayDayName = false, bool displayYear = true)` | `[Weekday, ]Month day[, year]`, English names, padded day by default. | dates.tsv (long_en, long_en_weekday, long_ne, long_ne_weekday) |
| NF04 | `string ToUnicodeString(DateFormats dateFormat = DateFormats.YearMonthDay, Separators separator = Separators.ForwardSlash, bool leadingZeros = true)` | `ToString` with every ASCII digit replaced by its Devanagari counterpart. | dates.tsv (unicode) |
| NF05 | `string ToLongDateUnicodeString(bool leadingZeros = true, bool displayDayName = false, bool displayYear = true)` | Long form with Devanagari month name, weekday name and digits. | dates.tsv (long_ne, long_ne_weekday) |
| NF06 | `string ToString(string format, IFormatProvider formatProvider = null)` | Standard patterns `""`, `G`, `g`, `d`, `D`, `s`; anything else is the custom pattern language. The provider is ignored. | format-pattern.tsv |
| NF07 | `bool ISpanFormattable.TryFormat(Span<char>, out int, ReadOnlySpan<char>, IFormatProvider)` | Formats into the caller's buffer; false when it does not fit. | format-matrix.tsv (try_format); format-pattern.tsv (content) |
| NF08 | `enum DateFormats` | Six orders: `YearMonthDay`, `YearDayMonth`, `MonthYearDay`, `MonthDayYear`, `DayYearMonth`, `DayMonthYear`. | format-matrix.tsv (order) |
| NF09 | `enum Separators` | Six separators: `/`, `\`, `.`, `_`, `-`, space. | format-matrix.tsv (separator) |
| NF10 | `enum NepaliMonths` | Twelve month names, `Baishakh` (1) to `Chaitra` (12). | dates.tsv (long_en, long_ne) |

**Required behaviour.** The long form is `[<Weekday>, ]<Month> <day>[, <year>]`. `pad` pads
month and day to two digits, the year is always four digits, and the weekday appears only when
requested. Formatter output is covered by the `to_string`, `unicode`, `long_en`,
`long_en_weekday`, `long_ne`, `long_ne_weekday` and `dmy_dash_nopad` columns of `dates.tsv`, and
by the 452 rows of `format-pattern.tsv`. The six orders and six separators are covered by
`format-matrix.tsv`, which also records the short-buffer case for `TryFormat`.

Custom pattern tokens: `yyyy` and longer runs write the four-digit year; `y`, `yy` and `yyy`
write the last two digits; `MMMM` writes the full month name; `MMM` writes the first three
characters of the English name; `MM` writes two digits; `M` writes the number; `dd` writes two
digits; any other run of `d` writes the number; a backslash escapes the next character; text
inside single quotes is literal.

`MMM` has a C# defect: `Ashad` and `Ashoj` both become `Ash`. `D-04` fixes them to `Asa` and
`Aso`. Every other `MMM` output matches C#.

The Unicode forms are the same formatters with the Nepali language selector, not separate
methods. `D-15` keeps the standard patterns and allows Nepali output through that selector.

**Naming.**

| ID | C# | Rust | Go | TypeScript | Python | Swift | Kotlin | C++ | C | PHP | Dart | Java | Ruby | Zig |
|---|---|---|---|---|---|---|---|---|---|---|---|------|
| NF01 | `override string ToString()` | `Display` | `String()` | `toString()` | `__str__` | `description` | `toString()` | `to_string()` | `nepdate_format(date, NULL, ...)` | `__toString()` | `toString()` | `toString()` | `#to_s` | `Date.write(buffer)`, `Date.toStringAlloc(allocator)`, `{f}` |
| NF02 | `string ToString(DateFormats dateFormat, Separators separator = Separators.ForwardSlash, bool leadingZeros = true)` | `short(order, sep, pad, lang)` | `Short(o DateOrder, sep Separator, pad bool, lang Lang) string` | `short({order, separator, pad, lang})` | `short(order, sep, pad, lang)` | `short(order:separator:pad:lang:)` | `toShortString(order, separator, pad, lang)` | `to_chars_short` | `nepdate_format_short` | `toShortString(DateOrder $order, Separator $separator, bool $pad, Lang $lang): string` | `String toShortString({DateOrder order = DateOrder.ymd, Separator separator = Separator.slash, bool pad = true, Lang lang = Lang.en})` | `toShortString(DateOrder, Separator, boolean, Language)` | `#to_short_string(order:, separator:, pad:, lang:)` | `Date.shortAlloc(allocator, options)` |
| NF03 | `string ToLongDateString(bool leadingZeros = true, bool displayDayName = false, bool displayYear = true)` | `long(LongDate, lang)` | `Long(o LongOptions, lang Lang) string` | `long({weekday, year, pad, lang})` | `long(weekday, year, pad, lang)` | `long(weekday:year:pad:lang:)` | `toLongString(weekday, year, pad, lang)` | `to_chars_long` | `nepdate_format_long` | `toLongString(bool $weekday = false, bool $year = true, bool $pad = true, Lang $lang = Lang::En): string` | `String toLongString({bool weekday = false, bool year = true, bool pad = true, Lang lang = Lang.en})` | `toLongString(LongDateOptions, Language)` | `#to_long_string(weekday:, year:, pad:, lang:)` | `Date.longAlloc(allocator, options)` |
| NF04 | `string ToUnicodeString(DateFormats dateFormat = DateFormats.YearMonthDay, Separators separator = Separators.ForwardSlash, bool leadingZeros = true)` | `short(..., Lang::Ne)` | `Short(..., Nepali)` | `short({lang: 'ne'})` | `short(..., lang=Lang.NE)` | `short(..., lang: .nepali)` | `toShortString(..., Lang.NEPALI)` | `to_chars_short(..., lang::ne)` | `NEPDATE_NEPALI` | `toShortString(..., Lang::Ne)` | `toShortString(..., lang: Lang.ne)` | `toShortString(..., Language.NEPALI)` | `#to_unicode_string(order:, separator:, pad:)` | `Date.shortAlloc(allocator, .{ .lang = .ne })` |
| NF05 | `string ToLongDateUnicodeString(bool leadingZeros = true, bool displayDayName = false, bool displayYear = true)` | `long(..., Lang::Ne)` | `Long(..., Nepali)` | `long({lang: 'ne'})` | `long(..., lang=Lang.NE)` | `long(..., lang: .nepali)` | `toLongString(..., Lang.NEPALI)` | `to_chars_long(..., lang::ne)` | `NEPDATE_NEPALI` | `toLongString(..., Lang::Ne)` | `toLongString(..., lang: Lang.ne)` | `toLongString(..., Language.NEPALI)` | `#to_long_unicode_string(weekday:, year:, pad:)` | `Date.longAlloc(allocator, .{ .lang = .ne })` |
| NF06 | `string ToString(string format, IFormatProvider formatProvider = null)` | `format_pattern(pattern, lang)` | `Format(pattern string, lang Lang) string` | `format(pattern, lang)` | `format(pattern, lang)` | `format(_:lang:)` | `format(pattern, lang)` | `to_chars_pattern` | `nepdate_format` | `format(string $pattern, Lang $lang = Lang::En): string` | `String format(String pattern, {Lang lang = Lang.en})` | `format(String, Language)` | `#format(pattern, lang:)` | `Date.formatAlloc(allocator, pattern, lang)` |
| NF07 | `bool ISpanFormattable.TryFormat(Span<char>, out int, ReadOnlySpan<char>, IFormatProvider)` | `write_to` | `AppendTo` | `none; string return` | `none; string return` | `append*` | `append*String`, `formatTo` | `to_chars*` | `snprintf` semantics | `none; string return` | `none; string return` | `tryFormat(char[], String, Language)`, `appendTo(StringBuilder, ...)` | not ported (no buffer) | `Date.formatTo(buffer, pattern, lang)` |
| NF08 | `enum DateFormats` | `DateOrder` | `DateOrder` | `DateOrder` string union | `DateOrder` | `DateOrder` | `DateOrder` | `date_order` | `nepdate_order` | `DateOrder` enum | `DateOrder` enum | `DateOrder` | `DateOrder` symbols | `Order` |
| NF09 | `enum Separators` | `Separator` | `Separator` | `Separator` string union | `Separator` | `Separator` | `Separator` | `separator` | `nepdate_separator` | `Separator` enum | `Separator` enum | `Separator` | `Separator` symbols | `Separator` |
| NF10 | `enum NepaliMonths` | `Month` | `Month` | `monthName`, `monthShortName` | `Month` | `Month` | `Month` | `month` | `nepdate_month` | `Month` enum | `Month` enum | `Month` | `Month` | `Month` |
|------|
## 5. Parsing

| ID | C# signature | What the code does | Vectors |
|---|---|---|---|
| NP01 | `static NepaliDate Parse(string rawNepaliDate)` | Strict parse; throws `InvalidNepaliDateFormatException` on failure. | parse.tsv (strict) |
| NP02 | `static NepaliDate Parse(string rawNepaliDate, bool autoAdjust, bool monthInMiddle = true)` | Strict parse plus the ND03 heuristics. | not ported (D-05, D-06); no vector |
| NP03 | `static bool TryParse(string rawNepDate, out NepaliDate result)` | Non-throwing strict parse; false leaves no result. | parse.tsv (strict, ERR) |
| NP04 | `static bool TryParse(string rawNepDate, out NepaliDate result, bool autoAdjust, bool monthInMiddle = true)` | Non-throwing parse with the ND03 heuristics. | not ported (D-05, D-06); no vector |
| NP05 | `static NepaliDate IParsable<NepaliDate>.Parse(string, IFormatProvider)` and `TryParse` | .NET 7 interface; delegates to NP01 and NP03; the provider is ignored. | language trait; no vector |
| NP06 | `static NepaliDate ISpanParsable<NepaliDate>.Parse(ReadOnlySpan<char>, IFormatProvider)` and `TryParse` | .NET 7 interface on spans; copies to a string and delegates. | language span type; no vector |
| NP07 | `static NepaliDate SmartDateParser.Parse(string input)` | Lenient parse: strict first, then normalization, Devanagari digits, month names, suffixes and permutation heuristics. | parse.tsv (lenient) |
| NP08 | `static bool SmartDateParser.TryParse(string input, out NepaliDate result)` | Non-throwing lenient parse; catches every exception. | parse.tsv (lenient, ERR) |
| NP09 | `sealed class InvalidNepaliDateFormatException : FormatException` | The parse and format failure type. | parse.tsv (ERR); error kinds in PARSING.md |

**Required behaviour.** Strict parsing accepts exactly year, month and day in that order.
Separators are `-`, `/`, `.`, `_`, `\`, space, `|` and `।` (U+0964). Leading, trailing and
repeated separators are ignored. A fourth group, fewer than three groups, or any other character
fails. C# does not cap the number of digits in a group, so a long run can overflow `int`; `D-05`
caps a group at four digits and accepts Devanagari digits, mixed with ASCII. `parse.tsv` has 69
rows with the strict and lenient result, and `csharp-golden/parse.tsv` has 42 rows recorded from
C# for comparison only.

`autoAdjust` is three heuristics: if the day is over 32, swap year and day; if `monthInMiddle` is
false, swap month and day; if the month is over 12 and the day is under 13, swap them; if the year
is under 1000, add 2000. `D-05` and `D-06` replace this with a strict parser and a deterministic
lenient parser, so no port needs NP02 or NP04.

`SmartDateParser` is deliberately fuzzy in C#: it normalizes whitespace, removes `B.S.`, `V.S.`,
`गते` and `मिति`, converts Devanagari digits, finds the longest month name anywhere in the string,
and then tries the YMD, DMY and MDY orders before a six-permutation fallback. It expands
two-digit years to 20xx and ignores extra numeric groups. `D-06` replaces this with the closed
month list and explicit rules of `PARSING.md`; the vectors are `parse.tsv`.

Every port has a non-throwing strict parse and a non-throwing lenient parse, under its own name.
The exception inherits `FormatException` in C#; a port uses its own error type but exposes the
parse error kinds of `PARSING.md` section 4.

**Naming.**

| ID | C# | Rust | Go | TypeScript | Python | Swift | Kotlin | C++ | C | PHP | Dart | Java | Ruby | Zig |
|---|---|---|---|---|---|---|---|---|---|---|---|------|
| NP01 | `static NepaliDate Parse(string rawNepaliDate)` | `FromStr::from_str` | `Parse(s)` | `NepaliDate.parse(s)` | `NepaliDate.parse(s)` | `NepaliDate.parse(_:)` | `NepaliDate.parse(text)` | `parse(string_view)` | `nepdate_parse(text, len, &out)` | `NepaliDate::parse(string $text): self` | `NepaliDate.parse(String text)` | `parse(CharSequence)` | `.parse` | `Date.parse(text)` |
| NP02 | `static NepaliDate Parse(string rawNepaliDate, bool autoAdjust, bool monthInMiddle = true)` | not ported (D-05, D-06) | not ported (D-05, D-06) | not ported (D-05, D-06) | not ported (D-05, D-06) | not ported (D-05, D-06) | not ported (D-05, D-06) | not ported (D-05, D-06) | not ported (D-05, D-06) | not ported (D-05, D-06) | not ported (D-05, D-06) | not ported (D-05, D-06) | not ported (D-05, D-06) | not ported (strict and lenient parsers) |
| NP03 | `static bool TryParse(string rawNepDate, out NepaliDate result)` | `try_parse(s) -> Result` | `TryParse(s string) (Date, bool)` | `NepaliDate.tryParse(s)` | `NepaliDate.try_parse(s)` | `init?(_:)` | `NepaliDate.parseOrNull(text)` | `try_parse(string_view) -> std::optional<date>` | `nepdate_parse` status | `NepaliDate::tryParse(string $text): ?self` | `NepaliDate? tryParse(String text)` | `tryParse(CharSequence) -> Optional` | `.try_parse` | `Date.tryParse(text)` |
| NP04 | `static bool TryParse(string rawNepDate, out NepaliDate result, bool autoAdjust, bool monthInMiddle = true)` | not ported (D-05, D-06) | not ported (D-05, D-06) | not ported (D-05, D-06) | not ported (D-05, D-06) | not ported (D-05, D-06) | not ported (D-05, D-06) | not ported (D-05, D-06) | not ported (D-05, D-06) | not ported (D-05, D-06) | not ported (D-05, D-06) | not ported (D-05, D-06) | not ported (D-05, D-06) | not ported (strict and lenient parsers) |
| NP05 | `static NepaliDate IParsable<NepaliDate>.Parse(string, IFormatProvider)` and `TryParse` | `FromStr` | `Parse` | `parse` | `parse` | `LosslessStringConvertible` | `parse` | `parse` | `nepdate_parse` | `parse` | `parse` | `parse` (language mechanism) | `.parse`, `.try_parse` | `Date.parse(text)` |
| NP06 | `static NepaliDate ISpanParsable<NepaliDate>.Parse(ReadOnlySpan<char>, IFormatProvider)` and `TryParse` | `parse(&str)` | `Parse` | `parse` | `parse` | `parse` (`StringProtocol`) | `parse(CharSequence)` | `parse(string_view)` | `nepdate_parse(text, len)` | `parse(string)` | `parse(String)` | `parse` (language mechanism) | `.parse`, `.try_parse` | `Date.parse(text)` (byte slice) |
| NP07 | `static NepaliDate SmartDateParser.Parse(string input)` | `parse_lenient(s)` | `ParseLenient(s)` | `NepaliDate.parseLenient(s)` | `NepaliDate.parse_lenient(s)` | `NepaliDate.parseLenient(_:)` | `NepaliDate.parseLenient(text)` | `parse_lenient(string_view)` | `nepdate_parse_lenient(text, len, &out)` | `NepaliDate::parseLenient(string $text): self` | `NepaliDate.parseLenient(String text)` | `parseLenient(CharSequence)` | `.parse_lenient` | `Date.parseLenient(text)` |
| NP08 | `static bool SmartDateParser.TryParse(string input, out NepaliDate result)` | `try_parse_lenient(s) -> Result` | `TryParseLenient(s string) (Date, bool)` | `tryParseLenient(s)` | `try_parse_lenient(s)` | `init?(lenient:)` | `parseLenientOrNull(text)` | `try_parse_lenient(string_view) -> std::optional<date>` | `nepdate_parse_lenient` status | `NepaliDate::tryParseLenient(string $text): ?self` | `NepaliDate? tryParseLenient(String text)` | `tryParseLenient(CharSequence) -> Optional` | `.try_parse_lenient` | `Date.tryParseLenient(text)` |
| NP09 | `sealed class InvalidNepaliDateFormatException : FormatException` | `Error::Parse(ParseErrorKind)` | `*ParseError` plus sentinels | `NepDateError` | `NepDateError` | `NepDateError` | `NepDateException` | `errc` and `result<T>` | `nepdate_status` | `NepDateException` | `NepDateException` | `NepDateException`, `NepDateError` | `Error` hierarchy | `Error` |
|------|
## 6. Ranges

| ID | C# signature | What the code does | Vectors |
|---|---|---|---|
| NR01 | `readonly struct NepaliDateRange : IEnumerable<NepaliDate>, IEquatable<NepaliDateRange>` | Inclusive range from `Start` to `End`; both ends are dates. | ranges.tsv |
| NR02 | `NepaliDateRange(NepaliDate start, NepaliDate end)` | Stores the bounds; `start > end` means an empty range in C#, and is refused in a port that has no empty range. | ranges.tsv (construct; D-01) |
| NR03 | `NepaliDate Start { get; }` | First date. | ranges.tsv (start) |
| NR04 | `NepaliDate End { get; }` | Last date, inclusive. | ranges.tsv (end) |
| NR05 | `bool IsEmpty { get; }` | `Start > End`. | not exposed; ports are non-empty (D-01); no vector |
| NR06 | `int Length { get; }` | Number of days, inclusive; 0 for an empty range. | ranges.tsv (length) |
| NR07 | `static NepaliDateRange SingleDay(NepaliDate date)` | Range of one date. | range-factories.tsv (single_day) |
| NR08 | `static NepaliDateRange FromDayCount(NepaliDate start, int days)` | `start` to `start + days - 1`; `days` must be at least 1. | range-factories.tsv (from_day_count) |
| NR09 | `static NepaliDateRange ForMonth(int year, int month)` | Day 1 to the last day of the BS month. | range-factories.tsv (for_month) |
| NR10 | `static NepaliDateRange ForFiscalYear(int fiscalYear)` | 1 Shrawan of the fiscal year to the last day of Ashad of the next year. | dates.tsv (fy_start, fy_end); range-factories.tsv (for_fiscal_year) |
| NR11 | `static NepaliDateRange ForCalendarYear(int year)` | 1 Baishakh to the last day of Chaitra. | range-factories.tsv (for_calendar_year) |
| NR12 | `static NepaliDateRange CurrentMonth()` | `ForMonth` of today's BS month. | clock; no vector |
| NR13 | `static NepaliDateRange CurrentFiscalYear()` | `ForFiscalYear` of today's fiscal year. | clock; no vector |
| NR14 | `static NepaliDateRange CurrentCalendarYear()` | `ForCalendarYear` of today's BS year. | clock; no vector |
| NR15 | `bool Contains(NepaliDate date)` | True when the range is not empty and the date is between the bounds. | ranges.tsv (contains_date) |
| NR16 | `bool Contains(NepaliDateRange other)` | True when this range fully contains the other; an empty other is contained by any non-empty range. | ranges.tsv (contains_range) |
| NR17 | `bool Overlaps(NepaliDateRange other)` | True when the two non-empty ranges share a date. | ranges.tsv (overlaps) |
| NR18 | `bool IsAdjacentTo(NepaliDateRange other)` | True when one range starts the day after the other ends. | ranges.tsv (is_adjacent_to; D-16) |
| NR19 | `NepaliDateRange Intersect(NepaliDateRange other)` | Overlap, or an empty range when they do not overlap. A port without an empty range returns no value. | ranges.tsv (intersect; D-01, D-16) |
| NR20 | `NepaliDateRange Union(NepaliDateRange other)` | Smallest range covering both; `this` or `other` when one is empty. | ranges.tsv (union) |
| NR21 | `NepaliDateRange[] Except(NepaliDateRange other)` | This range with the other removed; zero, one or two ranges. | ranges.tsv (except) |
| NR22 | `NepaliDateRange[] SplitByMonth()` | One range per month touched, clipped at the range ends. | range-splits.tsv (split_by_month; D-16) |
| NR23 | `NepaliDateRange[] SplitByFiscalQuarter()` | One range per fiscal quarter touched, clipped at the range ends. | range-splits.tsv (split_by_fiscal_quarter; D-16) |
| NR24 | `IEnumerable<NepaliDate> DatesWithInterval(int interval)` | Every `interval` days from `Start`; `interval` must be at least 1. | range-days.tsv (dates_with_interval; D-16) |
| NR25 | `IEnumerable<NepaliDate> WorkingDays(bool excludeSunday = false)` | Every day except Saturday, and except Sunday when asked. | range-days.tsv (working_days; D-16) |
| NR26 | `IEnumerable<NepaliDate> WeekendDays(bool includeSunday = true)` | Every Saturday, and Sunday when asked. | range-days.tsv (weekend_days; D-16) |
| NR27 | `IEnumerator<NepaliDate> GetEnumerator()` | Every day from `Start` to `End`. | ranges.tsv (enumerate; D-16) |
| NR28 | `override string ToString()` | `"start - end"`, or `"Empty Range"`. | range-format.tsv (default) |
| NR29 | `string ToString(DateFormats format, Separators separator = Separators.ForwardSlash)` | The two dates formatted with the same options, separated by `" - "`. | range-format.tsv (short; D-15) |
| NR30 | `Equals`, `GetHashCode`, `operator ==`, `operator !=` | Value equality of the bounds; two empty ranges are equal in C#. | ranges.tsv (equals) |

**Required behaviour.** A range is inclusive. A port that can represent only valid dates has no
empty range and refuses `start > end`; the C# empty range exists because a struct cannot refuse
the constructor. The full C# surface is now required: construction, bounds, length, containment
of a date and of another range, overlap, adjacency, intersection, union, difference, the month and
fiscal-quarter splits, the interval and working-day helpers, iteration, and both string forms.

C# has a defect worth not copying: two empty ranges compare equal but hash their different
bounds, so equal values can hash differently. A port that supports an empty range should hash only
the bounds when non-empty and a constant when empty. A port without an empty range hashes the two
bounds.

The range set operations are pure value operations. `Intersect` returns no value when the ranges
do not overlap. `Union` is the smallest covering range, not a set union. `Except` returns zero,
one or two ranges. `SplitByMonth` and `SplitByFiscalQuarter` clip the first and last piece to the
range ends.

C# advances to the next month, quarter or day after it has added the last piece or yielded the
last date, so a range that reaches 2199-12-30 makes both splits and every day helper throw. It
also computes the day after a range end in `IsAdjacentTo`, and `Intersect` throws for disjoint
ranges when the receiver starts at 1901-01-01. `D-16` is the decision: a port never throws on a
valid range because it touches the data end. Both splits return every piece, the last ending at
the range end; the enumerator, `WorkingDays`, `WeekendDays` and `DatesWithInterval` return every
date up to and including 2199-12-30; `IsAdjacentTo` returns `false` when the other range would
have to start after 2199-12-30, and in the mirror case before 1901-01-01; `Intersect` of disjoint
ranges returns the port's "no range" value. The `D-16` rows in `range-splits.tsv`,
`range-days.tsv` and `ranges.tsv` hold the port's expected value, not the C# error.

**Naming.**

| ID | C# | Rust | Go | TypeScript | Python | Swift | Kotlin | C++ | C | PHP | Dart | Java | Ruby | Zig |
|---|---|---|---|---|---|---|---|---|---|---|---|------|
| NR01 | `readonly struct NepaliDateRange : IEnumerable<NepaliDate>, IEquatable<NepaliDateRange>` | `NepaliDateRange` | `Range` | `DateRange` | `DateRange` | `NepaliDateRange` | `NepaliDateRange` | `date_range` | `nepdate_range` | `NepaliDateRange` | `NepaliDateRange` | `NepaliDateRange` | `DateRange` | `Range` |
| NR02 | `NepaliDateRange(NepaliDate start, NepaliDate end)` | `NepaliDateRange::new(start, end) -> Option` | `NewRange(start, end Date) (Range, bool)` | `DateRange.of(start, end)` | `DateRange(start, end)` | `NepaliDateRange(_:_:)` optional | `NepaliDateRange.of(start, end)` | `date_range::make(start, end) -> result<date_range>` | `nepdate_range_make(start, end, &out)` | `NepaliDateRange::of(NepaliDate $start, NepaliDate $end): self` | `NepaliDateRange.of(NepaliDate start, NepaliDate end)` | `NepaliDateRange.of(start, end)` | `.new(start, end)` | `Range.of(start, end)` |
| NR03 | `NepaliDate Start { get; }` | `start()` | `Start()` | `start` | `start` | `lowerBound` | `start` | `first()` | `start` | `start(): NepaliDate` | `NepaliDate get start` | `start()` | `#start` | `Range.start` |
| NR04 | `NepaliDate End { get; }` | `end()` | `End()` | `end` | `end` | `upperBound` | `endInclusive` | `last()` | `end` | `end(): NepaliDate` | `NepaliDate get end` | `end()` | `#end` | `Range.end` |
| NR05 | `bool IsEmpty { get; }` | not exposed; ranges are non-empty (D-01) | not exposed; ranges are non-empty (D-01) | not exposed; ranges are non-empty (D-01) | not exposed; ranges are non-empty (D-01) | not exposed; ranges are non-empty (D-01) | not exposed; ranges are non-empty (D-01) | not exposed; ranges are non-empty (D-01) | not exposed; ranges are non-empty (D-01) | not exposed; ranges are non-empty (D-01) | not exposed; ranges are non-empty (D-01) | not exposed (D-01) | not exposed (D-01) | not exposed (non-empty) |
| NR06 | `int Length { get; }` | `len() -> u32` | `Len() int` | `length` | `len(range)` | `count` | `size` | `size()` | `nepdate_range_length` | `count()` | `int get length` | `length()` | `#length`, `#size` | `Range.length()` |
| NR07 | `static NepaliDateRange SingleDay(NepaliDate date)` | `NepaliDateRange::single(d)` | `SingleDay(d Date) Range` | `DateRange.single(d)` | `DateRange.single(d)` | `NepaliDateRange.singleDay(_:)` | `NepaliDateRange.singleDay(d)` | `date_range::single(d)` | `nepdate_range_make(d, d)` | `NepaliDateRange::singleDay(NepaliDate $d): self` | `NepaliDateRange.singleDay(NepaliDate d)` | `singleDay(date)` | `.single_day` | `Range.single(date)` |
| NR08 | `static NepaliDateRange FromDayCount(NepaliDate start, int days)` | `from_day_count(start, days) -> Result` | `FromDayCount(start Date, days int) (Range, error)` | `DateRange.fromDayCount(start, days)` | `DateRange.from_day_count(start, days)` | `NepaliDateRange(start:dayCount:)` | `NepaliDateRange.fromDayCount(start, days)` | `date_range::from_day_count(start, days)` | `nepdate_range_from_day_count(start, days, &out)` | `NepaliDateRange::fromDayCount(NepaliDate $start, int $days): self` | `NepaliDateRange.fromDayCount(NepaliDate start, int days)` | `fromDayCount(start, days)` | `.from_day_count` | `Range.fromDayCount(start, days)` |
| NR09 | `static NepaliDateRange ForMonth(int year, int month)` | `NepaliDateRange::month(y, m)` | `MonthRange(y, m)` | `DateRange.month(y, m)` | `DateRange.month(y, m)` | `NepaliDateRange.month(year:month:)` | `NepaliDateRange.ofMonth(y, m)` | `date_range::of_month(y, m)` | `nepdate_range_of_month(y, m, &out)` | `NepaliDateRange::forMonth(int $year, int $month): self` | `NepaliDateRange.forMonth(int year, int month)` | `forMonth(year, month)` | `.for_month` | `Range.forMonth(year, month)` |
| NR10 | `static NepaliDateRange ForFiscalYear(int fiscalYear)` | `FiscalYear::range()` | `FiscalYear.Range()` | `FiscalYear.range()` | `FiscalYear.range()` | `FiscalYear.range()` | `FiscalYear.range()` | `fiscal_year::range()` | `nepdate_range_of_fiscal_year` | `FiscalYear::range()` | `FiscalYear.range()` | `FiscalYear.of(fy).range()` | `FiscalYear#range` | `Range.forFiscalYear(year)` |
| NR11 | `static NepaliDateRange ForCalendarYear(int year)` | `NepaliDateRange::year(y)` | `YearRange(y)` | `DateRange.year(y)` | `DateRange.year(y)` | `NepaliDateRange.year(_:)` | `NepaliDateRange.ofYear(y)` | `date_range::of_year(y)` | `nepdate_range_of_year` | `NepaliDateRange::forYear(int $year): self` | `NepaliDateRange.forYear(int year)` | `forYear(year)` | `.for_year` | `Range.forCalendarYear(year)` |
| NR12 | `static NepaliDateRange CurrentMonth()` | `current_month(zone)` | `CurrentMonth(loc)` | `DateRange.currentMonth(zone)` | `DateRange.current_month(tz)` | `NepaliDateRange.currentMonth(in:)` | `NepaliDateRange.currentMonth(zone)` | `current_month(offset)` | `nepdate_range_current_month(offset, unix, &out)` | `NepaliDateRange::currentMonth(DateTimeZone $zone): self` | `NepaliDateRange.currentMonth(String timeZone)` | `currentMonth(ZoneId)`, `currentMonth(Clock)` | `.current_month` | `Range.currentMonth(offset, unix)` |
| NR13 | `static NepaliDateRange CurrentFiscalYear()` | `current_fiscal_year(zone)` | `CurrentFiscalYear(loc)` | `DateRange.currentFiscalYear(zone)` | `DateRange.current_fiscal_year(tz)` | `NepaliDateRange.currentFiscalYear(in:)` | `NepaliDateRange.currentFiscalYear(zone)` | `current_fiscal_year(offset)` | `nepdate_range_current_fiscal_year` | `NepaliDateRange::currentFiscalYear(DateTimeZone $zone): self` | `NepaliDateRange.currentFiscalYear(String timeZone)` | `currentFiscalYear(ZoneId)`, `currentFiscalYear(Clock)` | `.current_fiscal_year` | `Range.currentFiscalYear(offset, unix)` |
| NR14 | `static NepaliDateRange CurrentCalendarYear()` | `current_calendar_year(zone)` | `CurrentCalendarYear(loc)` | `DateRange.currentCalendarYear(zone)` | `DateRange.current_calendar_year(tz)` | `NepaliDateRange.currentCalendarYear(in:)` | `NepaliDateRange.currentCalendarYear(zone)` | `current_calendar_year(offset)` | `nepdate_range_current_calendar_year` | `NepaliDateRange::currentCalendarYear(DateTimeZone $zone): self` | `NepaliDateRange.currentCalendarYear(String timeZone)` | `currentYear(ZoneId)`, `currentYear(Clock)` | `.current_calendar_year` | `Range.currentCalendarYear(offset, unix)` |
| NR15 | `bool Contains(NepaliDate date)` | `contains(date)` | `Contains(d)` | `contains(date)` | `date in range` | `contains(_:)` | `contains(element)` | `contains(date)` | `nepdate_range_contains` | `contains(NepaliDate): bool` | `bool contains(NepaliDate)` | `contains(NepaliDate)` | `#include?`, `#contains?` | `Range.contains(date)` |
| NR16 | `bool Contains(NepaliDateRange other)` | `contains_range(other)` | `ContainsRange` | `containsRange` | `contains_range` | `contains(_ other:)` | `containsRange(other)` | `contains(other_range)` | `nepdate_range_contains_range` | `containsRange` | `containsRange` | `contains(NepaliDateRange)` | `#contains_range?` | `Range.containsRange(other)` |
| NR17 | `bool Overlaps(NepaliDateRange other)` | `overlaps` | `Overlaps` | `overlaps` | `overlaps` | `overlaps(_:)` | `overlaps` | `overlaps` | `nepdate_range_overlaps` | `overlaps` | `overlaps` | `overlaps(other)` | `#overlaps?` | `Range.overlaps(other)` |
| NR18 | `bool IsAdjacentTo(NepaliDateRange other)` | `is_adjacent_to` | `IsAdjacentTo` | `isAdjacentTo` | `is_adjacent_to` | `isAdjacent(to:)` | `isAdjacentTo` | `is_adjacent_to` | `nepdate_range_is_adjacent_to` | `isAdjacentTo` | `isAdjacentTo` | `isAdjacentTo(other)` | `#adjacent_to?` | `Range.isAdjacentTo(other)` |
| NR19 | `NepaliDateRange Intersect(NepaliDateRange other)` | `intersect` -> Option | `Intersect` -> (Range, bool) | `intersect` -> DateRange? | `intersect` -> Optional[DateRange] | `intersection(_:)` -> NepaliDateRange? | `intersect` -> NepaliDateRange? | `intersect` -> std::optional<date_range> | `nepdate_range_intersect` | `intersect` -> ?NepaliDateRange | `intersect` -> NepaliDateRange? | `intersect(other) -> Optional` | `#intersect` -> `DateRange?` | `Range.intersect(other)` returns `!Range` |
| NR20 | `NepaliDateRange Union(NepaliDateRange other)` | `union` | `Union` | `union` | `union` | `union(_:)` | `union` | `union_with` | `nepdate_range_union` | `union` | `union` | `union(other)` | `#union` | `Range.unionWith(other)` |
| NR21 | `NepaliDateRange[] Except(NepaliDateRange other)` | `except` -> Vec | `Except` -> []Range | `except` -> DateRange[] | `difference` -> list | `subtracting(_:)` -> [NepaliDateRange] | `minus` -> List | `subtract` -> vector | `nepdate_range_except` | `except` -> array | `except` -> List | `except(other) -> List` | `#except` | `Range.except(other, *[2]Range)` |
| NR22 | `NepaliDateRange[] SplitByMonth()` | `split_by_month` | `SplitByMonth` | `splitByMonth` | `split_by_month` | `splitByMonth()` | `splitByMonth()` | `split_by_month` | `nepdate_range_split_by_month` | `splitByMonth` | `splitByMonth` | `splitByMonth()` | `#split_by_month` | `Range.splitByMonth(allocator)` |
| NR23 | `NepaliDateRange[] SplitByFiscalQuarter()` | `split_by_fiscal_quarter` | `SplitByFiscalQuarter` | `splitByFiscalQuarter` | `split_by_fiscal_quarter` | `splitByFiscalQuarter()` | `splitByFiscalQuarter()` | `split_by_fiscal_quarter` | `nepdate_range_split_by_fiscal_quarter` | `splitByFiscalQuarter` | `splitByFiscalQuarter` | `splitByFiscalQuarter()` | `#split_by_fiscal_quarter` | `Range.splitByFiscalQuarter(allocator)` |
| NR24 | `IEnumerable<NepaliDate> DatesWithInterval(int interval)` | `dates_with_interval(interval)` | `DatesWithInterval` | `datesWithInterval` | `dates_with_interval` | `dates(every:)` | `datesWithInterval` | `dates_with_interval` | `nepdate_range_dates_with_interval` | `datesWithInterval` | `datesWithInterval` | `datesWithInterval(int) -> Stream` | `#dates_with_interval` | `Range.datesWithInterval(interval)` |
| NR25 | `IEnumerable<NepaliDate> WorkingDays(bool excludeSunday = false)` | `working_days(exclude_sunday)` | `WorkingDays(excludeSunday)` | `workingDays({excludeSunday})` | `working_days(exclude_sunday=False)` | `workingDays(excludingSunday:)` | `workingDays(excludeSunday)` | `working_days(bool)` | `nepdate_range_working_days` | `workingDays(bool $excludeSunday = false)` | `workingDays({bool excludeSunday = false})` | `workingDays(boolean) -> Stream` | `#working_days` | `Range.workingDays(excludeSunday)` |
| NR26 | `IEnumerable<NepaliDate> WeekendDays(bool includeSunday = true)` | `weekend_days(include_sunday)` | `WeekendDays(includeSunday)` | `weekendDays({includeSunday})` | `weekend_days(include_sunday=True)` | `weekendDays(includingSunday:)` | `weekendDays(includeSunday)` | `weekend_days(bool)` | `nepdate_range_weekend_days` | `weekendDays(bool $includeSunday = true)` | `weekendDays({bool includeSunday = true})` | `weekendDays(boolean) -> Stream` | `#weekend_days` | `Range.weekendDays(includeSunday)` |
| NR27 | `IEnumerator<NepaliDate> GetEnumerator()` | `IntoIterator` | `All() iter.Seq[Date]` | `[Symbol.iterator]` | `__iter__` | `Collection` | `List` | `begin`/`end` | `nepdate_range_begin`/`next` | `IteratorAggregate` | `Iterable<NepaliDate>` | `Iterable.iterator()`, `stream()` | `#each` | `Range.iterator()` |
| NR28 | `override string ToString()` | `Display` | `String()` | `toString()` | `__str__` | `description` | `toString()` | `to_string(date_range)` | `nepdate_range_format` | `__toString()` | `toString()` | `toString()` | `#to_s` | `Range.formatAlloc(allocator)`, `{f}` |
| NR29 | `string ToString(DateFormats format, Separators separator = Separators.ForwardSlash)` | `to_string_with(order, sep, lang)` | `Format(order, sep, lang)` | `toString({order, separator, lang})` | `to_string(order, sep, lang)` | `formatted(order:separator:lang:)` | `toFormattedString(order, separator, lang)` | `to_chars_range` | `nepdate_range_format_short` | `toStringWith(DateOrder, Separator, Lang)` | `toStringWith(DateOrder order, Separator separator, Lang lang)` | `toShortString(DateOrder, Separator, Language)` | `#to_short_string` | `Range.formatShortAlloc(allocator, order, separator, lang)` |
| NR30 | `Equals`, `GetHashCode`, `operator ==`, `operator !=` | `PartialEq`, `Eq`, `Hash` | `==` | `equals(other)` | `__eq__`, `__hash__` | `Equatable`, `Hashable` | `equals`, `hashCode` | `operator==`, `std::hash` | `nepdate_range_equal` | `equals(NepaliDateRange): bool` | `operator ==`, `hashCode` | `equals`, `hashCode` | `#==`, `#hash` | `Range.eql(other)` |
|------|
## 7. Fiscal year

| ID | C# signature | What the code does | Vectors |
|---|---|---|---|
| NY01 | `enum FiscalYearQuarters` | `Current = 0`, `First = 4`, `Second = 7`, `Third = 10`, `Fourth = 1`; the value is the first month of the quarter. | dates.tsv (quarter_start, quarter_end); mapping in NY01 |
| NY02 | `NepaliDate FiscalYearStartDate(int yearOffset = 0)` | 1 Shrawan of the fiscal year containing this date, plus the offset. | dates.tsv (fy_start) |
| NY03 | `NepaliDate FiscalYearEndDate(int yearOffset = 0)` | Last day of Ashad of the fiscal year, plus the offset. | dates.tsv (fy_end) |
| NY04 | `(NepaliDate startDate, NepaliDate endDate) FiscalYearStartAndEndDate(int yearOffset = 0)` | The two values above. | dates.tsv (fy_start, fy_end) |
| NY05 | `NepaliDate FiscalYearQuarterStartDate(FiscalYearQuarters q = Current, int yearOffset = 0)` | First day of the requested quarter; `Current` resolves from this date's month. | dates.tsv (quarter_start) |
| NY06 | `NepaliDate FiscalYearQuarterEndDate(FiscalYearQuarters q = Current, int yearOffset = 0)` | Last day of the requested quarter. | dates.tsv (quarter_end) |
| NY07 | `(NepaliDate startDate, NepaliDate endDate) FiscalYearQuarterStartAndEndDate(FiscalYearQuarters q = Current, int yearOffset = 0)` | The two quarter values. | dates.tsv (quarter_start, quarter_end) |
| NY08 | `static NepaliDate GetFiscalYearStartDate(int fiscalYear)` | 1 Shrawan of the named fiscal year. | dates.tsv (fy_start) |
| NY09 | `static NepaliDate GetFiscalYearEndDate(int fiscalYear)` | Last day of Ashad of the named fiscal year. | dates.tsv (fy_end) |
| NY10 | `static (NepaliDate, NepaliDate) GetFiscalYearStartAndEndDate(int fiscalYear)` | The two static values above. | dates.tsv (fy_start, fy_end) |
| NY11 | `static NepaliDate GetFiscalYearQuarterStartDate(int fiscalYear, int month)` | Builds `fiscalYear-month-1` and asks for its quarter start. | not ported (D-08); use the quarter value; no vector |
| NY12 | `static NepaliDate GetFiscalYearQuarterEndDate(int fiscalYear, int month)` | Builds `fiscalYear-month-1` and asks for its quarter end. | not ported (D-08); use the quarter value; no vector |
| NY13 | `static (NepaliDate, NepaliDate) GetFiscalYearQuarterStartAndEndDate(int fiscalYear, int month)` | The two static quarter values. | not ported (D-08); use the quarter value; no vector |

**Required behaviour.** A fiscal year starts on 1 Shrawan (month 4) and ends on the last day of
Ashad (month 3) of the next BS year. For a date in months 1 to 3, the containing fiscal year is
the previous BS year. Quarters are Q1 Shrawan to Ashoj, Q2 Kartik to Poush, Q3 Magh to Chaitra
and Q4 Baishakh to Ashad of the next year. `dates.tsv` has `fy_start`, `fy_end`, `quarter_start`
and `quarter_end` for all 1018 rows; boundary rows are `ERR` where the result would leave the
supported range.

The static `Get*` methods take `fiscalYear` as a calendar year and `month` as a month within that
calendar year. That is why `GetFiscalYearQuarterStartDate(2080, 4)` is 2080-04-01 but the same
call with month 1 returns the Q4 range that starts in 2081. `D-08` replaces the instance offset
methods and the static helpers with a fiscal-year value: containing date, start, end, quarter
start and end, and a label. The vector columns still match. The three static `(fiscalYear, month)`
helpers are not ported; the fiscal-year value covers them.

**Naming.**

| ID | C# | Rust | Go | TypeScript | Python | Swift | Kotlin | C++ | C | PHP | Dart | Java | Ruby | Zig |
|---|---|---|---|---|---|---|---|---|---|---|---|------|
| NY01 | `enum FiscalYearQuarters` | `Quarter` | `Quarter` | `Quarter` | `Quarter` | `Quarter` | `Quarter` | `quarter` | `nepdate_quarter` | `Quarter` | `Quarter` | `Quarter` | `Quarter` | `Quarter` |
| NY02 | `NepaliDate FiscalYearStartDate(int yearOffset = 0)` | `FiscalYear::containing(d).start()` | `FiscalYearOf(d).Start()` | `FiscalYear.containing(d).start()` | `FiscalYear.containing(d).start()` | `FiscalYear(containing: d).start()` | `FiscalYear.containing(d).start()` | `fiscal_year::of(d).start()` | `nepdate_fiscal_year_of(d, &fy); nepdate_fiscal_year_start(fy, &out)` | `FiscalYear::containing($d)->start()` | `FiscalYear.containing(d).start()` | `FiscalYear.containing(d).start()` | `FiscalYear#start` | `FiscalYear.containing(date).start()` |
| NY03 | `NepaliDate FiscalYearEndDate(int yearOffset = 0)` | `.end()` | `.End()` | `.end()` | `.end()` | `.end()` | `.end()` | `.end()` | `nepdate_fiscal_year_end` | `->end()` | `.end()` | `.end()` | `#end` | `.end()` |
| NY04 | `(NepaliDate startDate, NepaliDate endDate) FiscalYearStartAndEndDate(int yearOffset = 0)` | `.range()` | `.Range()` | `.range()` | `.range()` | `.range()` | `.range()` | `.range()` | `none; two calls` | `->range()` | `.range()` | `.range()` | `#range` | `.range()` |
| NY05 | `NepaliDate FiscalYearQuarterStartDate(FiscalYearQuarters q = Current, int yearOffset = 0)` | `.quarter_range(q)` | `.QuarterRange(q)` | `.quarterRange(q)` | `.quarter_range(q)` | `.range(of: q)` | `.range(q)` | `.quarter_range(q)` | `nepdate_quarter_start(fy, q, &out)` | `->quarterRange($q)` | `quarterRange(q)` | `.quarterRange(q)` | `#quarter_start` | `.quarterStart(quarter)` |
| NY06 | `NepaliDate FiscalYearQuarterEndDate(FiscalYearQuarters q = Current, int yearOffset = 0)` | `.quarter_range(q).end()` | `.QuarterRange(q).End()` | `.quarterRange(q).end` | `.quarter_range(q).end` | `.range(of: q).upperBound` | `.range(q).endInclusive` | `.quarter_range(q).last()` | `nepdate_quarter_end` | `->quarterRange($q)->end()` | `quarterRange(q).end` | `.quarterRange(q).end()` | `#quarter_end` | `.quarterEnd(quarter)` |
| NY07 | `(NepaliDate startDate, NepaliDate endDate) FiscalYearQuarterStartAndEndDate(FiscalYearQuarters q = Current, int yearOffset = 0)` | `.quarter_range(q)` | `.QuarterRange(q)` | `.quarterRange(q)` | `.quarter_range(q)` | `.range(of: q)` | `.range(q)` | `.quarter_range(q)` | `none; two calls` | `->quarterRange($q)` | `quarterRange(q)` | `.quarterRange(q)` | `#quarter_range` | `.quarterRange(quarter)` |
| NY08 | `static NepaliDate GetFiscalYearStartDate(int fiscalYear)` | `FiscalYear::new(fy).start()` | `FiscalYear(fy).Start()` | `new FiscalYear(fy).start()` | `FiscalYear(fy).start()` | `FiscalYear(startYear: fy).start()` | `FiscalYear(fy).start()` | `fiscal_year(fy).start()` | `nepdate_fiscal_year_start(fy, &out)` | `(new FiscalYear($fy))->start()` | `FiscalYear(fy).start()` | `FiscalYear.of(fy).start()` | `FiscalYear.new(y).start` | `FiscalYear.init(year).start()` |
| NY09 | `static NepaliDate GetFiscalYearEndDate(int fiscalYear)` | `.end()` | `.End()` | `.end()` | `.end()` | `.end()` | `.end()` | `.end()` | `nepdate_fiscal_year_end` | `->end()` | `.end()` | `.end()` | `FiscalYear.new(y).end` | `.end()` |
| NY10 | `static (NepaliDate, NepaliDate) GetFiscalYearStartAndEndDate(int fiscalYear)` | `.range()` | `.Range()` | `.range()` | `.range()` | `.range()` | `.range()` | `.range()` | `none; two calls` | `->range()` | `.range()` | `.range()` | `FiscalYear.new(y).range` | `.range()` |
| NY11 | `static NepaliDate GetFiscalYearQuarterStartDate(int fiscalYear, int month)` | not ported (D-08) | not ported (D-08) | not ported (D-08) | not ported (D-08) | not ported (D-08) | not ported (D-08) | not ported (D-08) | not ported (D-08) | not ported (D-08) | not ported (D-08) | not ported (D-08) | not ported (D-08) | not ported (D-08) |
| NY12 | `static NepaliDate GetFiscalYearQuarterEndDate(int fiscalYear, int month)` | not ported (D-08) | not ported (D-08) | not ported (D-08) | not ported (D-08) | not ported (D-08) | not ported (D-08) | not ported (D-08) | not ported (D-08) | not ported (D-08) | not ported (D-08) | not ported (D-08) | not ported (D-08) | not ported (D-08) |
| NY13 | `static (NepaliDate, NepaliDate) GetFiscalYearQuarterStartAndEndDate(int fiscalYear, int month)` | not ported (D-08) | not ported (D-08) | not ported (D-08) | not ported (D-08) | not ported (D-08) | not ported (D-08) | not ported (D-08) | not ported (D-08) | not ported (D-08) | not ported (D-08) | not ported (D-08) | not ported (D-08) | not ported (D-08) |
|------|
## 8. Patro

| ID | C# signature | What the code does | Vectors |
|---|---|---|---|
| NT01 | `readonly struct CalendarInfo` | Immutable snapshot with the five members below. | PATRO.md anchors; patro-summary.json |
| NT02 | `string TithiNp { get; }` | Devanagari tithi name, or empty outside the data range. | PATRO.md anchors; patro-summary.json |
| NT03 | `string TithiEn { get; }` | English tithi name, or empty. | PATRO.md anchors; patro-summary.json |
| NT04 | `bool IsPublicHoliday { get; }` | Gazetted holiday flag. | PATRO.md anchors; patro-summary.json |
| NT05 | `string[] EventsNp { get; }` | Devanagari event names in file order; empty array when none. | PATRO.md anchors; patro-summary.json |
| NT06 | `string[] EventsEn { get; }` | English event names in file order; empty array when none. | PATRO.md anchors; patro-summary.json |
| NT07 | `CalendarInfo GetCalendarInfo()` | One lookup returning all five. | PATRO.md anchors; patro-summary.json |

**Required behaviour.** The C# properties and `GetCalendarInfo` return empty values outside the
data range, never null. The actual data covers 2077-01-01 to 2084-08-20 BS, not the 2001-2089
that the C# XML comments claim. `PATRO.md` section 4 has the anchor days: 2081-01-01 is Panchami,
a holiday, with New Year, Mesh Sankranti and Bisket Jatra; 2081-02-15 is Panchami, a holiday,
with Republic Day; 2080-11-01 has five events; 2081-04-15 has three events and is not a holiday;
2070-06-15 and 2090-01-01 are empty. `patro-summary.json` gives the counts: 2190 tithi days, 32
tithi names, 294 holidays, 1671 event days, 3032 events, 722 event names, 754 names and 59,851
name bytes.

`D-13` moves the patro to a separate package that depends on the core. The query shape is
`info(date)` returning a `CalendarInfo` with a tithi option, a holiday flag and an ordered event
list. The core package has no tithi, holiday or event data.

**Naming.**

| ID | C# | Rust | Go | TypeScript | Python | Swift | Kotlin | C++ | C | PHP | Dart | Java | Ruby | Zig |
|---|---|---|---|---|---|---|---|---|---|---|---|------|
| NT01 | `readonly struct CalendarInfo` | `nepdate_patro::CalendarInfo` | `patro.Info` | `CalendarInfo` | `CalendarInfo` | `CalendarInfo` | `CalendarInfo` | `patro::day_info` | `nepdate_patro_day_info` | `CalendarInfo` | `CalendarInfo` | `CalendarInfo` | `Patro::CalendarInfo` | `CalendarInfo` |
| NT02 | `string TithiNp { get; }` | `info(d).tithi()?.name(Lang::Ne)` | `Info.Tithi().Name(Nepali)` | `info(d).tithi?.np` | `info(d).tithi.label(Lang.NE)` | `d.calendarInfo.tithi?.nepali` | `d.calendarInfo.tithi?.nepali` | `info(d).tithi()->name(lang::ne)` | `info.tithi.np` | `info($d)->tithi()?->name(Lang::Ne)` | `info(d).tithi?.nepali` | `info(d).tithiNp()` | `#tithi_np`, `#tithi&.nepali` | `info.tithiNp()` |
| NT03 | `string TithiEn { get; }` | `...name(Lang::En)` | `...Name(English)` | `...tithi?.en` | `...label(Lang.EN)` | `...tithi?.english` | `...tithi?.english` | `...name(lang::en)` | `info.tithi.en` | `info($d)->tithi()?->name(Lang::En)` | `info(d).tithi?.english` | `info(d).tithiEn()` | `#tithi_en`, `#tithi&.english` | `info.tithiEn()` |
| NT04 | `bool IsPublicHoliday { get; }` | `info(d).is_public_holiday()` | `Info.IsPublicHoliday()` | `info(d).isPublicHoliday` | `info(d).is_public_holiday` | `...isPublicHoliday` | `...isPublicHoliday` | `...is_public_holiday()` | `info.is_public_holiday` | `info($d)->isPublicHoliday()` | `info(d).isPublicHoliday` | `info(d).isPublicHoliday()` | `#public_holiday?` | `info.is_public_holiday` |
| NT05 | `string[] EventsNp { get; }` | `info(d).events()` plus `name(Lang::Ne)` | `Info.Events()` plus `Name(Nepali)` | `info(d).events.map(e => e.np)` | `[e.label(Lang.NE) for e in info(d).events]` | `...events.map(\.nepali)` | `...events.map { it.nepali }` | `...events()` plus `name(lang::ne)` | `nepdate_patro_events` | `info($d)->events()` plus `name(Lang::Ne)` | `info(d).events.map((e) => e.nepali)` | `info(d).eventsNp()` | `#events_np` | `info.eventsNp(buffer)` |
| NT06 | `string[] EventsEn { get; }` | `...name(Lang::En)` | `...Name(English)` | `...map(e => e.en)` | `...label(Lang.EN)` | `...map(\.english)` | `...map { it.english }` | `...name(lang::en)` | `same, English bytes` | `info($d)->events()` plus `name(Lang::En)` | `info(d).events.map((e) => e.english)` | `info(d).eventsEn()` | `#events_en` | `info.eventsEn(buffer)` |
| NT07 | `CalendarInfo GetCalendarInfo()` | `nepdate_patro::info(d)` | `patro.Of(d)` | `info(d)` | `info(d)` | `d.calendarInfo` | `Patro.info(d)`, `d.calendarInfo` | `patro::info(d)` | `nepdate_patro_info` | `info($d)` | `info(d)` | `Patro.info(d)` | `Patro.info(date)`, `Date#calendar_info` | `patro.info`, `patro.infoInto` |
|------|
## C# convenience overloads

These exist for C# convenience. A port offers the idiomatic equivalent, not a copy of every
signature.

| Convenience | Equivalent |
|---|---|
| `NepaliDate(string)` | `NepaliDate.Parse(string)`; both split and validate. |
| `NepaliDate(string, autoAdjust, monthInMiddle)` | The strict parse plus the three heuristics. `D-05` and `D-06` replace them. |
| `Parse(string, bool, bool)` and `TryParse(string, out, bool, bool)` | The strict parse plus the heuristics. Not ported. |
| `ToString("d")` | `ToString()`. |
| `ToUnicodeString(...)` | `ToString(...)` with the Nepali language selector. |
| `ToLongDateUnicodeString(...)` | `ToLongDateString(...)` with the Nepali language selector. |
| `ToLongDateString`'s three bools | A long-date options object with weekday, year and pad. |
| `AddYears(years, away)` | `AddMonths(years * 12, away)`. |
| `AddMonths` with a negative value | The subtraction path, which mirrors the positive path. |
| `Subtract(date)` | The named day difference. The operator, where it exists, has the opposite sign (`D-07`). |
| `MonthEndDate()` | `new NepaliDate(Year, Month, MonthEndDay)`. |
| `FiscalYearStartAndEndDate` and the other paired methods | A tuple, record or two calls on the fiscal-year value. |
| `GetFiscalYear*` static methods | A fiscal-year value plus its methods. `D-08` uses the value. |
| `GetCalendarInfo()` | The individual tithi, holiday and event properties in one lookup. |
| `BulkConvert` and `BatchProcess*` | One ordered batch function each way; the C# PLINQ order is not kept. |
| `TypeConverter` | The string form plus the `YYYYMMDD` integer form (`NC09` and `D-11`); no converter type. |
| `Equals(object)` and `CompareTo(object)` | Typed equality and ordering. |
| `Today` and `Now` | The same date; `Now` maps to `today(zone)` and has no separate member. |
| `IsToday`, `IsYesterday`, `IsTomorrow` | The same three methods with an explicit zone. |
| `Deconstruct` | The language's destructuring form, or three accessors. |

## Rows that need vectors

None. Every row above has either a golden vector, a reference vector, a clock note, or a
decision that removes it from the port surface. `tools/api_coverage.py` fails when a row loses
that coverage, so the list cannot fill up again by accident.

The only reference vector is `year-length.tsv`: C# has no year-length API (`D-12`), so its
value is summed from the month lengths in `data/` by `spec/reference` rather than recorded from
C#. The C# rows are in `spec/vectors/csharp-golden/`; the reference files are beside them in
`spec/vectors/`. `tools/csharp-golden/regenerate.sh` rebuilds every one of them.

## Decisions

Made on 2026-11-02. There are no undecided rows.

| ID | Decision |
|---|---|
| O-01 | Yes. Every port has a year length, 364 to 367, as ND20. |
| O-02 | `Now` is the same date as `Today`, so it maps to `today(zone)` in ND15 and has no separate member. |
| O-03 | Yes. Every port has `isToday(zone)`, `isYesterday(zone)` and `isTomorrow(zone)`, each with an explicit zone (ND21 to ND23). |
| O-04 | No `autoAdjust` overload. `D-05` and `D-06` stay; ND03, NP02 and NP04 are not ported. |
| O-05 | Yes. Every port has a non-throwing strict parse and a non-throwing lenient parse (NP03 and NP08). |
| O-06 | Yes. Every port has an ordered batch conversion each way, taking its slice, iterator or array; C takes caller buffers (NC04 to NC08). |
| O-07 | Native hooks only, with the canonical string `YYYY-MM-DD`: Rust `serde` behind a feature; Go `TextMarshaler`/`TextUnmarshaler` and `json.Marshaler`; Swift `Codable`; Kotlin `KSerializer` in an optional artifact; TypeScript `toJSON` plus `fromJSON` or a reviver; Python `to_json`/`from_json` plus `__reduce__`; C++ `operator<<`/`std::formatter` and a from-string parser; C a format and parse pair; PHP `JsonSerializable` and `__toString`; Dart `toJson` and a `fromJson` factory. No object form, XML or `TypeConverter` type (NC10; NC11 to NC16 are not ported). |
| O-08 | Yes. Every port converts `YYYYMMDD` to and from a date (NC09). |
| O-09 | Yes. Every port has the range set operations, containment of a range, the splits, the interval, working days and weekend days (NR16 to NR26). |
| O-10 | Yes. Every port has `singleDay`, `fromDayCount`, the `current*` factories with a zone, and the range string forms (NR07, NR08, NR12 to NR14, NR28 and NR29). |
| O-11 | Operator `-` keeps `this - other` where the language overloads it; the named `days_until` keeps `other - this`. `D-07` states both. |
| O-12 | `Date` only in Go; every other port uses `NepaliDate`. |
| O-13 | Yes. Swift adds `FormatStyle` and `Strideable`. |
| O-14 | Yes. Kotlin overrides range `equals`, `hashCode` and `toString`. |
| O-15 | Yes. TypeScript `equals` returns false for a foreign value. |
| O-16 | Yes. Python adds `fromisoformat`, matching `datetime.date`, whose `isoformat` takes no arguments. |
| O-17 | Yes. C adds `NEPDATE_MIN` and `NEPDATE_MAX`. |
| O-18 | Yes. C++ `result::value()` throws on error; this is a breaking change. |
| O-19 | Yes. Go unifies the nil zone default between `Today` and `Time`. |
| O-20 | No `DateOnly` row; the platform date is the port's equivalent. |
