#include "interp.h"
#include "ast.h"
#include "common.h"
#include "diagnostic.h"
#include "env.h"
#include "value.h"

void interp_init(Interpreter *it, Diagnostics *diag) {
    it->global = env_new(NULL);
    it->env = it->global;
    it->diag = diag;
    it->return_value = value_nil();
}

void interp_shutdown(Interpreter *it) {
    free(it->env->entries);
    free(it->env);
}

InterpResult interp_run(Interpreter *it, SourceFile *src, Stmt *root) {
    for (size_t i = 0; i < root->as.block.count; i++) {
        InterpResult res = execute(it, src, root->as.block.items[i]);
        if (res == INTERP_ERROR) return INTERP_ERROR;

        if (res == INTERP_RETURN) {
            diag_emit(it->diag, DIAG_ERROR, src, root->as.block.items[i]->span, "'ret' outside of function.");
            return INTERP_ERROR;
        }
    }

    return INTERP_OK;
}

static inline bool check_number(Interpreter *it, SourceFile *src, Value v, Span span) {
    if (is_number(v)) return true;

    diag_emit(it->diag, DIAG_ERROR, src, span, "Operand must be a number.");
    return false;
}

static inline bool check_numbers(Interpreter *it, SourceFile *src, Value a, Value b, Span span) {
    if (is_number(a) && is_number(b)) return true;

    diag_emit(it->diag, DIAG_ERROR, src, span, "Both operands must be numbers.");
    return false;
}

static InterpResult evaluate_var(Interpreter *it, SourceFile *src, Expr *expr, Value *out) {
    StringSlice name = expr->as.var.name;
    
    if (!env_get(it->env, name, out)) {
        diag_emit(it->diag, DIAG_ERROR, src, expr->span, "'%.*s' is not defined", name.length, name.data);
        return INTERP_ERROR;
    }

    return INTERP_OK;
}

static InterpResult evaluate_call(Interpreter *it, SourceFile *src, Expr *expr, Value *out) {
    Value v;
    if (evaluate(it, src, expr->as.call.callee, &v) == INTERP_ERROR)
        return INTERP_ERROR;

    if (!value_is_fn(v)) {
        diag_emit(it->diag, DIAG_ERROR, src, expr->span, "Can only call functions.");
        return INTERP_ERROR;
    }

    ObjFn *fn = as_fn(v);

    if (expr->as.call.param_count != fn->param_count) {
        diag_emit(it->diag, DIAG_ERROR, src, expr->span, "Expected %lu arguments, but got %lu.", fn->param_count, expr->as.call.param_count);
        return INTERP_ERROR;
    }

    ObjEnv *call_env = env_new(it->env);
    
    for (size_t i = 0; i < fn->param_count; i++) {
        Value p;
        if (evaluate(it, src, expr->as.call.params[i], &p) == INTERP_ERROR)
            return INTERP_ERROR;

        env_define(call_env, fn->params[i], p);
    }

    ObjEnv *prev_env = it->env;
    it->env = call_env;


    for (size_t i = 0; i < fn->body->as.block.count; i++) {
        InterpResult res = execute(it, src, fn->body->as.block.items[i]);
        if (res == INTERP_ERROR) return INTERP_ERROR;

        if (res == INTERP_RETURN) {
            *out = it->return_value;
            break;
        }
    }

    it->env = prev_env;

    return INTERP_OK;
}

static InterpResult evaluate_assignment(Interpreter *it, SourceFile *src, Expr *expr, Value *out) {
    StringSlice name = expr->as.assignment.target->as.var.name;
    
    if (evaluate(it, src, expr->as.assignment.value, out) == INTERP_ERROR)
        return INTERP_ERROR;

    if (!env_assign(it->env, name, *out)) {
        diag_emit(it->diag, DIAG_ERROR, src, expr->span, "'%.*s' is not defined", name.length, name.data);
        return INTERP_ERROR;
    }

    return INTERP_OK;
}

static InterpResult evaluate_unary(Interpreter *it, SourceFile *src, Expr *expr, Value *out) {
    Value v;
    InterpResult result = evaluate(it, src, expr->as.unary.right, &v);
    if (result == INTERP_ERROR) return result;

    switch (expr->as.unary.op) {
        case TK_BANG:
            *out = value_bool(!value_is_truthy(v));
            break;
        case TK_MINUS:
            if (!check_number(it, src, v, expr->as.unary.right->span))
                return INTERP_ERROR;

            *out = value_number(-v.as.number);
            break;
        default:
            UNREACHABLE();
    }

    return INTERP_OK;
}

