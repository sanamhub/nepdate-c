#include <nepdate.h>
#ifdef CONSUMER_PATRO
#include <nepdate_patro.h>
#endif

#include <stdio.h>
#include <string.h>

int main(void) {
    const char *version = nepdate_version();
    if (strcmp(version, NEPDATE_VERSION_STRING) != 0) {
        (void)fprintf(stderr, "library %s, header %s\n", version, NEPDATE_VERSION_STRING);
        return 1;
    }
#ifdef CONSUMER_PATRO
    if (nepdate_patro_public_holidays(NULL, 0) != 294) {
        (void)fprintf(stderr, "the patro library is missing its holidays\n");
        return 1;
    }
#endif
    (void)printf("nepdate-c %s\n", version);
    return 0;
}
