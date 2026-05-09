#include "print_ast.h"
#include "common.h"

static void print_expr(Expr *expr, FILE *out) {
    switch (expr->kind) {
        case EX_NUMBER:
            fprintf(out, "%lf", expr->as.number);
            break;
        case EX_STRING:
            fprintf(out, "'%.*s'", (int)expr->as.string.length, expr->as.string.data);
            break;
        case EX_BOOLEAN:
            fprintf(out, "%s", expr->as.boolean ? "true" : "false");
            break;
        case EX_NIL:
            fprintf(out, "nil");
            break;
        case EX_VAR:
            fprintf(out, "var");
            break;
        case EX_UNARY:
            fprintf(out, "unary(%c", expr->as.unary.op == TK_BANG ? '!' : '-');
            print_expr(expr->as.unary.right, out);
            fprintf(out, ")");
            break;
        case EX_GROUPING:
            fprintf(out, "(");
            print_expr(expr->as.grouping.inner, out);
            fprintf(out, ")");
            break;
        case EX_BINARY:
        {
            TokenKind kind = expr->as.binary.op; 
            const char *op =
                kind == TK_PLUS  ? "+" :
                kind == TK_MINUS ? "-" :
                kind == TK_STAR  ? "*" :
                kind == TK_SLASH ? "/" :
                kind == TK_GT ? ">"  :
                kind == TK_GE ? ">=" :
                kind == TK_LT ? "<"  :
                kind == TK_LE ? "<=" :
                kind == TK_EQ ? "==" :
                kind == TK_NE ? "!=" :
                "";

            fprintf(out, "binary(");
            print_expr(expr->as.binary.left, out);
            fprintf(out, " %s ", op);
            print_expr(expr->as.binary.right, out);
            fprintf(out, ")");

            break;
        }
        case EX_ASSIGNMENT:
            fprintf(out, "(");
            print_expr(expr->as.assignment.target, out);
            fprintf(out, " = ");
            print_expr(expr->as.assignment.value, out);
            fprintf(out, ")");
            break;
        case EX_LOGIC:
        {
            TokenKind kind = expr->as.logic.op; 
            const char *op =
                kind == TK_AND ? "and" : "or";
            fprintf(out, "logic(");
            print_expr(expr->as.logic.left, out);
            fprintf(out, " %s ", op);
            print_expr(expr->as.logic.right, out);
            fprintf(out, ")");

            break;
        }
        default:
            UNREACHABLE();
    }
}

static void print_stmt(Stmt *stmt, FILE *out) {
    switch (stmt->kind) {
        case ST_BLOCK:
            fprintf(out, "{\n");
            for (size_t i = 0; i < stmt->as.block.count; i++) {
                print_stmt(stmt->as.block.items[i], out);
            }
            fprintf(out, "}\n");
            break;
        case ST_EXPR:
            print_expr(stmt->as.expr_stmt, out);
            fprintf(out, "\n");
            break;
        case ST_PRINT:
            fprintf(out, "print(");
            print_expr(stmt->as.print, out);
            fprintf(out, ")\n");
            break;
        case ST_LET:
            fprintf(out, "let(");
            if (stmt->as.let.initializer) print_expr(stmt->as.let.initializer, out);
            fprintf(out, ")\n");
            break;
        case ST_IF:
            fprintf(out, "if (");
            print_expr(stmt->as.if_else.condition, out);
            fprintf(out, ") {\n");
            print_stmt(stmt->as.if_else.if_branch, out);
            if (stmt->as.if_else.else_branch) {
                fprintf(out, "} else {\n");
                print_stmt(stmt->as.if_else.if_branch, out);
            }
            fprintf(out, "}\n");
            break;
        case ST_WHILE:
            fprintf(out, "while (");
            print_expr(stmt->as.while_do.condition, out);
            fprintf(out, ") {\n");
            print_stmt(stmt->as.while_do.body, out);
            fprintf(out, "}\n");
            break;
        case ST_FN:
            fprintf(out, "fn %.*s(%lu) {\n", (int)stmt->as.fn.name.length, stmt->as.fn.name.data, stmt->as.fn.param_count);
            print_stmt(stmt->as.fn.body, out);
            fprintf(out, "}\n");
            break;
        default:
            UNREACHABLE();
    }
}

void print_ast(Stmt *root, FILE *out) {
    print_stmt(root, out);
}
