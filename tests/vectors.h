/* Reads the shared vector files under shared/spec/vectors/. Test code may allocate. */
#ifndef NEPDATE_TESTS_VECTORS_H
#define NEPDATE_TESTS_VECTORS_H

#include <stdbool.h>
#include <stddef.h>

typedef struct vector_reader {
    char *data;
    size_t length;
    size_t offset;
} vector_reader;

/* Opens <NEPDATE_C_SHARED_DIR>/spec/vectors/<relative>. Returns false when the file is missing or
 * empty. */
bool vector_open(vector_reader *reader, const char *relative);

/* Returns the next line that is not empty and does not start with '#', with the line ending
 * removed, or NULL at the end. The pointer is valid until vector_close. */
char *vector_next(vector_reader *reader);

void vector_close(vector_reader *reader);

/* Splits line at separator in place and stores the fields in fields. Returns the field count, or
 * capacity when there are more fields than that. */
int vector_split(char *line, char separator, char **fields, int capacity);

/* "2081-04-15" -> year 2081, month 4, day 15. */
bool vector_parse_ymd(const char *text, int *year, int *month, int *day);

/* "Sunday" -> 0 .. "Saturday" -> 6, or -1 for any other name. */
int vector_weekday(const char *name);

#endif /* NEPDATE_TESTS_VECTORS_H */
