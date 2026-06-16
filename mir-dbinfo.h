/* mir-dbinfo.h -- Debug info table for MIR.
   Copyright (C) 2025 ClassyC project.

   A platform-agnostic debug info table that downstream consumers (DWARF
   emitters, DAP adapters, etc.) can translate to the native debug format.

   Populated by the front-end when -g is passed.  Attached to MIR_func_t
   and serialized in .bmir v2+.  Zero overhead when not populated.  */

#ifndef MIR_DBINFO_H
#define MIR_DBINFO_H

#ifndef MIR_NO_DBINFO
#define MIR_NO_DBINFO 0
#endif

#if !MIR_NO_DBINFO

#include <stdint.h>

/* ------------------------------------------------------------------ */
/*  Type descriptions                                                  */
/* ------------------------------------------------------------------ */

/* Every distinct source-level type gets a unique id within a module.
   Id 0 is reserved (unknown / void). */
typedef uint32_t MIR_dbtype_id_t;

typedef enum {
  MIR_DBT_VOID = 0,
  MIR_DBT_BASE,      /* int, char, float, _Bool, etc. */
  MIR_DBT_PTR,       /* pointer to another type */
  MIR_DBT_ARRAY,     /* fixed- or variable-length array */
  MIR_DBT_STRUCT,    /* struct / class */
  MIR_DBT_UNION,     /* union */
  MIR_DBT_ENUM,      /* enumeration */
  MIR_DBT_FUNC,      /* function type (for function pointers) */
  MIR_DBT_TYPEDEF,   /* typedef alias to another type */
  MIR_DBT_CONST,     /* const-qualified wrapper */
  MIR_DBT_VOLATILE,  /* volatile-qualified wrapper */
  MIR_DBT_RESTRICT,  /* restrict-qualified wrapper */
} MIR_dbtype_kind_t;

/* DWARF-compatible base-type encoding (DW_ATE_*) */
typedef enum {
  MIR_DBENC_NONE         = 0x00,
  MIR_DBENC_ADDRESS      = 0x01, /* DW_ATE_address */
  MIR_DBENC_BOOLEAN      = 0x02, /* DW_ATE_boolean */
  MIR_DBENC_FLOAT        = 0x04, /* DW_ATE_float */
  MIR_DBENC_SIGNED       = 0x05, /* DW_ATE_signed */
  MIR_DBENC_SIGNED_CHAR  = 0x06, /* DW_ATE_signed_char */
  MIR_DBENC_UNSIGNED     = 0x07, /* DW_ATE_unsigned */
  MIR_DBENC_UNSIGNED_CHAR= 0x08, /* DW_ATE_unsigned_char */
  MIR_DBENC_UTF          = 0x10, /* DW_ATE_UTF */
} MIR_dbencoding_t;

/* A member of a struct/union/class. */
typedef struct MIR_dbmember {
  const char *name;           /* member name */
  MIR_dbtype_id_t type_id;   /* type of this member */
  uint32_t byte_offset;       /* offset within the struct (bytes) */
  uint32_t byte_size;         /* size (bytes), 0 = use type's size */
  int16_t bit_offset;         /* bit offset for bitfields, -1 = not a bitfield */
  int16_t bit_size;           /* bit size for bitfields, 0 = not a bitfield */
} MIR_dbmember_t;

/* An enumerator (name-value pair). */
typedef struct MIR_dbenumerator {
  const char *name;
  int64_t value;
} MIR_dbenumerator_t;

/* A single type record. Discriminated by `kind`. */
typedef struct MIR_dbtype {
  MIR_dbtype_id_t id;         /* unique id within the module */
  MIR_dbtype_kind_t kind;
  const char *name;           /* type name (may be NULL for anonymous types) */
  uint32_t byte_size;         /* sizeof(), 0 = incomplete / unknown */
  uint32_t align;             /* alignof() */
  union {
    /* MIR_DBT_BASE */
    struct { MIR_dbencoding_t encoding; } base;
    /* MIR_DBT_PTR, MIR_DBT_CONST, MIR_DBT_VOLATILE, MIR_DBT_RESTRICT, MIR_DBT_TYPEDEF */
    struct { MIR_dbtype_id_t target_id; } ref;
    /* MIR_DBT_ARRAY */
    struct {
      MIR_dbtype_id_t element_id;
      int64_t count;          /* -1 = VLA / unknown */
    } array;
    /* MIR_DBT_STRUCT, MIR_DBT_UNION */
    struct {
      uint32_t num_members;
      MIR_dbmember_t *members; /* allocated array */
    } aggregate;
    /* MIR_DBT_ENUM */
    struct {
      MIR_dbtype_id_t underlying_id; /* underlying integer type */
      uint32_t num_enumerators;
      MIR_dbenumerator_t *enumerators;
    } enumeration;
    /* MIR_DBT_FUNC */
    struct {
      MIR_dbtype_id_t return_id;
      uint32_t num_params;
      MIR_dbtype_id_t *param_ids; /* allocated array */
      int variadic;
    } func;
  } u;
} MIR_dbtype_t;

/* ------------------------------------------------------------------ */
/*  Variable descriptions                                              */
/* ------------------------------------------------------------------ */

typedef enum {
  MIR_DBLOC_UNKNOWN = 0,
  MIR_DBLOC_REG,       /* MIR virtual register (name = MIR reg name) */
  MIR_DBLOC_FRAME,     /* stack frame at [FP + offset] */
  MIR_DBLOC_STATIC,    /* static / global at a fixed address (name = item name) */
} MIR_dbloc_kind_t;