static InterpResult evaluate_binary(Interpreter *it, SourceFile *src, Expr *expr, Value *out) {
    Value l, r;
    InterpResult result_l = evaluate(it, src, expr->as.binary.left, &l);
    InterpResult result_r = evaluate(it, src, expr->as.binary.right, &r);
    if (result_l == INTERP_ERROR || result_r == INTERP_ERROR) return INTERP_ERROR;

    switch (expr->as.binary.op) {
        case TK_PLUS:
            if (!check_numbers(it, src, r, l, expr->span))
                return INTERP_ERROR;
            *out = value_number(l.as.number + r.as.number);
            break;
        case TK_MINUS:
            if (!check_numbers(it, src, r, l, expr->span))
                return INTERP_ERROR;
            *out = value_number(l.as.number - r.as.number);
            break;
        case TK_STAR:
            if (!check_numbers(it, src, r, l, expr->span))
                return INTERP_ERROR;
            *out = value_number(l.as.number * r.as.number);
            break;
        case TK_SLASH:
            if (!check_numbers(it, src, r, l, expr->span))
                return INTERP_ERROR;
            *out = value_number(l.as.number / r.as.number);
            break;
        case TK_GT:
            if (!check_numbers(it, src, r, l, expr->span))
                return INTERP_ERROR;
            *out = value_bool(l.as.number > r.as.number);
            break;
        case TK_GE:
            if (!check_numbers(it, src, r, l, expr->span))
                return INTERP_ERROR;
            *out = value_bool(l.as.number >= r.as.number);
            break;
        case TK_LT:
            if (!check_numbers(it, src, r, l, expr->span))
                return INTERP_ERROR;
            *out = value_bool(l.as.number < r.as.number);
            break;
        case TK_LE:
            if (!check_numbers(it, src, r, l, expr->span))
                return INTERP_ERROR;
            *out = value_bool(l.as.number <= r.as.number);
            break;
        case TK_EQ:
            *out = value_bool(value_equals(l, r));
            break;
        case TK_NE:
            *out = value_bool(!value_equals(l, r));
            break;
        default:
            UNREACHABLE();
    }

    return INTERP_OK;
}

static InterpResult evaluate_logic(Interpreter *it, SourceFile *src, Expr *expr, Value *out) {
    Value l, r;
    InterpResult result_l = evaluate(it, src, expr->as.binary.left, &l);
    if (result_l == INTERP_ERROR) return INTERP_ERROR;
    

    if (expr->as.logic.op == TK_AND) {
        if (!value_is_truthy(l)) {
            *out = l;
        } else {
            InterpResult result_r = evaluate(it, src, expr->as.binary.right, &r);
            if (result_r == INTERP_ERROR) return INTERP_ERROR;
            *out = r;
        }
    } else if (expr->as.logic.op == TK_OR) {
        if (value_is_truthy(l)) {
            *out = l;
        } else {
            InterpResult result_r = evaluate(it, src, expr->as.binary.right, &r);
            if (result_r == INTERP_ERROR) return INTERP_ERROR;
            *out = r;
        }
    } else {
        UNREACHABLE();
    }

    return INTERP_OK;
}

InterpResult evaluate(Interpreter *it, SourceFile *src, Expr *expr, Value *out) {
    switch (expr->kind) {
        case EX_NIL:
            *out =  value_nil();
            break;
        case EX_BOOLEAN:
            *out = value_bool(expr->as.boolean);
            break;
        case EX_NUMBER:
            *out = value_number(expr->as.number);
            break;
        case EX_GROUPING:
            return evaluate(it, src, expr->as.grouping.inner, out);
            break;
        case EX_UNARY:
            return evaluate_unary(it, src, expr, out);
        case EX_BINARY:
            return evaluate_binary(it, src, expr, out);
        case EX_LOGIC:
            return evaluate_logic(it, src, expr, out);
        case EX_CALL:
            return evaluate_call(it, src, expr, out);
        case EX_VAR:
            return evaluate_var(it, src, expr, out);
        case EX_ASSIGNMENT:
            return evaluate_assignment(it, src, expr, out);
        case EX_STRING:
            *out = (Value) {
                .kind = V_OBJ,
                .as.obj = (Obj *)obj_string_new(expr->as.string.data, expr->as.string.length),
            };
            break;
        default:
            UNREACHABLE();
    }

    return INTERP_OK;
}

