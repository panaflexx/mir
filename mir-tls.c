/* mir-tls.c — Emulated thread-local storage runtime for MIR / ClassyC
 *
 * MIR_load_module registers each module's TLS template via mir_tls_register.
 * Generated code calls mir_tls_addr(mod_id, offset) (or mir_tls_base) to get
 * the live per-OS-thread cell.  This is the N1 "emulated TLS" path; AOT may
 * later use real ELF LE instead (see TLS-IMPLEMENTATION.md).
 *
 * Implementation: one pthread_key holds a per-thread array of bases indexed
 * by module id.  First touch for a module allocates and copies the template.
 */
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "mir.h"

#ifndef MIR_TLS_MAX_MODULES
#define MIR_TLS_MAX_MODULES 256
#endif

struct mir_tls_desc {
  size_t size;
  size_t align;
  const void *tmpl; /* not owned; valid for life of registered module */
  int live;
};

static struct mir_tls_desc g_descs[MIR_TLS_MAX_MODULES];
static pthread_mutex_t g_desc_lock = PTHREAD_MUTEX_INITIALIZER;

static pthread_key_t g_tls_key;
static pthread_once_t g_tls_once = PTHREAD_ONCE_INIT;

static void mir_tls_thread_dtor (void *p) {
  void **bases = (void **) p;
  if (bases == NULL) return;
  for (uint32_t i = 0; i < MIR_TLS_MAX_MODULES; i++) {
    if (bases[i] != NULL) free (bases[i]);
  }
  free (bases);
}

static void mir_tls_key_init (void) {
  (void) pthread_key_create (&g_tls_key, mir_tls_thread_dtor);
}

static void **mir_tls_thread_bases (void) {
  void **bases;
  (void) pthread_once (&g_tls_once, mir_tls_key_init);
  bases = (void **) pthread_getspecific (g_tls_key);
  if (bases == NULL) {
    bases = (void **) calloc (MIR_TLS_MAX_MODULES, sizeof (void *));
    if (bases == NULL) return NULL;
    (void) pthread_setspecific (g_tls_key, bases);
  }
  return bases;
}

void mir_tls_register (uint32_t id, size_t size, size_t align, const void *tmpl) {
  if (id == 0 || id >= MIR_TLS_MAX_MODULES) return;
  if (align == 0) align = 8;
  pthread_mutex_lock (&g_desc_lock);
  g_descs[id].size = size;
  g_descs[id].align = align;
  g_descs[id].tmpl = tmpl;
  g_descs[id].live = 1;
  pthread_mutex_unlock (&g_desc_lock);
}

void mir_tls_unregister (uint32_t id) {
  if (id == 0 || id >= MIR_TLS_MAX_MODULES) return;
  pthread_mutex_lock (&g_desc_lock);
  g_descs[id].live = 0;
  g_descs[id].tmpl = NULL;
  g_descs[id].size = 0;
  pthread_mutex_unlock (&g_desc_lock);
  /* Per-thread blocks for this id are freed on thread exit (dtor). */
}

/* AOT (N1): b2obj emits a strong `__mir_tls_aot_regs` table of
 * {id, size, tmpl_ptr} entries terminated by id==0.  On first TLS access we
 * register any entries that are not yet live.  JIT never defines the strong
 * symbol, so the weak empty table below is used (no-op bootstrap). */
typedef struct {
  uint32_t id;
  uint32_t size;
  const void *tmpl;
} mir_tls_aot_entry_t;

__attribute__ ((weak)) mir_tls_aot_entry_t __mir_tls_aot_regs[] = {{0, 0, NULL}};

static void mir_tls_bootstrap_aot (void) {
  static int done;
  mir_tls_aot_entry_t *e;
  if (done) return;
  done = 1;
  for (e = __mir_tls_aot_regs; e != NULL && e->id != 0; e++) {
    if (e->id < MIR_TLS_MAX_MODULES)
      mir_tls_register (e->id, e->size, 8, e->tmpl);
  }
}

void *mir_tls_base (uint32_t id) {
  void **bases;
  struct mir_tls_desc desc;
  void *block;

  if (id == 0 || id >= MIR_TLS_MAX_MODULES) return NULL;
  mir_tls_bootstrap_aot ();
  bases = mir_tls_thread_bases ();
  if (bases == NULL) return NULL;
  if (bases[id] != NULL) return bases[id];

  pthread_mutex_lock (&g_desc_lock);
  desc = g_descs[id];
  pthread_mutex_unlock (&g_desc_lock);
  if (!desc.live || desc.size == 0) {
    /* Empty TLS image: still hand out a unique non-NULL base. */
    block = malloc (1);
    if (block == NULL) return NULL;
    bases[id] = block;
    return block;
  }

  /* malloc is fine: TLS objects are not over-aligned beyond 8 in N1. */
  (void) desc.align;
  block = malloc (desc.size);
  if (block == NULL) return NULL;
  if (desc.tmpl != NULL)
    memcpy (block, desc.tmpl, desc.size);
  else
    memset (block, 0, desc.size);
  bases[id] = block;
  return block;
}

void *mir_tls_addr (uint32_t id, size_t offset) {
  void *base = mir_tls_base (id);
  if (base == NULL) return NULL;
  return (uint8_t *) base + offset;
}
