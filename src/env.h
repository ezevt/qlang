#ifndef ENV_H
#define ENV_H

#include "source.h"
#include "value.h"

typedef struct ObjEnv ObjEnv;

typedef struct {
    StringSlice name;
    Value value;
} EnvEntry;

typedef struct ObjEnv {   
    Obj obj;
    EnvEntry *entries;
    size_t cap;
    size_t count;

    ObjEnv *enclosing;
} ObjEnv;

ObjEnv *env_new(ObjEnv *enclosing);
bool env_define(ObjEnv *env, StringSlice name, Value v);
bool env_get(ObjEnv *env, StringSlice name, Value *out);
bool env_assign(ObjEnv *env, StringSlice name, Value v);

#endif
