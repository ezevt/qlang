#ifndef LEXER_H
#define LEXER_H

#include "token.h"
#include "arena.h"
#include "source.h"
#include "diagnostic.h"

typedef struct {
    SourceFile *source;
    Arena *arena;
    Diagnostics *diag;

    size_t pos;
    TokenArray tokens;
} Lexer;

void lex(Lexer *lex);
void lex_free(Lexer *lex);

#endif
