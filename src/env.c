#include "env.h"
#include "array.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static bool slice_eq(StringSlice a, StringSlice b) {
    if (a.length != b.length) return false;
    return memcmp(a.data, b.data, a.length) == 0;
}
 
static long env_find_local(ObjEnv *env, StringSlice name) {
    for (size_t i = 0; i < env->count; i++) {
        if (slice_eq(env->entries[i].name, name)) {
            return (long)i;
        }
    }
    return -1;
}
 
ObjEnv *env_new(ObjEnv *enclosing) {
    ObjEnv *env = malloc(sizeof(ObjEnv));

    env->obj.kind = OBJ_ENV;
    env->enclosing = enclosing;
    env->entries = NULL;
    env->count = 0;
    env->cap = 0;

    return env;
}

bool env_define(ObjEnv *env, StringSlice name, Value v) {
    if (env_find_local(env, name) >= 0)
        return false;

    EnvEntry entry = {
        .name = name,
        .value = v,
    };

    ARRAY_PUSH(env->entries, env->count, env->cap, entry);

    return true;
}

bool env_get(ObjEnv *env, StringSlice name, Value *out) {
    long idx = env_find_local(env, name);
    if (idx >= 0) {
        *out = env->entries[idx].value;
        return true;
    };

    if (!env->enclosing)
        return false;

    return env_get(env->enclosing, name, out);
}

bool env_assign(ObjEnv *env, StringSlice name, Value v) {
    long idx = env_find_local(env, name);
    if (idx >= 0) {
        env->entries[idx].value = v;
        return true;
    };

    if (!env->enclosing)
        return false;

    return env_assign(env->enclosing, name, v);
}
