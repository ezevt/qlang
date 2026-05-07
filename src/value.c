#include "value.h"

#include <stdio.h>

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
            printf("<obj>");
            break;
        default:
            printf("<unknown>");
    }
}
