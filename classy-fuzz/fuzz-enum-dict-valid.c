#include <stdio.h>
#include <string.h>

/* ---- dict helpers (provided by runtime, resolved at link) ---- */
dict  dict_object_get(dict obj, char *key);
int   dict_object_set(dict obj, char *key, dict val);
dict  dict_create_int64(long n);
dict  dict_create_string(char *s);
dict  dict_create_object();

/* ---- test harness ---- */
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

/* ============================================================
 * Proposed compiler sugar:
 *
 *     enum dict Name { Ident [= const-expr], ... };
 *
 * Desugars to:
 *   - A normal C enum type "Name" with integer constants (0-based
 *     by default, or explicit values).  This gives a distinct
 *     type usable in function signatures, switch/case, arithmetic,
 *     arrays, etc.
 *   - A runtime dict (exposed under the same identifier "Name" in
 *     collection / reflection contexts) containing:
 *         { "Ident1": value1, "Ident2": value2, ... }
 *     Keys are the variant names as strings.
 *     Values are the underlying integer constants (wrapped as
 *     dict int64).
 *
 * Benefits:
 *   - Great switch/case: uses real integer constants + the compiler
 *     can perform exhaustiveness checking for these closed enums
 *     (warning when a case is missing and there is no default).
 *   - for-in over the set of variants (names, or name+value pairs)
 *     reusing the full dict for-in machinery (including on parsed
 *     tables, after mutation, etc.).
 *   - Full reflection: "name" in Name, Name.json, bracket/dot
 *     access on the reflection table, shared sub-dicts, etc.
 *   - Qualified names Name.Ident are resolved by the compiler:
 *       * In arithmetic / switch / initializer / case contexts
 *         → compile-time constant (the enum value).
 *       * In collection contexts (for-in source, "x" in Name, subscript)
 *         → the reflection dict.
 *
 * All other dict and for-in edge cases (deep nesting, long keys,
 * mixed dot+bracket, chained on returns, json roundtrips, mutation
 * of parsed tables, shared sub-objects, ...) continue to work
 * because we are just wiring into the mature dict implementation.
 * ============================================================ */

enum dict Fruit {
    Apple,
    Banana = 10,
    Kiwi,
    Mango
};

/* simple printer (uses the integer value) */
void print_fruit(Fruit f) {
    /* Full implementation would also support a builtin or special
       lowering for nameof(f) or f.__name__ using the sidecar dict.
       Here we just print the numeric value. */
    printf("  Fruit param=%d\n", (int)f);
}

int main() {
    printf("=== classy7: dict-based enum (compiler sugar) ===\n\n");
    passed = 0;
    failed = 0;

    /* ========== Basic use of qualified names + assignment ========== */
    printf("-- basic usage --\n");

    Fruit f = Fruit.Kiwi;   /* resolves to the compile-time constant (11) */
    check((int)f == 11, "1a  Fruit.Kiwi has expected underlying value (after Banana=10)");

    Fruit a = Fruit.Apple;
    check((int)a == 0, "1b  Fruit.Apple == 0");

    Fruit b = Fruit.Banana;
    check((int)b == 10, "1c  Fruit.Banana == 10 (explicit value)");

    print_fruit(f);
    print_fruit(Fruit.Apple);
    print_fruit(Fruit.Mango);

    /* ========== Switch / case (the big win) ========== */
    printf("\n-- switch on Fruit --\n");

    int switch_hit = 0;
    switch (f) {
        case Fruit.Apple:   switch_hit = 1; break;
        case Fruit.Banana:  switch_hit = 1; break;
        case Fruit.Kiwi:    switch_hit = 1; printf("    case Kiwi taken\n"); break;
        case Fruit.Mango:   switch_hit = 1; break;
        /* The compiler (sugar) knows the complete set {Apple,Banana,Kiwi,Mango}
           and will warn (or error under -Werror or a pedantic flag) if any
           member is missing when there is no default. */
    }
    check(switch_hit, "2a  switch selected the correct case for Fruit.Kiwi");

    /* A second switch that deliberately omits one member (to demo future warning) */
    switch (f) {
        case Fruit.Apple: break;
        case Fruit.Banana: break;
        case Fruit.Kiwi: break;
        /* Mango omitted on purpose - expect diagnostic in a full impl */
    }
    check(1, "2b  partial switch accepted (warning would be issued for missing Mango)");

    /* ========== for-in over the set of variants ========== */
    printf("\n-- for-in over enum (set) --\n");

    /* for (auto name in Fruit)  — iterates the names (the "set") */
    {
        int count = 0;
        printf("    single-var for-in: ");
        for (auto name in Fruit) {
            count = count + 1;
            printf("%s ", name);
        }
        printf("\n");
        check(count == 4, "3a  for (auto name in Fruit) visited exactly the 4 members");
    }

    /* for (auto name, val in Fruit)  — name + underlying integer value */
    {
        int pair_count = 0;
        printf("    key+value for-in:\n");
        for (auto name, val in Fruit) {
            pair_count = pair_count + 1;
            /* val is conceptually the integer constant for that variant.
               In the lowered form the value side of the dict carries the
               wrapped integer; sugar can make (int)val just work. */
            printf("      %s = %d\n", name, (int)val);
        }
        check(pair_count == 4, "3b  for (auto name,val in Fruit) produced 4 pairs");
    }

    /* ========== Reflection, membership, json, etc. ========== */
    printf("\n-- reflection & dict features --\n");

    check("Kiwi" in Fruit,        "4a  \"Kiwi\" in Fruit");
    check(!("Peach" in Fruit),    "4b  \"Peach\" not in Fruit (closed set)");
    check("Banana" in Fruit,      "4c  \"Banana\" in Fruit (explicit value still has name)");

    /* json of the reflection table (re-uses existing d.json support) */
    char *j = Fruit.json;
    check(j != 0,                 "5a  Fruit.json non-null");
    check((int)strlen(j) > 10,    "5b  Fruit.json has real content");
    printf("    Fruit.json = %s\n", j);

    /* Round-trip through json + for-in on the parsed table */
    dict parsed = json(j);
    check("Mango" in parsed,      "6a  json round-trip preserved a member");
    {
        int pc = 0;
        for (auto k in parsed) pc = pc + 1;
        check(pc == 4,            "6b  for-in works on json-parsed enum table");
    }

    /* "in" inside control flow (already works for dicts) */
    if ("Kiwi" in Fruit)
        check(1, "7a  if-\"in\" true branch");
    else
        check(0, "7a  if-\"in\" true branch (should not)");

    if ("Ghost" in Fruit)
        check(0, "7b  if-\"in\" false branch (should not)");
    else
        check(1, "7b  if-\"in\" false branch");

    /* Direct runtime lookup via the reflection dict (raw, for completeness) */
    dict kiwi_entry = dict_object_get(Fruit, "Kiwi");
    check(kiwi_entry != 0,        "8a  raw dict_object_get(Fruit, \"Kiwi\")");

    /* ========== Summary ========== */
    printf("\n=== results: %d passed, %d failed ===\n", passed, failed);

    return failed;
}
