#include "value.h"
#include "common.h"
#include "gc.h"

#include <stdio.h>
#include <string.h>

ObjString *obj_string_new(GC *gc, const char *src, size_t length) {
    ObjString *obj = gc_alloc_obj(gc, sizeof(ObjString), OBJ_STRING);

    char *data = malloc((length + 1) * sizeof(char));
    memcpy(data, src, length);
    data[length] = '\0';

    obj->obj.kind = OBJ_STRING;
    obj->data = data;
    obj->length = length;
    
    return obj;
}

ObjFn *obj_fn_new(GC *gc, StringSlice *params, size_t param_count, Stmt *body, ObjEnv *closure) {
    ObjFn *obj = gc_alloc_obj(gc, sizeof(ObjFn), OBJ_FN);

    obj->obj.kind = OBJ_FN;
    obj->params = params;
    obj->param_count = param_count;
    obj->body = body;
    obj->closure = closure;

    return obj;
}

bool value_equals(Value a, Value b) {
       if (a.kind != b.kind)
           return false;
       if (a.kind == V_NUMBER)
           return a.as.number == b.as.number;
       if (a.kind == V_BOOL)
           return a.as.boolean == b.as.boolean;
       if (a.kind == V_NIL)
           return true;

       // TODO: STRINGS
       return false;
}

bool value_is_truthy(Value v) {
    if (v.kind == V_NIL)
        return false;
    if (v.kind == V_BOOL)
        return v.as.boolean;
    if (v.kind == V_NUMBER)
        return v.as.number > 0;
    return true;
}

void print_value(Value v) {
    switch (v.kind) {
        case V_NUMBER:
            printf("%lf", v.as.number);
            break;
        case V_BOOL:
            printf("%s", v.as.boolean ? "true" : "false");
            break;
        case V_NIL:
            printf("nil");
            break;
        case V_OBJ:
            if (value_is_string(v)) {
                ObjString *str = as_string(v);
                printf("%.*s", (int)str->length, str->data);
            } else {
                printf("<obj>");
            }
            break;
        default:
            printf("<unknown>");
    }
}
