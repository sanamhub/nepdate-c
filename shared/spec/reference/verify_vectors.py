"""Executable reference of spec/ALGORITHM.md, FORMATTING.md and PARSING.md (not a library).

Run from any directory: py spec/reference/verify_vectors.py
The calendar functions live in nepcal.py next to this file. Ports can diff their behaviour
against these two files line by line.
"""
import csv, sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
import nepcal
from nepcal import MIN, dfc, cfd
ROOT=nepcal.ROOT
CAL=nepcal.load(ROOT)
YS=CAL.year_start; ml=CAL.ml; ts=CAL.ts; fs=CAL.fs
E=dfc(1844,4,11); assert E==-45920, E; assert YS[-1]==109212
W=nepcal.WEEKDAYS
n=0
for row in csv.DictReader(open(ROOT/'spec/vectors/month-boundaries.csv',encoding='utf-8')):
    y,m,d=map(int,row['bs'].split('-')); a=tuple(map(int,row['ad'].split('-')))
    s=ts(y,m,d); assert cfd(s+E)==a,(row); assert fs(dfc(*a)-E)==(y,m,d); assert W[(s+4)%7]==row['weekday']; assert ml(y,m)==int(row['month_length']); n+=1
for s in range(0,109212): assert ts(*fs(s))==s
print('spec OK', n, 'vectors; full serial roundtrip OK; last', fs(109211), cfd(109211+E))

# ---- C# golden vectors (spec/vectors/csharp-golden) --------------------------
def valid(y,m,d): return MIN<=y<=2199 and 1<=m<=12 and 1<=d<=ml(y,m)
def fmt(t): return f"{t[0]:04}-{t[1]:02}-{t[2]:02}"
def parse(s): return tuple(map(int,s.split('-')))
def add_days(t,n):
    s=ts(*t)+n
    return fmt(fs(s)) if 0<=s<=109211 else 'ERR'
def add_months(t,n,over):
    y,m,d=t; tot=(y-MIN)*12+(m-1)+n
    if not 0<=tot<299*12: return 'ERR'
    y2=MIN+tot//12; m2=tot%12+1; L=ml(y2,m2)
    if d<=L: return fmt((y2,m2,d))
    if not over: return fmt((y2,m2,L))
    y3,m3=(y2+1,1) if m2==12 else (y2,m2+1)
    return fmt((y3,m3,d-L)) if y3<=2199 else 'ERR'
def fy_of(t): return t[0] if t[1]>=4 else t[0]-1
def safe(t): return fmt(t) if valid(*t) else 'ERR'
def last(y,m): return (y,m,ml(y,m)) if MIN<=y<=2199 else (y,m,99)
QSTART={1:(0,4),2:(0,7),3:(0,10),4:(1,1)}
def q_of(t): return {4:1,5:1,6:1,7:2,8:2,9:2,10:3,11:3,12:3}.get(t[1],4)
G=ROOT/'spec/vectors/csharp-golden'
def tsv(path):
    """Rows of a vector file, ignoring blank lines and lines that start with '#'."""
    lines=[line for line in Path(path).read_text(encoding='utf-8').splitlines()
           if line and not line.startswith('#')]
    return list(csv.DictReader(lines,delimiter='\t',quoting=csv.QUOTE_NONE))
rd=lambda f: tsv(G/f)
n=0
for r in rd('dates.tsv'):
    t=parse(r['bs']); fy=fy_of(t); q=q_of(t); dy,qm=QSTART[q]
    assert cfd(ts(*t)+E)==parse(r['ad']); assert W[(ts(*t)+4)%7]==r['weekday']
    assert ts(*t)-YS[t[0]-MIN]+1==int(r['day_of_year'])
    assert safe((fy,4,1))==r['fy_start'], r; assert safe(last(fy+1,3))==r['fy_end'], r
    assert safe((fy+dy,qm,1))==r['quarter_start'], r; assert safe(last(fy+dy,qm+2))==r['quarter_end'], r
    assert r['to_string']==fmt(t).replace('-','/'); n+=1
for r in rd('add-days.tsv'):
    assert add_days(parse(r['bs']),int(r['days']))==r['result'], r; n+=1
