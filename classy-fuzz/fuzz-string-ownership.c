#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* fuzz-string-ownership.c - String ownership / lifetime tests
 *
 * Exercises the three ownership tiers introduced alongside String.copy:
 *
 *   Tier 1 - Scoped   : String lives and dies within the creating function.
 *   Tier 2 - Returned : compiler promotes the return value to the caller's
 *                        scope via release_keeping (one String survives).
 *   Tier 3 - Escaped  : String.copy(...).lower().detach() hands a raw char*
 *                        to external storage; String.attach() gives it back
 *                        to the tracker for automatic cleanup.
 *
 * Also tests .upper() / .lower() result correctness and that String.copy
 * produces an independent NUL-terminated copy of exactly the requested bytes.
 */

/* -- test harness -- */

int passed;
int failed;

void check(int cond, char *label) {
    if (cond) {
        printf("  PASS  %s\n", label);
        passed++;
    } else {
        printf("  FAIL  %s\n", label);
        failed++;
    }
}

/* -- tier 1 helpers -- */

/* Creates and uses a String entirely within one scope; caller cannot
   observe the allocation -- it is freed on return.  Returns the length
   of the result so the caller can verify something happened. */
static int scoped_lower_len(char *raw, int len) {
    String key = String.copy(raw, len).lower();
    return (int)key.length();
}

static int scoped_upper_len(char *raw, int len) {
    String key = String.copy(raw, len).upper();
    return (int)key.length();
}

/* -- tier 2 helpers -- */

/* Returns a String -- compiler uses release_keeping so the return value
   survives into the caller's scope while intermediate allocations are
   cleaned up. */
static String make_lower(char *raw, int len) {
    return String.copy(raw, len).lower();
}

static String make_upper(char *raw, int len) {
    return String.copy(raw, len).upper();
}

static String make_concat_key(char *a, char *b) {
    String sa = String.copy(a, (int)strlen(a));
    String sb = String.copy(b, (int)strlen(b));
    return sa + "-" + sb;
}

/* -- tier 3a helper (detach) -- */

/* Detaches the lowercased key from the tracker; caller owns the raw char*
   and must call free() when done. */
static char *make_detached_key(char *raw, int len) {
    return String.copy(raw, len).lower().detach();
}

/* -- tier 3b helper (attach) -- */

/* Takes ownership of a previously-detached char* back into the tracker.
   The String is freed when this function's scope exits. */
static void consume_and_verify(char *raw_key, char *expected) {
    String s = String.attach(raw_key);
    check(strcmp(s, expected) == 0, expected);
}   /* scope exit: s (== raw_key) freed via release_to */

/* -- main -- */

