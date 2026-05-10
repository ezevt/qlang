#ifndef GC_H
#define GC_H

#include "value.h"

typedef struct Interpreter Interpreter;

typedef struct GC {
    Obj *objects;
    size_t bytes_allocated;
    size_t next_threshold;

    Interpreter *interp;

    bool stress;
    bool log;

    Obj **temp_roots;
    size_t temp_root_count;
    size_t temp_root_cap;
} GC;

void gc_init(GC *gc, Interpreter *interp);
void gc_shutdown(GC *gc);

void *gc_alloc_obj(GC *gc, size_t size, ObjKind kind);
void gc_collect(GC *gc);

void gc_push_root(GC *gc, Obj *obj);
void gc_pop_root(GC *gc);

#endif
