#ifndef AST_H
#define AST_H

#include "diagnostic.h"
#include "token.h"

typedef struct Expr Expr;
typedef struct Stmt Stmt;

typedef enum {
    EX_NUMBER,
    EX_BOOLEAN,
    EX_STRING,
    EX_NIL,
    EX_VAR,
    EX_GROUPING,
    EX_UNARY,
    EX_BINARY,
    EX_ASSIGNMENT,
    EX_LOGIC,
} ExprKind;

typedef struct Expr {
    ExprKind kind;
    Span span;

    union {
        double number;
        bool boolean;
        struct { const char *data; size_t length; } string;
        struct { Span span; } var;
        struct { Expr *inner; } grouping;

        struct { Expr *right; TokenKind op; } unary;
        struct { Expr *left; Expr *right; TokenKind op; } binary;
        struct { Expr *target; Expr *value; } assignment;
        struct { Expr *left; Expr *right; TokenKind op; } logic;
    } as;
} Expr;

typedef enum {
    ST_EXPR,
    ST_PRINT,
    ST_BLOCK,
    ST_LET,
    ST_IF,
    ST_WHILE,
} StmtKind;

typedef struct Stmt {
    StmtKind kind;
    Span span;

    union {
        Expr *expr_stmt;
        Expr *print;
        
        struct {
            Stmt **items;
            size_t count;
        } block;

        struct {
            Span identifier;
            Expr *initializer;
        } let;
        
        struct {
            Expr *condition;
            Stmt *if_branch;
            Stmt *else_branch;
        } if_else;
        
        struct {
            Expr *condition;
            Stmt *body;
        } while_do;
    } as;
} Stmt;

#endif
