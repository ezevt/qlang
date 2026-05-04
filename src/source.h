#ifndef SOURCE_H
#define SOURCE_H

#include <stdlib.h>

typedef struct {
    const char *path; // path o "<repl>"
    char *data; // null-terminated
    size_t length;

    size_t *line_starts;
    size_t line_count;
} SourceFile;

SourceFile *source_load(const char *path);
SourceFile *source_from_string(const char *src, const char *label);
void source_free(SourceFile *s);

typedef struct { size_t line, col; } SourcePos;
SourcePos source_pos_at(SourceFile *s, size_t offset);

typedef struct { const char *data; size_t length; } StringSlice;
StringSlice source_line(SourceFile *s, size_t line);


#endif
