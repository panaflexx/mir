# Plan: Importing MadC MIR fixes & features into the ClassyC MIR fork

Date: 2026-08-13
Source: `ext/mir-madc` (derekbsnider/mir, frozen @ `eda520ab`, tag `v1.0-madc.0.76.0`)
Target: `ext/mir` (panaflexx/mir, classyc backend @ `c0017607`)
Merge base: `99c65079` (upstream vnmakarov/mir, 2024-08-29)

## Background & constraints

- The MadC standalone repo is **frozen**; its live successor is
  `github.com/derekbsnider/madc` → `third_party/mir`. This plan treats the local
  `ext/mir-madc` snapshot as final. A local branch `madc` in `ext/mir` points at
  MadC's master for cherry-picking (`git -C ext/mir log master..madc`).
- ClassyC's frontend is `src/classyc.c`, a 36k-line fork of `c2mir.c`. Every
  MadC fix that touches `c2mir/c2mir.c` must be **ported by hand into
  `classyc.c`** (function names largely survive the fork) and applied to
  `ext/mir/c2mir/c2mir.c` only to keep the bundled `c2m` correct.
- ClassyC-only backend features that MadC lacks and that must be preserved
  through every merge: MIR atomics (`MIR_ALOAD..MIR_ACAS`, `mir-gen-atomic.c`),
  TLS (`mir-gen-tls.c`, x86_64+arm64), aarch64 struct-ABI/call fixes, Mach-O
  `S_REGULAR` fix, interp break/continue hooks, loop-phi fix, `mir-dbinfo.h`,
  the `mir-debug.h`/GDB-JIT import (`c0017607`).
- MadC deliberately **removed the `class` keyword** from its c2mir
  (`e0f94eef`) — never merge its c2mir wholesale; cherry-pick only.

## Validation harness (run after every phase)

1. Build: cmake rebuild of `classyc`, `classyc-lsp`, `b2obj` (root `CMakeLists.txt`,
   links `mir_static` from `ext/mir`).
2. MIR's own tests in `ext/mir` (c-tests; at minimum the ones named below per commit).
3. `cy-validate/run-validate.sh` (language conformance).
4. `cy-testhard/` suite + `examples/` smoke (JIT mode).
5. AOT path: `b2obj` round-trip on at least one example; run `bin/classyc.aot`.
6. gdb smoke test of the JIT debug path (mir-debug import regression check).

Commit per phase (or per cherry-pick group) so regressions can be bisected.

---

## Progress log