static InterpResult execute_block(Interpreter *it, SourceFile *src, Stmt *stmt) {
    ObjEnv *local = env_new(it->env);
    it->env = local;

    for (size_t i = 0; i < stmt->as.block.count; i++) {
        InterpResult res = execute(it, src, stmt->as.block.items[i]);

        if (res == INTERP_ERROR) return INTERP_ERROR;
        if (res == INTERP_RETURN) return INTERP_RETURN;
    }

    it->env = it->env->enclosing;

    free(local->entries);
    free(local);
    
    return INTERP_OK;
}

static InterpResult execute_print(Interpreter *it, SourceFile *src, Stmt *stmt) {
    Value v;
    InterpResult res = evaluate(it, src, stmt->as.print, &v);

    if (res == INTERP_ERROR) return INTERP_ERROR;

    print_value(v);
    printf("\n");

    return INTERP_OK;
}

static InterpResult execute_expr(Interpreter *it, SourceFile *src, Stmt *stmt) {
    Value v;
    InterpResult res = evaluate(it, src, stmt->as.expr_stmt, &v);

    if (res == INTERP_ERROR) return INTERP_ERROR;
    return INTERP_OK;
}

static InterpResult execute_let(Interpreter *it, SourceFile *src, Stmt *stmt) {
    Value v;

    if (stmt->as.let.initializer) {
        InterpResult res = evaluate(it, src, stmt->as.let.initializer, &v);
        if (res == INTERP_ERROR) return INTERP_ERROR;
    } else {
        v = value_nil();
    }

    StringSlice name = stmt->as.let.identifier;

    if (!env_define(it->env, name, v)) {
        diag_emit(it->diag, DIAG_ERROR, src, stmt->span, "'%.*s' is already defined.", name.length, name.data);
        return INTERP_ERROR;
    }

    return INTERP_OK;
}

static InterpResult execute_if(Interpreter *it, SourceFile *src, Stmt *stmt) {
    Value c;
    InterpResult res = evaluate(it, src, stmt->as.if_else.condition, &c);

    if (res == INTERP_ERROR) return INTERP_ERROR;

    if (value_is_truthy(c)) {
        return execute(it, src, stmt->as.if_else.if_branch);
    } else if (stmt->as.if_else.else_branch) {
        return execute(it, src, stmt->as.if_else.else_branch);
    }

    return INTERP_OK;
}

static InterpResult execute_while(Interpreter *it, SourceFile *src, Stmt *stmt) {
    Value c;

    while (true) {
        InterpResult res = evaluate(it, src, stmt->as.while_do.condition, &c);
        if (res == INTERP_ERROR) return INTERP_ERROR;
        
        if (!value_is_truthy(c)) break;

        res = execute(it, src, stmt->as.while_do.body);
        if (res == INTERP_ERROR) return INTERP_ERROR;
    }

    return INTERP_OK;
}

static InterpResult execute_fn(Interpreter *it, SourceFile *src, Stmt *stmt) {
    Value v = {
        .kind = V_OBJ,
        .as.obj = (Obj *)obj_fn_new(stmt->as.fn.params, stmt->as.fn.param_count, stmt->as.fn.body, it->env),
    };

    if (!env_define(it->env, stmt->as.fn.name, v)) {
        diag_emit(it->diag, DIAG_ERROR, src, stmt->span, "'%.*s' is already defined.", stmt->as.fn.name.length, stmt->as.fn.name.data);
        return INTERP_ERROR;
    }
    
    return INTERP_OK;
}

static InterpResult execute_ret(Interpreter *it, SourceFile *src, Stmt *stmt) {
    Value v;
    if (evaluate(it, src, stmt->as.ret, &v) == INTERP_ERROR)
        return INTERP_ERROR;
    
    it->return_value = v;

    return INTERP_RETURN;
}

InterpResult execute(Interpreter *it, SourceFile *src, Stmt *stmt) {
    switch (stmt->kind) {
        case ST_BLOCK:      return execute_block(it, src, stmt);
        case ST_PRINT:      return execute_print(it, src, stmt);
        case ST_EXPR:       return execute_expr(it, src, stmt);
        case ST_LET:        return execute_let(it, src, stmt);
        case ST_IF:         return execute_if(it, src, stmt);
        case ST_WHILE:      return execute_while(it, src, stmt);
        case ST_FN:         return execute_fn(it, src, stmt);
        case ST_RET:        return execute_ret(it, src, stmt);
        default:
            UNREACHABLE();
    }

    return INTERP_OK;
}
