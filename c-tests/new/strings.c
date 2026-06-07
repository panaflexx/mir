/* Regression test for built-in String method support in c2mir.
 *
 * Self-validating: main() returns 0 on success, otherwise the number of failed
 * checks.  Restricted to the in-process c2m runners (see strings.c.opt), which
 * link the String runtime (cstring.h) via the import resolver.
 */
#include <stddef.h>

int printf (const char *, ...);
int strcmp (const char *, const char *);

static int fails;

static void check (int cond, const char *label) {
  if (!cond) {
    printf ("FAIL: %s\n", label);
    fails++;
  }
}

int main (void) {
  String s = "Hello, this is a test";

  /* length / find */
  check (s.length () == 21, "length");
  check (s.find ("this") == 7, "find");
  check (s.find ("nope") == (size_t) -1, "find-miss");
  check (s.find ("") == 0, "find-empty");

  /* substr (with clamping) */
  String sub = s.substr (0, 5);
  check (strcmp (sub, "Hello") == 0, "substr");
  String tail = s.substr (17, 100); /* length clamps to end */
  check (strcmp (tail, "test") == 0, "substr-clamp");
  String oob = s.substr (1000, 3); /* start past end -> empty */
  check (oob.empty (), "substr-oob");

  /* replace (in-place) */
  s.replace (7, 4, "that");
  check (strcmp (s, "Hello, that is a test") == 0, "replace");

  /* null / empty */
  String x;
  check (x == null, "null-default");
  check (x.empty (), "empty-null");

  String y = "";
  check (y.empty (), "empty-str");
  check (!s.empty (), "nonempty");
  check (s != null, "not-null");

  /* UTF-8: 'Grüße' is 5 code points but more bytes */
  String u = "Grüße";
  check (u.length () == 5, "utf8-length");
  String us = u.substr (0, 3);
  check (strcmp (us, "Grü") == 0, "utf8-substr");

  if (fails != 0) printf ("strings test: %d failure(s)\n", fails);
  return fails;
}
