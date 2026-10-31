/* Reads the counts of shared/spec/vectors/patro-summary.json in a test, so a test never pins a
 * number that a data update changes. NEPDATE_C_SHARED_DIR must name the shared directory. */
#ifndef NEPDATE_C_TEST_SUMMARY_H
#define NEPDATE_C_TEST_SUMMARY_H

#include <stddef.h>

/* The integer value of key. Exits 1 with a message when the file or the key is missing. */
unsigned long summary_number(const char *key);

/* The string value of key, which must fit in size bytes including the NUL. */
void summary_day(const char *key, char *out, size_t size);

#endif /* NEPDATE_C_TEST_SUMMARY_H */
