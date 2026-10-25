/* A small JSON reader for the generator (ADR-0001). It allocates and uses libc; the library never
 * links it. */
#ifndef NEPDATE_CODEGEN_JSON_H
#define NEPDATE_CODEGEN_JSON_H

#include <stdbool.h>
#include <stddef.h>

typedef enum { JSON_NULL, JSON_BOOL, JSON_NUMBER, JSON_STRING, JSON_ARRAY, JSON_OBJECT } json_type;

typedef struct json_value json_value;

struct json_value {
    json_type type;
    bool boolean;
    /* JSON_NUMBER: is_integer is true when the text was an integer that fits in long long. */
    bool is_integer;
    long long integer;
    /* JSON_STRING: the decoded UTF-8 text, NUL-terminated. */
    char *string;
    /* JSON_ARRAY and JSON_OBJECT: count items; an object also has one key per item. */
    size_t count;
    json_value *items;
    char **keys;
};

/* Parses length bytes of text. Returns a tree to free with json_free, or NULL with a message in
 * error (always NUL-terminated when error_size is not 0). */
json_value *json_parse(const char *text, size_t length, char *error, size_t error_size);

void json_free(json_value *value);

/* The member named key of an object, or NULL when value is not an object or has no such key. */
const json_value *json_get(const json_value *value, const char *key);

#endif /* NEPDATE_CODEGEN_JSON_H */
