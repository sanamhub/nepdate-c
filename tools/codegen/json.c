#include "json.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Deeper nesting than this is an error, so a hostile file can't exhaust the stack. */
enum { JSON_MAX_DEPTH = 64 };

typedef struct {
    const char *text;
    size_t length;
    size_t pos;
    char *error;
    size_t error_size;
    bool failed;
} parser;

static void fail(parser *p, const char *message) {
    if (!p->failed && p->error_size > 0) {
        (void)snprintf(p->error, p->error_size, "JSON error at byte %zu: %s", p->pos, message);
    }
    p->failed = true;
}

static bool at_end(const parser *p) { return p->pos >= p->length; }

static char peek(const parser *p) { return at_end(p) ? '\0' : p->text[p->pos]; }

static void skip_space(parser *p) {
    while (!at_end(p)) {
        const char c = p->text[p->pos];
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
            return;
        }
        p->pos++;
    }
}

static bool expect_literal(parser *p, const char *literal) {
    const size_t n = strlen(literal);
    if (p->length - p->pos < n || memcmp(p->text + p->pos, literal, n) != 0) {
        fail(p, "unexpected token");
        return false;
    }
    p->pos += n;
    return true;
}

/* A growable byte buffer for decoded strings. */
typedef struct {
    char *data;
    size_t size;
    size_t capacity;
} buffer;

static bool buffer_push(buffer *b, char c) {
    if (b->size + 1 >= b->capacity) {
        const size_t capacity = b->capacity == 0 ? 32 : b->capacity * 2;
        char *data = realloc(b->data, capacity);
        if (data == NULL) {
            return false;
        }
        b->data = data;
        b->capacity = capacity;
    }
    b->data[b->size++] = c;
    b->data[b->size] = '\0';
    return true;
}

