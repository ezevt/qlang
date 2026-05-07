#include "interpreter.h"
#include "common.h"
#include "diagnostic.h"
#include "value.h"

void interpreter_init(Interpreter *it) {}
void interpreter_shutdown(Interpreter *it) {}
void interpreter_run(Interpreter *it, SourceFile *src, Stmt *root) {}

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

InterpResult execute(Interpreter *it, SourceFile *src, Stmt *stmt) {}
