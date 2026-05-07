#include "interpreter.h"
#include "ast.h"
#include "common.h"
#include "diagnostic.h"
#include "value.h"

void interpreter_init(Interpreter *it) {
    // init global env
}

void interpreter_shutdown(Interpreter *it) {
    //free global env
}

InterpResult interpreter_run(Interpreter *it, SourceFile *src, Stmt *root) {
    for (size_t i = 0; i < root->as.block.count; i++) {
        InterpResult res = execute(it, src, root->as.block.items[i]);
        if (res == INTERP_ERROR) return INTERP_ERROR;

        if (res == INTERP_RETURN) {
            diag_emit(it->diag, DIAG_ERROR, src, root->as.block.items[i]->span, "'return' outside of function.");
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
        case EX_VAR:
        case EX_STRING:
        case EX_ASSIGNMENT:
        default:
            UNREACHABLE();
    }

    return INTERP_OK;
}

InterpResult execute_block(Interpreter *it, SourceFile *src, Stmt *stmt) {
    // Create env

    for (size_t i = 0; i < stmt->as.block.count; i++) {
        InterpResult res = execute(it, src, stmt->as.block.items[i]);

        if (res == INTERP_ERROR) return INTERP_ERROR;
        if (res == INTERP_RETURN) return INTERP_RETURN;
    }

    return INTERP_OK;
}

InterpResult execute_print(Interpreter *it, SourceFile *src, Stmt *stmt) {
    Value v;
    InterpResult res = evaluate(it, src, stmt->as.print, &v);

    if (res == INTERP_ERROR) return INTERP_ERROR;

    print_value(v);
    printf("\n");

    return INTERP_OK;
}

InterpResult execute_expr(Interpreter *it, SourceFile *src, Stmt *stmt) {
    Value v;
    InterpResult res = evaluate(it, src, stmt->as.expr_stmt, &v);

    if (res == INTERP_ERROR) return INTERP_ERROR;
    return INTERP_OK;
}

InterpResult execute(Interpreter *it, SourceFile *src, Stmt *stmt) {
    switch (stmt->kind) {
        case ST_BLOCK:
            return execute_block(it, src, stmt);
        case ST_PRINT:
            return execute_print(it, src, stmt);
        case ST_EXPR:
            return execute_expr(it, src, stmt);
        case ST_LET:
        case ST_IF:
        case ST_WHILE:
        default:
            UNREACHABLE();
    }

    return INTERP_OK;
}
