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
rd=lambda f: csv.DictReader(open(G/f,encoding='utf-8'),delimiter='\t')
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
        if c=='\\':   # FORMATTING §5: escapes the next char; a trailing backslash writes nothing
            out.append(p[i+1:i+2]); i+=2; continue
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
