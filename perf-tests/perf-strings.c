/* perf-strings.c — String performance test using classes
 *
 * Stresses the generated code for the String type:
 *   - auto-cast `+` concatenation (String + int + String + String)
 *   - class methods that build and return a String
 *   - String methods: length(), find(), substr()
 *   - strcmp on String results
 *
 * Each iteration builds a fresh formatted String via a class method, then
 * inspects it with String methods.  This exercises both the concat/allocation
 * path and the method-call path heavily (default ~250k records).
 *
 * The printed checksums are deterministic and must be identical across
 * -ei / -el / -eg, so they double as a correctness check.
 *
 * Build / run:
 *   ./c2m -O2 perf-tests/perf-strings.c -eg
 *   ./c2m -O2 perf-tests/perf-strings.c -el
 *   ./c2m      perf-tests/perf-strings.c -ei
 */

#include <time.h>
#include <stdio.h>

/* a record that knows how to render itself to a String */
class Record {
    int    id;
    String tag;

    /* "rec#<id> [<tag>]" — built entirely with auto-cast `+` concatenation */
    String render() {
        String s = "rec#" + this.id + " [" + this.tag + "]";
        return s;
    }
};

/* ---- tiny check harness for the method-correctness section ---- */
int passed;
int failed;

void check(int cond, char *label) {
    if (cond) {
        printf("  PASS  %s\n", label);
        passed = passed + 1;
    } else {
        printf("  FAIL  %s\n", label);
        failed = failed + 1;
    }
}

/* Verify the String methods return the *right* values (not just "don't crash").
 * length()/find()/substr() are UTF-8 code-point based; find() returns
 * (size_t)-1 when the needle is absent; replace() mutates in place. */
void check_string_methods() {
    printf("-- String method correctness --\n");

    /* a known rendered line (matches Record.render of id=42, tag=gamma) */
    String s = "rec#42 [gamma]";

    /* length() — 14 code points */
    check((int) s.length() == 14, "length() counts code points");

    /* find() — code-point index of a substring */
    check((int) s.find("[") == 7,        "find(\"[\") == 7");
    check((int) s.find("gamma") == 8,    "find(\"gamma\") == 8");
    check((int) s.find("rec#") == 0,     "find(\"rec#\") == 0 (at start)");

    /* find() — absent needle returns (size_t)-1 */
    check(s.find("ZZZ") == (size_t) -1,  "find(missing) == npos");

    /* substr(start, count) — code-point based */
    check(strcmp(s.substr(0, 4), "rec#") == 0,     "substr(0,4) == rec#");
    check(strcmp(s.substr(7, 7), "[gamma]") == 0,  "substr(7,7) == [gamma]");
    check(strcmp(s.substr(8, 5), "gamma") == 0,    "substr(8,5) == gamma");

    /* empty() */
    String e = "";
    check(e.empty() != 0, "empty() true for \"\"");
    check(s.empty() == 0, "empty() false for non-empty");

    /* UTF-8: length() / substr() count code points, not bytes */
    String u = "Grüße";
    check((int) u.length() == 5,                 "utf8 length() == 5 code points");
    check(strcmp(u.substr(0, 3), "Grü") == 0,    "utf8 substr(0,3) == Grü");

    /* replace(pos, len, repl) — in-place mutation */
    String r = "Hello, this is a test";
    size_t p = r.find("this");
    check((int) p == 7, "replace: find(\"this\") == 7");
    r.replace(p, 4, "that");
    check(strcmp(r, "Hello, that is a test") == 0, "replace(7,4,\"that\")");

    /* concatenation result is itself a queryable String */
    String c = "foo" + "bar";
    check((int) c.length() == 6,        "concat result length() == 6");
    check((int) c.find("bar") == 3,     "concat result find(\"bar\") == 3");
}

int main() {
    passed = 0;
    failed = 0;
    check_string_methods();
    printf("method checks: %d passed, %d failed\n\n", passed, failed);
    String tags[4] = {"alpha", "beta", "gamma", "delta"};
    int    N = 250000;

    printf("=== perf-strings: build + search via class method ===\n");
    printf("records %d\n", N);

    clock_t t0 = clock();

    long total_len   = 0; /* checksum: sum of rendered lengths            */
    long pos_sum     = 0; /* checksum: sum of '[' positions found         */
    long prefix_hits = 0; /* checksum: how many lines start with "rec#"   */

    Record r;
    for (int i = 0; i < N; i++) {
        r.id  = i;
        r.tag = tags[i & 3];

        String line = r.render();

        /* length() on a String returned from a class method */
        total_len = total_len + (long) line.length();

        /* find() the tag bracket */
        size_t p = line.find("[");
        pos_sum = pos_sum + (long) p;

        /* substr() + strcmp to confirm the rendered prefix */
        String head = line.substr(0, 4);
        if (strcmp(head, "rec#") == 0) prefix_hits = prefix_hits + 1;
    }

    clock_t t1 = clock();
    double ms = (double) (t1 - t0) * 1000.0 / (double) CLOCKS_PER_SEC;

    double mrec = ms > 0.0 ? (double) N / (ms * 1000.0) : 0.0;

    printf("\n--- results (checksum must be stable across modes) ---\n");
    printf("total_len   = %ld\n", total_len);
    printf("pos_sum     = %ld\n", pos_sum);
    printf("prefix_hits = %ld\n", prefix_hits);
    printf("\n--- timing ---\n");
    printf("elapsed     = %.2f ms\n", ms);
    printf("throughput  = %.2f Mrecord/s\n", mrec);

    /* every line must start with "rec#" */
    int loop_ok = (prefix_hits == (long) N);
    if (loop_ok)
        printf("\nPASS  all %d records rendered + searched correctly\n", N);
    else
        printf("\nFAIL  prefix_hits %ld != N %d\n", prefix_hits, N);

    /* overall result = method correctness checks + perf-loop check */
    int ok = loop_ok && (failed == 0);
    printf("\n=== perf-strings: %s (%d method checks passed, %d failed) ===\n",
           ok ? "PASS" : "FAIL", passed, failed);
    return ok ? 0 : 1;
}
