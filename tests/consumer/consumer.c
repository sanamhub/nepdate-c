#include <nepdate.h>

#include <stdio.h>
#include <string.h>

int main(void) {
    const char *version = nepdate_version();
    if (strcmp(version, NEPDATE_VERSION_STRING) != 0) {
        (void)fprintf(stderr, "library %s, header %s\n", version, NEPDATE_VERSION_STRING);
        return 1;
    }
    (void)printf("nepdate-c %s\n", version);
    return 0;
}
