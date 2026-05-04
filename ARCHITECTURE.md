# Arquitectura — Q lang

Notas de diseño para refactor / próxima versión. No es lo que el código hace hoy, es el norte hacia donde se quiere ir.

## Las cuatro decisiones clave

Si solo me llevo cuatro ideas de todo esto, son estas:

1. **Arena para AST/tokens, GC heap para runtime.** Dos mundos de memoria, separados, con disciplinas distintas.
2. **Header `Obj` común para todo lo heap-allocado.** El GC marca/sweep sin saber de qué tipo es cada cosa.
3. **`Diagnostics` en lugar de `printf`.** Errores como datos: testeables, formateables, acumulables.
4. **AST autocontenido (datos copiados, no slices al source).** El AST sobrevive aunque se libere el `SourceFile`.

Todo lo demás del documento son detalles que sirven a estas cuatro ideas.

---

## Estructura de archivos

```
src/
  common.h          // tipos básicos, macros (UNREACHABLE, UNUSED)
  arena.h/.c        // allocator de arena para AST/tokens
  array.h           // macros de array dinámico (PUSH, FREE)
  string_buf.h/.c   // builder de strings

  source.h/.c       // SourceFile + utilidades de span/posición
  diagnostic.h/.c   // sistema de errores

  token.h           // Token, TokenKind, Span
  lexer.h/.c

  ast.h             // Expr, Stmt
  parser.h/.c
  ast_print.h/.c    // printer separado, para snapshots/tests

  value.h           // Value, Obj (header GC)
  gc.h/.c
  env.h/.c

  interp.h/.c

  main.c
tests/
  cases/*.q
  expected/*.txt
  run.sh
LANG.md             // semánticas decididas
```

### Por qué separar `ast_print` del AST

El printer es para tests (snapshots) y debugging (`--print-ast`). Mantenerlo aparte evita que cambios en el AST rompan formato de tests, y mantiene el header del AST limpio.

---

## Memoria: dos mundos separados

### Mundo 1: memoria del compilador (arena)

Vive: tokens, nodos del AST, identificadores, strings literales (ya con escapes resueltos).
Muere: cuando termina el programa (o, en REPL, cuando se cierra el intérprete).
Disciplina: arena allocator. Un solo `arena_free()` libera todo.

```c
typedef struct ArenaBlock ArenaBlock;
typedef struct {
  ArenaBlock *current;
  size_t total_allocated;
} Arena;

void arena_init(Arena *a);
void *arena_alloc(Arena *a, size_t size, size_t align);
char *arena_strdup(Arena *a, const char *src, size_t len);
void arena_free(Arena *a);

#define ARENA_NEW(arena, T) \
  ((T*)arena_alloc((arena), sizeof(T), _Alignof(T)))
```

**Ventaja clave:** se eliminan los `free_expr` / `free_stmt` recursivos, con sus bugs latentes (ej: olvidar liberar args individuales de un CALL).

### Mundo 2: memoria del runtime (GC heap)

Vive: strings dinámicos (resultados de concat, conversiones), funciones (closures), environments.
Muere: cuando ya no es alcanzable desde las roots.
Disciplina: mark-and-sweep.

**Regla de oro:** nada del runtime referencia memoria de la arena de forma que se rompa si la arena se libera primero. La arena se libera SIEMPRE después del último GC.

---

## El sistema de errores: `Diagnostic`

```c
typedef enum { DIAG_ERROR, DIAG_WARNING, DIAG_NOTE } DiagLevel;

typedef struct { size_t offset, length; } Span;

typedef struct {
  DiagLevel level;
  Span span;
  SourceFile *source;
  char *message;       // owned
} Diagnostic;

typedef struct {
  Diagnostic *items;
  size_t count, cap;
} Diagnostics;

void diag_emit(Diagnostics *d, DiagLevel lvl, SourceFile *src, Span s,
               const char *fmt, ...);
void diag_print_all(Diagnostics *d, FILE *out);
bool diag_has_errors(Diagnostics *d);
void diag_clear(Diagnostics *d);
```

