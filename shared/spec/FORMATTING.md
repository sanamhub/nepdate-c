# Formatting: normative spec (v1)

Verified by `spec/reference/verify_vectors.py` against `spec/vectors/csharp-golden/`.
No allocation is required for any format: implementations write into a stream/buffer.

## 1. Names

| # | English (`Lang::En`) | Short En (D-04) | Nepali (`Lang::Ne`) |
|---|---|---|---|
| 1 | Baishakh | Bai | बैशाख |
| 2 | Jestha | Jes | जेठ |
| 3 | Ashad | **Asa** | असार |
| 4 | Shrawan | Shr | साउन |
| 5 | Bhadra | Bha | भदौ |
| 6 | Ashoj | **Aso** | असोज |
| 7 | Kartik | Kar | कार्तिक |
| 8 | Mangsir | Man | मंसिर |
| 9 | Poush | Pou | पुष |
| 10 | Magh | Mag | माघ |
| 11 | Falgun | Fal | फागुन |
| 12 | Chaitra | Cha | चैत |

Nepali has no customary month abbreviation: `short_name(Ne)` = full name.

| Weekday (0 = Sunday) | En | En short | Ne | Ne short |
|---|---|---|---|---|
| 0 | Sunday | Sun | आइतवार | आइत |
| 1 | Monday | Mon | सोमवार | सोम |
| 2 | Tuesday | Tue | मङ्गलवार | मङ्गल |
| 3 | Wednesday | Wed | बुधवार | बुध |
| 4 | Thursday | Thu | बिहिवार | बिहि |
| 5 | Friday | Fri | शुक्रवार | शुक्र |
| 6 | Saturday | Sat | शनिवार | शनि |

(Full names are exactly the C# strings. Short forms are new and used by the widget
grid header; C# has none.)

Digits: `Lang::Ne` maps ASCII `0`–`9` to U+0966–U+096F (`०१२३४५६७८९`). No other
characters change.

## 2. Default display

`Display` / `toString` = `YYYY/MM/DD`, zero padded, ASCII digits. Example `2081/04/15`.
(= C# `ToString()`.)

## 3. Short format: `short(order, separator, pad, lang)`

- `order` ∈ `Ymd, Ydm, Myd, Mdy, Dym, Dmy` (C# `DateFormats` in the same order).
- `separator` ∈ `Slash '/'`, `Backslash '\'`, `Dot '.'`, `Underscore '_'`, `Dash '-'`, `Space ' '`.
- `pad = true`: year 4 digits, month/day 2 digits. `pad = false`: no padding.
- Digits localised per `lang`.

Examples (2080-05-15): `Ymd,Dash,true,En` → `2080-05-15`; `Dmy,Dash,false,En` → `15-5-2080`;
`Ymd,Slash,true,Ne` → `२०८०/०५/१५`.

## 4. Long format: `long(options, lang)`

Options: `weekday: bool` (default false), `year: bool` (default true), `pad: bool`
(default **true**, matching C#).

```
[<Weekday>, ]<Month> <day>[, <year>]
```
`day` is 2-digit when `pad`, else unpadded. `year` is always 4-digit. Names and digits per `lang`.

| Date | Options | En | Ne |
|---|---|---|---|
| 2081-04-15 | default | `Shrawan 15, 2081` | `साउन १५, २०८१` |
| 2081-04-15 | weekday | `Tuesday, Shrawan 15, 2081` | `मङ्गलवार, साउन १५, २०८१` |
| 1901-01-01 | default | `Baishakh 01, 1901` | `बैशाख ०१, १९०१` |
| 2079-02-06 | weekday, no year, no pad | `Friday, Jestha 6` | `शुक्रवार, जेठ ६` |

## 5. Pattern format: `format_pattern(pattern, lang)`

Standard single-letter patterns (whole pattern equals exactly one of these):

| Pattern | Output |
|---|---|
| `""`, `G`, `g`, `d` | default display (§2), digits per `lang` |
| `D` | long format, default options (§4) |
| `s` | `YYYY-MM-DD`, digits per `lang` |

Custom pattern: scanned left to right, one pass.

| Token | Meaning |
|---|---|
| `\x` | literal character `x`; a `\` at the very end of the pattern writes nothing |
| `'...'` | literal text; an unterminated quote runs to the end of the pattern |
| run of `y`, length ≥ 4 | 4-digit year |
| run of `y`, length 1–3 | `year % 100`, 2 digits |
| run of `M`, length ≥ 4 | month full name |
| `MMM` | month short name (§1, D-04) |
| `MM` | month, 2 digits |
| `M` | month, unpadded |
| `dd` | day, 2 digits |
| run of `d`, any other length | day, unpadded (so `ddd` → `15`) |
| anything else | copied as-is |

Digits produced by tokens are localised per `lang`; literal characters are never changed.

Examples (2081-04-15, En): `yyyy-MM-dd` → `2081-04-15`; `M/d/yyyy` → `4/15/2081`;
`yyyy'BS'` → `2081BS`; `yyyy\M\M-\d\d` → `2081MM-dd`; `MMM yyyy` → `Shr 2081`;
`dd.MM.yy` → `15.04.81`.

## 6. Parity

With `lang = En`, every output must equal the corresponding golden column
(`to_string`, `unicode`* , `long_en`, `long_en_weekday`, `dmy_dash_nopad`,
`format-pattern.tsv`), except `MMM` for months 3 and 6 (D-04).
*`unicode` = short `Ymd, Slash, pad, Ne`; `long_ne*` = long with `Lang::Ne`.
