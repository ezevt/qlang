#include "parser.h"
#include "arena.h"
#include "ast.h"
#include "diagnostic.h"
#include "array.h"
#include "source.h"
#include "token.h"

#include <string.h>

static Expr *expression(Parser *p);

static Token *peek(Parser *p) {
    return &p->tokens[p->pos];
}

static Token *previous(Parser *p) {
    return &p->tokens[p->pos-1];
}

static bool is_at_end(Parser *p) {
    return peek(p)->kind == TK_EOF;
}

static Token *advance(Parser *p) {
    if (!is_at_end(p)) p->pos++;
    return previous(p);
}

static bool check(Parser *p, TokenKind kind) {
    return peek(p)->kind == kind;
}

static bool match(Parser *p, TokenKind kind) {
    if (!check(p, kind)) return false;
    advance(p);
    return true;
}

static void synchronize(Parser *p);

static bool consume(Parser *p, TokenKind kind, const char *msg) {
    if (match(p, kind)) return true;
    Token *tok = peek(p);
    diag_emit(p->diag, DIAG_ERROR, p->source, tok->span, msg);
    synchronize(p);
    return false;
}

static void synchronize(Parser *p) {
    advance(p);

    while (!is_at_end(p)) {
        switch (peek(p)->kind) {
            case TK_FN:
            case TK_LET:
            case TK_IF:
            case TK_WHILE:
            case TK_RET:
            case TK_PRINT:
                return;
            default:
                advance(p);
        }
    }
}

static Expr *new_expr(Parser *p, ExprKind kind, Span span) {
    Expr *expr = ARENA_NEW(p->arena, Expr);
    expr->kind = kind;
    expr->span = span;
    return expr;
}

static Expr *primary(Parser *p) {
    if (match(p, TK_NUMBER)) {
        Token *tok = previous(p);
        Expr *expr = new_expr(p, EX_NUMBER, tok->span);
        expr->as.number = tok->as.number;
        
        return expr;
    }

    if (match(p, TK_TRUE) || match(p, TK_FALSE)) {
        Token *tok = previous(p);
        Expr *expr = new_expr(p, EX_BOOLEAN, tok->span);
        expr->as.boolean = tok->kind == TK_TRUE;
        
        return expr;
    }

    if (match(p, TK_NIL)) {
        Token *tok = previous(p);
        Expr *expr = new_expr(p, EX_NIL, tok->span);
        
        return expr;
    }

    if (match(p, TK_STRING)) {
        Token *tok = previous(p);
        Expr *expr = new_expr(p, EX_STRING, tok->span);
        expr->as.string.data = tok->as.string.data;
        expr->as.string.length = tok->as.string.length;
        
        return expr;
    }

    if (match(p, TK_IDENT)) {
        Token *tok = previous(p);
        Expr *expr = new_expr(p, EX_VAR, tok->span);
        expr->as.var.name = tok->as.ident;
        
        return expr;
    }

    if (match(p, TK_LPAREN)) {
        Expr *expr = expression(p);
        
        Token *tok = previous(p);
        Expr *grouping = new_expr(p, EX_GROUPING, tok->span);
        grouping->as.grouping.inner = expr;

        consume(p, TK_RPAREN, "Expected ')' after expression.");

        return grouping;
    }
    
    diag_emit(p->diag, DIAG_ERROR, p->source, peek(p)->span, "Expected expression.");
    return NULL;
}

static Expr *call(Parser *p) {
    Expr *expr = primary(p);

    if (match(p, TK_LPAREN)) {
        Expr **params = NULL;
        size_t count = 0;
        size_t cap = 0;

        while (peek(p)->kind != TK_RPAREN && !is_at_end(p)) {
            Expr *param = expression(p);
            ARRAY_PUSH(params, count, cap, param);

            if (peek(p)->kind != TK_RPAREN && !consume(p, TK_COMMA, "Expected ',' between parameters.")) {
                ARRAY_FREE(params, count, cap);
                return NULL;
            }
        }

        if (!consume(p, TK_RPAREN, "Expected ')' after function call.")) {
            ARRAY_FREE(params, count, cap);
            return NULL;
        }

        Expr **array = ARENA_NEW_ARRAY(p->arena, Expr*, count);
        memcpy(array, params, count * sizeof(Expr *));

        Expr *new = new_expr(p, EX_CALL, expr->span);
        new->as.call.callee = expr;
        new->as.call.params = array;
        new->as.call.param_count = count;

        expr = new;

        ARRAY_FREE(params, count, cap);
    }

    return expr;
}

