# NepDate Calendar Algorithm: normative spec (v1)

Language-neutral. Every port (Rust, Go, Java, TypeScript, Python) MUST implement exactly this
and MUST pass every file in `spec/vectors/`. How a port stores the tables is its own
decision; the results are not.

Notation: `/` is integer division. Within the supported range every intermediate value
in this document is **non-negative**, so truncating division (C, Rust, Go, Java) and
floor division (Python `//`, JS `Math.floor`) give identical results.

## 1. Inputs from `data/calendar/bs-calendar.json`

- `MIN_YEAR = 1901`, `MAX_YEAR = 2199`, `YEARS = 299`
- `LEN[y][m]` for `y in MIN_YEAR..=MAX_YEAR`, `m in 1..=12`, values in `29..=32`
- Epoch: 1901-01-01 BS == 1844-04-11 AD

## 2. Derived constants

```
EPOCH_UNIX_DAYS = days_from_civil(1844, 4, 11) = -45920
EPOCH_WEEKDAY   = 4            # Thursday, with 0 = Sunday .. 6 = Saturday

PACKED[i]   = Σ_{m=1..12} (LEN[MIN_YEAR+i][m] - 29) << (2*(m-1))      # i in 0..YEARS
YEAR_START[0] = 0
YEAR_START[i+1] = YEAR_START[i] + Σ_m LEN[MIN_YEAR+i][m]               # i in 0..YEARS
TOTAL_DAYS = YEAR_START[YEARS] = 109212
MAX_SERIAL = TOTAL_DAYS - 1 = 109211                                   # 2199-12-30 BS
```

`month_length(y, m) = 29 + ((PACKED[y - MIN_YEAR] >> (2*(m-1))) & 3)`

## 3. Validation

A BS date `(y, m, d)` is valid iff
`MIN_YEAR <= y <= MAX_YEAR` and `1 <= m <= 12` and `1 <= d <= month_length(y, m)`.

Error precedence (first failing check wins): year → `OutOfRange`; month → `InvalidMonth`;
day → `InvalidDay`.

A serial `s` is valid iff `0 <= s <= MAX_SERIAL`.
An AD date is supported iff `days_from_civil(y,m,d) - EPOCH_UNIX_DAYS` is a valid serial
(i.e. 1844-04-11 ..= 2143-04-15) **and** the AD date itself is a real Gregorian date.
AD error precedence: not a real Gregorian date (month outside 1–12, or day outside 1 to that month's
length, with leap years) → `InvalidGregorian`; a real date outside the range → `OutOfRange`. So
`2023-02-29` is `InvalidGregorian` and `1844-04-10` is `OutOfRange`.

## 4. BS ↔ serial

```
to_serial(y, m, d):
    s = YEAR_START[y - MIN_YEAR]
    for k in 1 .. m-1: s += month_length(y, k)
    return s + (d - 1)

from_serial(s):                      # precondition: 0 <= s <= MAX_SERIAL
    i = (largest i such that YEAR_START[i] <= s)     # binary search over YEAR_START[0..YEARS]
    y = MIN_YEAR + i
    r = s - YEAR_START[i]            # 0-based day of year
    m = 1
    while r >= month_length(y, m): r -= month_length(y, m); m += 1
    return (y, m, r + 1)
```

## 5. Gregorian ↔ Unix days (Howard Hinnant, public domain)

Source: https://howardhinnant.github.io/date_algorithms.html

```
days_from_civil(y, m, d):            # proleptic Gregorian → days since 1970-01-01
    if m <= 2: y = y - 1
    era = y / 400                    # y >= 0 in our range
    yoe = y - era * 400              # [0, 399]
    mp  = (m + 9) % 12               # March = 0 .. February = 11
    doy = (153 * mp + 2) / 5 + d - 1 # [0, 365]
    doe = yoe * 365 + yoe / 4 - yoe / 100 + doy
    return era * 146097 + doe - 719468

civil_from_days(z):                  # days since 1970-01-01 → (y, m, d)
    z   = z + 719468
    era = z / 146097
    doe = z - era * 146097
    yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365
    y   = yoe + era * 400
    doy = doe - (365 * yoe + yoe / 4 - yoe / 100)
    mp  = (5 * doy + 2) / 153
    d   = doy - (153 * mp + 2) / 5 + 1
    m   = mp + 3 if mp < 10 else mp - 9
    if m <= 2: y = y + 1
    return (y, m, d)
```

Gregorian validity (needed before `days_from_civil`):
`1 <= m <= 12`, `1 <= d <= greg_len(y, m)` where February has 29 days iff
`(y % 4 == 0 and y % 100 != 0) or y % 400 == 0`.

## 6. Conversions

```
bs_to_ad(y, m, d)  = civil_from_days(to_serial(y, m, d) + EPOCH_UNIX_DAYS)
ad_to_bs(y, m, d)  = from_serial(days_from_civil(y, m, d) - EPOCH_UNIX_DAYS)   # after range check
```

## 7. Derived properties

```
weekday(date)      = (to_serial(date) + EPOCH_WEEKDAY) % 7      # 0 = Sunday
day_of_year(date)  = to_serial(date) - YEAR_START[y - MIN_YEAR] + 1
year_length(y)     = YEAR_START[y - MIN_YEAR + 1] - YEAR_START[y - MIN_YEAR]   # 364..=367
days_between(a, b) = to_serial(b) - to_serial(a)                 # signed
```

## 8. Arithmetic

