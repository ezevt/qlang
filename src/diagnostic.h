#ifndef DIAGNOSTIC_H
#define DIAGNOSTIC_H

#include "source.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

typedef enum {
    DIAG_ERROR,
    DIAG_WARNING,
    DIAG_NOTE,
} DiagLevel;

typedef struct {
    size_t offset;
    size_t length;
} Span;

typedef struct {
    DiagLevel level;
    Span span;
    SourceFile *source;
    char *message; // owned
} Diagnostic;

typedef struct {
    Diagnostic *items;
    size_t count, cap;
} Diagnostics;

void diag_emit(Diagnostics *d, DiagLevel lvl, SourceFile *src, Span span, const char *fmt, ...);
void diag_print_all(Diagnostics *d, FILE *out);
bool diag_has_errors(Diagnostics *d);
void diag_clear(Diagnostics *d);

#endif