**Por qué esto y no `printf`:**

- Tests pueden inspeccionar errores sin parsear stderr.
- REPL puede formatearlos lindos (línea del source + caret).
- Se puede seguir parseando tras un error y reportar varios juntos.
- Modo silencioso para benchmarks / pipes.

Reemplaza a las tres funciones actuales: `error()`, `parser_error()`, `interpreter_error()`.

---

## SourceFile como objeto central

```c
typedef struct {
  const char *path;
  const char *data;
  size_t length;

  size_t *line_starts;   // cache para mapear offset → line/col
  size_t line_count;
} SourceFile;

typedef struct { size_t line, col; } SourcePos;
typedef struct { const char *data; size_t length; } StringSlice;

SourceFile *source_load(const char *path);
SourceFile *source_from_string(const char *src, const char *label);
SourcePos   source_pos_at(SourceFile *s, size_t offset);
StringSlice source_line(SourceFile *s, size_t line);
void        source_free(SourceFile *s);
```

Los tokens guardan `Span` (offset + length), no `line` directo. La línea/columna se calcula desde el `SourceFile` cuando hace falta. Esto deja la puerta abierta a mensajes de error con contexto, multi-archivo (imports), y posición + columna para LSP eventual.

---

## Tokens: ya parseados, no slices

```c
typedef enum {
  TK_NUMBER, TK_STRING, TK_IDENT,
  TK_PLUS, TK_MINUS, TK_STAR, TK_SLASH,
  // ...
  TK_EOF,
} TokenKind;

typedef struct {
  TokenKind kind;
  Span span;            // para errores
  union {
    double number;            // ya parseado
    StringSlice ident;        // copia en arena
    StringSlice string;       // ya con escapes resueltos
  } as;
} Token;
```

**Importante:** `ident` y `string` apuntan a la arena, NO al source. Esto hace que el AST sea independiente del `SourceFile`.

---

## AST autocontenido

```c
typedef struct Expr Expr;
typedef struct Stmt Stmt;

typedef enum {
  EX_NUMBER, EX_BOOL, EX_STRING, EX_NIL,
  EX_VAR, EX_ASSIGN, EX_BINARY, EX_UNARY, EX_LOGICAL,
  EX_CALL, EX_GROUP,
} ExprKind;

struct Expr {
  ExprKind kind;
  Span span;            // para errores

  union {
    double number;
    bool boolean;
    StringSlice string;
    StringSlice var_name;

    struct { Expr *target; Expr *value; }       assign;
    struct { Expr *left; Expr *right; TokenKind op; } binary;
    struct { Expr *operand; TokenKind op; }     unary;
    struct { Expr *left; Expr *right; TokenKind op; } logical;
    struct { Expr *callee; Expr **args; size_t arg_count; } call;
    struct { Expr *inner; }                     group;
  } as;
};
```

**Diferencias con la versión actual:**

- `assign.target` es un `Expr*`, no un token. Permite `arr[0] = x`, `obj.f = x` mañana sin reescribir.
- Operadores guardados como `TokenKind`, no como puntero a token. AST autocontenido.
- `var_name` es `StringSlice` a la arena, no al source.
- Sin `expr->token` genérico — cada variante guarda solo lo que necesita.

---

## Values con header GC común

