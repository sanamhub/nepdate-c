#include "app_patro.h"

#include <nepdate_patro.h>

int app_patro_holiday_count(void) { return (int)nepdate_patro_public_holidays(NULL, 0); }