| Date | Phase | Status | Commit / notes |
|---|---|---|---|
| 2026-08-13 | 1 | **DONE** | `8b0eb6cd` on `madc-import` — merge `vnmakarov/master` @ `a8ab7c31` |
| | | validation | build classyc/lsp/b2obj/c2m OK; 10/10 new MIR c-tests PASS; cy-validate 58/58 PASS; JIT example + b2obj AOT smoke OK |
| | | conflicts | `mir-gen.c` (kept ClassyC atomics/TLS/dbinfo + upstream opts); `mir.c` (kept TLS/isolation + code reserve); `c2mir/c2mir.c` (kept ClassyC-extended c2mir — already had MIR #448/#449/#451/#452/#459 backports with `TM_CLASS`) |
| | | note | Working-tree `mir.c` was truncated mid-merge (lost first ~945 lines); rebuilt via clean 3-way. Arch backends (aarch64/x86_64/ppc/riscv/s390x) already carried the upstream ABI fixes in ClassyC HEAD. |
| 2026-08-13 | 2 | **DONE** | cherry-picks on `madc-import` (see below) |
| | | applied | `ebf825f7` → `037154c8`; `5df536f6` → `85bf6a39`; `d3a5cced` → `948bc73a`; `7a909601` → `58cd3f91` + ClassyC adapt `2e018fb0` |
| | | skipped | `c40ed469` (empty — already covered by Phase 1 `ded4b82c`/`940eeacf`); `8864a739` (all 5 fixes already present) |
| | | classyc.c | ported `MIR_add_alias_conflict` registration (`get_type_alias` / `add_union_member_conflicts` / `union_alias_done`) — uncommitted in parent until parent commit |
| | | validation | build OK; cy-validate 58/58 PASS; struct-ret-multi-return + va-struct-args + issue456/458 PASS; JIT + b2obj smoke OK |
| 2026-08-13 | 3 | **DONE** | frontend ports into c2mir + classyc.c |
| | | done | all Phase 3 commits including `e2c0ae95`+`731c2234` unprototyped call ABI |
| | | note | `bde8658d` superseded by `9c7e7f3b`; unprototyped uses VARARG proto with fixed actual args |
| | | validation | build OK; cy-validate 58/58; unproto add3 + undeclared printf PASS; JIT smoke OK |

---

## Phase 1 — Merge upstream `vnmakarov/master` (48 commits)

**Status: DONE** (`8b0eb6cd`, branch `madc-import`).

Already fetched in `ext/mir` as `vnmakarov/master` @ `a8ab7c31` (2026-06-19).
Contains the upstream-merged versions of most MadC/Cyan fixes — the cheapest
third of the total value.

Highlights:

- gen: GVN sign/zero-extension loss on store→narrower reload (`29264593`);
  `VA_BLOCK_ARG` fixed-place vs GVN CSE (`7f7a3cae`); out-of-SSA lost copy on
  self-loop blocks (`c42fd238`); `LADDR` output operand (`54a82231`); simplified
  RA for indirect jumps (`a6db87d4`); laddr/lref labels alive in jump_opt
  (`d896c669`); re-enable accidentally-disabled inlining (`5675bee7`); aarch64
  jcall callee-saved reg (`a8ab7c31`).
- ABI/varargs: x86-64 `va_start` offsets with block args (`ded4b82c`); aarch64
  bb-thunk x9 clobber (`7ce608e9`); aarch64 by-ref block-arg arg-reg clobber
  (`126fdd9b` + riscv64/s390x/ppc64 `e9c9576e`); aarch64 long-double stack
  alignment (`750b7201`); mixed-class struct varargs runtime helpers
  (`940eeacf`).
- c2mir: stmt-expr value bugs (`140dc852`); zero-length-array member offset
  (`9ec66fe2`); struct/union va_arg rvalue storage (`bbe38856`); static-init
  out-of-line pre-emit (`dc421268`); string-literal const addr (`ba58f041`);
  deterministic long-double output (`37de60ea`); uninit narrow-local extend
  (`2a157cc2`); stmt-expr scope reservation (`807c640b`).
- infra: contiguous code-holder reservation (`9f8dbce7`), musl support
  (`d867ffb4`, `1bb2fe6b`).

Expected conflicts: `mir-gen.c` (our atomics/TLS/dbinfo hooks vs their
optimizer fixes — textual, resolvable), `mir.c` (same), `mir.h` (our opcode
enum tail vs their API additions — keep both). `c2mir/c2mir.c` conflicts don't
matter for classyc but keep `c2m` building.

> ⚠️ Upstream's `2a157cc2` *restricted* the `force_val` narrow-extend to
> address-taken vars; MadC found this regresses gcc-torture `pr34099-2` and
> re-broadened it (`bde8658d`, Phase 3). Take both, in that order.

## Phase 2 — Backend (mir/mir-gen) cherry-picks from `madc`

**Status: DONE** on `madc-import`.

| Commit | What | Result |
|---|---|---|
| `ebf825f7` | vararg functions with no named parameter | **applied** `037154c8` |
| `5df536f6` | x86-64: skip code-holder reservation (rel32 ±2 GB) | **applied** `85bf6a39` (composes with Phase 1 reserve: `#if !x86_64`) |
| `c40ed469` | SysV varargs RSA boundary + mixed-class va_arg | **skipped** (empty after resolve; Phase 1 `ded4b82c` prologue-state va_start + `940eeacf` fp_offset+=16 already present) |
| `d3a5cced` | canonical result regs for multi-RET (struct-by-value) | **applied** `948bc73a` + test `struct-ret-multi-return.c` |
| `7a909601` | union access classes must conflict with member classes | **applied** `58cd3f91`; ClassyC adapt `2e018fb0` (TM_CLASS not TM_VECTOR); **ported to `src/classyc.c`** |
| `8864a739` (residue) | 5 community fixes | **skipped** — all present: `MAX_INSN_RELOAD_MEM_OPS 4`, addr_regs rebuild, lref jump_opt, remove_item NULL-guards, RET-count `cf_nres` save |

Skip (verified already present in our fork): `MAX_INSN_RELOAD_MEM_OPS` bump,
addr_regs rebuild, jump_opt lref labels, remove_item guards.

## Phase 3 — Frontend bug-fix ports (c2mir.c → classyc.c)

**Status: DONE.**

For each: cherry-pick into `ext/mir/c2mir/c2mir.c` **and** hand-port the same
change into `src/classyc.c` (locate by function name; the fork preserves most).

| Commit | What |
|---|---|
| `bde8658d` | force_val extends ALL narrow int reg values (pr34099-2) — **after** Phase 1 |
| `9c7e7f3b` | extend uninitialized narrow auto local at birth; restore addr_p read gate |
| `e2c0ae95` + `731c2234` | unprototyped call is not variadic; keep vararg flag, fix args |
| `3582b48e` | empty-struct call results reserve a real call-arg slot |
| `8a6a6c57` | `&array` lvalue types as pointer-to-array, not decayed pointer |
| `8f3934ac` | zero-length arrays: gcc/clang parity, warn under `-pedantic` |
| `01f999bb` | lay out auto-locals by actual scope depth, not `scope->uid` |
| `1fdf44d8` | honor member-level `_Alignas`/`aligned(N)` in struct layout |

Validation per port: the MadC commit's added c-test (most carry one) plus a
ClassyC equivalent under `cy-validate/`.

## Phase 4 — `mir-debug.c` + AOT object layer (decision point, then import)

We imported `mir-debug.h` + `mir-debug-gdb.c` (`c0017607`) but **not**
`mir-debug.c` (~1,400 lines: DWARF builder *and* the `MIR_object_*` AOT layer).

Step 4a — **Evaluate against `b2obj`/`b2objmac`** first. MadC's layer provides:
ELF `.o` capture (x86-64 `187e41c6`, aarch64 PIC addrpool `e2a43ee5`),
direct ET_EXEC/ET_DYN/**PIE** emission with no external toolchain
(`ef89761d`, `5b8b6f34`, `798e329b`), in-process ET_REL loader as JIT cache
(`96c13c98`), multi-object linking (`94b2b259`), `.init_array` (`99f23921`),
weak/linkonce (`314b6d94`), full RELRO + non-exec stack (`60384999`),
`DT_DEBUG` (`49e840e6`), PIC addrpool killing DT_TEXTREL (`0c2450db`),
DWARF in AOT artifacts + multi-CU merge (`5584f2d3`, `a6cdb992`, `704be488`).

Step 4b — If adopted: import `mir-debug.c` + the gen capture-mode hooks as one
cohesive workstream (the commits above, in MadC chronological order), plus
`.debug_frame` CFI for unwinding out of JIT frames (`39963953`). Reconcile with
our `mir-dbinfo.h` / `dwarf-gen.h` — two parallel debug-info implementations;
pick one writer per artifact (suggestion: keep ours for `.bmir`, use MadC's for
ELF objects) or consolidate.

Step 4c — If rejected: still take `39963953` if feasible, and mine
`60384999`/`49e840e6`/`0c2450db` ideas for `b2obj` instead.

## Phase 5 — Big features (optional, each its own branch)

Priority order suggested by ClassyC's README gap list:

1. **`__attribute__((cleanup))`** (`f53a46e2`..`53cdb85f`, 8 commits) — pure
   c2mir→classyc.c port; complements our `defer`; gcc-differential tested.
2. **scalar `__int128`** (`1ee0961a` + `adc55808`; skip Apple-only `b219e411`)
   — c2mir + gen lowering + SysV ABI.
3. **`__builtin_{add,sub,mul}_overflow`** (`545ad469`) — self-contained.
4. **`_Complex`** (~25 commits, `a0d899a6`..`4573a0f3`) — closes the documented
   "C11 w/o complex" gap; biggest frontend port of the set.
5. **GCC/clang vector extensions** (~50 commits, `62577800`..`2ffebff0`) —
   memory-backed, lowered in c2mir (no new MIR opcodes), includes a GCC vector
   torture suite (`c69f4da6`). Large; only if SIMD is wanted in ClassyC.
6. Misc builtins, one commit each: `bcdd0c52` (optimize attr), `fbe5efb4`
   (memcmp), `fbb47f31` (abort), `59117d8a` (fp classification), `48cd7beb`
   (empty asm barriers).

## Explicitly excluded

- Mach-O writer / `x86_64-macos` target / cross-target selection
  (`e760b5fb`, `77c1e056`, `9c1fd55f`, `8ddc74c6`, `f5f6273f`, `94e1b60b`,
  `fde19e17`, `9f0d0e89`) — revisit only if we replace `b2objmac`.
- MadC external-AST API (`128c134a`, `4ad894d5`, `acab090e`, `6d66dadf`,
  `6518cfd7`, `42471dbd`, `062dd977`, `9d573993`, `824c7c86`,
  `4ad333dc`, `34ab8852`) — madc-compiler-specific.
- `e0f94eef` (disables `class` keyword) — antithetical to ClassyC.
- MadC c2mir `-g` capture (`f789567d`, `34e96b31`, `f2cf3b9a`, `49442aaa`) —
  our gdb path goes through classyc.c; port later only if we want `-g` there.

## Reverse direction (do when convenient)

Upstream our atomics + TLS to `vnmakarov/mir` as PRs (MadC's repo documents the
PR-transport workflow; upstream merged ~10 such PRs in June 2026). Landing them
upstream converges all three trees and shrinks our permanent delta.

## Rollback / safety

- One git commit per cherry-pick/port; never squash phases.
- Full validation harness after each phase; bisect within a phase on failure.
- Keep `ext/mir-madc` untouched as reference; the `madc` branch in `ext/mir`
  can be deleted after the final phase (`git branch -D madc`).