for r in rd('add-months.tsv'):
    t=parse(r['bs']); k=int(r['months'])
    assert add_months(t,k,False)==r['clamp'], r; assert add_months(t,k,True)==r['overflow'], r; n+=1
print('csharp-golden OK', n, 'rows (dates, add-days, add-months)')

# ---- Formatting (spec/FORMATTING.md) ----------------------------------------
MEN=['Baishakh','Jestha','Ashad','Shrawan','Bhadra','Ashoj','Kartik','Mangsir','Poush','Magh','Falgun','Chaitra']
MSH=['Bai','Jes','Asa','Shr','Bha','Aso','Kar','Man','Pou','Mag','Fal','Cha']
MNE=['बैशाख','जेठ','असार','साउन','भदौ','असोज','कार्तिक','मंसिर','पुष','माघ','फागुन','चैत']
WNE=['आइतवार','सोमवार','मङ्गलवार','बुधवार','बिहिवार','शुक्रवार','शनिवार']
def dig(s,ne): return s.translate({ord(c):0x966+i for i,c in enumerate('0123456789')}) if ne else s
def wd(t): return (ts(*t)+4)%7
def long_(t,ne,weekday=False,year=True,pad=True):
    y,m,d=t; s=(MNE if ne else MEN)[m-1]+' '+(f'{d:02}' if pad else str(d))
    if year: s+=f', {y}'
    if weekday: s=(WNE if ne else W)[wd(t)]+', '+s
    return dig(s,ne)
def pattern(t,p,ne=False):
    y,m,d=t
    if p in ('','G','g','d'): return dig(f'{y:04}/{m:02}/{d:02}',ne)
    if p=='D': return long_(t,ne)
    if p=='s': return dig(f'{y:04}-{m:02}-{d:02}',ne)
    out=[]; i=0
    while i<len(p):
        c=p[i]
        if c=='\\':   # FORMATTING §5: escapes the next char; a trailing backslash writes itself
            out.append(p[i+1] if i + 1 < len(p) else '\\'); i+=2; continue
        if c=="'":
            j=p.find("'",i+1); j=len(p) if j<0 else j; out.append(p[i+1:j]); i=j+1; continue
        if c in 'yMd':
            j=i
            while j<len(p) and p[j]==c: j+=1
            k=j-i; i=j
            if c=='y': out.append(dig(f'{y:04}' if k>=4 else f'{y%100:02}',ne))
            elif c=='M': out.append((MNE if ne else MEN)[m-1] if k>=4 else ((MNE if ne else MSH)[m-1] if k==3 else dig(f'{m:02}' if k==2 else str(m),ne)))
            else: out.append(dig(f'{d:02}' if k==2 else str(d),ne))
            continue
        out.append(c); i+=1
    return ''.join(out)
n=0
for r in rd('dates.tsv'):
    t=parse(r['bs'])
    assert dig(r['to_string'],True)==r['unicode']
    assert long_(t,False)==r['long_en'], r; assert long_(t,False,True)==r['long_en_weekday'], r
    assert long_(t,True)==r['long_ne'], r; assert long_(t,True,True)==r['long_ne_weekday'], r
    assert f'{t[2]}-{t[1]}-{t[0]}'==r['dmy_dash_nopad']; n+=1
dev=0
for r in rd('format-pattern.tsv'):
    t=parse(r['bs']); got=pattern(t,r['pattern'])
    if 'MMM' in r['pattern'] and 'MMMM' not in r['pattern'] and t[1] in (3,6): dev+=1; continue  # D-04
    assert got==r['result'], (r,got); n+=1
print('formatting OK', n, 'rows;', dev, 'rows skipped as deviation D-04')

# ---- Parsing (spec/PARSING.md) ----------------------------------------------
SEPS=set('-/._ ।|') | {chr(92)}   # - / . _ space । | and backslash (chr 92)
def ddig(c):
    if '0'<=c<='9': return c
    if '०'<=c<='९': return chr(ord(c)-0x966+48)
    return None