```
add_days(date, n):
    s = to_serial(date) + n
    if s < 0 or s > MAX_SERIAL: error OutOfRange
    return from_serial(s)

add_months(date, n, overflow):       # overflow ∈ {Clamp, Overflow}
    total = (y - MIN_YEAR) * 12 + (m - 1) + n
    if total < 0 or total >= YEARS * 12: error OutOfRange
    y2 = MIN_YEAR + total / 12 ;  m2 = total % 12 + 1
    len = month_length(y2, m2)
    if d <= len: return (y2, m2, d)
    Clamp    → return (y2, m2, len)
    Overflow → (y3, m3) = next month after (y2, m2)            # 12 → next year's 1
               if y3 > MAX_YEAR: error OutOfRange
               return (y3, m3, d - len)
add_years(date, n, overflow) = add_months(date, 12 * n, overflow)
```

`Clamp` == C# `AddMonths(n)` (default). `Overflow` == C# `AddMonths(n, awayFromMonthEnd: true)`.
Both are verified against `spec/vectors/csharp-golden/add-months.tsv` (integers only:
fractional months are deviation D-02, `PARITY.md`). Example: 2081-04-32 +2 months →
Clamp 2081-06-30, Overflow 2081-07-02; +5 months → Clamp 2081-09-29, Overflow 2081-10-03.

## 8a. Calendar breakdown between two dates

```
diff_ymd(a, b):                       # returns (sign, years, months, days)
    if a > b: swap, sign = -1 else sign = +1
    n = largest integer >= 0 such that add_months(a, n, Clamp) <= b
    rest = days_between(add_months(a, n, Clamp), b)            # 0 <= rest < 33
    return (sign, n / 12, n % 12, rest)
```
Implementation hint: start from `n0 = (b.y - a.y) * 12 + (b.m - a.m)`, then step down
by at most 1 until the condition holds. No loops over days.

Examples (computed with the reference functions; `total` is `days_between(a, b)`):

| a | b | sign, years, months, days | total |
|---|---|---|---|
| 2080-01-01 | 2081-04-15 | +1, 1, 3, 14 | 473 |
| 2081-04-15 | 2080-01-01 | -1, 1, 3, 14 | -473 |
| 2081-04-32 | 2081-05-31 | +1, 0, 1, 0 (Clamp: Bhadra 2081 has 31 days) | 31 |
| 2081-04-32 | 2081-06-01 | +1, 0, 1, 1 | 32 |
| 2081-01-31 | 2081-02-30 | +1, 0, 0, 30 | 30 |
| 2081-03-15 | 2081-03-15 | +1, 0, 0, 0 | 0 |

## 8b. Fiscal year (Nepal: 1 Shrawan → end of Ashad)

```
fiscal_year_of(date) = y if m >= 4 else y - 1
fy_start(fy)  = (fy, 4, 1)
fy_end(fy)    = (fy + 1, 3, month_length(fy + 1, 3))
quarter(date) = Q1 if m in 4..=6, Q2 if 7..=9, Q3 if 10..=12, Q4 if 1..=3
quarter_start(fy, Q1) = (fy, 4, 1)    Q2 → (fy, 7, 1)   Q3 → (fy, 10, 1)   Q4 → (fy+1, 1, 1)
quarter_end(fy, q)    = last day of (quarter_start month + 2)
label(fy)     = "{fy}/{(fy + 1) % 100 :02}"      # e.g. "2082/83"
```
Any result outside the supported range → `OutOfRange` (e.g. `fy_start(1900)`,
`fy_end(2199)`), matching the `ERR` cells in `csharp-golden/dates.tsv`.

## 9. Reference values (also in vectors)

| BS | AD | Weekday |
|---|---|---|
| 1901-01-01 | 1844-04-11 | Thursday |
| 2081-01-01 | 2024-04-13 | Saturday |
| 2082-01-01 | 2025-04-14 | Monday |
| 2083-01-01 | 2026-04-14 | Tuesday |
| 2199-12-30 | 2143-04-15 | none (last supported day) |

## 10. How C# NepDate computes the same results (informative)

Read this to see why the serial-day model above is a faithful port. Source: NepDate commit
`cb05cb65` (the NuGet 2.0.7 package was built from `649e45ea`; `src/NepDate` is identical in both).

| Operation | C# NepDate | This spec |
|---|---|---|
| BS → AD | `NepaliToEnglish.data[(y-1901)*12 + m-1]` holds the month length and the AD date of that BS month's **last** day. Result = that AD date + (`day` − month length) days | serial of the BS date + epoch, then `civil_from_days` (§4–6) |
| AD → BS | `EnglishToNepali.data[(y-1844)*12 + m-4]` holds the AD month length and the BS date of that AD month's **last** day. Result = walk back (AD month length − `day`) days through BS months, using the BS month lengths | `days_from_civil` − epoch, then serial → BS by binary search (§4–6) |
| Month length, validation | `NepMonthEndDay` of the BS table; day must be 1..=length, year 1901..=2199 | §3 |
| Weekday | `DateTime.DayOfWeek` of the AD date | `(serial + 4) mod 7` (§7) |
| Add days | convert to AD, `DateTime.AddDays`, convert back | serial + n (§8) |
| Add months | month arithmetic with clamp, or spill into the next month when `awayFromMonthEnd` | §8 |
| Metadata | a third table, `CalendarOffsets.MonthStart` (BS 2001–2089), gives a day offset since 2001-01-01 BS; tithi, holidays and events are sorted offset arrays searched with `Array.BinarySearch` | `PATRO.md`, keyed by BS date |

The two C# conversion tables and `CalendarOffsets` encode the same month lengths three times.
`tools/extract_from_csharp.py` checks that they agree (3588 BS months, 3588 AD month ends, 1068
metadata month starts), and `tools/compare_exhaustive.py` checks the result against the running
C# code for all 109,212 days in both directions and every metadata day. Both pass on `cb05cb65`
with zero differences.
