/* tools/codegen: writes the generated headers under src/generated/ from shared/data (ADR-0001).
 *
 *   nepdate_c_codegen --shared <shared dir> --out <repository root> [--check]
 *
 * --check generates in memory, prints each file that differs from the disk and exits 1 without
 * writing. Exit 2 means bad arguments, unreadable input, invalid data or an output directory that
 * doesn't exist (the tool creates files, not directories). */
#include "calendar.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *shared;
    const char *out;
    bool check;
} options;

static bool parse_args(int argc, char **argv, options *opts) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--check") == 0) {
            opts->check = true;
        } else if (strcmp(argv[i], "--shared") == 0 && i + 1 < argc) {
            opts->shared = argv[++i];
        } else if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) {
            opts->out = argv[++i];
        } else {
            return false;
        }
    }
    return opts->shared != NULL && opts->out != NULL;
}

static char *join(const char *dir, const char *relative) {
    const size_t size = strlen(dir) + 1 + strlen(relative) + 1;
    char *path = malloc(size);
    if (path != NULL) {
        (void)snprintf(path, size, "%s/%s", dir, relative);
    }
    return path;
}

/* Reads a whole file in binary mode, so Windows neither adds nor hides a CR. Returns a malloc'd,
 * NUL-terminated buffer, or NULL when the file can't be read. */
static char *read_file(const char *path, size_t *length) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return NULL;
    }
    size_t size = 0;
    size_t capacity = 4096;
    char *data = malloc(capacity);
    while (data != NULL) {
        const size_t n = fread(data + size, 1, capacity - size - 1, file);
        size += n;
        if (n == 0) {
            break;
        }
        if (capacity - size - 1 == 0) {
            char *bigger = realloc(data, capacity * 2);
            if (bigger == NULL) {
                free(data);
                data = NULL;
                break;
            }
            data = bigger;
            capacity *= 2;
        }
    }
    const bool read_error = ferror(file) != 0;
    if (fclose(file) != 0 || read_error) {
        free(data);
        return NULL;
    }
    if (data != NULL) {
        data[size] = '\0';
        *length = size;
    }
    return data;
}

static bool write_file(const char *path, const char *content) {
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        return false;
    }
    const size_t length = strlen(content);
    const bool wrote = fwrite(content, 1, length, file) == length;
    return fclose(file) == 0 && wrote;
}

/* Writes or checks one generated file. Returns 0 when it matches, 1 when --check found a
 * difference, 2 when it can't be written. */
static int emit(const options *opts, const char *relative, const char *content) {
    char *path = join(opts->out, relative);
    if (path == NULL) {
        (void)fprintf(stderr, "out of memory\n");
        return 2;
    }
    int status = 0;
    size_t length = 0;
    char *existing = read_file(path, &length);
    const bool same =
        existing != NULL && length == strlen(content) && memcmp(existing, content, length) == 0;
    free(existing);
    /* An unchanged file isn't rewritten, so its timestamp doesn't trigger a rebuild. */
    if (!same) {
        if (opts->check) {
            (void)fprintf(stderr, "out of date: %s\n", path);
            status = 1;
        } else if (!write_file(path, content)) {
            (void)fprintf(stderr, "cannot write %s\n", path);
            status = 2;
        }
    }
    free(path);
    return status;
}

int main(int argc, char **argv) {
    options opts = {NULL, NULL, false};
    if (!parse_args(argc, argv, &opts)) {
        (void)fprintf(stderr, "usage: nepdate_c_codegen --shared <dir> --out <dir> [--check]\n");
        return 2;
    }

    char *calendar_path = join(opts.shared, "data/calendar/bs-calendar.json");
    size_t length = 0;
    char *json_text = calendar_path == NULL ? NULL : read_file(calendar_path, &length);
    if (json_text == NULL) {
        (void)fprintf(stderr, "cannot read %s\n", calendar_path == NULL ? "" : calendar_path);
        free(calendar_path);
        return 2;
    }
    free(calendar_path);

    codegen_calendar calendar;
    char error[256];
    const bool parsed = codegen_parse_calendar(json_text, length, &calendar, error, sizeof error);
    free(json_text);
    if (!parsed) {
        (void)fprintf(stderr, "%s\n", error);
        return 2;
    }

    char *tables = codegen_render_calendar_tables(&calendar);
    if (tables == NULL) {
        (void)fprintf(stderr, "out of memory\n");
        return 2;
    }
    const int status = emit(&opts, "src/generated/calendar_tables.h", tables);
    free(tables);
    return status;
}
