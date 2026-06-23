#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* fuzz-dict-valid.c — dict edge-case tests that should all compile & pass
 *
 * Exercises unusual but valid dict patterns to stress the compiler:
 *   - Single-key dict
 *   - Many keys in one dict
 *   - Very long key names
 *   - Special characters in string values
 *   - Deeply nested dicts (3+ levels)
 *   - Dict as function argument and return value
 *   - Dict equality/inequality with 0
 *   - Bracket access with computed keys
 *   - for-in over dict (single and key,value)
 *   - "in" operator edge cases
 *   - json() with nested structures
 *   - json() round-trip
 *   - d.json shorthand
 *   - Multiple dicts sharing sub-objects
 *   - Chained dot access on return value
 */

/* -- extern helpers (resolved at link) -- */
dict  dict_object_get(dict obj, char *key);
int   dict_object_set(dict obj, char *key, dict val);
dict  dict_create_int64(long n);
dict  dict_create_string(char *s);
dict  dict_create_object();

/* -- test harness -- */
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

/* -- helpers -- */

dict identity(dict d) {
    return d;
}

int count_keys(dict d) {
    int n = 0;
    for (auto k in d)
        n = n + 1;
    return n;
}

dict make_point(int x, int y) {
    dict pt = { "x": 0, "y": 0 };
    pt.x = x;
    pt.y = y;
    return pt;
}

/* -- main -- */

