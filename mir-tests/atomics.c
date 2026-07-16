/* Smoke test for MIR atomic opcodes (interpreter + generator). */
#include "../mir.h"
#include "../mir-gen.h"
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

static int64_t counter;
static int64_t flag;

static MIR_item_t create_atomic_add_func (MIR_context_t ctx, MIR_item_t counter_imp) {
  MIR_item_t func;
  MIR_reg_t t, p;
  MIR_type_t res = MIR_T_I64;

  func = MIR_new_func (ctx, "atomic_add1", 1, &res, 0);
  t = MIR_new_func_reg (ctx, func->u.func, MIR_T_I64, "t");
  p = MIR_new_func_reg (ctx, func->u.func, MIR_T_I64, "p");
  MIR_append_insn (ctx, func,
                   MIR_new_insn (ctx, MIR_MOV, MIR_new_reg_op (ctx, p),
                                 MIR_new_ref_op (ctx, counter_imp)));
  MIR_append_insn (ctx, func,
                   MIR_new_insn (ctx, MIR_AADD, MIR_new_reg_op (ctx, t),
                                 MIR_new_mem_op (ctx, MIR_T_I64, 0, p, 0, 1),
                                 MIR_new_int_op (ctx, 1)));
  MIR_append_insn (ctx, func, MIR_new_ret_insn (ctx, 1, MIR_new_reg_op (ctx, t)));
  MIR_finish_func (ctx);
  return func;
}

static MIR_item_t create_atomic_cas_func (MIR_context_t ctx, MIR_item_t flag_imp) {
  MIR_item_t func;
  MIR_reg_t old, exp, des, p;
  MIR_type_t res = MIR_T_I64;
  MIR_var_t args[2];

  args[0].name = "expected";
  args[0].type = MIR_T_I64;
  args[1].name = "desired";
  args[1].type = MIR_T_I64;
  func = MIR_new_func_arr (ctx, "atomic_cas_set", 1, &res, 2, args);
  old = MIR_new_func_reg (ctx, func->u.func, MIR_T_I64, "old");
  p = MIR_new_func_reg (ctx, func->u.func, MIR_T_I64, "p");
  exp = MIR_reg (ctx, "expected", func->u.func);
  des = MIR_reg (ctx, "desired", func->u.func);
  MIR_append_insn (ctx, func,
                   MIR_new_insn (ctx, MIR_MOV, MIR_new_reg_op (ctx, p),
                                 MIR_new_ref_op (ctx, flag_imp)));
  MIR_append_insn (ctx, func,
                   MIR_new_insn (ctx, MIR_ACAS, MIR_new_reg_op (ctx, old),
                                 MIR_new_mem_op (ctx, MIR_T_I64, 0, p, 0, 1),
                                 MIR_new_reg_op (ctx, exp), MIR_new_reg_op (ctx, des)));
  MIR_append_insn (ctx, func, MIR_new_ret_insn (ctx, 1, MIR_new_reg_op (ctx, old)));
  MIR_finish_func (ctx);
  return func;
}

static MIR_item_t create_atomic_load_func (MIR_context_t ctx, MIR_item_t counter_imp) {
  MIR_item_t func;
  MIR_reg_t t, p;
  MIR_type_t res = MIR_T_I64;

  func = MIR_new_func (ctx, "atomic_load_counter", 1, &res, 0);
  t = MIR_new_func_reg (ctx, func->u.func, MIR_T_I64, "t");
  p = MIR_new_func_reg (ctx, func->u.func, MIR_T_I64, "p");
  MIR_append_insn (ctx, func,
                   MIR_new_insn (ctx, MIR_MOV, MIR_new_reg_op (ctx, p),
                                 MIR_new_ref_op (ctx, counter_imp)));
  MIR_append_insn (ctx, func,
                   MIR_new_insn (ctx, MIR_ALOAD, MIR_new_reg_op (ctx, t),
                                 MIR_new_mem_op (ctx, MIR_T_I64, 0, p, 0, 1)));
  MIR_append_insn (ctx, func, MIR_new_ret_insn (ctx, 1, MIR_new_reg_op (ctx, t)));
  MIR_finish_func (ctx);
  return func;
}

