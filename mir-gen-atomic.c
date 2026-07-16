/* Shared MIR atomic builtins and machinize lowering.
   Included from mir-gen.c after gen_* forward decls.  See CLASSY-ATOMICS.md. */

#include <stdint.h>

static size_t atomic_type_size (MIR_type_t t) {
  switch (t) {
  case MIR_T_I8:
  case MIR_T_U8: return 1;
  case MIR_T_I16:
  case MIR_T_U16: return 2;
  case MIR_T_I32:
  case MIR_T_U32: return 4;
  case MIR_T_I64:
  case MIR_T_U64:
  case MIR_T_P: return sizeof (void *);
  default: return 8;
  }
}

/* Host seq_cst helpers used as MIR builtins after machinize. */

static uint64_t mir_atomic_load (void *p, uint64_t size) {
  switch ((int) size) {
  case 1: return (uint64_t) __atomic_load_n ((uint8_t *) p, __ATOMIC_SEQ_CST);
  case 2: return (uint64_t) __atomic_load_n ((uint16_t *) p, __ATOMIC_SEQ_CST);
  case 4: return (uint64_t) __atomic_load_n ((uint32_t *) p, __ATOMIC_SEQ_CST);
  default: return __atomic_load_n ((uint64_t *) p, __ATOMIC_SEQ_CST);
  }
}

static void mir_atomic_store (void *p, uint64_t v, uint64_t size) {
  switch ((int) size) {
  case 1: __atomic_store_n ((uint8_t *) p, (uint8_t) v, __ATOMIC_SEQ_CST); break;
  case 2: __atomic_store_n ((uint16_t *) p, (uint16_t) v, __ATOMIC_SEQ_CST); break;
  case 4: __atomic_store_n ((uint32_t *) p, (uint32_t) v, __ATOMIC_SEQ_CST); break;
  default: __atomic_store_n ((uint64_t *) p, v, __ATOMIC_SEQ_CST); break;
  }
}

static uint64_t mir_atomic_xchg (void *p, uint64_t v, uint64_t size) {
  switch ((int) size) {
  case 1: return (uint64_t) __atomic_exchange_n ((uint8_t *) p, (uint8_t) v, __ATOMIC_SEQ_CST);
  case 2: return (uint64_t) __atomic_exchange_n ((uint16_t *) p, (uint16_t) v, __ATOMIC_SEQ_CST);
  case 4: return (uint64_t) __atomic_exchange_n ((uint32_t *) p, (uint32_t) v, __ATOMIC_SEQ_CST);
  default: return __atomic_exchange_n ((uint64_t *) p, v, __ATOMIC_SEQ_CST);
  }
}

static uint64_t mir_atomic_fetch_add (void *p, uint64_t v, uint64_t size) {
  switch ((int) size) {
  case 1: return (uint64_t) __atomic_fetch_add ((uint8_t *) p, (uint8_t) v, __ATOMIC_SEQ_CST);
  case 2: return (uint64_t) __atomic_fetch_add ((uint16_t *) p, (uint16_t) v, __ATOMIC_SEQ_CST);
  case 4: return (uint64_t) __atomic_fetch_add ((uint32_t *) p, (uint32_t) v, __ATOMIC_SEQ_CST);
  default: return __atomic_fetch_add ((uint64_t *) p, v, __ATOMIC_SEQ_CST);
  }
}

static uint64_t mir_atomic_fetch_sub (void *p, uint64_t v, uint64_t size) {
  switch ((int) size) {
  case 1: return (uint64_t) __atomic_fetch_sub ((uint8_t *) p, (uint8_t) v, __ATOMIC_SEQ_CST);
  case 2: return (uint64_t) __atomic_fetch_sub ((uint16_t *) p, (uint16_t) v, __ATOMIC_SEQ_CST);
  case 4: return (uint64_t) __atomic_fetch_sub ((uint32_t *) p, (uint32_t) v, __ATOMIC_SEQ_CST);
  default: return __atomic_fetch_sub ((uint64_t *) p, v, __ATOMIC_SEQ_CST);
  }
}

