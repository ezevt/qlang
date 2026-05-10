#include "repl.h"
#include "lexer.h"
#include "print_ast.h"
#include "source.h"
#include "parser.h"
#include "interp.h"

#include <string.h>

static void run(const char* buffer, Diagnostics *diag, Arena *arena, Interpreter *it) {
    SourceFile *src = source_from_string(buffer, "<repl>");
    printf("file %s: %lu lines, %lu chars\n", src->path, src->line_count, src->length);


    Lexer lexer = {
        .source = src,
        .diag = diag,
        .arena = arena,
        .pos = 0,
        .tokens = {0},
    };

    lex(&lexer);
    
    if (!diag_has_errors(diag)) {
        Parser parser = {
            .tokens = lexer.tokens.items,
            .count = lexer.tokens.count,
            .source = src,
            .diag = diag,
            .arena = arena,
            .pos = 0,
        };

        Stmt *root = parse(&parser);

        if (!diag_has_errors(diag)) {
            print_ast(root, stdout);
            printf("\n");

            interp_run(it, src, root);
        }
    }

    diag_print_all(diag, stdout);

    lex_free(&lexer);
    diag_clear(diag);
    source_free(src);
}

void run_loop(void) {
    printf("Q lang\n");
    printf("Type \"help\" for more information\n");
    printf("Type \"q\" to exit Q lang\n");

    char buffer[1024];
    Diagnostics diag = {0};
    Arena arena = {0};
    Interpreter it = {0};


    interp_init(&it, &diag);

    while (true) {
        printf(">> ");

        if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
            buffer[strcspn(buffer, "\n")] = '\0';

            if (strcmp(buffer, "q") == 0) {
                printf("\nInterrupted\n");
                break;
            }

            if (strcmp(buffer, "help") == 0) {
                printf("Help message not implemented yet.\n");
                continue;
            }

            run(buffer, &diag, &arena, &it);
        }
    }

    interp_shutdown(&it);
    arena_free(&arena);
}

