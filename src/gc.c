#include "gc.h"
#include "array.h"
#include "value.h"
#include "env.h"
#include "interp.h"

#define GC_INITIAL_THRESHOLD (1024 * 1024) // 1 MiB
#define GC_GROWTH_FACTOR 2

void gc_init(GC *gc, Interpreter *interp) {
    gc->interp = interp;
    gc->objects = NULL;
    gc->bytes_allocated = 0;
    gc->next_threshold = GC_INITIAL_THRESHOLD;
    gc->stress = true;
    gc->log = false;
    gc->temp_roots = NULL;
    gc->temp_root_count = 0;
    gc->temp_root_cap = 0;
}

void *gc_alloc_obj(GC *gc, size_t size, ObjKind kind) {
    if (gc->stress) {
        gc_collect(gc);
    } else if (gc->bytes_allocated + size > gc->next_threshold) {
        gc_collect(gc);
    }

    Obj* obj = malloc(size);
    obj->kind = kind;
    obj->marked = false;
    obj->next = gc->objects;

    gc->objects = obj;
    gc->bytes_allocated += size;

    return obj;
}

static void mark_obj(Obj *obj);
static void mark_value(Value v);
static void mark_env(ObjEnv *env);

static void mark_value(Value v) {
    if (v.kind == V_OBJ)
        mark_obj(v.as.obj);
}

static void mark_env(ObjEnv *env) {
    if (!env) return;
    mark_obj((Obj *)env);
}

static void mark_obj(Obj *obj) {
    if (!obj) return;
    if (obj->marked) return;

    obj->marked = true;

    switch (obj->kind) {
        case OBJ_STRING:
            break;
        case OBJ_FN: {
            ObjFn *fn = (ObjFn *)obj;
            mark_env(fn->closure);
            break;
        }
        case OBJ_ENV: {
            ObjEnv *env = (ObjEnv *)obj;
            for (size_t i = 0; i < env->count; i++) {
                mark_value(env->entries[i].value);
            }
            mark_env(env->enclosing);
            break;
        }
    }
}

static void mark_roots(GC *gc) {
    Interpreter *it = gc->interp;
    if (!it) return;

    if (it->global) mark_env(it->global);
    if (it->env) mark_env(it->env);

    mark_value(it->return_value);

    for (size_t i = 0; i < gc->temp_root_count; i++) {
        mark_obj(gc->temp_roots[i]);
    }
}

static void free_obj(Obj *obj) {
    switch (obj->kind) {
        case OBJ_STRING: {
            ObjString *str = (ObjString *)obj;
            free(str->data);
            break;
        }
        case OBJ_FN:
            break;
        case OBJ_ENV: {
            ObjEnv *env = (ObjEnv *)obj;
            free(env->entries);
            break;
        }
    }

    free(obj);
}

static void sweep(GC *gc) {
    Obj **link = &gc->objects;
    while (*link) {
        Obj *obj = *link;
        if (obj->marked) {
            obj->marked = false;
            link = &obj->next;
        } else {
            *link = obj->next;
            free_obj(obj);
        }
    }
}

void gc_push_root(GC *gc, Obj *obj) {
    ARRAY_PUSH(gc->temp_roots, gc->temp_root_count, gc->temp_root_cap, obj);
}

void gc_pop_root(GC *gc) {
    gc->temp_root_count--;
}

void gc_collect(GC *gc) {
    if (gc->log) {
        fprintf(stderr, "[gc] collect start, %zu bytes\n", gc->bytes_allocated);
    }

    mark_roots(gc);
    sweep(gc);

    gc->next_threshold = gc->bytes_allocated * GC_GROWTH_FACTOR;
    if (gc->next_threshold < GC_INITIAL_THRESHOLD)
        gc->next_threshold = GC_INITIAL_THRESHOLD;
  
    if (gc->log) {
        fprintf(stderr, "[gc] collect end, %zu bytes, next at %zu\n",
                gc->bytes_allocated, gc->next_threshold);
    }
}


void gc_shutdown(GC *gc) {
    Obj *obj = gc->objects;
    while (obj) {
        Obj *next = obj->next;
        free_obj(obj);
        obj = next;
    }
    gc->objects = NULL;
    gc->bytes_allocated = 0;
    ARRAY_FREE(gc->temp_roots, gc->temp_root_count, gc->temp_root_cap);
}
