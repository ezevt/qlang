#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define UNUSED(x) ((void)(x))

#define UNREACHABLE() \
    do { fprintf(stderr, "Unreachable at %s:%d", __FILE__, __LINE__); abort(); } while (0)

#endif
