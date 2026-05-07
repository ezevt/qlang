#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "ast.h"
#include "value.h"
#include "diagnostic.h"

typedef enum {
    INTERP_OK,
    INTERP_ERROR,
    INTERP_RETURN,
} InterpResult;

typedef struct {
    Diagnostics *diag;
    Value return_value;
} Interpreter;

void interp_init(Interpreter *it);
void interp_shutdown(Interpreter *it);
InterpResult interp_run(Interpreter *it, SourceFile *src, Stmt *root);

InterpResult evaluate(Interpreter *it, SourceFile *src, Expr *expr, Value *out);
InterpResult execute(Interpreter *it, SourceFile* src, Stmt *stmt);

#endif