def validate(y,m,d):
    if not MIN<=y<=2199: return 'ERR:OutOfRange'
    if not 1<=m<=12: return 'ERR:InvalidMonth'
    if not 1<=d<=ml(y,m): return 'ERR:InvalidDay'
    return fmt((y,m,d))
def strict(s):
    groups=['']
    for c in s:
        g=ddig(c)
        if g is not None:
            if len(groups[-1])==4: return 'ERR:NumberTooLong'
            groups[-1]+=g
        elif c in SEPS:
            if groups[-1]: groups.append('')
        else: return 'ERR:InvalidCharacter'
        if len(groups)==4 and groups[-1]: return 'ERR:WrongGroupCount'
    groups=[g for g in groups if g]
    if len(groups)!=3: return 'ERR:WrongGroupCount'
    return validate(*map(int,groups))
MONTHS={}
for i,names in enumerate([
 'baishakh baisakh baishak vaishakh vaisakh bai बैशाख वैशाख बैसाख','jestha jeth jyestha jes जेठ जेष्ठ ज्येष्ठ',
 'ashad ashadh asar asadh asa असार आषाढ असाढ','shrawan shravan srawan sawan saun shr साउन श्रावण सावन',
 'bhadra bhadau bha भदौ भाद्र भाद्रपद','ashoj asoj ashwin aso असोज आश्विन असौज','kartik kattik kar कार्तिक कात्तिक',
 'mangsir mansir margashirsha man मंसिर मङ्सिर मार्गशीर्ष','poush paush push pus pou पुष पौष पुस','magh mag माघ',
 'falgun phalgun fagun fal फागुन फाल्गुन','chaitra chait cha चैत चैत्र'],1):
    for nm in names.split(): MONTHS[nm]=i
ASCII=set('0123456789')
ERA={'bs','b.s.','b.s','vs','v.s.','v.s','बि.सं.','वि.सं.','बि.सं','वि.सं'}
def lenient(s):
    r=strict(s)
    if not r.startswith('ERR'): return r
    s=''.join(ddig(c) or c for c in s)
    raw=[]; cur=''
    for c in s:
        if c in SEPS-{'.'} or c==',':
            if cur: raw.append(cur); cur=''
        else: cur+=c
    if cur: raw.append(cur)
    toks=[]
    for t in raw:
        if all(ch in ASCII or ch=='.' for ch in t): toks+= [x for x in t.split('.') if x]
        else: toks.append(t)
    toks=[t for t in toks if t.lower() not in ERA and t not in ('गते','मिति')]
    nums=[t for t in toks if t and all(ch in ASCII for ch in t)]; words=[t for t in toks if t not in nums]
    if any(len(n)>4 for n in nums): return 'ERR:NumberTooLong'
    if len(words)==1 and words[0].lower() in MONTHS and len(nums)==2:
        m=MONTHS[words[0].lower()]; four=[n for n in nums if len(n)==4]
        if len(four)!=1: return 'ERR:Ambiguous'
        other=nums[1] if len(nums[0])==4 else nums[0]
        if len(other)>2: return 'ERR:Ambiguous'
        return validate(int(four[0]),m,int(other))
    if not words and len(nums)==3:
        if len(nums[0])==4: return validate(int(nums[0]),int(nums[1]),int(nums[2]))
        if len(nums[2])==4: return validate(int(nums[2]),int(nums[1]),int(nums[0]))
        return 'ERR:Ambiguous'
    return 'ERR:Unrecognized'