int main() {
    printf("=== fuzz-dict-valid ===\n\n");
    passed = 0;
    failed = 0;

    /* 1. Empty dict (create manually) */
    printf("-- empty/minimal dict --\n");
    dict empty = dict_create_object();
    check(empty != 0, "1a  empty dict is non-null");
    check(count_keys(empty) == 0, "1b  empty dict has 0 keys");

    /* 2. Single-key dict */
    dict single = { "only": 42 };
    check(single != 0, "2a  single-key dict non-null");
    check(single.only != 0, "2b  single.only readable");
    check(count_keys(single) == 1, "2c  single-key count == 1");

    /* 3. Many keys in one initializer */
    dict many = {
        "a": 1, "b": 2, "c": 3, "d": 4, "e": 5,
        "f": 6, "g": 7, "h": 8, "i": 9, "j": 10
    };
    check(many != 0, "3a  many-key dict non-null");
    check(count_keys(many) == 10, "3b  many-key count == 10");
    check(many.j != 0, "3c  many.j readable");

    /* 4. Long key names */
    dict longkey = dict_create_object();
    longkey.a_very_long_key_name_that_goes_on_and_on = 1;
    check("a_very_long_key_name_that_goes_on_and_on" in longkey,
          "4a  long key name survives");

    /* 5. String values with special characters */
    dict special = {
        "tab":     "hello\tworld",
        "newline": "line1\nline2",
        "quote":   "say \"hi\""
    };
    check(special.tab != 0,     "5a  tab in value");
    check(special.newline != 0, "5b  newline in value");
    check(special.quote != 0,   "5c  quote in value");

    /* 6. Deeply nested dicts (3 levels via initializer) */
    printf("\n-- deep nesting --\n");
    dict deep3 = {
        "l1": {
            "l2": {
                "l3": 9999
            }
        }
    };
    check(deep3 != 0,                  "6a  3-level deep non-null");
    check(deep3.l1 != 0,               "6b  deep3.l1");
    check(deep3.l1.l2 != 0,            "6c  deep3.l1.l2");
    check(deep3.l1.l2.l3 != 0,         "6d  deep3.l1.l2.l3");

    /* 7. Dict as function return (identity) */
    printf("\n-- function return --\n");
    dict orig = { "id": 100 };
    dict same = identity(orig);
    check(same != 0, "7a  identity return non-null");

    /* 8. Dict from helper function */
    dict pt = make_point(10, 20);
    check(pt != 0,    "8a  make_point returned non-null");
    check(pt.x != 0,  "8b  pt.x is non-null");
    check(pt.y != 0,  "8c  pt.y is non-null");

    /* 9. Dict comparison with 0 */
    printf("\n-- comparison --\n");
    dict nonnull = { "k": 1 };
    check(nonnull != 0, "9a  dict != 0");
    check(!(nonnull == 0), "9b  !(dict == 0)");

    /* 10. Bracket access with variable key */
    printf("\n-- bracket access --\n");
    dict bkt = { "alpha": 1, "beta": 2, "gamma": 3 };
    char *keys[] = {"alpha", "beta", "gamma"};
    int i;
    int bkt_ok = 1;
    for (i = 0; i < 3; i++) {
        if (bkt[keys[i]] == 0) bkt_ok = 0;
    }
    check(bkt_ok, "10a  bracket access with array of keys");

    /* 11. Bracket write then dot read */
    dict mix_access = dict_create_object();
    mix_access["foo"] = 42;
    check(mix_access.foo != 0, "11a  bracket write, dot read");

    /* 12. Dot write then bracket read */
    dict mix2 = dict_create_object();
    mix2.bar = 99;
    check(mix2["bar"] != 0, "12a  dot write, bracket read");

    /* 13. for-in over empty dict */
    printf("\n-- for-in edge cases --\n");
    {
        dict empty2 = dict_create_object();
        int cnt = 0;
        for (auto k in empty2)
            cnt = cnt + 1;
        check(cnt == 0, "13a  for-in over empty dict => 0 iterations");
    }

    /* 14. for-in with key,value over populated dict */
    {
        dict d = { "x": 10, "y": 20 };
        int kv_count = 0;
        for (auto k, v in d) {
            kv_count = kv_count + 1;
        }
        check(kv_count == 2, "14a  for-in k,v over 2-key dict");
    }

    /* 15. for-in single-variable */
    {
        dict d3 = { "p": 1, "q": 2, "r": 3 };
        int cnt = 0;
        for (auto k in d3)
            cnt = cnt + 1;
        check(cnt == 3, "15a  for-in single-var count == 3");
    }

    /* 16. "in" operator edge cases */
    printf("\n-- in operator --\n");
    dict inf = { "present": 1 };
    check("present" in inf,        "16a  existing key in dict");
    check(!("absent" in inf),      "16b  missing key not in dict");

    /* 17. "in" inside if-statement */
    if ("present" in inf)
        check(1, "17a  if-in true branch taken");
    else
        check(0, "17a  if-in true branch taken");

    if ("nope" in inf)
        check(0, "17b  if-in false branch skipped");
    else
        check(1, "17b  if-in false branch skipped");

    /* 18. json() round-trip with nested dict */
    printf("\n-- json round-trip --\n");
    dict jrt = { "a": { "b": { "c": 1 } } };
    char *serialized = json(jrt);
    check(serialized != 0, "18a  json(nested) non-null");
    check((int)strlen(serialized) > 5, "18b  json(nested) has content");
    printf("    serialized: %s\n", serialized);

    dict parsed = json(serialized);
    check(parsed != 0, "18c  json(string) parse non-null");
    check("a" in parsed, "18d  parsed has key a");

    /* 19. json() with all value types */
    dict mixed_vals = { "int_val": 42, "str_val": "hello", "obj_val": { "nested": 1 } };
    char *mv_json = json(mixed_vals);
    check(mv_json != 0, "19a  mixed-type dict serializes");
    printf("    mixed json: %s\n", mv_json);

    /* 20. d.json shorthand */
    char *shorthand = mixed_vals.json;
    check(shorthand != 0, "20a  d.json shorthand non-null");

    /* 21. Multiple dicts sharing a sub-object */
    printf("\n-- shared sub-objects --\n");
    dict sub = { "shared": 1 };
    dict parent1 = dict_create_object();
    dict parent2 = dict_create_object();
    parent1.child = sub;
    parent2.child = sub;
    check(parent1.child != 0, "21a  parent1.child shared");
    check(parent2.child != 0, "21b  parent2.child shared");

    /* 22. Overwrite key repeatedly */
    printf("\n-- repeated overwrite --\n");
    dict ow = { "key": 0 };
    ow.key = 1;
    ow.key = 2;
    ow.key = 3;
    check(ow.key != 0, "22a  overwritten key readable");

    /* 23. Dict in a loop */
    printf("\n-- dict in loop --\n");
    {
        int n;
        for (n = 0; n < 5; n++) {
            dict tmp = { "iter": n };
            /* Just check it doesn't crash — dict in loop body */
        }
        check(1, "23a  dict created in loop body without crash");
    }

    /* 24. Chained dot on function return */
    check(identity(single).only != 0, "24a  identity(d).field chained read");

    /* 25. Adding many new keys to existing dict */
    printf("\n-- add many keys --\n");
    dict grow = dict_create_object();
    grow.k1 = 1;
    grow.k2 = 2;
    grow.k3 = 3;
    grow.k4 = 4;
    grow.k5 = 5;
    check(count_keys(grow) == 5, "25a  grew dict to 5 keys");

    /* 26. Mutate parsed dict then re-serialize */
    printf("\n-- mutate parsed --\n");
    dict md = json("{\"color\":\"red\"}");
    md.size = 42;
    md.color = "blue";
    char *md_json = md.json;
    check(md_json != 0, "26a  mutated parsed dict serializable");
    check("size" in md,  "26b  mutated parsed dict has new key");
    printf("    mutated: %s\n", md_json);

    /* 27. for-in over a parsed dict */
    {
        dict fd = json("{\"p\":1,\"q\":2,\"r\":3}");
        int count = 0;
        for (auto k in fd)
            count = count + 1;
        check(count == 3, "27a  for-in over parsed dict count == 3");
    }

    /* 28. Global-scope dict (via function) */
    printf("\n-- serialized state --\n");
    printf("    single = %s\n", single.json);
    printf("    many   = %s\n", many.json);
    printf("    deep3  = %s\n", deep3.json);
    check(1, "28a  serialized dicts printed (visual)");

    /* -- summary -- */
    printf("\n=== fuzz-dict-valid: %d passed, %d failed ===\n", passed, failed);
    return failed;
}
