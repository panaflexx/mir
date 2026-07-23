/* Shared MIR TLS ref lowering (included from mir-gen.c).
 *
 * After MIR_load_module, TLS items have module_tls_id + offset.  A
 *   mov r, ref(tls_item)
 * is not a process-global absolute address.  Rewrite to:
 *   call mir.tls_addr(mod_id, offset) → r
 *
 * See TLS-IMPLEMENTATION.md (N1 emulated path). */

static MIR_insn_t lower_one_tls_mov (gen_ctx_t gen_ctx, MIR_insn_t insn) {
  MIR_context_t ctx = gen_ctx->ctx;
  MIR_func_t func = curr_func_item->u.func;
  MIR_item_t tls_item, proto_item, func_import_item;
  MIR_op_t dest, freg_op, ops[6];
  MIR_insn_t first, new_insn;
  MIR_type_t res_type = MIR_T_I64;
  uint32_t mod_id, off;

  if (insn->code != MIR_MOV) return NULL;
  if (insn->nops < 2 || insn->ops[1].mode != MIR_OP_REF) return NULL;
  tls_item = insn->ops[1].u.ref;
  if (!MIR_tls_item_p (tls_item)) return NULL;
  if (tls_item->module == NULL || tls_item->module->tls_module_id == 0) {
    /* Not loaded yet — should not happen after MIR_load_module. */
    return NULL;
  }
  dest = insn->ops[0];
  mod_id = tls_item->module->tls_module_id;
  off = MIR_tls_item_offset (tls_item);

  proto_item = _MIR_builtin_proto (ctx, curr_func_item->module, "mir.tls_addr.p", 1, &res_type, 2,
                                   MIR_T_I64, "id", MIR_T_I64, "off");
  func_import_item
    = _MIR_builtin_func (ctx, curr_func_item->module, "mir.tls_addr", (void *) mir_tls_addr);

  freg_op = MIR_new_reg_op (ctx, _MIR_new_temp_reg (ctx, MIR_T_I64, func));
  first = new_insn = MIR_new_insn (ctx, MIR_MOV, freg_op, MIR_new_ref_op (ctx, func_import_item));
  MIR_insert_insn_before (ctx, curr_func_item, insn, new_insn);

  ops[0] = MIR_new_ref_op (ctx, proto_item);
  ops[1] = freg_op;
  ops[2] = dest; /* result */
  ops[3] = MIR_new_int_op (ctx, (int64_t) mod_id);
  ops[4] = MIR_new_int_op (ctx, (int64_t) off);
  new_insn = MIR_new_insn_arr (ctx, MIR_CALL, 5, ops);
  MIR_insert_insn_before (ctx, curr_func_item, insn, new_insn);
  MIR_remove_insn (ctx, curr_func_item, insn);
  return first;
}

/* Rewrite all MOV-from-TLS-ref insns in the current function.
   Object-file / native-AOT mode leaves refs for ELF LE emission in target_translate. */
static void lower_tls_refs (gen_ctx_t gen_ctx) {
  MIR_insn_t insn, next;

  if (gen_ctx->gen_object_file && MIR_tls_native_aot_p (gen_ctx->ctx)) return;
  for (insn = DLIST_HEAD (MIR_insn_t, curr_func_item->u.func->insns); insn != NULL; insn = next) {
    next = DLIST_NEXT (MIR_insn_t, insn);
    (void) lower_one_tls_mov (gen_ctx, insn);
  }
}

static int tls_native_mov_p (gen_ctx_t gen_ctx, MIR_insn_t insn) {
  if (!gen_ctx->gen_object_file || !MIR_tls_native_aot_p (gen_ctx->ctx)) return 0;
  if (insn->code != MIR_MOV || insn->nops < 2) return 0;
  if (insn->ops[1].mode != MIR_OP_REF) return 0;
  return MIR_tls_item_p (insn->ops[1].u.ref);
}
