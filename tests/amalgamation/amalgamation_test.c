#include <nepdate.h>
#include <nepdate_patro.h>

#include <stdio.h>

int main(void) {
    nepdate_date date;
    nepdate_ymd bs;
    if (nepdate_from_ad(2024, 7, 30, &date) != NEPDATE_OK ||
        nepdate_to_bs(date, &bs) != NEPDATE_OK) {
        return 1;
    }
    if (bs.year != 2081 || bs.month != 4 || bs.day != 15) {
        (void)fprintf(stderr, "amalgamation: got BS %d-%d-%d\n", bs.year, bs.month, bs.day);
        return 1;
    }
    if (nepdate_patro_public_holidays(NULL, 0) != 294) {
        (void)fprintf(stderr, "amalgamation: the patro tables are missing\n");
        return 1;
    }
    (void)printf("BS %04d-%02d-%02d\n", bs.year, bs.month, bs.day);
    return 0;
}