static uint64_t mir_atomic_fetch_and (void *p, uint64_t v, uint64_t size) {
  switch ((int) size) {
  case 1: return (uint64_t) __atomic_fetch_and ((uint8_t *) p, (uint8_t) v, __ATOMIC_SEQ_CST);
  case 2: return (uint64_t) __atomic_fetch_and ((uint16_t *) p, (uint16_t) v, __ATOMIC_SEQ_CST);
  case 4: return (uint64_t) __atomic_fetch_and ((uint32_t *) p, (uint32_t) v, __ATOMIC_SEQ_CST);
  default: return __atomic_fetch_and ((uint64_t *) p, v, __ATOMIC_SEQ_CST);
  }
}

static uint64_t mir_atomic_fetch_or (void *p, uint64_t v, uint64_t size) {
  switch ((int) size) {
  case 1: return (uint64_t) __atomic_fetch_or ((uint8_t *) p, (uint8_t) v, __ATOMIC_SEQ_CST);
  case 2: return (uint64_t) __atomic_fetch_or ((uint16_t *) p, (uint16_t) v, __ATOMIC_SEQ_CST);
  case 4: return (uint64_t) __atomic_fetch_or ((uint32_t *) p, (uint32_t) v, __ATOMIC_SEQ_CST);
  default: return __atomic_fetch_or ((uint64_t *) p, v, __ATOMIC_SEQ_CST);
  }
}

static uint64_t mir_atomic_fetch_xor (void *p, uint64_t v, uint64_t size) {
  switch ((int) size) {
  case 1: return (uint64_t) __atomic_fetch_xor ((uint8_t *) p, (uint8_t) v, __ATOMIC_SEQ_CST);
  case 2: return (uint64_t) __atomic_fetch_xor ((uint16_t *) p, (uint16_t) v, __ATOMIC_SEQ_CST);
  case 4: return (uint64_t) __atomic_fetch_xor ((uint32_t *) p, (uint32_t) v, __ATOMIC_SEQ_CST);
  default: return __atomic_fetch_xor ((uint64_t *) p, v, __ATOMIC_SEQ_CST);
  }
}

/* Returns previous *p.  Strong CAS, seq_cst. */
static uint64_t mir_atomic_cas (void *p, uint64_t expected, uint64_t desired, uint64_t size) {
  switch ((int) size) {
  case 1: {
    uint8_t e = (uint8_t) expected;
    __atomic_compare_exchange_n ((uint8_t *) p, &e, (uint8_t) desired, 0, __ATOMIC_SEQ_CST,
                                 __ATOMIC_SEQ_CST);
    return (uint64_t) e;
  }
  case 2: {
    uint16_t e = (uint16_t) expected;
    __atomic_compare_exchange_n ((uint16_t *) p, &e, (uint16_t) desired, 0, __ATOMIC_SEQ_CST,
                                 __ATOMIC_SEQ_CST);
    return (uint64_t) e;
  }
  case 4: {
    uint32_t e = (uint32_t) expected;
    __atomic_compare_exchange_n ((uint32_t *) p, &e, (uint32_t) desired, 0, __ATOMIC_SEQ_CST,
                                 __ATOMIC_SEQ_CST);
    return (uint64_t) e;
  }
  default: {
    uint64_t e = expected;
    __atomic_compare_exchange_n ((uint64_t *) p, &e, desired, 0, __ATOMIC_SEQ_CST,
                                 __ATOMIC_SEQ_CST);
    return e;
  }
  }
}

static void mir_atomic_fence (void) { __atomic_thread_fence (__ATOMIC_SEQ_CST); }

/* Expand one atomic MIR insn into a builtin CALL (and delete the original).
   Returns the first inserted insn (so the machinize loop re-scans the CALL),
   or NULL if insn is not atomic.  Same pattern as UI2F / long-double builtins. */
