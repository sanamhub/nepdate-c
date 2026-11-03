#include "vectors.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool vector_open(vector_reader *reader, const char *relative) {
    char path[1024];
    const int length =
        snprintf(path, sizeof path, "%s/spec/vectors/%s", NEPDATE_C_SHARED_DIR, relative);
    if (length < 0 || (size_t)length >= sizeof path) {
        return false;
    }
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return false;
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        (void)fclose(file);
        return false;
    }
    const long size = ftell(file);
    if (size <= 0 || fseek(file, 0, SEEK_SET) != 0) {
        (void)fclose(file);
        return false;
    }
    char *data = malloc((size_t)size + 1);
    if (data == NULL) {
        (void)fclose(file);
        return false;
    }
    const size_t read = fread(data, 1, (size_t)size, file);
    (void)fclose(file);
    if (read != (size_t)size) {
        free(data);
        return false;
    }
    data[read] = '\0';
    reader->data = data;
    reader->length = read;
    reader->offset = 0;
    return true;
}

char *vector_next(vector_reader *reader) {
    while (reader->offset < reader->length) {
        char *line = reader->data + reader->offset;
        size_t end = reader->offset;
        while (end < reader->length && reader->data[end] != '\n') {
            end++;
        }
        const bool has_newline = end < reader->length;
        reader->offset = has_newline ? end + 1 : reader->length;
        size_t line_length = end - (size_t)(line - reader->data);
        if (line_length > 0 && line[line_length - 1] == '\r') {
            line[line_length - 1] = '\0';
            line_length--;
        }
        reader->data[end] = '\0';
        if (line_length > 0 && line[0] != '#') {
            return line;
        }
    }
    return NULL;
}

void vector_close(vector_reader *reader) {
    free(reader->data);
    reader->data = NULL;
    reader->length = 0;
    reader->offset = 0;
}

int vector_split(char *line, char separator, char **fields, int capacity) {
    int count = 0;
    char *field = line;
    for (;;) {
        if (count < capacity) {
            fields[count] = field;
        }
        count++;
        char *at = strchr(field, separator);
        if (at == NULL) {
            return count < capacity ? count : capacity;
        }
        *at = '\0';
        field = at + 1;
    }
}

static bool parse_integer(const char **text, int *out) {
    char *end = NULL;
    const long value = strtol(*text, &end, 10);
    if (end == *text) {
        return false;
    }
    *out = (int)value;
    *text = end;
    return true;
}

bool vector_parse_ymd(const char *text, int *year, int *month, int *day) {
    if (!parse_integer(&text, year) || *text != '-') {
        return false;
    }
    text++;
    if (!parse_integer(&text, month) || *text != '-') {
        return false;
    }
    text++;
    return parse_integer(&text, day);
}

bool vector_parse_date(const char *text, nepdate_date *out) {
    int year = 0;
    int month = 0;
    int day = 0;
    if (!vector_parse_ymd(text, &year, &month, &day)) {
        return false;
    }
    return nepdate_from_bs(year, month, day, out) == NEPDATE_OK;
}

bool vector_parse_range(const char *text, nepdate_range *out) {
    /* The two ends are separated by "..", which never appears inside a date. */
    const char *separator = strstr(text, "..");
    if (separator == NULL) {
        return false;
    }
    char first[16];
    char last[16];
    const size_t first_length = (size_t)(separator - text);
    if (first_length == 0 || first_length >= sizeof first) {
        return false;
    }
    memcpy(first, text, first_length);
    first[first_length] = '\0';
    const size_t last_length = strlen(separator + 2);
    if (last_length == 0 || last_length >= sizeof last) {
        return false;
    }
    memcpy(last, separator + 2, last_length + 1);
    nepdate_date start = {0};
    nepdate_date end = {0};
    if (!vector_parse_date(first, &start) || !vector_parse_date(last, &end)) {
        return false;
    }
    return nepdate_range_make(start, end, out) == NEPDATE_OK;
}

void vector_format_date(nepdate_date date, char *buffer, size_t size) {
    if (size == 0) {
        return;
    }
    /* The interchange form needs 11 bytes with the terminator; a smaller buffer truncates. */
    const size_t length = nepdate_format_iso(date, buffer, size);
    if (length >= size) {
        buffer[size - 1] = '\0';
    }
}

void vector_format_range(nepdate_range range, char *buffer, size_t size) {
    if (size == 0) {
        return;
    }
    char first[16];
    char last[16];
    vector_format_date(range.start, first, sizeof first);
    vector_format_date(range.end, last, sizeof last);
    const int written = snprintf(buffer, size, "%s..%s", first, last);
    if (written < 0 || (size_t)written >= size) {
        buffer[size - 1] = '\0';
    }
}

int vector_weekday(const char *name) {
    static const char *const names[] = {"Sunday",   "Monday", "Tuesday", "Wednesday",
                                        "Thursday", "Friday", "Saturday"};
    for (int i = 0; i < 7; i++) {
        if (strcmp(names[i], name) == 0) {
            return i;
        }
    }
    return -1;
}
