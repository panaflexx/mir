# classy-fuzz — Stress Tests for dict, class, and String Support

This directory contains test files designed to validate and stress the `dict`,
`class`, and `String` extensions in c2mir.  Each file exercises edge cases,
incorrect usage, and potential compiler bugs.

## Test files

| File | Feature | Purpose |
|------|---------|---------|
| `fuzz-dict-valid.c` | dict | Valid edge-case usage that should compile and run |
| `fuzz-dict-bad.c` | dict | Intentionally bad / ambiguous dict code (expect errors) |
| `fuzz-class-valid.c` | class | Valid edge-case class usage |
| `fuzz-class-bad.c` | class | Intentionally bad class code (expect errors) |
| `fuzz-string-valid.c` | String | Valid edge-case String usage |
| `fuzz-string-bad.c` | String | Intentionally bad String code (expect errors) |
| `fuzz-mixed.c` | all | Interactions between dict, class, and String |
| `fuzz-ctor.c` | constructors / `new` | Dedicated tests for `new Class(...)`, implicit `this`, named args, fluent chaining, defaults, heap objects in dicts/loops/arrays, etc. |
| `fuzz-defer.c` | `defer` / destructors / `delete` | Stress test: allocates 1000 objects per run and cleans them with `defer delete` (loop-scoped) and a single bulk `defer` block, repeated 100x (200k ctor/dtor pairs). Verifies ctor/dtor balance, no leaks, and correct results at scale. |

## Running

### Run a single valid test (should exit 0):
```sh
./c2m classy-fuzz/fuzz-dict-valid.c -eg
./c2m classy-fuzz/fuzz-class-valid.c -eg
./c2m classy-fuzz/fuzz-string-valid.c -eg
./c2m classy-fuzz/fuzz-mixed.c -eg
```

### Run a bad test (expect compile errors, non-zero exit):
```sh
./c2m classy-fuzz/fuzz-dict-bad.c -eg 2>&1 | head -40
./c2m classy-fuzz/fuzz-class-bad.c -eg 2>&1 | head -40
./c2m classy-fuzz/fuzz-string-bad.c -eg 2>&1 | head -40
```

### Run all tests via the runner script:
```sh
sh classy-fuzz/run-tests.sh ./c2m
```

## Bugs Found

### 1. Class layout bug with many members (fuzz-class-valid.c, test 5)
Classes with 8+ int members plus a String member have incorrect memory layout.
Values assigned to members read back as wrong values.  Likely an alignment or
offset calculation issue in `set_class_layout()`.

### 2. Compiler SEGFAULT on nonexistent class member (fuzz-class-bad.c, B6)
Accessing a nonexistent member on a class instance (`s.nonexistent`) correctly
produces an error message, but then the compiler crashes with a segmentation
fault while continuing error recovery.

### 3. String in ternary not supported
`String s = flag ? "yes" : "no";` produces "incompatible types in assignment
to an string type lvalue".

### 4. Class String members incompatible with prototyped functions
When `strlen`/`strcmp` are properly declared (via `#include <string.h>`),
passing a class String member like `strlen(obj.name)` fails with
"incompatible argument type for pointer type parameter".  Without the
include (implicit declarations), it works fine.  This suggests the String
member access produces a type that doesn't match `const char*`.

### 5. Class arrays not supported
`Point pts[3];` followed by `pts[0].x = 1;` produces "subscripted value is
neither array nor pointer".  Class instances cannot be used in arrays.

### 6. Passing class by value to functions fails
Calling a function that takes a class parameter by value produces
"incompatible argument type for pointer type parameter".

### 7. Dict reassignment with initializer — FIXED
`dict d = { "x": 1 }; d = { "y": 2 };` now works.  The parser accepts a brace
dict-literal on the RHS of an assignment (`assign_expr`), the checker validates
the value expressions and requires the LHS to be a dict, and the generator
builds a fresh dict object, populates it from the initializer list, and stores
the pointer into the LHS.  Nested objects and string/int values are supported.
Assigning a brace literal to a non-dict lvalue is now a clean error.
