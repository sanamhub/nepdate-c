/* ALGORITHM section 5, typed line by line (Howard Hinnant, public domain). The library uses Neri
 * and Schneider; the exhaustive test checks that both agree on every supported day. Every value is
 * non-negative in the supported range, so int division matches the spec. */
#ifndef NEPDATE_TESTS_HINNANT_REFERENCE_H
#define NEPDATE_TESTS_HINNANT_REFERENCE_H

typedef struct reference_civil {
    int year;
    int month;
    int day;
} reference_civil;

static inline int reference_days_from_civil(int year, int month, int day) {
    int y = month <= 2 ? year - 1 : year;
    const int era = y / 400;
    const int yoe = y - era * 400;
    const int mp = (month + 9) % 12;
    const int doy = (153 * mp + 2) / 5 + day - 1;
    const int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}

static inline reference_civil reference_civil_from_days(int days) {
    const int z = days + 719468;
    const int era = z / 146097;
    const int doe = z - era * 146097;
    const int yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    int y = yoe + era * 400;
    const int doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const int mp = (5 * doy + 2) / 153;
    const int d = doy - (153 * mp + 2) / 5 + 1;
    const int m = mp < 10 ? mp + 3 : mp - 9;
    if (m <= 2) {
        y = y + 1;
    }
    reference_civil civil = {y, m, d};
    return civil;
}

#endif /* NEPDATE_TESTS_HINNANT_REFERENCE_H */