PARSE_FILE = ROOT/'spec/vectors/parse.tsv'
TAB, NL = '\t', '\n'
if '--write-parse-vectors' in sys.argv:
    # Existing rows first (some were added by hand), then the C# golden inputs, then the list
    # below; dict.fromkeys keeps the first occurrence, so regenerating never drops or reorders
    # a row.
    existing = [r['input'] for r in csv.DictReader(open(PARSE_FILE, encoding='utf-8'),
                delimiter=TAB, quoting=csv.QUOTE_NONE)] if PARSE_FILE.exists() else []
    inputs = existing + [r['input'] for r in rd('parse.tsv')] + [
        '15.04.2080', '15-04-080', '2080/04/15/', '2080 Shrawan 15', 'Shrawan Bhadra 2080', '15 Foo 2080',
        '12345/01/01', '2080/01/32', '2080/02/32', '२०८०-०४-३२ बि.सं.', '2080/04/15 VS', 'Shr 15 2080',
        '15 SHRAWAN 2080', '15 shrawan 80', '2080/4/15 ', '٢٠٨٠/٠٥/١٥', '2080 Shrawan ²',
        # Case folding is str.lower() onto ASCII only (PARSING §2): Kelvin sign yes, long s and
        # dotless i no, and the same for era markers.
        '15 Kartik 2080', '15 ſhrawan 2080', '15 Ashwın 2080', '15 Shrawan 2080 Bſ']
    with open(PARSE_FILE, 'w', encoding='utf-8', newline=NL) as f:
        f.write('input' + TAB + 'strict' + TAB + 'lenient' + NL)
        for x in dict.fromkeys(inputs):
            f.write(x + TAB + strict(x) + TAB + lenient(x) + NL)
n = 0
for r in csv.DictReader(open(PARSE_FILE, encoding='utf-8'), delimiter=TAB, quoting=csv.QUOTE_NONE):
    assert strict(r['input']) == r['strict'], r
    assert lenient(r['input']) == r['lenient'], r
    n += 1
print('parsing OK', n, 'rows')

# ---- New API vectors (spec/API.md "Rows that need vectors") ------------------
MAX_S = 109211
SEPCHAR = {'Slash': '/', 'Backslash': '\\', 'Dot': '.', 'Underscore': '_', 'Dash': '-', 'Space': ' '}


def short(t, order, separator, pad, ne=False):
    """FORMATTING section 3: order is Ymd, Ydm, Myd, Mdy, Dym or Dmy."""
    y, m, d = t
    parts = {'Y': f'{y:04}' if pad else str(y), 'M': f'{m:02}' if pad else str(m),
             'D': f'{d:02}' if pad else str(d)}
    return dig(separator.join(parts[c.upper()] for c in order), ne)


def range_text(start, end):
    return f'{fmt(start)}..{fmt(end)}'


def range_list_text(items):
    return 'none' if not items else ';'.join(range_text(a, b) for a, b in items)


def date_list_text(items):
    return 'none' if not items else ';'.join(fmt(t) for t in items)


def argument_range(text):
    a, b = text.split('..')
    return parse(a), parse(b)


# year-length.tsv is a reference vector: C# has no year-length API (D-12).
n = 0
for r in tsv(ROOT / 'spec/vectors/year-length.tsv'):
    y = int(r['bs_year'])
    length = YS[y - MIN + 1] - YS[y - MIN]
    assert MIN <= y <= 2199, r
    assert 364 <= length <= 367, r
    assert r['year_length'] == str(length), (r, length)
    n += 1
assert n == 299, n
print('year length OK', n, 'years (reference vector, D-12)')

# day-diff.tsv (NA04, NA05; D-07).
n = 0
for r in tsv(G / 'day-diff.tsv'):
    a, b = parse(r['a']), parse(r['b'])
    assert int(r['days_until']) == ts(*b) - ts(*a), r
    assert int(r['operator_minus']) == ts(*a) - ts(*b), r
    n += 1
print('day difference OK', n, 'rows (D-07)')


# ymd-int.tsv (NC09; D-11).
def cdiv(a, b):
    """C# integer division truncates toward zero."""
    q = abs(a) // abs(b)
    return -q if (a < 0) != (b < 0) else q


def crem(a, b):
    return a - cdiv(a, b) * b


n = 0
for r in tsv(G / 'ymd-int.tsv'):
    if r['kind'] == 'date':
        t = parse(r['input'])
        assert valid(*t), r
        assert int(r['ymd']) == t[0] * 10000 + t[1] * 100 + t[2], r
        assert r['result'] == r['input'], r
    elif r['kind'] == 'int':
        value = int(r['ymd'])
        y = cdiv(value, 10000)
        m = crem(cdiv(value, 100), 100)
        d = crem(value, 100)
        expected = fmt((y, m, d)) if valid(y, m, d) else 'ERR'
        assert r['result'] == expected, (r, expected)
    else:
        raise AssertionError(r)
    n += 1
