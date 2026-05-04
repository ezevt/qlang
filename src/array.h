#ifndef ARRAY_H
#define ARRAY_H

#include <stdlib.h>

#define ARRAY_INIT_CAP 8

#define ARRAY_PUSH(arr, count, cap, val) do {                        \
  if ((count) == (cap)) {                                            \
    (cap) = (cap) < ARRAY_INIT_CAP ? ARRAY_INIT_CAP : (cap) * 2;     \
    (arr) = realloc((arr), sizeof(*(arr)) * (cap));                  \
  }                                                                  \
  (arr)[(count)++] = (val);                                          \
} while (0)

#define ARRAY_FREE(arr, count, cap) do {                             \
  free(arr); (arr) = NULL; (count) = 0; (cap) = 0;                   \
} while (0)

#endif