static Expr *unary(Parser *p) {
    if (match(p, TK_BANG) || match(p, TK_MINUS)) {
        Token *op = previous(p);
        Expr *right = unary(p);

        Expr *expr = new_expr(p, EX_UNARY, op->span);
        expr->as.unary.op = op->kind;
        expr->as.unary.right = right;

        return expr;
    }

    return call(p);
}

static Expr *factor(Parser *p) {
    Expr *expr = unary(p);

    while (match(p, TK_SLASH) || match(p, TK_STAR)) {
        Token *op = previous(p);
        Expr *right = unary(p);

        Expr *new = new_expr(p, EX_BINARY, op->span);
        new->as.binary.left = expr;
        new->as.binary.right = right;
        new->as.binary.op = op->kind;

        expr = new;
    }

    return expr;
}

static Expr *term(Parser *p) {
    Expr *expr = factor(p);

    while (match(p, TK_PLUS) || match(p, TK_MINUS)) {
        Token *op = previous(p);
        Expr *right = factor(p);

        Expr *new = new_expr(p, EX_BINARY, op->span);
        new->as.binary.left = expr;
        new->as.binary.right = right;
        new->as.binary.op = op->kind;

        expr = new;
    }

    return expr;
}

static Expr *comparison(Parser *p) {
    Expr *expr = term(p);

    while (match(p, TK_GT) || match(p, TK_GE) ||
           match(p, TK_LT) || match(p, TK_LE)) {
        Token *op = previous(p);
        Expr *right = term(p);

        Expr *new = new_expr(p, EX_BINARY, op->span);
        new->as.binary.left = expr;
        new->as.binary.right = right;
        new->as.binary.op = op->kind;

        expr = new;
    }

    return expr;
}

static Expr *equality(Parser *p) {   
    Expr *expr = comparison(p);

    while (match(p, TK_EQ) || match(p, TK_NE)) {
        Token *op = previous(p);
        Expr *right = comparison(p);

        Expr *new = new_expr(p, EX_BINARY, op->span);
        new->as.binary.left = expr;
        new->as.binary.right = right;
        new->as.binary.op = op->kind;

        expr = new;
    }

    return expr;
}

static Expr *assignment(Parser *p) {
    Expr* expr = equality(p);

    if (match(p, TK_EQUAL)) {
        Token *op = previous(p);
        Expr *value = assignment(p);

        if (expr->kind == EX_VAR) {
            Expr *new = new_expr(p, EX_ASSIGNMENT, op->span);
            new->as.assignment.target = expr;
            new->as.assignment.value = value;

            return new;
        }

        diag_emit(p->diag, DIAG_ERROR, p->source, expr->span, "Invalid assignment target.");
    }

    return expr;
}

static Expr *and(Parser *p) {
    Expr *expr = assignment(p);

    while (match(p, TK_AND)) {
        Token *op = previous(p);
        Expr *right = assignment(p);

        Expr *new = new_expr(p, EX_LOGIC, op->span);
        new->as.logic.left = expr;
        new->as.logic.right = right;
        new->as.logic.op = TK_AND;

        expr = new;
    }

    return expr;
}

static Expr *or(Parser *p) {
    Expr *expr = and(p);

    while (match(p, TK_OR)) {
        Token *op = previous(p);
        Expr *right = and(p);

        Expr *new = new_expr(p, EX_LOGIC, op->span);
        new->as.logic.left = expr;
        new->as.logic.right = right;
        new->as.logic.op = TK_OR;

        expr = new;
    }

    return expr;
}

static Expr *expression(Parser *p) {
    return or(p);
}

static Stmt *new_stmt(Parser *p, StmtKind kind, Span span) {
    Stmt *stmt = ARENA_NEW(p->arena, Stmt);
    stmt->kind = kind;
    stmt->span = span;
    return stmt;
}

static Stmt *block(Parser *p);

static Stmt *print_statement(Parser *p) {
    Token *tok = advance(p);

    if (!consume(p, TK_LPAREN, "Expected '('."))
        return NULL;
    
    Expr *expr = expression(p);
    
    if (!consume(p, TK_RPAREN, "Expected ')'."))
        return NULL;

    Stmt *stmt = new_stmt(p, ST_PRINT, tok->span);
    stmt->as.print = expr;

    return stmt;
}

static Stmt *let_statement(Parser *p) {
    Token *tok = advance(p);

    if (!consume(p, TK_IDENT, "Expected identifier.")) return NULL;

    Token *ident = previous(p);

    Expr *initializer = NULL;
    if (match(p, TK_EQUAL)) {
        initializer = expression(p);
    }

    Stmt *stmt = new_stmt(p, ST_LET, tok->span);
    stmt->as.let.identifier = ident->as.ident;
    stmt->as.let.initializer = initializer;

    return stmt;
}

