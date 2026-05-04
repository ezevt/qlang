#ifndef TOKEN_H
#define TOKEN_H

#include "source.h"
#include "diagnostic.h"

#include <stdlib.h>

typedef enum {
    TK_NUMBER, TK_IDENT, TK_STRING,

    TK_PLUS, TK_MINUS, TK_STAR, TK_SLASH, TK_BANG,
    TK_LPAREN, TK_RPAREN,
    TK_DOT, TK_COMMA,
    TK_EQUAL,
    TK_EQ, TK_NE, TK_GT, TK_GE, TK_LT, TK_LE,

    TK_LET, TK_IF, TK_THEN, TK_ELSE,
    TK_WHILE, TK_DO, TK_FOR, TK_CONTINUE, TK_BREAK,
    TK_FN, TK_RETURN, TK_END,
    TK_TRUE, TK_FALSE, TK_NIL,
    TK_AND, TK_OR,
    TK_PRINT,

    TK_EOF,
} TokenKind;

typedef struct {
    TokenKind kind;
    Span span;

    union {
        double number;
        StringSlice ident;
        StringSlice string;
    } as;
} Token;

typedef struct {
    Token *items;
    size_t count, cap;
} TokenArray;

#endif