```c
typedef enum { OBJ_STRING, OBJ_FN, OBJ_ENV } ObjKind;

typedef struct Obj {
  ObjKind kind;
  bool marked;
  struct Obj *next;       // linked list global del GC
} Obj;

typedef enum { V_NIL, V_BOOL, V_NUMBER, V_OBJ } ValueKind;

typedef struct {
  ValueKind kind;
  union {
    bool boolean;
    double number;
    Obj *obj;
  } as;
} Value;

// Helpers
static inline bool is_string(Value v) {
  return v.kind == V_OBJ && v.as.obj->kind == OBJ_STRING;
}
// ... is_fn, as_string, as_fn, etc.

typedef struct {
  Obj obj;
  size_t length;
  char *data;
  uint32_t hash;           // pre-computado, sirve para interning / hash maps
} ObjString;

typedef struct ObjEnv ObjEnv;
typedef struct {
  Obj obj;
  StringSlice *params;     // apunta al AST (arena), no own
  size_t param_count;
  Stmt *body;              // apunta al AST, no own
  ObjEnv *closure;
} ObjFn;
```

**Decisiones:**

- Un solo `V_OBJ` en `Value`, en vez de `V_STRING`, `V_FN`, `V_ARRAY`, etc. Agregar tipos heap nuevos no toca `Value`.
- `hash` precomputado abre la puerta a hash maps y string interning sin reformar.
- Funciones referencian al AST por puntero, sin ser dueñas. Seguro porque el AST sobrevive al último GC.

---

## Garbage Collector

```c
typedef struct {
  Obj *all_objects;          // linked list
  size_t bytes_allocated;
  size_t next_gc_threshold;

  // Roots provistas por el interpreter
  ObjEnv *globals;
  ObjEnv **current_env;      // doble puntero: ve cambios al cambiar scope
  Value *return_slot;

  bool stress;               // collect en cada alloc, para testing
  bool log;                  // imprimir colecciones
} GC;

void  gc_init(GC *gc);
void *gc_alloc_obj(GC *gc, size_t size, ObjKind kind);
void  gc_collect(GC *gc);
void  gc_shutdown(GC *gc);
```

### Reglas

- **Roots:** env actual + globals + return_slot (si `returning`).
- **Trigger:** entre statements (NO en medio de `evaluate`). Esto evita el problema de temporaries en el stack de C que no son visibles para el GC.
- **`stress` mode:** corre GC antes de cada alloc. Destapa bugs de "olvidé marcar X como root" instantáneamente. Activar con `--gc-stress` durante desarrollo.
- **Algoritmo:** mark-and-sweep clásico. Refcount queda descartado por el problema de ciclos en closures.

### Por qué entre statements y no continuo

Si `evaluate` aloca un string para `left`, después aloca otro para `right`, y un GC corre en el medio, `left` solo está en el stack de C como `Value` local — no es alcanzable desde ninguna root. Se liberaría incorrectamente.

Soluciones posibles:
- (a) Solo collectar entre statements. **Elegida**, simple y suficiente.
- (b) Stack explícito de temporaries con push/pop. Más complicado, lo dejamos para el día que importe.

---

## Array dinámico genérico

```c
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
```

Reemplaza el patrón de capacity-doubling repetido (lexer.tokens, stmt_array, params, args, env entries). Las macros valen la pena acá: el patrón es idéntico literal cinco veces.

---

## Interpreter

```c
typedef struct {
  GC gc;
  Arena ast_arena;         // dueña del AST y los tokens
  Diagnostics diag;

  ObjEnv *env;             // env actual (puede cambiar con scopes)
  ObjEnv *globals;

  Value return_value;
  bool returning;
} Interpreter;

void interp_init(Interpreter *it);
void interp_run_source(Interpreter *it, SourceFile *src);
void interp_shutdown(Interpreter *it);
```

El `Interpreter` es dueño de la arena del AST. En modo REPL, cada línea agrega más al arena (no se libera entre líneas porque las funciones definidas referencian sus bodies). El arena se libera en `interp_shutdown`.

---

## Convenciones generales

### `UNREACHABLE()` en lugar de `default: break`

```c
#define UNREACHABLE() do { \
  fprintf(stderr, "unreachable at %s:%d\n", __FILE__, __LINE__); \
  abort(); \
} while (0)
```

Cuando se agrega una variante nueva a un enum, los `default: break` silenciosos esconden bugs. `UNREACHABLE()` te avisa. Los switches sobre enums deberían cubrir todos los casos explícitamente.

