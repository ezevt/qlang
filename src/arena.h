#ifndef ARENA_H
#define ARENA_H

#include <stdlib.h>

typedef struct ArenaBlock ArenaBlock;

typedef struct {
    ArenaBlock *current;
    size_t total_allocated;
} Arena;

void arena_init(Arena *a);
void *arena_alloc(Arena *a, size_t size, size_t align);
char *arena_strdup(Arena *a, const char* src, size_t len);
void arena_free(Arena *a);

#define ARENA_NEW(arena, T) \
    ((T*)arena_alloc(arena, sizeof(T), _Alignof(T)))

#define ARENA_NEW_ARRAY(arena, T, n) \
    ((T*)arena_alloc(arena, sizeof(T) * (n), _Alignof(T)))

#endif