static MIR_insn_t machinize_atomic_insn (gen_ctx_t gen_ctx, MIR_insn_t insn) {
  MIR_context_t ctx = gen_ctx->ctx;
  MIR_func_t func = curr_func_item->u.func;
  MIR_insn_code_t code = insn->code;
  MIR_item_t proto_item, func_import_item;
  MIR_op_t freg_op, ops[8];
  MIR_op_t mem_op, ptr_op, size_op, res_op, val_op, exp_op, des_op;
  MIR_insn_t new_insn, first_insn;
  MIR_type_t res_type = MIR_T_I64;
  size_t size, ncall;
  MIR_type_t mtype;
  const char *fname, *pname;

  if (!MIR_atomic_code_p (code)) return NULL;

#if defined(__x86_64__) || defined(_M_AMD64)
  /* Native patterns in mir-gen-x86_64.c for ALOAD/ASTORE/AADD/ASUB/AXCHG/AFENCE.
     Keep those as MIR atomics so we do not CALL out of minicoro fiber stacks.
     CAS and bitops still lower to host builtins below. */
  if (code == MIR_ALOAD || code == MIR_ASTORE || code == MIR_AADD || code == MIR_ASUB
      || code == MIR_AXCHG || code == MIR_AFENCE)
    return NULL;
#endif

  freg_op = _MIR_new_var_op (ctx, gen_new_temp_reg (gen_ctx, MIR_T_I64, func));

  if (code == MIR_AFENCE) {
    pname = "mir.atomic_fence.p";
    fname = "mir.atomic_fence";
    proto_item = _MIR_builtin_proto (ctx, curr_func_item->module, pname, 0, NULL, 0);
    func_import_item
      = _MIR_builtin_func (ctx, curr_func_item->module, fname, (void *) mir_atomic_fence);
    first_insn = new_insn
      = MIR_new_insn (ctx, MIR_MOV, freg_op, MIR_new_ref_op (ctx, func_import_item));
    gen_add_insn_before (gen_ctx, insn, new_insn);
    ops[0] = MIR_new_ref_op (ctx, proto_item);
    ops[1] = freg_op;
    new_insn = MIR_new_insn_arr (ctx, MIR_CALL, 2, ops);
    gen_add_insn_before (gen_ctx, insn, new_insn);
    gen_delete_insn (gen_ctx, insn);
    return first_insn;
  }

  /* Locate mem operand and width. */
  if (code == MIR_ASTORE) {
    mem_op = insn->ops[0];
    val_op = insn->ops[1];
  } else {
    mem_op = insn->ops[1];
    if (code != MIR_ALOAD) val_op = insn->ops[2];
  }
  gen_assert (mem_op.mode == MIR_OP_VAR_MEM);
  mtype = mem_op.u.var_mem.type;
  size = atomic_type_size (mtype);
  gen_assert (mem_op.u.var_mem.index == MIR_NON_VAR || mem_op.u.var_mem.index == 0);
  gen_assert (mem_op.u.var_mem.disp == 0);
  ptr_op = _MIR_new_var_op (ctx, mem_op.u.var_mem.base);
  size_op = MIR_new_int_op (ctx, (int64_t) size);

  if (code == MIR_ALOAD) {
    res_op = insn->ops[0];
    pname = "mir.atomic_load.p";
    fname = "mir.atomic_load";
    proto_item
      = _MIR_builtin_proto (ctx, curr_func_item->module, pname, 1, &res_type, 2, MIR_T_I64, "p",
                            MIR_T_I64, "sz");
    func_import_item
      = _MIR_builtin_func (ctx, curr_func_item->module, fname, (void *) mir_atomic_load);
    first_insn = new_insn
      = MIR_new_insn (ctx, MIR_MOV, freg_op, MIR_new_ref_op (ctx, func_import_item));
    gen_add_insn_before (gen_ctx, insn, new_insn);
    ops[0] = MIR_new_ref_op (ctx, proto_item);
    ops[1] = freg_op;
    ops[2] = res_op;
    ops[3] = ptr_op;
    ops[4] = size_op;
    ncall = 5;
  } else if (code == MIR_ASTORE) {
    pname = "mir.atomic_store.p";
    fname = "mir.atomic_store";
    proto_item = _MIR_builtin_proto (ctx, curr_func_item->module, pname, 0, NULL, 3, MIR_T_I64, "p",
                                     MIR_T_I64, "v", MIR_T_I64, "sz");
    func_import_item
      = _MIR_builtin_func (ctx, curr_func_item->module, fname, (void *) mir_atomic_store);
    first_insn = new_insn
      = MIR_new_insn (ctx, MIR_MOV, freg_op, MIR_new_ref_op (ctx, func_import_item));
    gen_add_insn_before (gen_ctx, insn, new_insn);
    ops[0] = MIR_new_ref_op (ctx, proto_item);
    ops[1] = freg_op;
    ops[2] = ptr_op;
    ops[3] = val_op;
    ops[4] = size_op;
    ncall = 5;
  } else if (code == MIR_ACAS) {
    res_op = insn->ops[0];
    exp_op = insn->ops[2];
    des_op = insn->ops[3];
    pname = "mir.atomic_cas.p";
    fname = "mir.atomic_cas";
    proto_item
      = _MIR_builtin_proto (ctx, curr_func_item->module, pname, 1, &res_type, 4, MIR_T_I64, "p",
                            MIR_T_I64, "e", MIR_T_I64, "d", MIR_T_I64, "sz");
    func_import_item
      = _MIR_builtin_func (ctx, curr_func_item->module, fname, (void *) mir_atomic_cas);
    first_insn = new_insn
      = MIR_new_insn (ctx, MIR_MOV, freg_op, MIR_new_ref_op (ctx, func_import_item));
    gen_add_insn_before (gen_ctx, insn, new_insn);
    ops[0] = MIR_new_ref_op (ctx, proto_item);
    ops[1] = freg_op;
    ops[2] = res_op;
    ops[3] = ptr_op;
    ops[4] = exp_op;
    ops[5] = des_op;
    ops[6] = size_op;
    ncall = 7;
  } else {
    /* AXCHG / AADD / ASUB / AAND / AOR / AXOR */
    void *fn;
    res_op = insn->ops[0];
    switch (code) {
    case MIR_AXCHG:
      pname = "mir.atomic_xchg.p";
      fname = "mir.atomic_xchg";
      fn = (void *) mir_atomic_xchg;
      break;
    case MIR_AADD:
      pname = "mir.atomic_fetch_add.p";
      fname = "mir.atomic_fetch_add";
      fn = (void *) mir_atomic_fetch_add;
      break;
    case MIR_ASUB:
      pname = "mir.atomic_fetch_sub.p";
      fname = "mir.atomic_fetch_sub";
      fn = (void *) mir_atomic_fetch_sub;
      break;
    case MIR_AAND:
      pname = "mir.atomic_fetch_and.p";
      fname = "mir.atomic_fetch_and";
      fn = (void *) mir_atomic_fetch_and;
      break;
    case MIR_AOR:
      pname = "mir.atomic_fetch_or.p";
      fname = "mir.atomic_fetch_or";
      fn = (void *) mir_atomic_fetch_or;
      break;
    case MIR_AXOR:
      pname = "mir.atomic_fetch_xor.p";
      fname = "mir.atomic_fetch_xor";
      fn = (void *) mir_atomic_fetch_xor;
      break;
    default: gen_assert (FALSE); return NULL;
    }
    proto_item
      = _MIR_builtin_proto (ctx, curr_func_item->module, pname, 1, &res_type, 3, MIR_T_I64, "p",
                            MIR_T_I64, "v", MIR_T_I64, "sz");
    func_import_item = _MIR_builtin_func (ctx, curr_func_item->module, fname, fn);
    first_insn = new_insn
      = MIR_new_insn (ctx, MIR_MOV, freg_op, MIR_new_ref_op (ctx, func_import_item));
    gen_add_insn_before (gen_ctx, insn, new_insn);
    ops[0] = MIR_new_ref_op (ctx, proto_item);
    ops[1] = freg_op;
    ops[2] = res_op;
    ops[3] = ptr_op;
    ops[4] = val_op;
    ops[5] = size_op;
    ncall = 6;
  }

  /* Materialize immediate size into a temp if needed for call ABI.
     Insert before the CALL (after first_insn MOV) so next_insn still hits size mov + call. */
  if (size_op.mode == MIR_OP_INT || size_op.mode == MIR_OP_UINT) {
    MIR_op_t sz_reg = _MIR_new_var_op (ctx, gen_new_temp_reg (gen_ctx, MIR_T_I64, func));
    new_insn = MIR_new_insn (ctx, MIR_MOV, sz_reg, size_op);
    gen_add_insn_before (gen_ctx, insn, new_insn);
    ops[ncall - 1] = sz_reg;
  }

  new_insn = MIR_new_insn_arr (ctx, MIR_CALL, ncall, ops);
  gen_add_insn_before (gen_ctx, insn, new_insn);
  gen_delete_insn (gen_ctx, insn);
  return first_insn;
}
