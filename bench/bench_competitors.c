/* The C competitor of docs/competitors.md. Its source is fetched by bench/CMakeLists.txt and
 * compiled only in this benchmark; nothing is vendored or installed. */

#include "bench_competitors.h"

#include <stddef.h>

#if defined(NEPDATE_BENCH_COMPETITOR_AATISHNN)
#include "date.h"

static bool aatishnn_to_ad(int year, int month, int day, int *ad_year, int *ad_month, int *ad_day) {
    struct nep_date bs;
    struct eng_date ad;
    bs.year = year;
    bs.month = month;
    bs.date = day;
    if (nep_to_eng(bs, &ad) == 0) {
        return false;
    }
    *ad_year = ad.year;
    *ad_month = ad.month;
    *ad_day = ad.date;
    return true;
}

static bool aatishnn_to_bs(int year, int month, int day, int *bs_year, int *bs_month, int *bs_day) {
    /* eng_to_nep prints to stderr outside 1944..2033; guard here so the benchmark stays quiet
     * and counts a date outside its published range as "no answer". */
    if (year < 1944 || year > 2033) {
        return false;
    }
    struct eng_date ad;
    struct nep_date bs;
    ad.year = year;
    ad.month = month;
    ad.date = day;
    if (eng_to_nep(ad, &bs) == 0) {
        return false;
    }
    *bs_year = bs.year;
    *bs_month = bs.month;
    *bs_day = bs.date;
    return true;
}

const bench_rival bench_rivals[1] = {
    {"aatishnn/nepdate", aatishnn_to_ad, aatishnn_to_bs},
};
const int bench_rival_count = 1;
#else
const bench_rival bench_rivals[1] = {{"", NULL, NULL}};
const int bench_rival_count = 0;
#endif
