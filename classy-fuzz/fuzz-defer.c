/* fuzz-defer.c — stress test for `defer`, destructors and `delete` at scale.
 *
 * Goal: create classes in great numbers (a thousand at a time), clean them up
 * with `defer` / `delete`, and do it repeatedly, verifying:
 *   - every constructor is balanced by exactly one destructor (no leaks),
 *   - no double-frees / missed-frees (live count returns to 0),
 *   - results stay correct across many repetitions.
 *
 * Two cleanup styles are exercised:
 *   A. loop-scoped `defer delete` — one object per iteration, freed at the end
 *      of each iteration's block (so the defer stack churns 1000x per call).
 *   B. bulk cleanup — a thousand live objects at once, all freed by a single
 *      deferred block that runs at function exit.
 *
 * Run:  ./c2m classy-fuzz/fuzz-defer.c -eg
 */

#include <stdio.h>

/* -- global bookkeeping updated from ctor/dtor (methods are ordinary funcs) -- */
int g_alive;     /* currently-live object count: must be 0 between runs */
int g_ctor;      /* total constructor calls */
int g_dtor;      /* total destructor calls */

/* -- test harness (same style as the other fuzz-*.c) -- */
int passed;
int failed;

void check (int cond, char *label) {
    if (cond) {
        printf ("  PASS  %s\n", label);
        passed = passed + 1;
    } else {
        printf ("  FAIL  %s\n", label);
        failed = failed + 1;
    }
}

#define N 1000   /* objects per run */

class Node {
    int id;

    Node (int id) {
        this.id = id;
        g_ctor = g_ctor + 1;
        g_alive = g_alive + 1;
    }

    ~Node () {
        g_dtor = g_dtor + 1;
        g_alive = g_alive - 1;
    }

    int get () { return this.id; }
};

/* A. loop-scoped defer: each iteration allocates one Node and `defer delete`s
   it, so the destructor + free run at the end of *that* iteration's block. */
long loop_defer_delete (int n) {
    long s = 0;
    for (int i = 0; i < n; i++) {
        Node *p = new Node (i);
        defer delete p;          /* dtor + free at end of this iteration */
        s = s + p->get ();
    }
    return s;
}

/* B. bulk defer: keep N objects live at once, then free them all with a single
   deferred block that runs when the function returns. */
long bulk_defer_delete (int n) {
    Node *arr[N];
    long s = 0;

    for (int i = 0; i < n; i++) arr[i] = new Node (i);

    defer {
        for (int j = 0; j < n; j++) delete arr[j];
    }

    for (int i = 0; i < n; i++) s = s + arr[i]->get ();
    return s;                    /* deferred block frees all N here */
}

int main (void) {
    int reps = 100;
    long expect = (long) (N - 1) * N / 2;   /* 0 + 1 + ... + (N-1) = 499500 */
    int a_ok = 1, b_ok = 1;

    printf ("=== fuzz-defer: %d reps x %d objects ===\n", reps, N);

    /* A: loop-scoped defer delete, repeated */
    for (int r = 0; r < reps; r++)
        if (loop_defer_delete (N) != expect) a_ok = 0;
    check (a_ok, "A: loop-scoped `defer delete` sums correct over all reps");
    check (g_alive == 0, "A: live count back to 0 (no leaks / double frees)");

    /* B: bulk defer cleanup, repeated */
    for (int r = 0; r < reps; r++)
        if (bulk_defer_delete (N) != expect) b_ok = 0;
    check (b_ok, "B: bulk single-`defer` cleanup sums correct over all reps");
    check (g_alive == 0, "B: live count back to 0 (no leaks / double frees)");

    /* global balance across everything */
    check (g_ctor == g_dtor, "every ctor balanced by exactly one dtor");
    check (g_ctor == 2 * reps * N, "exact ctor count (2 * reps * N)");

    printf ("ctor=%d dtor=%d alive=%d\n", g_ctor, g_dtor, g_alive);
    printf ("\n=== fuzz-defer: %d passed, %d failed ===\n", passed, failed);
    return failed;
}
