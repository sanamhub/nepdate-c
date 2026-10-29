#include <nepdate_patro.h>

#include <stdio.h>

int main(void) {
    const size_t holidays = nepdate_patro_public_holidays(NULL, 0);
    if (holidays != 294) {
        return 1;
    }
    (void)printf("%zu holidays\n", holidays);
    return 0;
}