print('YYYYMMDD integer OK', n, 'rows (D-11)')


# bulk.tsv (NC04-NC08). The output must be in input order; the inputs are deliberately unsorted.
def date_text(t):
    return f'{t[0]:04}-{t[1]:02}-{t[2]:02}'


n = 0
for r in tsv(G / 'bulk.tsv'):
    inputs = r['input'].split(';')
    out = []
    failed = False
    for item in inputs:
        if r['operation'] in ('to_nepali_dates', 'batch_to_nepali_dates'):
            try:
                t = CAL.from_ad(*parse(item))
            except (ValueError, IndexError):
                failed = True
                break
            if not valid(*t):
                failed = True
                break
            out.append(fmt(t))
        elif r['operation'] == 'to_english_dates_from_strings':
            try:
                t = parse(item)
            except ValueError:
                failed = True
                break
            if not valid(*t):
                failed = True
                break
            out.append(date_text(CAL.to_ad(*t)))
        elif r['operation'] in ('to_english_dates_from_dates', 'batch_to_english_dates'):
            t = parse(item)
            if not valid(*t):
                failed = True
                break
            out.append(date_text(CAL.to_ad(*t)))
        else:
            raise AssertionError(r)
    expected = 'ERR' if failed else ';'.join(out)
    assert r['output'] == expected, (r, expected)
    if not failed:
        assert len(out) == len(inputs), r
        # The whole point of the row: element i of the output belongs to element i of the input.
        for item, converted in zip(inputs, out):
            assert converted != '' and item != '', r
    n += 1
print('bulk conversion OK', n, 'rows, input order asserted')

# serialization.tsv (NC10; D-01, D-11).
n = 0
for r in tsv(G / 'serialization.tsv'):
    if r['kind'] == 'write':
        t = parse(r['input'])
        assert r['json'] == '"' + fmt(t) + '"', r
        assert r['result'] == '-', r
    elif r['kind'] == 'roundtrip':
        t = parse(r['input'])
        assert r['json'] == '"' + fmt(t) + '"', r
        assert r['result'] == r['input'], r
    elif r['kind'] == 'read':
        if r['input'] == 'null':
            assert r['result'] == 'ERR', r  # D-01
        else:
            assert r['input'].startswith('"') and r['input'].endswith('"'), r
            try:
                t = parse(r['input'][1:-1])
            except ValueError:
                expected = 'ERR'
            else:
                expected = fmt(t) if valid(*t) else 'ERR'
            assert r['result'] == expected, (r, expected)
    else:
        raise AssertionError(r)
    n += 1
print('serialization OK', n, 'rows (D-01, D-11)')

# format-matrix.tsv (NF02, NF07; D-15).
n = 0
for r in tsv(G / 'format-matrix.tsv'):
    t = parse(r['bs'])
    if r['kind'] == 'short':
        pad = r['pad'] == 'true'
        expected = short(t, r['order'], SEPCHAR[r['separator']], pad, r['lang'] == 'ne')
    elif r['kind'] == 'try_format':
        text = pattern(t, '') if r['format'] == 'default' else pattern(t, r['format'])
        expected = text if len(text) <= int(r['capacity']) else 'ERR'
    else:
        raise AssertionError(r)
    assert r['result'] == expected, (r, expected)
    n += 1
print('format matrix OK', n, 'rows (D-15)')