static int hex_digit(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

static bool read_hex4(parser *p, unsigned *out) {
    unsigned value = 0;
    for (int i = 0; i < 4; i++) {
        const int digit = hex_digit(peek(p));
        if (at_end(p) || digit < 0) {
            fail(p, "bad \\u escape");
            return false;
        }
        value = value * 16U + (unsigned)digit;
        p->pos++;
    }
    *out = value;
    return true;
}

static bool push_utf8(buffer *b, unsigned code) {
    if (code < 0x80U) {
        return buffer_push(b, (char)code);
    }
    if (code < 0x800U) {
        return buffer_push(b, (char)(0xC0U | (code >> 6))) &&
               buffer_push(b, (char)(0x80U | (code & 0x3FU)));
    }
    if (code < 0x10000U) {
        return buffer_push(b, (char)(0xE0U | (code >> 12))) &&
               buffer_push(b, (char)(0x80U | ((code >> 6) & 0x3FU))) &&
               buffer_push(b, (char)(0x80U | (code & 0x3FU)));
    }
    return buffer_push(b, (char)(0xF0U | (code >> 18))) &&
           buffer_push(b, (char)(0x80U | ((code >> 12) & 0x3FU))) &&
           buffer_push(b, (char)(0x80U | ((code >> 6) & 0x3FU))) &&
           buffer_push(b, (char)(0x80U | (code & 0x3FU)));
}

static bool read_escape(parser *p, buffer *b) {
    const char c = peek(p);
    p->pos++;
    switch (c) {
    case '"':
    case '\\':
    case '/':
        return buffer_push(b, c);
    case 'b':
        return buffer_push(b, '\b');
    case 'f':
        return buffer_push(b, '\f');
    case 'n':
        return buffer_push(b, '\n');
    case 'r':
        return buffer_push(b, '\r');
    case 't':
        return buffer_push(b, '\t');
    case 'u': {
        unsigned code = 0;
        if (!read_hex4(p, &code)) {
            return false;
        }
        if (code >= 0xD800U && code <= 0xDBFFU) {
            unsigned low = 0;
            if (!expect_literal(p, "\\u") || !read_hex4(p, &low) || low < 0xDC00U ||
                low > 0xDFFFU) {
                fail(p, "unpaired surrogate");
                return false;
            }
            code = 0x10000U + ((code - 0xD800U) << 10) + (low - 0xDC00U);
        } else if (code >= 0xDC00U && code <= 0xDFFFU) {
            fail(p, "unpaired surrogate");
            return false;
        }
        return push_utf8(b, code);
    }
    default:
        p->pos--;
        fail(p, "bad escape");
        return false;
    }
}

/* Reads a string starting at its opening quote. Returns a malloc'd copy, or NULL on error. */
static char *read_string(parser *p) {
    buffer b = {NULL, 0, 0};
    p->pos++; /* the opening quote */
    if (!buffer_push(&b, '\0')) {
        fail(p, "out of memory");
        return NULL;
    }
    b.size = 0;
    for (;;) {
        if (at_end(p)) {
            fail(p, "unterminated string");
            break;
        }
        const char c = p->text[p->pos];
        if (c == '"') {
            p->pos++;
            return b.data;
        }
        if ((unsigned char)c < 0x20U) {
            fail(p, "control character in string");
            break;
        }
        p->pos++;
        const bool ok = c == '\\' ? read_escape(p, &b) : buffer_push(&b, c);
        if (!ok) {
            fail(p, "out of memory");
            break;
        }
    }
    free(b.data);
    return NULL;
}

static bool is_digit(char c) { return c >= '0' && c <= '9'; }

static void read_number(parser *p, json_value *out) {
    const size_t start = p->pos;
    bool negative = false;
    if (peek(p) == '-') {
        negative = true;
        p->pos++;
    }
    if (!is_digit(peek(p))) {
        fail(p, "bad number");
        return;
    }
    /* Accumulate as a negative number, so LLONG_MIN fits too. */
    long long value = 0;
    bool fits = true;
    const bool leading_zero = peek(p) == '0';
    while (is_digit(peek(p))) {
        const int digit = p->text[p->pos] - '0';
        if (value < (LLONG_MIN + digit) / 10) {
            fits = false;
        } else {
            value = value * 10 - digit;
        }
        p->pos++;
    }
    if (leading_zero && p->pos - start - (negative ? 1U : 0U) > 1U) {
        fail(p, "leading zero in number");
        return;
    }
    bool integral = true;
    if (peek(p) == '.') {
        integral = false;
        p->pos++;
        if (!is_digit(peek(p))) {
            fail(p, "bad fraction");
            return;
        }
        while (is_digit(peek(p))) {
            p->pos++;
        }
    }
    if (peek(p) == 'e' || peek(p) == 'E') {
        integral = false;
        p->pos++;
        if (peek(p) == '+' || peek(p) == '-') {
            p->pos++;
        }
        if (!is_digit(peek(p))) {
            fail(p, "bad exponent");
            return;
        }
        while (is_digit(peek(p))) {
            p->pos++;
        }
    }
    out->type = JSON_NUMBER;
    if (integral && fits && (negative || value != LLONG_MIN)) {
        out->is_integer = true;
        out->integer = negative ? value : -value;
    }
}

static void read_value(parser *p, json_value *out, int depth);

static bool grow_items(parser *p, json_value *out, size_t *capacity, bool with_keys) {
    if (out->count < *capacity) {
        return true;
    }
    const size_t next = *capacity == 0 ? 8 : *capacity * 2;
    json_value *items = realloc(out->items, next * sizeof *items);
    if (items == NULL) {
        fail(p, "out of memory");
        return false;
    }
    out->items = items;
    if (with_keys) {
        char **keys = realloc(out->keys, next * sizeof *keys);
        if (keys == NULL) {
            fail(p, "out of memory");
            return false;
        }
        out->keys = keys;
    }
    *capacity = next;
    return true;
}

static void read_container(parser *p, json_value *out, int depth, bool object) {
    const char close = object ? '}' : ']';
    size_t capacity = 0;
    out->type = object ? JSON_OBJECT : JSON_ARRAY;
    p->pos++; /* the opening bracket */
    skip_space(p);
    if (peek(p) == close) {
        p->pos++;
        return;
    }
    for (;;) {
        if (!grow_items(p, out, &capacity, object)) {
            return;
        }
        json_value *item = &out->items[out->count];
        memset(item, 0, sizeof *item);
        if (object) {
            skip_space(p);
            if (peek(p) != '"') {
                fail(p, "expected a key");
                return;
            }
            char *key = read_string(p);
            if (key == NULL) {
                return;
            }
            out->keys[out->count] = key;
            skip_space(p);
            if (peek(p) != ':') {
                out->count++;
                fail(p, "expected ':'");
                return;
            }
            p->pos++;
        }
        out->count++;
        read_value(p, item, depth + 1);
        if (p->failed) {
            return;
        }
        skip_space(p);
        if (peek(p) == ',') {
            p->pos++;
            continue;
        }
        if (peek(p) == close) {
            p->pos++;
            return;
        }
        fail(p, object ? "expected ',' or '}'" : "expected ',' or ']'");
        return;
    }
}

static void read_value(parser *p, json_value *out, int depth) {
    if (depth > JSON_MAX_DEPTH) {
        fail(p, "nesting too deep");
        return;
    }
    skip_space(p);
    switch (peek(p)) {
    case '{':
        read_container(p, out, depth, true);
        break;
    case '[':
        read_container(p, out, depth, false);
        break;
    case '"':
        out->type = JSON_STRING;
        out->string = read_string(p);
        break;
    case 't':
        out->type = JSON_BOOL;
        out->boolean = expect_literal(p, "true");
        break;
    case 'f':
        out->type = JSON_BOOL;
        (void)expect_literal(p, "false");
        break;
    case 'n':
        out->type = JSON_NULL;
        (void)expect_literal(p, "null");
        break;
    default:
        read_number(p, out);
        break;
    }
}

static void free_members(json_value *value) {
    for (size_t i = 0; i < value->count; i++) {
        free_members(&value->items[i]);
        if (value->keys != NULL) {
            free(value->keys[i]);
        }
    }
    free(value->items);
    free((void *)value->keys);
    free(value->string);
}

void json_free(json_value *value) {
    if (value != NULL) {
        free_members(value);
        free(value);
    }
}

json_value *json_parse(const char *text, size_t length, char *error, size_t error_size) {
    parser p = {text, length, 0, error, error_size, false};
    if (error_size > 0) {
        error[0] = '\0';
    }
    json_value *root = calloc(1, sizeof *root);
    if (root == NULL) {
        fail(&p, "out of memory");
        return NULL;
    }
    read_value(&p, root, 0);
    if (!p.failed) {
        skip_space(&p);
        if (!at_end(&p)) {
            fail(&p, "trailing characters");
        }
    }
    if (p.failed) {
        json_free(root);
        return NULL;
    }
    return root;
}

const json_value *json_get(const json_value *value, const char *key) {
    if (value == NULL || value->type != JSON_OBJECT) {
        return NULL;
    }
    for (size_t i = 0; i < value->count; i++) {
        if (strcmp(value->keys[i], key) == 0) {
            return &value->items[i];
        }
    }
    return NULL;
}
