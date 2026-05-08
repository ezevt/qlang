#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "diagnostic.h"
#include "interp.h"
#include "lexer.h"
#include "parser.h"
#include "print_ast.h"
#include "repl.h"
#include "source.h"

static bool run_source(
    SourceFile *src,
    Diagnostics *diag,
    Arena *arena,
    Interpreter *interp
) {
    Lexer lexer = {
        .source = src,
        .diag = diag,
        .arena = arena,
        .pos = 0,
        .tokens = {0},
    };

    lex(&lexer);

    if (diag_has_errors(diag)) {
        lex_free(&lexer);
        return false;
    }

    Parser parser = {
        .source = src,
        .diag = diag,
        .arena = arena,
        .tokens = lexer.tokens.items,
        .count = lexer.tokens.count,
        .pos = 0,
    };

    Stmt *program = parse(&parser);

    if (diag_has_errors(diag)) {
        lex_free(&lexer);
        return false;
    }

    print_ast(program, stdout);

    bool ok = interp_run(interp, src, program) != INTERP_ERROR;

    lex_free(&lexer);

    return ok;
}

static bool run_file(const char *path) {
    SourceFile *src = source_load(path);

    if (!src) {
        fprintf(
            stderr,
            "qlang: could not open '%s': %s\n",
            path,
            strerror(errno)
        );
        return false;
    }

    printf(
        "file %s: %zu lines, %zu chars\n",
        path,
        src->line_count,
        src->length
    );

    Diagnostics diag = {0};
    Arena arena = {0};
    Interpreter interp;

    interp_init(&interp, &diag);

    bool ok = run_source(src, &diag, &arena, &interp);

    diag_print_all(&diag, stdout);

    interp_shutdown(&interp);
    arena_free(&arena);
    diag_clear(&diag);
    source_free(src);

    return ok;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        run_loop();
        return 0;
    }

    return run_file(argv[1]) ? 0 : 1;
}