typedef int64_t (*i64_fun_t) (void);
typedef int64_t (*i64_fun2_t) (int64_t, int64_t);

static void run_suite (const char *mode, i64_fun_t add1, i64_fun_t loadc, i64_fun2_t casf) {
  int64_t old, v;

  counter = 10;
  old = add1 ();
  assert (old == 10);
  assert (counter == 11);
  old = add1 ();
  assert (old == 11);
  assert (counter == 12);
  v = loadc ();
  assert (v == 12);

  flag = 0;
  old = casf (0, 42);
  assert (old == 0);
  assert (flag == 42);
  old = casf (0, 99);
  assert (old == 42);
  assert (flag == 42);
  old = casf (42, 7);
  assert (old == 42);
  assert (flag == 7);

  printf ("atomics %s: OK\n", mode);
}

static void build_module (MIR_context_t ctx, MIR_item_t *add_item, MIR_item_t *load_item,
                          MIR_item_t *cas_item) {
  MIR_module_t m;
  MIR_item_t counter_imp, flag_imp;

  m = MIR_new_module (ctx, "atomics_mod");
  counter_imp = MIR_new_import (ctx, "counter");
  flag_imp = MIR_new_import (ctx, "flag");
  *add_item = create_atomic_add_func (ctx, counter_imp);
  *load_item = create_atomic_load_func (ctx, counter_imp);
  *cas_item = create_atomic_cas_func (ctx, flag_imp);
  MIR_finish_module (ctx);
  MIR_load_module (ctx, m);
  MIR_load_external (ctx, "counter", &counter);
  MIR_load_external (ctx, "flag", &flag);
}

int main (void) {
  MIR_context_t ctx;
  MIR_item_t add_item, load_item, cas_item;
  MIR_val_t res;
  i64_fun_t add1, loadc;
  i64_fun2_t casf;

  /* Interpreter (own context — gen asserts data == NULL) */
  ctx = MIR_init ();
  build_module (ctx, &add_item, &load_item, &cas_item);
  MIR_link (ctx, MIR_set_interp_interface, NULL);
  counter = 10;
  MIR_interp (ctx, add_item, &res, 0);
  assert (res.i == 10 && counter == 11);
  MIR_interp (ctx, add_item, &res, 0);
  assert (res.i == 11 && counter == 12);
  MIR_interp (ctx, load_item, &res, 0);
  assert (res.i == 12);
  {
    MIR_val_t args[2], cres;
    args[0].i = 0;
    args[1].i = 42;
    flag = 0;
    MIR_interp_arr (ctx, cas_item, &cres, 2, args);
    assert (cres.i == 0 && flag == 42);
    args[0].i = 0;
    args[1].i = 99;
    MIR_interp_arr (ctx, cas_item, &cres, 2, args);
    assert (cres.i == 42 && flag == 42);
  }
  printf ("atomics interp: OK\n");
  MIR_finish (ctx);

  /* Generator / JIT at -O2 (barriers matter here) */
  ctx = MIR_init ();
  build_module (ctx, &add_item, &load_item, &cas_item);
  MIR_gen_init (ctx);
  MIR_gen_set_optimize_level (ctx, 2);
  MIR_link (ctx, MIR_set_gen_interface, NULL);
  add1 = (i64_fun_t) MIR_gen (ctx, add_item);
  loadc = (i64_fun_t) MIR_gen (ctx, load_item);
  casf = (i64_fun2_t) MIR_gen (ctx, cas_item);
  run_suite ("gen-O2", add1, loadc, casf);
  MIR_gen_finish (ctx);
  MIR_finish (ctx);
  return 0;
}