static Stmt *if_statement(Parser *p) {
    Token *tok = advance(p);

    Expr *condition = expression(p);

    if (!consume(p, TK_THEN, "Expected 'then' after condition.")) return NULL;
    
    Stmt *if_branch = block(p);
    Stmt *else_branch = NULL;
    
    if (match(p, TK_ELSE))
        else_branch = block(p);

    if (!consume(p, TK_END, "Expected 'end' after if-else statement.")) return NULL;

    Stmt *stmt = new_stmt(p, ST_IF, tok->span);
    stmt->as.if_else.condition = condition;
    stmt->as.if_else.if_branch = if_branch;
    stmt->as.if_else.else_branch = else_branch;

    return stmt;
}

static Stmt *while_statement(Parser *p) {
    Token *tok = advance(p);

    Expr *condition = expression(p);

    if (!consume(p, TK_DO, "Expected 'then' after condition.")) return NULL;
    
    Stmt *body = block(p);

    if (!consume(p, TK_END, "Expected 'end' after while-do loop.")) return NULL;

    Stmt *stmt = new_stmt(p, ST_WHILE, tok->span);
stmt->as.while_do.condition = condition;
    stmt->as.while_do.body = body;

    return stmt;
}

static Stmt *fn_statement(Parser *p) {
    Token *tok = advance(p);

    if (!consume(p, TK_IDENT, "Expected function name."))
            return NULL;

    Token *name = previous(p);

    if (!consume(p, TK_LPAREN, "Expected '(' after function name."))
        return NULL;
 
    StringSlice *params = NULL;
    size_t count = 0;
    size_t cap = 0;

    while (peek(p)->kind != TK_RPAREN && peek(p)->kind != TK_EOF) {
        if (!consume(p, TK_IDENT, "Expected parameter name.")) goto fail;
        
        Token *param = previous(p);
        ARRAY_PUSH(params, count, cap, param->as.ident);
        
        if (peek(p)->kind != TK_RPAREN && !consume(p, TK_COMMA, "Expected ',' between parameters."))
            goto fail;
    }

    if (!consume(p, TK_RPAREN, "Expected ')' after function parameters."))
        goto fail;

    Stmt *body = block(p);

    if (!consume(p, TK_END, "Expected 'end' after function declaration.")) goto fail;

    StringSlice *arena_params = ARENA_NEW_ARRAY(p->arena, StringSlice, count);
    memcpy(arena_params, params, count * sizeof(StringSlice));
    
    
    Stmt *stmt = new_stmt(p, ST_FN, tok->span);
    stmt->as.fn.name = name->as.ident;
    stmt->as.fn.params = arena_params;
    stmt->as.fn.param_count = count;
    stmt->as.fn.body = body;

    ARRAY_FREE(params, count, cap);
    return stmt;

fail:
    ARRAY_FREE(params, count, cap);
    return NULL;
}

static Stmt *ret_statement(Parser *p) {
    Token *tok = advance(p);
    
    Expr *value = expression(p);

    Stmt *stmt = new_stmt(p, ST_RET, tok->span);
    stmt->as.ret = value;

    return stmt;
}

static Stmt *expression_statement(Parser *p) {
    Token *start = peek(p);
    Expr *expr = expression(p);
    if (!expr) return NULL;
    
    Stmt *stmt = new_stmt(p, ST_EXPR, start->span);
    stmt->as.expr_stmt = expr;
    return stmt;
}

static Stmt *statement(Parser *p) {
    switch (peek(p)->kind) {
        case TK_PRINT:
            return print_statement(p);
        case TK_LET:
            return let_statement(p);
        case TK_IF:
            return if_statement(p);
        case TK_WHILE:
            return while_statement(p);
        case TK_FN:
            return fn_statement(p);
        case TK_RET:
            return ret_statement(p);
        default:
            return expression_statement(p);
    }
}

static Stmt *block(Parser *p) {
    Token *start = peek(p);

    Stmt **items = NULL;
    size_t count = 0;
    size_t cap = 0;

    while (peek(p)->kind != TK_END &&
           peek(p)->kind != TK_ELSE &&
           peek(p)->kind != TK_EOF) {
        Stmt *stmt = statement(p);

        if (!stmt) {
            synchronize(p);
            continue;
        }

        ARRAY_PUSH(items, count, cap, stmt);
    }

    Stmt **array = ARENA_NEW_ARRAY(p->arena, Stmt*, count);
    memcpy(array, items, count * sizeof(Stmt *));

    Stmt *block = new_stmt(p, ST_BLOCK, start->span);
    block->as.block.items = array;
    block->as.block.count = count;

    ARRAY_FREE(items, count, cap);
    
    return block;
}


Stmt *parse(Parser *p) {
    return block(p);
}
