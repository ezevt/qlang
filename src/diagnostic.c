#include "diagnostic.h"
#include "array.h"
#include "source.h"

#include <stdarg.h>
#include <stdio.h>

void diag_emit(Diagnostics *d, DiagLevel lvl, SourceFile *src, Span span, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    char *msg;
    vasprintf(&msg, fmt, ap);

    va_end(ap);
    
    Diagnostic diag = {
        .span = span,
        .level = lvl,
        .message = msg,
        .source = src,
    };

    ARRAY_PUSH(d->items, d->count, d->cap, diag);
}

static void diag_print(Diagnostic *d, FILE *out) {
    const char *lvl = d->level == DIAG_ERROR ? "error" : d->level == DIAG_WARNING ? "warning" : "note";

    fprintf(out, "%s: %s\n", lvl, d->message);

    SourcePos pos = source_pos_at(d->source, d->span.offset);
    fprintf(out, " --> %s:%lu:%lu\n", d->source->path, pos.line, pos.col);
    
    StringSlice line = source_line(d->source, pos.line);
    int width = snprintf(NULL, 0, "%lu", pos.line);
    fprintf(out, "%*lu | %.*s\n", width, pos.line, (int)line.length, line.data);
    fprintf(out, "%*s | %*s^\n", (int)width, "", (int)pos.col - 1, "");
}

void diag_print_all(Diagnostics *d, FILE *out) {
    for (size_t i = 0; i < d->count; i++) {
        diag_print(&d->items[i], out);
    }
}

bool diag_has_errors(Diagnostics *d) {
    for (size_t i = 0; i < d->count; i++) {
        if (d->items[i].level == DIAG_ERROR) {
            return true;
        }
    }

    return false;
}

void diag_clear(Diagnostics *d) {
    for (size_t i = 0; i < d->count; i++) {
        free(d->items[i].message);
    }

    ARRAY_FREE(d->items, d->count, d->cap);
}