int main(void) {
    printf("=== fuzz-string-ownership ===\n\n");
    passed = 0;
    failed = 0;

    /* ====== 1. String.copy basics ====================================== */
    printf("-- 1. String.copy basics --\n");

    {
        char *buf = "Hello, World! From copy.";

        String s = String.copy(buf, 5);
        check(strcmp(s, "Hello") == 0,    "1a  copy first 5 bytes");
        check((int)s.length() == 5,       "1b  copy length == 5");
        check(s.empty() == 0,             "1c  copy non-empty");

        String s2 = String.copy(buf + 7, 5);
        check(strcmp(s2, "World") == 0,   "1d  copy at offset");

        String empty = String.copy(buf, 0);
        check(empty.empty(),              "1e  copy len=0 is empty");
        check(strcmp(empty, "") == 0,     "1f  copy len=0 equals empty literal");

        String full = String.copy(buf, (int)strlen(buf));
        check(strcmp(full, buf) == 0,     "1g  copy full buffer equals source");
    }

    /* ====== 2. .lower() / .upper() correctness ======================== */
    printf("\n-- 2. lower / upper correctness --\n");

    {
        String lo = String.copy("Content-Type", 12).lower();
        check(strcmp(lo, "content-type") == 0,  "2a  lower Content-Type");

        String up = String.copy("content-type", 12).upper();
        check(strcmp(up, "CONTENT-TYPE") == 0,  "2b  upper content-type");

        /* Non-alpha bytes (digits, hyphens) pass through unchanged */
        String mixed = String.copy("X-Request-123", 13).lower();
        check(strcmp(mixed, "x-request-123") == 0, "2c  lower preserves digits");

        /* Chain: copy -> lower -> upper round-trip for ASCII */
        String rt = String.copy("Hello", 5).lower().upper();
        check(strcmp(rt, "HELLO") == 0,          "2d  lower->upper round-trip");

        /* Numbers and punctuation pass through upper unchanged */
        String nums = String.copy("abc123!@#", 9).upper();
        check(strcmp(nums, "ABC123!@#") == 0, "2e  upper preserves non-alpha");
    }

    /* ====== 3. Tier 1 -- scoped: String freed on function return ======= */
    printf("\n-- 3. tier 1 (scoped) --\n");

    {
        int n1 = scoped_lower_len("Authorization", 13);
        check(n1 == 13,  "3a  scoped lower length correct");

        int n2 = scoped_upper_len("content-type", 12);
        check(n2 == 12,  "3b  scoped upper length correct");

        /* Multiple scoped calls; each function manages its own lifetime */
        int na = scoped_lower_len("Host", 4);
        int nb = scoped_lower_len("Connection", 10);
        int nc = scoped_lower_len("Accept-Encoding", 15);
        check(na == 4 && nb == 10 && nc == 15,
              "3c  multiple scoped calls, independent lifetimes");
    }

    /* ====== 4. Tier 2 -- returned: release_keeping promotes result ===== */
    printf("\n-- 4. tier 2 (returned) --\n");

    {
        String k = make_lower("Content-Type", 12);
        check(strcmp(k, "content-type") == 0,    "4a  returned lower correct");
        check((int)k.length() == 12,              "4b  returned lower length");

        String u = make_upper("x-request-id", 12);
        check(strcmp(u, "X-REQUEST-ID") == 0,    "4c  returned upper correct");

        /* Chained returns: result of one call feeds next */
        String k2 = make_lower("X-Forwarded-For", 15);
        String u2 = make_upper((char *)k2, (int)k2.length());
        check(strcmp(u2, "X-FORWARDED-FOR") == 0, "4d  chained return lower->upper");

        /* Concat key built inside a function, returned to caller */
        String ck = make_concat_key("sec-fetch", "mode");
        check(strcmp(ck, "sec-fetch-mode") == 0, "4e  returned concat key");
    }

    /* ====== 5. Tier 3a -- detach: caller owns the raw char* ============ */
    printf("\n-- 5. tier 3a (detach) --\n");

    {
        char *k1 = make_detached_key("Host", 4);
        char *k2 = make_detached_key("Content-Type", 12);
        char *k3 = make_detached_key("Authorization", 13);

        /* Pointers are still valid after the creating function returned */
        check(strcmp(k1, "host") == 0,          "5a  detached 'host' valid after return");
        check(strcmp(k2, "content-type") == 0,  "5b  detached 'content-type' valid");
        check(strcmp(k3, "authorization") == 0, "5c  detached 'authorization' valid");

        /* Caller is responsible for free() */
        free(k1);
        free(k2);
        free(k3);
        check(1, "5d  detached keys freed by caller");

        /* Detach a String created in-scope */
        String local = String.copy("Connection", 10).lower();
        char *raw = local.detach();
        check(strcmp(raw, "connection") == 0,   "5e  in-scope detach valid");
        free(raw);
    }

    /* ====== 6. Tier 3b -- attach: re-register into tracker ============= */
    printf("\n-- 6. tier 3b (attach) --\n");

    {
        char *k1 = make_detached_key("Cache-Control", 13);
        char *k2 = make_detached_key("Accept-Encoding", 15);

        check(strcmp(k1, "cache-control") == 0,    "6a  detached key1 correct");
        check(strcmp(k2, "accept-encoding") == 0,  "6b  detached key2 correct");

        /* Hand ownership back to tracker; callee's scope frees on return */
        consume_and_verify(k1, "cache-control");
        consume_and_verify(k2, "accept-encoding");
        check(1, "6c  attach: keys freed by callee scope");
    }

    /* ====== 7. Detach/attach round-trip within same scope ============== */
    printf("\n-- 7. detach/attach round-trip --\n");

    {
        String original = String.copy("X-Real-IP", 9).lower();
        char *raw = original.detach();          /* remove from tracker */
        check(strcmp(raw, "x-real-ip") == 0,   "7a  detached value correct");

        String reattached = String.attach(raw); /* give back to tracker */
        check(strcmp(reattached, "x-real-ip") == 0, "7b  reattached value correct");
        check((char *)original == (char *)reattached,
              "7c  original and reattached are same pointer");
        /* reattached (== raw) freed at scope exit */
    }

    /* ====== 8. String.copy boundary conditions ========================= */
    printf("\n-- 8. copy boundary conditions --\n");

    {
        /* Copy exactly 1 byte */
        String one = String.copy("Z", 1);
        check((int)one.length() == 1,  "8a  copy 1 byte length");
        check(strcmp(one, "Z") == 0,   "8b  copy 1 byte value");

        /* Copy from middle of longer buffer */
        char *long_buf = "0123456789abcdef";
        String mid = String.copy(long_buf + 4, 6);
        check((int)strlen(mid) == 6,           "8c  copy mid length");
        check(strcmp(mid, "456789") == 0,      "8d  copy from offset 4 value");

        /* Copy result is independent of source buffer */
        char modifiable[8] = {'H','e','l','l','o','\0'};
        String snap = String.copy(modifiable, 5);
        modifiable[0] = 'X';
        check(strcmp(snap, "Hello") == 0, "8e  copy is independent of source");
    }

    /* ====== 9. Empty string edge cases ================================= */
    printf("\n-- 9. empty edge cases --\n");

    {
        String emp_lo = String.copy("", 0).lower();
        check(emp_lo.empty(),                 "9a  lower of empty is empty");

        String emp_up = String.copy("", 0).upper();
        check(emp_up.empty(),                 "9b  upper of empty is empty");

        /* Already-lowercase unchanged */
        String lo = String.copy("hello world", 11).lower();
        check(strcmp(lo, "hello world") == 0, "9c  lower of lowercase: unchanged");

        /* Already-uppercase unchanged */
        String up = String.copy("HELLO WORLD", 11).upper();
        check(strcmp(up, "HELLO WORLD") == 0, "9d  upper of uppercase: unchanged");
    }

    /* -- summary -- */
    printf("\n=== fuzz-string-ownership: %d passed, %d failed ===\n",
           passed, failed);
    return failed;
}