# ranges.tsv (NR01-NR04, NR06, NR15-NR21, NR27, NR30; D-01, D-16). D-16: IsAdjacentTo is
# false when the day after this range would pass 2199-12-30, and Intersect of disjoint ranges
# is none even when the receiver starts at 1901-01-01.
n = 0
for r in tsv(G / 'ranges.tsv'):
    operation = r['operation']
    if operation == 'construct':
        try:
            a, b = parse(r['start']), parse(r['end'])
        except ValueError:
            expected = 'ERR'
        else:
            expected = range_text(a, b) if valid(*a) and valid(*b) and ts(*a) <= ts(*b) else 'ERR'
    else:
        a, b = parse(r['start']), parse(r['end'])
        assert valid(*a) and valid(*b) and ts(*a) <= ts(*b), r
        sa, sb = ts(*a), ts(*b)
        if operation == 'length':
            expected = str(sb - sa + 1)
        elif operation == 'contains_date':
            d = ts(*parse(r['argument']))
            expected = 'true' if sa <= d <= sb else 'false'
        elif operation == 'contains_range':
            x, y = argument_range(r['argument'])
            expected = 'true' if sa <= ts(*x) and ts(*y) <= sb else 'false'
        elif operation == 'overlaps':
            x, y = argument_range(r['argument'])
            expected = 'true' if sa <= ts(*y) and sb >= ts(*x) else 'false'
        elif operation == 'is_adjacent_to':
            x, y = argument_range(r['argument'])
            # D-16: a day outside 1901-01-01..2199-12-30 is no adjacency, not an error.
            after = sb + 1 if sb < MAX_S else None
            before = sa - 1 if sa > 0 else None
            expected = 'true' if (after is not None and after == ts(*x)) or \
                (before is not None and before == ts(*y)) else 'false'
        elif operation == 'intersect':
            x, y = argument_range(r['argument'])
            sx, sy = ts(*x), ts(*y)
            expected = range_text(fs(max(sa, sx)), fs(min(sb, sy))) if sa <= sy and sb >= sx else 'none'
        elif operation == 'union':
            x, y = argument_range(r['argument'])
            expected = range_text(fs(min(sa, ts(*x))), fs(max(sb, ts(*y))))
        elif operation == 'except':
            x, y = argument_range(r['argument'])
            sx, sy = ts(*x), ts(*y)
            if not (sa <= sy and sb >= sx):
                expected = range_text(a, b)
            elif sx <= sa and sb <= sy:
                expected = 'none'
            else:
                parts = []
                if sa < sx:
                    parts.append((fs(sa), fs(sx - 1)))
                if sb > sy:
                    parts.append((fs(sy + 1), fs(sb)))
                expected = range_list_text(parts)
        elif operation == 'equals':
            x, y = argument_range(r['argument'])
            expected = 'true' if (sa, sb) == (ts(*x), ts(*y)) else 'false'
        elif operation == 'to_string':
            expected = f'{fmt(a).replace("-", "/")} - {fmt(b).replace("-", "/")}'
        elif operation == 'enumerate':
            expected = date_list_text([fs(s) for s in range(sa, sb + 1)])
        else:
            raise AssertionError(operation)
    assert r['result'] == expected, (r, expected)
    n += 1
print('ranges OK', n, 'rows')

# range-factories.tsv (NR07-NR11; D-01).
n = 0
for r in tsv(G / 'range-factories.tsv'):
    factory, argument = r['factory'], r['argument']
    if factory == 'construct':
        start, end = argument.split(',')
        try:
            a, b = parse(start), parse(end)
        except ValueError:
            expected = 'ERR'
        else:
            expected = range_text(a, b) if valid(*a) and valid(*b) and ts(*a) <= ts(*b) else 'ERR'
    elif factory == 'single_day':
        try:
            a = parse(argument)
        except ValueError:
            expected = 'ERR'
        else:
            expected = range_text(a, a) if valid(*a) else 'ERR'
    elif factory == 'from_day_count':
        start, days = argument.split(',')
        a, days = parse(start), int(days)
        if not valid(*a) or days < 1 or ts(*a) + days - 1 > MAX_S:
            expected = 'ERR'
        else:
            expected = range_text(a, fs(ts(*a) + days - 1))
    elif factory == 'for_month':
        y, m = map(int, argument.split(','))
        expected = range_text((y, m, 1), (y, m, ml(y, m))) if MIN <= y <= 2199 and 1 <= m <= 12 else 'ERR'
    elif factory == 'for_calendar_year':
        y = int(argument)
        expected = range_text((y, 1, 1), (y, 12, ml(y, 12))) if MIN <= y <= 2199 else 'ERR'
    elif factory == 'for_fiscal_year':
        y = int(argument)
        expected = range_text((y, 4, 1), (y + 1, 3, ml(y + 1, 3))) if MIN <= y and y + 1 <= 2199 else 'ERR'
    else:
        raise AssertionError(factory)
    assert r['result'] == expected, (r, expected)
    n += 1
