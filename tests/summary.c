#include "summary.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The whole file, NUL-terminated. The tests fail loudly rather than skip: shared/ always holds the
 * summary and a checkout without it is not a checkout the tests can judge. */
static char *summary_text(void) {
    const char *path = NEPDATE_C_SHARED_DIR "/spec/vectors/patro-summary.json";
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        (void)fprintf(stderr, "cannot read %s\n", path);
        exit(1);
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        (void)fprintf(stderr, "cannot seek %s\n", path);
        (void)fclose(file);
        exit(1);
    }
    const long size = ftell(file);
    if (size < 0 || fseek(file, 0, SEEK_SET) != 0) {
        (void)fprintf(stderr, "cannot size %s\n", path);
        (void)fclose(file);
        exit(1);
    }
    char *text = malloc((size_t)size + 1U);
    if (text == NULL || fread(text, 1, (size_t)size, file) != (size_t)size) {
        (void)fprintf(stderr, "cannot read %s\n", path);
        free(text);
        (void)fclose(file);
        exit(1);
    }
    (void)fclose(file);
    text[size] = '\0';
    return text;
}

/* The text just after "\"key\":". */
static const char *summary_value(const char *text, const char *key) {
    char needle[64];
    (void)snprintf(needle, sizeof needle, "\"%s\":", key);
    const char *at = strstr(text, needle);
    if (at == NULL) {
        (void)fprintf(stderr, "patro-summary.json has no %s\n", key);
        exit(1);
    }
    return at + strlen(needle);
}

unsigned long summary_number(const char *key) {
    char *text = summary_text();
    const char *at = summary_value(text, key);
    const unsigned long value = strtoul(at, NULL, 10);
    free(text);
    return value;
}

void summary_day(const char *key, char *out, size_t size) {
    char *text = summary_text();
    const char *at = summary_value(text, key);
    while (*at == ' ') {
        at++;
    }
    if (*at != '"') {
        (void)fprintf(stderr, "patro-summary.json: %s is not a string\n", key);
        free(text);
        exit(1);
    }
    at++;
    const char *end = strchr(at, '"');
    if (end == NULL || (size_t)(end - at) + 1U > size) {
        (void)fprintf(stderr, "patro-summary.json: %s does not fit\n", key);
        free(text);
        exit(1);
    }
    memcpy(out, at, (size_t)(end - at));
    out[end - at] = '\0';
    free(text);
}
