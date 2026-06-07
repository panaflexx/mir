/* perf-dict.c — dict performance test using classes
 *
 * Stresses the generated code for the dict type:
 *   - dynamic bracket-key writes  d[key] = <runtime value>
 *   - class methods that produce keys/values for the dict
 *   - dict_object_get lookups
 *   - "key" in dict membership tests
 *   - for (auto k in d) iteration
 *   - dict_object_count + json serialization
 *
 * NOTE on scaling: the bundled dict (inc/dict.h) stores object pairs in a flat
 * array with a linear key scan, so both insert and lookup are O(n) per key —
 * the whole build/lookup phase is therefore O(n^2).  N is kept modest by
 * default so the run stays well under a second; raise it to probe how the dict
 * runtime scales.
 *
 * Checksums are deterministic and must match across -ei / -el / -eg.
 *
 * Build / run:
 *   ./c2m -O2 perf-tests/perf-dict.c -eg
 *   ./c2m -O2 perf-tests/perf-dict.c -el
 *   ./c2m      perf-tests/perf-dict.c -ei
 */

#include <time.h>
#include <stdio.h>

/* dict runtime helpers (resolved by the c2m import resolver) */
dict dict_create_object();
dict dict_object_get(dict obj, char *key);
long dict_object_count(dict obj);

/* an account whose id/balance feed the dict */
class Account {
    int  id;
    long balance;

    void init(int i) {
        this.id      = i;
        this.balance = (long) i * 7 + 3;
    }

    /* write "acct<id>" into the caller's buffer; returns the buffer */
    char *key(char *buf) {
        snprintf(buf, 32, "acct%d", this.id);
        return buf;
    }
};

int main() {
    int N = 6000;

    printf("=== perf-dict: build / lookup / iterate via class ===\n");
    printf("entries %d\n", N);

    dict d = dict_create_object();
    char key[32];

    /* ---- phase 1: build ---- */
    clock_t t0 = clock();
    Account a;
    for (int i = 0; i < N; i++) {
        a.init(i);
        d[a.key(key)] = a.balance; /* dynamic key, runtime int64 value */
    }
    clock_t t1 = clock();

    /* ---- phase 2: lookup every key ---- */
    long hits = 0;
    for (int i = 0; i < N; i++) {
        snprintf(key, sizeof key, "acct%d", i);
        if (dict_object_get(d, key) != 0) hits = hits + 1;
    }
    clock_t t2 = clock();

    /* ---- phase 3: membership + iteration ---- */
    long present = 0;
    snprintf(key, sizeof key, "acct%d", N / 2);
    if (key in d) present = present + 1; /* a known-present key */
    if (!("acct999999" in d)) present = present + 1; /* a known-absent key */

    long iter_count = 0;
    for (auto k in d)
        iter_count = iter_count + 1;
    clock_t t3 = clock();

    long count = dict_object_count(d);

    double build_ms  = (double) (t1 - t0) * 1000.0 / (double) CLOCKS_PER_SEC;
    double lookup_ms = (double) (t2 - t1) * 1000.0 / (double) CLOCKS_PER_SEC;
    double iter_ms   = (double) (t3 - t2) * 1000.0 / (double) CLOCKS_PER_SEC;
    double total_ms  = (double) (t3 - t0) * 1000.0 / (double) CLOCKS_PER_SEC;

    printf("\n--- results (checksum must be stable across modes) ---\n");
    printf("count       = %ld\n", count);
    printf("hits        = %ld\n", hits);
    printf("iter_count  = %ld\n", iter_count);
    printf("present     = %ld  (expect 2)\n", present);
    printf("\n--- timing ---\n");
    printf("build       = %.2f ms  (%.2f Kinsert/s)\n",
           build_ms, build_ms > 0.0 ? (double) N / build_ms : 0.0);
    printf("lookup      = %.2f ms  (%.2f Klookup/s)\n",
           lookup_ms, lookup_ms > 0.0 ? (double) N / lookup_ms : 0.0);
    printf("iterate     = %.2f ms\n", iter_ms);
    printf("total       = %.2f ms\n", total_ms);

    /* small sample of the serialized dict for a visual sanity check */
    dict sample = dict_create_object();
    Account s;
    for (int i = 0; i < 4; i++) {
        s.init(i);
        sample[s.key(key)] = s.balance;
    }
    printf("\nsample json = %s\n", sample.json);

    int ok = (count == (long) N) && (hits == (long) N)
             && (iter_count == (long) N) && (present == 2);
    if (ok)
        printf("\nPASS  built/looked-up/iterated %d entries correctly\n", N);
    else
        printf("\nFAIL  count=%ld hits=%ld iter=%ld present=%ld (N=%d)\n",
               count, hits, iter_count, present, N);

    return ok ? 0 : 1;
}
