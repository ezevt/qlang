#include "arena.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARENA_INITIAL_BLOCK_SIZE (16 * 1024) // 16 kb
#define ARENA_MAX_BLOCK_SIZE (1024 * 1024) //  1 mb

#define MIN(a,b) ((a) < (b) ? (a) : (b))

typedef struct ArenaBlock {
    ArenaBlock *next;
    size_t capacity;
    size_t used;
    char data[];
} ArenaBlock;

void arena_init(Arena *a) {
    a->current = NULL;
    a->total_allocated = 0;
}

void arena_free(Arena *a) {
    ArenaBlock *block = a->current;
    while (block != NULL) {
        ArenaBlock *next = block->next;
        free(block);
        block = next;
    }

    a->current = NULL;
    a->total_allocated = 0;
}

static ArenaBlock *arena_new_block(Arena *a, size_t min_size) {
    size_t cap = ARENA_INITIAL_BLOCK_SIZE;
    if (a->current != NULL) {
        cap = MIN(a->current->capacity * 2, ARENA_MAX_BLOCK_SIZE);
    }
    if (cap < min_size) cap = min_size;

    ArenaBlock *block = (ArenaBlock *)malloc(sizeof(ArenaBlock) + cap);
    if (block == NULL) {
        fprintf(stderr, "arena: out of memory (requested %zu bytes)\n", cap);
        abort();
    }

    block->next = a->current;
    block->capacity = cap;
    block->used = 0;

    a->current = block;
    a->total_allocated += cap;

    return block;
}

static size_t align_up(size_t n, size_t align) {
    return (n + (align - 1)) & ~(align - 1);
}

void *arena_alloc(Arena *a, size_t size, size_t align) {
    if (a->current == NULL) {
        arena_new_block(a, size);
    }

    ArenaBlock *block = a->current;
    size_t aligned_used = align_up(block->used, align);

    if (aligned_used + size > block->capacity) {
        block = arena_new_block(a, size);
        aligned_used = align_up(block->used, align);
    }

    void *ptr = block->data + aligned_used;
    block->used = aligned_used + size;
 
    return ptr;
}

char *arena_strdup(Arena *a, const char* src, size_t len) {
    char *dst = arena_alloc(a, len + 1, 1);
    memcpy(dst, src, len);
    dst[len] = '\0';
    return dst;
}
