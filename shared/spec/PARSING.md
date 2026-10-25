# Parsing: normative spec (v1)

Two parsers. Both are deterministic, allocation-free, and reject rather than guess.
Normative vectors: `spec/vectors/parse.tsv` (columns `input`, `strict`, `lenient`;
`ERR:<Kind>` on failure). The C# behaviour is in `csharp-golden/parse.tsv` for
comparison only (deviations D-05, D-06 in `PARITY.md`).

## 1. Strict: `FromStr` / `parse`

Accepts exactly **year, month, day** in that order.

1. Characters are processed left to right:
   - **digit**: ASCII `0`–`9` or Devanagari `०`–`९` (U+0966–U+096F). Mixing both
     systems is allowed (D-05). The digit is appended to the current group.
   - **separator**: one of `- / . _ \ space । |` (`।` = U+0964). If the current group
     has at least one digit, the next group starts. Otherwise the separator is ignored, so
     leading, trailing and repeated separators are harmless.
   - anything else → `ERR:InvalidCharacter`.
2. A 4th group → `ERR:WrongGroupCount`. A group longer than 4 digits → `ERR:NumberTooLong`.
3. End of input: fewer than 3 groups (including empty input) → `ERR:WrongGroupCount`.
4. Validate with `NepaliDate::new` (ALGORITHM §3). The failure kinds are
   `ERR:OutOfRange`, `ERR:InvalidMonth`, `ERR:InvalidDay`.

Examples: `2080/05/15`, `2080-5-15`, `  2080 / 05 / 15  `, `/2080/05/15/`, `2080।05।15`,
`२०८०/०५/१५` → 2080-05-15.

## 2. Lenient: `parse_lenient`

For human input (e.g. the widget converter). Steps:

1. If strict parsing succeeds, return its result.
2. **Normalise**, producing a token list:
   - Map Devanagari digits to ASCII.
   - Treat `,` as a separator, in addition to the strict separators.
   - Split into tokens: split on the separators except `.`; then a token made only of digits
     and dots is re-split on dots (`2080.05.15` gives three numbers). Any other token stays one
     word, dots included: `B.S.` is a word, and so are `Shr.15` and `2080.B.S.`, which therefore
     match no month and no era (vectors in `parse.tsv`).
   - "Case-insensitive" here and in §3 means exactly what the reference's Python `str.lower()`
     maps onto ASCII: `A` to `Z` fold to `a` to `z`, and the Kelvin sign U+212A folds to `k`. No
     other character folds: the long s `ſ` (U+017F) does not match `s`, and the dotless `ı`
     (U+0131) does not match `i`. Full Unicode case folding (for example Go's
     `strings.EqualFold`) is wrong here (vectors in `parse.tsv`).
   - No Unicode normalisation: input is matched as given. There is no NFC or NFKC step, and the
     zero-width joiners U+200C and U+200D are not removed, so `साउन` with a joiner inside is not
     a month name (vectors in `parse.tsv`). This is the reference behaviour; a port that
     normalises would accept input that the others reject.
   - Drop tokens that are (case-insensitive) era markers: `BS`, `B.S.`, `B.S`, `VS`,
     `V.S.`, `V.S`, `बि.सं.`, `वि.सं.`, `बि.सं`, `वि.सं`.
   - Drop tokens `गते`, `मिति`.
   - Digits are ASCII `0`–`9` after the Devanagari mapping. Any other character Unicode calls a
     digit (Arabic-Indic `٢`, superscript `²`, ...) is a letter, so the token is a word.
   - If any numeric token has more than 4 digits → `ERR:NumberTooLong` (for example `12345/01/01`).
3. **Month-name form**: if exactly one token is a month name (§3, whole-token,
   case-insensitive) and exactly 2 numeric tokens remain:
   - Exactly one numeric token has 4 digits: it is the year. The other (1–2 digits) is
     the day. Order doesn't matter: `15 Shrawan 2080`, `Shrawan 15, 2080` and `2080 Shrawan 15` are all accepted.
   - Otherwise → `ERR:Ambiguous`.
4. **Numeric form**: if there are no month tokens and exactly 3 numeric tokens:
   - First token has 4 digits → `Y M D`.
   - Else last token has 4 digits → `D M Y`.
   - Otherwise → `ERR:Ambiguous`. 2-digit years are never expanded (D-06).
5. Two or more month tokens, any other unknown letter token, or any other token count
   (including no tokens at all, so empty or blank input) → `ERR:Unrecognized`. Strict parsing
   reports empty input as `ERR:WrongGroupCount`; lenient reports `ERR:Unrecognized`.
6. Validate as in strict step 4.

## 3. Month names (closed list, case-insensitive for Latin)

| Month | Accepted tokens |
|---|---|
| 1 | baishakh, baisakh, baishak, baisakh, vaishakh, vaisakh, bai, बैशाख, वैशाख, बैसाख |
| 2 | jestha, jeth, jyestha, jes, जेठ, जेष्ठ, ज्येष्ठ |
| 3 | ashad, ashadh, asar, asadh, asa, असार, आषाढ, असाढ |
| 4 | shrawan, shravan, srawan, sawan, saun, shr, साउन, श्रावण, सावन |
| 5 | bhadra, bhadau, bhadra, bha, भदौ, भाद्र, भाद्रपद |
| 6 | ashoj, asoj, ashwin, aso, असोज, आश्विन, असौज |
| 7 | kartik, kattik, kar, कार्तिक, कात्तिक |
| 8 | mangsir, mansir, margashirsha, man, मंसिर, मङ्सिर, मार्गशीर्ष |
| 9 | poush, paush, push, pus, pou, पुष, पौष, पुस |
| 10 | magh, mag, माघ |
| 11 | falgun, phalgun, fagun, fal, फागुन, फाल्गुन |
| 12 | chaitra, chait, cha, चैत, चैत्र |

Duplicates in a row are harmless. Adding spellings is a spec change (new version +
vectors), not an implementation detail.

## 4. Error kinds

`Empty` is not separate: empty input is `WrongGroupCount`. Kinds:
`InvalidCharacter`, `WrongGroupCount`, `NumberTooLong`, `Ambiguous`, `Unrecognized`,
plus validation errors `OutOfRange`, `InvalidMonth`, `InvalidDay`.
Rust: `Error::Parse(ParseErrorKind::…)` for the first five, the plain `Error` variants for
validation.
