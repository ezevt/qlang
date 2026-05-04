#include <stdio.h>
#include <string.h>

#include "source.h"

static SourceFile *source_create(char* data, size_t length, const char* label) {
    size_t line_count = 1;
    printf("length: %lu\n", length);
    for (size_t i = 0; i < length; i++)
        if (data[i] == '\n') line_count++;

    size_t *line_starts = (size_t *)malloc((line_count) * sizeof(size_t));

    size_t idx = 0;
    line_starts[idx++] = 0;

    for (size_t i = 0; i < length; i++) {
        if (data[i] == '\n' && i + 1 < length) {
            line_starts[idx++] = i + 1;
        }
    }

    SourceFile *source_file = (SourceFile *)malloc(sizeof(SourceFile));
    *source_file = (SourceFile) {
        .path = label,
        .data = data,
        .length = length,
        .line_starts = line_starts,
        .line_count = line_count,
    };

    return source_file;
}

SourceFile *source_load(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size_t size = (size_t)ftell(file);
    rewind(file);

    char *buffer = (char *)malloc((size + 1) * sizeof(char));
    if (!buffer) {
        fclose(file);
        return NULL;
    }

    size_t read_size = (size_t)fread(buffer, 1, size, file);
    if (read_size != (size_t)size) {
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[size] = '\0';

    fclose(file);

    return source_create(buffer, size, path);
}

SourceFile *source_from_string(const char* src, const char *label) {
    size_t length = strlen(src);

    char* data = (char *)malloc((length + 1) * sizeof(char));
    data = memcpy(data, src, length);
    data[length] = '\0';

    return source_create(data, length, label);
}

void source_free(SourceFile *s) {
    free((void *)s->data);
    free(s->line_starts);
    free(s);
}

SourcePos source_pos_at(SourceFile *s, size_t offset) {
    SourcePos pos = {0};

    for (size_t i = s->line_count-1; i >= 0; i--) {
        if (s->line_starts[i] > offset) continue;

        pos.line = i + 1;
        pos.col = offset - s->line_starts[i] + 1;

        break;
    }

    return pos;
}

StringSlice source_line(SourceFile *s, size_t line) {
    if (line == 0 || line > s->line_count) {
        return (StringSlice) {
             .data = NULL,
             .length = 0,
        };
    }

    size_t start = s->line_starts[line-1];
    
    size_t length = 0;
    while (start + length < s->length &&
           s->data[start + length] != '\n') {
        length++;
    }

    return (StringSlice) {
        .data = s->data+start,
        .length = length,
    };
}
