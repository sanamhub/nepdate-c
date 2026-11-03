/* Reads the shared vector files under shared/spec/vectors/. Test code may allocate. */
#ifndef NEPDATE_TESTS_VECTORS_H
#define NEPDATE_TESTS_VECTORS_H

#include <nepdate.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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

/* "2081-04-15" -> the date. Returns false for a text that is not a valid date. */
bool vector_parse_date(const char *text, nepdate_date *out);

/* "2081-04-10..2081-04-20" -> the range. Returns false for anything that is not a valid range,
 * "none" and "ERR" included. */
bool vector_parse_range(const char *text, nepdate_range *out);

/* Writes the date as "2081-04-15" and NUL-terminates. A buffer too small keeps a truncated text. */
void vector_format_date(nepdate_date date, char *buffer, size_t size);

/* Writes the range as "2081-04-10..2081-04-20", the form of a vector result. */
void vector_format_range(nepdate_range range, char *buffer, size_t size);

/* "Sunday" -> 0 .. "Saturday" -> 6, or -1 for any other name. */
int vector_weekday(const char *name);

/* Reads a whole decimal number from a vector field into *out. Returns false when the field is not
 * one, so a caller turns a changed row shape into a failed check instead of a reading of zero. */
bool vector_read_int(const char *text, int *out);

/* The same for a 64-bit number. */
bool vector_read_int64(const char *text, int64_t *out);

#endif /* NEPDATE_TESTS_VECTORS_H */
