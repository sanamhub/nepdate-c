"""Calendar functions of spec/ALGORITHM.md, for the hub's checks and tools (not a library).

Nothing runs on import. Paths resolve from this file, so the module works from any directory and
from a spoke's vendored copy under shared/spec/reference/.

    import nepcal
    cal = nepcal.load()          # reads <root>/data/calendar/bs-calendar.json
    cal.to_ad(2081, 1, 1)        # (2024, 4, 13)
"""
import bisect
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MIN = 1901
MAX = 2199
YEARS = MAX - MIN + 1
WEEKDAYS = ['Sunday', 'Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday', 'Saturday']


def dfc(y, m, d):
    """days_from_civil: proleptic Gregorian date to days since 1970-01-01 (ALGORITHM §5)."""
    if m <= 2: y -= 1
    era = y // 400; yoe = y - era * 400; mp = (m + 9) % 12; doy = (153 * mp + 2) // 5 + d - 1
    doe = yoe * 365 + yoe // 4 - yoe // 100 + doy; return era * 146097 + doe - 719468


def cfd(z):
    """civil_from_days: days since 1970-01-01 to a Gregorian (y, m, d) (ALGORITHM §5)."""
    z += 719468; era = z // 146097; doe = z - era * 146097
    yoe = (doe - doe // 1460 + doe // 36524 - doe // 146096) // 365; y = yoe + era * 400
    doy = doe - (365 * yoe + yoe // 4 - yoe // 100); mp = (5 * doy + 2) // 153; d = doy - (153 * mp + 2) // 5 + 1
    m = mp + 3 if mp < 10 else mp - 9
    return (y + (m <= 2), m, d)


class Calendar:
    """Month lengths of BS 1901 to 2199 and the derived tables of ALGORITHM §2."""

    def __init__(self, month_lengths):
        self.lengths = [month_lengths[str(y)] for y in range(MIN, MAX + 1)]
        self.packed = [sum((n - 29) << (2 * k) for k, n in enumerate(ls)) for ls in self.lengths]
        self.year_start = [0]
        for ls in self.lengths: self.year_start.append(self.year_start[-1] + sum(ls))
        self.epoch = dfc(1844, 4, 11)
        self.total_days = self.year_start[-1]

    def ml(self, y, m):
        """Length of BS month m of year y."""
        return 29 + ((self.packed[y - MIN] >> (2 * (m - 1))) & 3)

    def ts(self, y, m, d):
        """BS date to serial day, 0 = 1901-01-01 BS."""
        return self.year_start[y - MIN] + sum(self.ml(y, k) for k in range(1, m)) + d - 1

    def fs(self, s):
        """Serial day to a BS (y, m, d)."""
        i = bisect.bisect_right(self.year_start[:YEARS], s) - 1; y = MIN + i; r = s - self.year_start[i]; m = 1
        while r >= self.ml(y, m): r -= self.ml(y, m); m += 1
        return (y, m, r + 1)

    def to_ad(self, y, m, d):
        """BS date to a Gregorian (y, m, d)."""
        return cfd(self.ts(y, m, d) + self.epoch)

    def from_ad(self, y, m, d):
        """Gregorian date to a BS (y, m, d)."""
        return self.fs(dfc(y, m, d) - self.epoch)

    def weekday(self, y, m, d):
        """Weekday of a BS date, 0 = Sunday."""
        return (self.ts(y, m, d) + 4) % 7


def load(root=ROOT):
    """Calendar from <root>/data/calendar/bs-calendar.json."""
    path = Path(root) / 'data' / 'calendar' / 'bs-calendar.json'
    return Calendar(json.loads(path.read_text(encoding='utf-8'))['month_lengths'])
