#ifndef VALUE_H
#define VALUE_H

#include <stdbool.h>
#include <stdlib.h>

typedef enum {
    OBJ_FN,
    OBJ_STRING,
    OBJ_ENV,
} ObjKind;

typedef struct {
    ObjKind kind;
} Obj;

typedef enum {
    V_NIL,
    V_BOOL,
    V_NUMBER,
    V_OBJ,
} ValueKind;

typedef struct {
    ValueKind kind;

    union {
        double number;
        bool boolean;
        Obj *obj;
    } as;
} Value;

static inline Value value_nil(void)        { return (Value){.kind = V_NIL}; }
static inline Value value_bool(bool b)     { return (Value){.kind = V_BOOL,   .as.boolean = b}; }
static inline Value value_number(double n) { return (Value){.kind = V_NUMBER, .as.number =  n}; }
static inline Value value_obj(Obj *o)      { return (Value){.kind = V_OBJ,    .as.obj =     o}; }

static inline bool is_nil(Value v) { return v.kind == V_NIL; }
static inline bool is_boolean(Value v) { return v.kind == V_BOOL; }
static inline bool is_number(Value v) { return v.kind == V_NUMBER; }
static inline bool is_obj(Value v) { return v.kind == V_OBJ; }

static inline bool value_is_obj_kind(Value v, ObjKind k) {
    return v.kind == V_OBJ && v.as.obj->kind == k;
}

static inline bool value_is_string(Value v) { return value_is_obj_kind(v, OBJ_STRING); }
static inline bool value_is_fn(Value v) { return value_is_obj_kind(v, OBJ_FN); }

typedef struct {
    Obj obj;
    size_t length;
    char *data;
} ObjString;

ObjString *obj_string_new(const char *src, size_t length);
ObjString *obj_string_take(char *data, size_t length);

static inline ObjString *as_string(Value v) { return (ObjString *)v.as.obj; }

bool value_equals(Value a, Value b);
bool value_is_truthy(Value v);

void print_value(Value v);

ObjString value_to_string(Value v);

#endif
