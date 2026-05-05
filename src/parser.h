#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "arena.h"
#include "token.h"

typedef struct {
    Token *tokens;
    size_t count;
    size_t pos;
    Arena *arena;
    Diagnostics *diag;
    SourceFile *source;
} Parser;

Stmt *parse(Parser *p);


#endif