print('range factories OK', n, 'rows')

# range-splits.tsv (NR22, NR23; D-16). A port never throws at the data end: every piece is
# returned and the last one ends at the range end. C# advances to the next month or quarter
# after the last piece and throws when that would pass 2199-12-30; those rows are D-16.
n = 0
for r in tsv(G / 'range-splits.tsv'):
    a, b = parse(r['start']), parse(r['end'])
    assert valid(*a) and valid(*b) and ts(*a) <= ts(*b), r
    sa, sb = ts(*a), ts(*b)
    pieces = []
    s = sa
    if r['operation'] == 'split_by_month':
        while s <= sb:
            y, m, _ = fs(s)
            piece_end = min(ts(y, m, ml(y, m)), sb)
            pieces.append((fs(s), fs(piece_end)))
            if piece_end >= sb:
                break
            s = piece_end + 1
    elif r['operation'] == 'split_by_fiscal_quarter':
        while s <= sb:
            y, m, _ = fs(s)
            end_month = 3 if m <= 3 else 6 if m <= 6 else 9 if m <= 9 else 12
            piece_end = min(ts(y, end_month, ml(y, end_month)), sb)
            pieces.append((fs(s), fs(piece_end)))
            if piece_end >= sb:
                break
            s = ts(y + 1, 1, 1) if end_month == 12 else ts(y, end_month + 1, 1)
    else:
        raise AssertionError(r)
    expected = range_list_text(pieces)
    assert r['result'] == expected, (r, expected)
    n += 1
print('range splits OK', n, 'rows')

# range-days.tsv (NR24-NR27; D-16). A port never throws at the data end: every date up to and
# including 2199-12-30 is returned. C# advances one day (or one interval) after the last item
# and throws there; those rows are D-16. ERR is only a rejected argument, interval below 1.
n = 0
for r in tsv(G / 'range-days.tsv'):
    a, b = parse(r['start']), parse(r['end'])
    assert valid(*a) and valid(*b) and ts(*a) <= ts(*b), r
    sa, sb = ts(*a), ts(*b)
    items = []
    failed = False
    if r['operation'] == 'dates_with_interval':
        interval = int(r['argument'])
        if interval < 1:
            failed = True
        else:
            s = sa
            while s <= sb:
                items.append(fs(s))
                if s + interval > sb:
                    break
                s += interval
    elif r['operation'] in ('working_days', 'weekend_days'):
        flag = r['argument'] == 'true'
        for s in range(sa, sb + 1):
            weekday = (s + 4) % 7
            if r['operation'] == 'working_days':
                if weekday != 6 and (not flag or weekday != 0):
                    items.append(fs(s))
            elif weekday == 6 or (flag and weekday == 0):
                items.append(fs(s))
    elif r['operation'] == 'enumerate':
        items = [fs(s) for s in range(sa, sb + 1)]
    else:
        raise AssertionError(r)
    expected = 'ERR' if failed else date_list_text(items)
    assert r['result'] == expected, (r, expected)
    n += 1
print('range day helpers OK', n, 'rows')

# range-format.tsv (NR28, NR29; D-01, D-15).
n = 0
for r in tsv(G / 'range-format.tsv'):
    a, b = parse(r['start']), parse(r['end'])
    if ts(*a) > ts(*b):
        expected = 'ERR'
    elif r['kind'] == 'default':
        expected = f'{fmt(a).replace("-", "/")} - {fmt(b).replace("-", "/")}'
    elif r['kind'] == 'short':
        separator = SEPCHAR[r['separator']]
        ne = r['lang'] == 'ne'
        expected = f'{short(a, r["order"], separator, True, ne)} - {short(b, r["order"], separator, True, ne)}'
    else:
        raise AssertionError(r)
    assert r['result'] == expected, (r, expected)
    n += 1
print('range strings OK', n, 'rows (D-15)')
