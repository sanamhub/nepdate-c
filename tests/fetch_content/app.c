#include "app_patro.h"

#include <nepdate.h>

#include <stdio.h>

int main(void) {
    nepdate_date date;
    nepdate_ymd bs;
    if (nepdate_from_ad(2024, 7, 30, &date) != NEPDATE_OK ||
        nepdate_to_bs(date, &bs) != NEPDATE_OK) {
        return 1;
    }
    const int holidays = app_patro_holiday_count();
    if (holidays <= 0) {
        (void)fprintf(stderr, "the patro library is missing its holidays: %d\n", holidays);
        return 1;
    }
    (void)printf("C BS %04d-%02d-%02d\n", bs.year, bs.month, bs.day);
    return 0;
}
