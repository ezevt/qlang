#include "repl.h"
#include "lexer.h"
#include "source.h"
#include <string.h>

static void run(const char* buffer) {
    SourceFile *src = source_from_string(buffer, "<repl>");
    printf("file %s: %lu lines, %lu chars\n", src->path, src->line_count, src->length);

    Diagnostics d = {0};
    Arena a = {0};

    Lexer lexer = {
        .source = src,
        .diag = &d,
        .arena = &a,
        .pos = 0,
        .tokens = {0},
    };

    lex(&lexer);
    printf("Lexer found %lu tokens\n", lexer.tokens.count);

    diag_print_all(&d, stdout);
    
    lex_free(&lexer);
    arena_free(&a);
    diag_clear(&d);
    source_free(src);
}

void run_loop(void) {
    printf("Q lang\n");
    printf("Type \"help\" for more information\n");
    printf("Type \"q\" to exit Q lang\n");

    char buffer[1024];

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

            run(buffer);
        }
    }
}

