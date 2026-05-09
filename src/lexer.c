#include "lexer.h"
#include "arena.h"
#include "array.h"
#include "diagnostic.h"
#include "token.h"

#include <ctype.h>
#include <string.h>

static inline bool is_newline(char c) {
    return c == '\n' || c == '\0';
}

typedef struct {
  const char *word;
  TokenKind token;
} KeywordEntry;

static const KeywordEntry keywords[] = {
    {"if", TK_IF},         {"then", TK_THEN},
    {"else", TK_ELSE},     {"end", TK_END},
    {"for", TK_FOR},       {"while", TK_WHILE},
    {"do", TK_DO},         {"fn", TK_FN},
    {"ret", TK_RET},       {"continue", TK_CONTINUE},
    {"break", TK_BREAK},   {"true", TK_TRUE},
    {"false", TK_FALSE},   {"nil", TK_NIL},
    {"and", TK_AND},       {"or", TK_OR},
    {"print", TK_PRINT},   {"let", TK_LET},
};

static char advance(Lexer *lex) {
    return lex->source->data[lex->pos++];
}

static char peek(Lexer *lex) {
    return lex->source->data[lex->pos];
}

static bool match(Lexer *lex, char c) {
    if (peek(lex) != c) return false;

    advance(lex);
    return true;
}

static void push_short(Lexer *lex, TokenKind kind) {
    Token token = (Token) {
        .kind = kind,
        .span = (Span) { lex->pos - 1, 1 },
    };

    ARRAY_PUSH(lex->tokens.items, lex->tokens.count, lex->tokens.cap, token);
}

static Token *push_long(Lexer *lex, TokenKind kind, size_t start, size_t end) {
    Token token = (Token) {
        .kind = kind,
        .span = (Span) { start, end - start },
    };

    ARRAY_PUSH(lex->tokens.items, lex->tokens.count, lex->tokens.cap, token);
    return &lex->tokens.items[lex->tokens.count - 1];
}

static void number(Lexer *lex) {
    size_t start = lex->pos - 1;
    
    while (isdigit(peek(lex))) advance(lex);
    
    if (peek(lex) == '.') {
        advance(lex);
        while (isdigit(peek(lex))) advance(lex);
    }

    size_t end = lex->pos;

    char saved = lex->source->data[end];
    lex->source->data[end] = '\0';

    double value = strtod(&lex->source->data[start], NULL);

    lex->source->data[end] = saved;

    Token *token = push_long(lex, TK_NUMBER, start, end);
    token->as.number = value;
}

static void ident(Lexer *lex) {
  size_t start = lex->pos - 1;
  while (isalnum(peek(lex)))
    advance(lex);

  size_t token_len = lex->pos - start;
  size_t keyword_count = sizeof(keywords) / sizeof(keywords[0]);

  for (size_t i = 0; i < keyword_count; i++) {
    const char *w = keywords[i].word;
    size_t len = strlen(w);

    if (len == token_len && memcmp(w, lex->source->data + start, len) == 0) {
      push_long(lex, keywords[i].token, start, lex->pos);
      return;
    }
  }

  Token *token = push_long(lex, TK_IDENT, start, lex->pos);
  token->as.ident.data = arena_strdup(lex->arena, lex->source->data + start, token_len);
  token->as.ident.length = token_len;
}

static void string(Lexer *lex) {
    size_t start = lex->pos; // ignore "

    while (peek(lex) != '"' && peek(lex) != '\0') advance(lex);
    
    size_t end = lex->pos;
    size_t length = end - start;

    advance(lex); // consume "

    Token *token = push_long(lex, TK_STRING, start, end);
    token->as.string.data = arena_strdup(lex->arena, lex->source->data + start, length);
    token->as.string.length = length;
}

static void next_token(Lexer *lex) {
    char c = advance(lex);

    switch(c) {
        case '+':
            push_short(lex, TK_PLUS);
            break;
        case '-':
            push_short(lex, TK_MINUS);
            break;
        case '*':
            push_short(lex, TK_STAR);
            break;
        case '(':
            push_short(lex, TK_LPAREN);
            break;
        case ')':
            push_short(lex, TK_RPAREN);
            break;
        case '.':
            push_short(lex, TK_DOT);
            break;
        case ',':
            push_short(lex, TK_COMMA);
            break;
        case '/':
            if (match(lex, '/')) {
                while (!is_newline(peek(lex))) advance(lex);
            } else push_short(lex, TK_SLASH);
            break;
        case '=':
            if (match(lex, '=')) push_short(lex, TK_EQ);
            else push_short(lex, TK_EQUAL);
            break;
        case '!':
            if (match(lex, '=')) push_short(lex, TK_NE);
            else push_short(lex, TK_BANG);
            break;
        case '>':
            if (match(lex, '=')) push_short(lex, TK_GE);
            else push_short(lex, TK_GT);
            break;
        case '<':
            if (match(lex, '=')) push_short(lex, TK_LE);
            else push_short(lex, TK_LT);
            break;
        case '"':
            string(lex);
            break;
        case ' ':
        case '\n':
        case '\t':
            break;
        case '\0':
            push_short(lex, TK_EOF);
            break;
        default:
            if (isdigit(c)) {
                number(lex);
            }
            else if (isalpha(c)) {
                ident(lex);
            }
            else {
                Span span = { lex->pos - 1, 1 };
                diag_emit(lex->diag, DIAG_ERROR, lex->source, span, "Unexpected character '%c'", c);
            }
    }
}

void lex(Lexer *lex) {
    while (lex->pos <= lex->source->length) {
        next_token(lex);
    }
}

void lex_free(Lexer *lex) {
    ARRAY_FREE(lex->tokens.items, lex->tokens.count, lex->tokens.cap);
    lex->pos = 0;
}
