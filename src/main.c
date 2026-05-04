#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "lexer.h"
#include "source.h"
#include "diagnostic.h"

int main(void) {
    const char *filepath = "tests/hello-world.q";
    SourceFile *src = source_load(filepath);
    if (!src) {
        fprintf(stderr, "qlang: could not open '%s': %s\n", filepath, strerror(errno));
        return 1;
    }
    
    printf("file %s: %lu lines, %lu chars\n", filepath, src->line_count, src->length);

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
    return 0;
}
