#include <nepdate.h>

#include <stdio.h>

int main(void) {
    nepdate_date date;
    nepdate_ymd ad;
    if (nepdate_from_bs(2081, 4, 15, &date) != NEPDATE_OK ||
        nepdate_to_ad(date, &ad) != NEPDATE_OK) {
        return 1;
    }
    (void)printf("%04d-%02d-%02d\n", ad.year, ad.month, ad.day);
    return 0;
}
