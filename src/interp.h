#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "ast.h"
#include "value.h"
#include "diagnostic.h"
#include "env.h"
#include "gc.h"

typedef enum {
    INTERP_OK,
    INTERP_ERROR,
    INTERP_RETURN,
} InterpResult;

typedef struct Interpreter {
    Diagnostics *diag;
    GC gc;
    ObjEnv *global;
    ObjEnv *env;
    Value return_value;
} Interpreter;

void interp_init(Interpreter *it, Diagnostics *diag);
void interp_shutdown(Interpreter *it);
InterpResult interp_run(Interpreter *it, SourceFile *src, Stmt *root);

InterpResult evaluate(Interpreter *it, SourceFile *src, Expr *expr, Value *out);
InterpResult execute(Interpreter *it, SourceFile* src, Stmt *stmt);

#endif