/* ------------------------------------------------------------------ */
/*  Resolved machine location (the location contract)                  */
/* ------------------------------------------------------------------ */

/* The front-end (e.g. classyc) populates `loc_kind`/`loc` above with a
   MIR-level intent: "this variable lives in MIR register R" or "at MIR
   frame offset O".  That intent is not directly consumable by a DWARF
   emitter, because a MIR register has no fixed machine home until after
   register allocation.

   To bridge that gap the code generator (mir-gen, after register
   allocation) resolves every variable to a concrete machine location and
   writes it back into the fields below.  Downstream consumers (b2obj's
   DWARF emitter, DAP adapters, ...) should prefer `mach_kind` when it is
   not MIR_DBMACH_NONE and only fall back to `loc_kind`/`loc` otherwise.

   Register numbers are DWARF register numbers (DW_OP_reg/DW_OP_breg
   operands), so the target back-end owns the MIR-hard-reg -> DWARF-reg
   mapping and consumers stay target-agnostic. */
typedef enum {
  MIR_DBMACH_NONE = 0, /* not resolved / value unavailable */
  MIR_DBMACH_REG,      /* value lives in machine register mach_reg (DWARF #) */
  MIR_DBMACH_MEM,      /* value lives at [base reg mach_reg (DWARF #) + mach_offset];
                          when mach_deref is set the address is one level deeper:
                          [[mach_reg + mach_offset]] + mach_offset2 (used for an
                          aggregate reached through a frame/alloca pointer that
                          itself was spilled to the stack) */
} MIR_dbmach_kind_t;

typedef struct MIR_dbvar {
  const char *source_name;    /* original source-level variable name */
  MIR_dbtype_id_t type_id;   /* index into module type table */
  MIR_dbloc_kind_t loc_kind;
  union {
    const char *reg_name;     /* MIR_DBLOC_REG: MIR register name (e.g. "I0_x") */
    int64_t frame_offset;     /* MIR_DBLOC_FRAME: byte offset from FP */
    const char *item_name;    /* MIR_DBLOC_STATIC: MIR item name */
  } loc;
  /* Resolved machine location, filled by the code generator after register
     allocation.  MIR_DBMACH_NONE until then (and in serialized .bmir, which
     is produced before codegen runs). */
  MIR_dbmach_kind_t mach_kind;
  uint16_t mach_reg;          /* DWARF register number: REG -> the value reg;
                                 MEM -> the base reg (e.g. 6 = rbp on x86-64) */
  int32_t mach_offset;        /* MEM: byte offset from base reg */
  uint8_t mach_deref;         /* MEM: 1 = dereference [base+offset], then add offset2 */
  int32_t mach_offset2;       /* MEM + mach_deref: offset added after the deref */
  uint32_t scope_num;         /* function scope number (0 = function scope) */
  uint32_t decl_line;         /* source line of declaration */
  uint16_t decl_col;          /* source column */
  uint16_t decl_file_id;      /* index into module source_files */
  int is_param;               /* 1 = function parameter, 0 = local variable */
} MIR_dbvar_t;

/* ------------------------------------------------------------------ */
/*  Line map: PC offset -> source location (built during codegen)      */
/* ------------------------------------------------------------------ */

typedef struct MIR_line_map_entry {
  uint32_t pc_offset;         /* byte offset within the function's machine code */
  uint32_t source_line;
  uint16_t source_col;
  uint16_t source_file_id;    /* index into module source_files */
} MIR_line_map_entry_t;

typedef struct MIR_line_map {
  uint32_t num_entries;
  MIR_line_map_entry_t *entries; /* allocated array, sorted by pc_offset */
} MIR_line_map_t;

/* ------------------------------------------------------------------ */
/*  Per-function debug info table                                      */
/* ------------------------------------------------------------------ */

typedef struct MIR_dbinfo {
  /* Variable table */
  uint32_t num_vars;
  MIR_dbvar_t *vars;          /* allocated array, NULL when empty */
  /* Line map (populated after code generation) */
  MIR_line_map_t *line_map;   /* NULL until target_translate runs */
} MIR_dbinfo_t;

/* ------------------------------------------------------------------ */
/*  Per-module debug type table                                        */
/* ------------------------------------------------------------------ */

typedef struct MIR_dbtype_table {
  uint32_t num_types;
  MIR_dbtype_t *types;        /* allocated array, indexed by dbtype_id (0 = void) */
} MIR_dbtype_table_t;

/* ------------------------------------------------------------------ */
/*  API (implemented in mir.c)                                         */
/* ------------------------------------------------------------------ */

struct MIR_context;
struct MIR_module;
struct MIR_func;

/* Module-level type table */
extern MIR_dbtype_id_t MIR_dbinfo_add_type (struct MIR_context *ctx, struct MIR_module *module,
                                             const MIR_dbtype_t *type);
extern MIR_dbtype_t *MIR_dbinfo_get_type (struct MIR_module *module, MIR_dbtype_id_t id);

/* Function-level variable info */
extern void MIR_dbinfo_add_var (struct MIR_context *ctx, struct MIR_func *func,
                                const MIR_dbvar_t *var);

/* Cleanup (called automatically by MIR_finish) */
extern void MIR_dbinfo_free_func (struct MIR_context *ctx, struct MIR_func *func);
extern void MIR_dbinfo_free_module (struct MIR_context *ctx, struct MIR_module *module);

#endif /* !MIR_NO_DBINFO */
#endif /* MIR_DBINFO_H */
