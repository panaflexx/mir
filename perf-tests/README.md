# perf-tests — classy performance suite

Three high-throughput micro-benchmarks that exercise the code **c2m** generates
for the classy extensions (`class` + methods, `String`, `dict`).  Each test
centres on a class and drives a tight workload, then prints both a throughput
number and a deterministic checksum.

The checksum is the important part for compiler correctness: it must be
**identical** under `-ei` (interpreter), `-el` (lazy JIT), and `-eg` (full JIT),
and at every `-O` level.  Only the timing should change.

| Test | Class(es) | Feature focus |
|------|-----------|---------------|
| `perf-math.c` | `Complex` | stack class instances, `this`-mutating + value-returning methods, tight FP loops (Mandelbrot escape-time). ~30M method calls at default size. |
| `perf-strings.c` | `Record` | auto-cast `+` concatenation, class methods returning `String`, plus a 16-assert correctness section for `length()` / `find()` / `substr()` / `empty()` / `replace()` (incl. UTF-8 code-point semantics and `find` npos), then ~250k rendered + searched strings. |
| `perf-dict.c` | `Account` | dynamic bracket-key writes, `dict_object_get` lookups, `in` membership, `for (auto k in d)` iteration, `dict_object_count`, `json`. |

## Running

Run the whole suite across all three execution modes:

```sh
sh perf-tests/run-perf.sh ./c2m -O2
```

Or run a single test in a single mode:

```sh
./c2m -O2 perf-tests/perf-math.c    -eg   # fully generated (fastest)
./c2m -O2 perf-tests/perf-strings.c -el   # lazily generated
./c2m      perf-tests/perf-dict.c    -ei   # interpreted (baseline)
```

Each test returns `0` on success (checksums valid), non-zero on failure.

## Tuning the workload

Sizes are plain locals at the top of `main()` so they are easy to scale:

- `perf-math.c`: `W`, `H`, `MAXIT` (changing these changes the checksum; the
  `EXPECT` constant is for the default 700×460 / 256-iter grid).
- `perf-strings.c`: `N` (record count).
- `perf-dict.c`: `N` (entry count). Note the bundled `inc/dict.h` uses a flat
  array with a linear key scan, so build + lookup are O(n²) overall — raise `N`
  to probe how the dict runtime scales.

## Sample numbers

Reference run on this machine (`-O2`, single-threaded; absolute numbers depend
on hardware):

| Test | `-ei` | `-el` | `-eg` |
|------|------:|------:|------:|
| math (700×460, Mpixel/s) | ~0.26 | ~3.6 | ~3.7 |
| strings (250k, Mrecord/s) | — | ~1.2 | ~1.4 |
| dict (6k, build+lookup ms) | — | ~310 | ~257 |

The math test shows the ~14× speedup of generated code over the interpreter
while producing the same checksum, which is exactly what you want to see from a
working code generator.
