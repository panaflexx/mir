/* Smoke test for MIR TLS items + emulated runtime (N1).
 *
 * Issue #394 semantics without the C frontend:
 *   - each OS thread gets its own TLS image copy
 *   - child write must not affect main's cell
 *   - initialized tls_data template is copied on first touch
 *
 * Run:  ./bin/tls_test   (from build dir) or cmake --build … --target tls_test
 */
#include "../mir.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

struct tls_ids {
  uint32_t mod_id;
  uint32_t off_x; /* _Thread_local int x  (bss) */
  uint32_t off_y; /* _Thread_local int y = 7 (data) */
};

static void *child_fn (void *arg) {
  struct tls_ids *ids = (struct tls_ids *) arg;
  int *px = (int *) mir_tls_addr (ids->mod_id, ids->off_x);
  int *py = (int *) mir_tls_addr (ids->mod_id, ids->off_y);
  assert (px != NULL && py != NULL);
  assert (*px == 0); /* bss zeroed */
  assert (*py == 7); /* template init */
  *px = 1;
  *py = 99;
  /* Distinct addresses from main will be checked after join via child-only write. */
  return (void *) (uintptr_t) px;
}

static int test_api_and_threads (void) {
  MIR_context_t ctx = MIR_init ();
  MIR_module_t m;
  MIR_item_t xb, yd;
  int init = 7;
  struct tls_ids ids;
  pthread_t th;
  void *child_ptr;
  int *main_x, *main_y;
  int err;

  m = MIR_new_module (ctx, "tls_mod");
  xb = MIR_new_tls_bss (ctx, "x", sizeof (int));
  yd = MIR_new_tls_data (ctx, "y", MIR_T_I32, 1, &init);
  MIR_finish_module (ctx);

  assert (MIR_tls_item_p (xb));
  assert (MIR_tls_item_p (yd));
  assert (!MIR_tls_item_p (NULL));

  MIR_load_module (ctx, m);
  assert (m->tls_module_id != 0);
  assert (m->tls_size >= 2 * sizeof (int));
  assert (m->tls_template != NULL);

  ids.mod_id = m->tls_module_id;
  ids.off_x = MIR_tls_item_offset (xb);
  ids.off_y = MIR_tls_item_offset (yd);
  assert (ids.off_x != ids.off_y || sizeof (int) == 0);

  main_x = (int *) mir_tls_addr (ids.mod_id, ids.off_x);
  main_y = (int *) mir_tls_addr (ids.mod_id, ids.off_y);
  assert (main_x != NULL && main_y != NULL);
  assert (*main_x == 0);
  assert (*main_y == 7);
  *main_x = 0;
  *main_y = 7;

  err = pthread_create (&th, NULL, child_fn, &ids);
  assert (err == 0);
  err = pthread_join (th, &child_ptr);
  assert (err == 0);

  /* Main must still see original values (issue #394). */
  assert (*main_x == 0);
  assert (*main_y == 7);
  /* Child had a different cell. */
  assert ((void *) main_x != child_ptr);

  printf ("tls api+threads: OK (mod_id=%u off_x=%u off_y=%u main_x=%p child_x=%p)\n",
          (unsigned) ids.mod_id, (unsigned) ids.off_x, (unsigned) ids.off_y, (void *) main_x,
          child_ptr);

  MIR_finish (ctx);
  return 0;
}

static int test_binary_roundtrip (void) {
  MIR_context_t ctx = MIR_init ();
  MIR_module_t m;
  int init = 42;
  FILE *f;
  const char *path = "/tmp/mir-tls-roundtrip.bmir";
  MIR_item_t item;
  int found_tls = 0;

  m = MIR_new_module (ctx, "tls_bin");
  MIR_new_tls_bss (ctx, "a", 8);
  MIR_new_tls_data (ctx, "b", MIR_T_I32, 1, &init);
  MIR_finish_module (ctx);

  f = fopen (path, "wb");
  assert (f != NULL);
  MIR_write_module (ctx, f, m);
  fclose (f);

  MIR_finish (ctx);

  ctx = MIR_init ();
  f = fopen (path, "rb");
  assert (f != NULL);
  MIR_read (ctx, f);
  fclose (f);

  m = DLIST_HEAD (MIR_module_t, *MIR_get_module_list (ctx));
  assert (m != NULL);
  for (item = DLIST_HEAD (MIR_item_t, m->items); item != NULL;
       item = DLIST_NEXT (MIR_item_t, item)) {
    if (MIR_tls_item_p (item)) found_tls++;
  }
  assert (found_tls == 2);

  MIR_load_module (ctx, m);
  assert (m->tls_module_id != 0);
  {
    int *pb = NULL;
    for (item = DLIST_HEAD (MIR_item_t, m->items); item != NULL;
         item = DLIST_NEXT (MIR_item_t, item)) {
      if (item->item_type == MIR_tls_data_item
          && item->u.tls_data->name != NULL && strcmp (item->u.tls_data->name, "b") == 0) {
        pb = (int *) mir_tls_addr (m->tls_module_id, MIR_tls_item_offset (item));
        break;
      }
    }
    assert (pb != NULL);
    assert (*pb == 42);
  }

  printf ("tls binary roundtrip: OK\n");
  MIR_finish (ctx);
  remove (path);
  return 0;
}

int main (void) {
  if (test_api_and_threads () != 0) return 1;
  if (test_binary_roundtrip () != 0) return 1;
  printf ("ALL TLS TESTS PASSED\n");
  return 0;
}