### Naming

- Tipos: `PascalCase` (`Token`, `ObjString`, `Interpreter`).
- Funciones: `snake_case` con prefijo del módulo (`lexer_advance`, `gc_collect`, `arena_alloc`).
- Constantes/macros: `UPPER_SNAKE`.
- Enums: prefijo corto (`TK_`, `EX_`, `OBJ_`, `V_`, `DIAG_`).

### Pragmas y headers

`#pragma once` en todos los headers. Forward declarations donde haya dependencias circulares (típicamente entre `ast.h` y `value.h`).

---

## Testing

### Estructura

```
tests/
  cases/
    basic.q
    strings.q
    closures.q
    recursion.q
    fizzbuzz.q
  expected/
    basic.txt
    strings.txt
    ...
  run.sh
```

### Runner mínimo (`run.sh`)

```bash
#!/bin/bash
set -e
fails=0
for f in tests/cases/*.q; do
  name=$(basename "$f" .q)
  expected="tests/expected/$name.txt"
  actual=$(./build/qlang "$f" 2>&1)
  if ! diff <(echo "$actual") "$expected" > /dev/null; then
    echo "FAIL: $name"
    diff <(echo "$actual") "$expected" | head -20
    fails=$((fails+1))
  fi
done
echo "$fails failures"
exit $fails
```

### Casos imprescindibles antes del GC

- Recursión que aloque muchos strings (estresa el GC).
- Closure que capture un env que después se libera (caso clásico de cierre con counter).
- Closure que tenga un ciclo (función que se referencia a sí misma a través del env). Refcount fallaría, mark-sweep no.
- Funciones recursivas profundas (estresa el call stack y el chaining de envs).

### Snapshots del AST

`./qlang --print-ast file.q` genera una representación textual estable. Guardarla como snapshot en `tests/ast_expected/`. Cambios al parser que rompan el snapshot son señales tempranas.

---

## Argumentos de CLI

Cosas que vale la pena tener:

- `--print-tokens` : dump de tokens.
- `--print-ast` : dump del AST.
- `--gc-stress` : GC en cada alloc.
- `--gc-log` : imprime cada colección.
- `--no-color` : para output a archivos.

No es prioritario, pero `--gc-stress` es muy útil cuando se está debuggeando GC.

---

## LANG.md (semánticas decididas)

Documento separado, corto, con decisiones explícitas:

- `let` re-declarar: ¿error o shadowing? (actual: error en mismo scope)
- Funciones: ¿hoisted? (actual: no, orden de ejecución)
- Strings: ¿mutables? (actual: no)
- `==` entre tipos distintos: ¿false o error? (actual: false)
- Bloques de `if`/`while`/`fn`: ¿crean scope nuevo? (actual: sí)
- Aritmética string + número: ¿coerción implícita? (actual: sí)
- División por cero: ¿error o `inf`/`nan`? (decidir)
- Truthiness: ¿qué es falsy? (actual: solo `nil` y `false`)

Cada decisión que se toma implícitamente es deuda. Mejor escrita.

---

## Orden de implementación sugerido (refactor del código actual)

Para no hacer un big-bang rewrite. Cada paso es testeable de forma aislada:

1. **`Diagnostics` system.** Reemplaza los `printf` de error. No toca arquitectura, mejora todo lo demás.
2. **Arena para AST.** Elimina `free_expr`/`free_stmt`. Local al parser y al cleanup.
3. **`Span` en tokens.** Cambio mecánico, abre la puerta a mejores errores.
4. **Refactor de Value con `Obj` header.** Prepara para GC sin agregarlo todavía.
5. **GC mark-and-sweep.** Ahora con todo en su lugar, es mucho más simple.
6. **Tests + CI.** Con todo lo anterior, los tests son confiables.

Cada paso debería poder hacerse en una rama, mergear con tests pasando, y seguir.
