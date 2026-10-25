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
