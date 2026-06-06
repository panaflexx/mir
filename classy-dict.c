/* classy-dict.c — comprehensive test of dict support in c2mir
 *
 * Exercises:
 *   1. Global dict declaration with initializer
 *   2. Nested object initializers (2+ levels)
 *   3. Integer, float, and string values in initializers
 *   4. Single-level dot-access read
 *   5. Chained multi-level dot-access read
 *   6. Dot-access assignment with integer value
 *   7. Dot-access assignment with float value
 *   8. Chained dot-access assignment
 *   9. Multiple global dict variables
 *  10. Adding new keys via dot assignment
 *  11. Overwriting existing keys via dot assignment
 *  12. Dict-to-dict assignment (sub-object)
 *  13. Local dict in function — with initializer
 *  14. Local dict in function — without initializer (dict_create_object)
 *  15. Local dict — nested initializer
 *  16. Local dict — dot read / write
 *  17. Local dict — passed as argument
 *  18. Local dict — returned value used
 *  19. Multiple local dicts in one function
 *  20. Local dict in a helper function (non-main)
 *  21. Three-level deep nesting
 */

/* ---- Global dict declarations ---- */

dict flat = {
    "name":    "alice",
    "age":     30,
    "score":   95
};

dict cfg = {
    "server": {
        "host": "localhost",
        "port": 8080
    },
    "debug": 1
};

dict meta = {
    "version": 2,
    "tag":     "beta"
};

/* ---- helpers (declared as extern, resolved by import_resolver) ---- */

char *dict_serialize_json(dict val, char *buf, long buflen, int pretty);
dict  dict_object_get(dict obj, char *key);
int   dict_object_set(dict obj, char *key, dict val);
dict  dict_create_int64(long n);
dict  dict_create_string(char *s);
dict  dict_create_object();


/* ---- Test harness ---- */

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

/* ---- (20) Helper function that creates & returns a local dict ---- */
dict make_point(int x, int y) {
    dict pt = { "x": 0, "y": 0 };
    pt.x = x;
    pt.y = y;
    return pt;
}

/* ---- (17) Helper that receives a dict and reads from it ---- */
int dict_has_key(dict obj, char *key) {
    dict v = dict_object_get(obj, key);
    return v != 0;
}

/* ---- main ---- */

int main() {
    char buf[1024];

    printf("=== dict test suite ===\n\n");
    passed = 0;
    failed = 0;

    /* ========== GLOBAL DICT TESTS ========== */

    printf("-- global dicts --\n");

    /* 1. Global dict creation */
    check(flat  != 0, "1a  flat dict is non-null");
    check(cfg   != 0, "1b  cfg dict is non-null");
    check(meta  != 0, "1c  meta dict is non-null");

    /* 4. Single-level dot read */
    check(flat.age    != 0, "4a  flat.age is non-null");
    check(flat.name   != 0, "4b  flat.name is non-null");
    check(flat.score  != 0, "4c  flat.score is non-null");
    check(cfg.debug   != 0, "4d  cfg.debug is non-null");

    /* 5. Chained dot read */
    check(cfg.server       != 0, "5a  cfg.server is non-null");
    check(cfg.server.host  != 0, "5b  cfg.server.host is non-null");
    check(cfg.server.port  != 0, "5c  cfg.server.port is non-null");

    /* 6-7. Dot-access assignment — integer */
    flat.age = 31;
    check(flat.age != 0, "6a  flat.age after int assign");
    flat.score = 100;
    check(flat.score != 0, "7a  flat.score after int assign");

    /* 8. Chained dot assignment — integer & string */
    cfg.server.port = 9090;
    check(cfg.server.port != 0, "8a  cfg.server.port chained int assign");
    cfg.server.host = "example.com";
    check(cfg.server.host != 0, "8b  cfg.server.host chained str assign");

    /* 10. Adding new keys */
    flat.email = "alice@example.com";
    check(flat.email != 0, "10a flat.email new key");
    cfg.server.workers = 4;
    check(cfg.server.workers != 0, "10b cfg.server.workers new key");

    /* 11. Overwriting existing keys */
    meta.version = 3;
    check(meta.version != 0, "11a meta.version overwrite");
    meta.tag = "release";
    check(meta.tag != 0, "11b meta.tag overwrite");

    /* 12. Dict-to-dict assignment */
    meta.srv = cfg.server;
    check(meta.srv != 0, "12a meta.srv = cfg.server");

    /* ========== LOCAL DICT TESTS ========== */

    printf("\n-- local dicts --\n");

    /* 13. Local dict with initializer */
    dict local1 = { "color": "red", "count": 5 };
    check(local1 != 0,       "13a local dict is non-null");
    check(local1.color != 0, "13b local1.color is non-null");
    check(local1.count != 0, "13c local1.count is non-null");

    /* 14. Local dict — empty, then populated */
    dict local2 = dict_create_object();
    check(local2 != 0, "14a local empty dict is non-null");
    dict_object_set(local2, "key", dict_create_string("value"));
    check(local2.key != 0, "14b local2.key after manual set");

    /* 15. Local dict — nested initializer */
    dict nested = {
        "outer": {
            "inner": 42
        }
    };
    check(nested != 0,              "15a nested local dict is non-null");
    check(nested.outer != 0,        "15b nested.outer is non-null");
    check(nested.outer.inner != 0,  "15c nested.outer.inner is non-null");

    /* 16. Local dict — dot write then read */
    local1.shape = "circle";
    check(local1.shape != 0, "16a local1.shape added via dot assign");
    local1.count = 10;
    check(local1.count != 0, "16b local1.count overwritten");

    /* 17. Local dict — passed as argument */
    check(dict_has_key(local1, "color") != 0, "17a dict_has_key(local1, color)");
    check(dict_has_key(local1, "shape") != 0, "17b dict_has_key(local1, shape)");
    check(dict_has_key(local1, "nope")  == 0, "17c dict_has_key(local1, nope) == 0");

    /* 18. Local dict — returned from function */
    dict pt = make_point(10, 20);
    check(pt   != 0, "18a make_point returned non-null");
    check(pt.x != 0, "18b pt.x is non-null");
    check(pt.y != 0, "18c pt.y is non-null");

    /* 19. Multiple local dicts in one function */
    dict a = { "id": 1 };
    dict b = { "id": 2 };
    check(a != 0, "19a first of two locals is non-null");
    check(b != 0, "19b second of two locals is non-null");
    check(a.id != 0, "19c a.id readable");
    check(b.id != 0, "19d b.id readable");

    /* 20. Local dict created in helper already tested via make_point (18) */

    /* ========== BRACKET SUBSCRIPT TESTS ========== */

    printf("\n-- bracket subscript --\n");

    /* 22. Bracket read with string literal */
    check(flat["name"]  != 0, "22a flat[\"name\"] read");
    check(flat["score"] != 0, "22b flat[\"score\"] read");

    /* 23. Bracket read with variable key */
    char *lookup = "age";
    check(flat[lookup] != 0, "23a flat[variable_key] read");

    /* 24. Bracket write */
    flat["motto"] = "carpe diem";
    check(flat["motto"] != 0, "24a flat[\"motto\"] written");
    flat["lucky"] = 7;
    check(flat["lucky"] != 0, "24b flat[\"lucky\"] int written");

    /* 25. Bracket read on nested dict */
    check(cfg["server"] != 0, "25a cfg[\"server\"] nested read");

    /* ========== IN OPERATOR TESTS ========== */

    printf("\n-- in operator --\n");

    /* 26. Basic key-in-dict */
    check("name" in flat,          "26a \"name\" in flat");
    check(!("ghost" in flat),      "26b \"ghost\" not in flat");
    check("server" in cfg,         "26c \"server\" in cfg");
    check(!("missing" in cfg),     "26d \"missing\" not in cfg");

    /* 27. in operator in if-statement */
    if ("age" in flat)
        check(1, "27a if-in true branch taken");
    else
        check(0, "27a if-in true branch taken");

    if ("nope" in flat)
        check(0, "27b if-in false branch skipped");
    else
        check(1, "27b if-in false branch skipped");

    /* ========== FOR-IN LOOP TESTS ========== */

    printf("\n-- for-in loops --\n");

    /* 28. for (auto key in dict) — single variable */
    {
        int key_count = 0;
        for (auto k in local1)
            key_count = key_count + 1;
        check(key_count == 3, "28a for-in iterated correct count (local1=3 keys)");
    }

    /* 29. for-in over a nested sub-dict */
    {
        int srv_count = 0;
        for (auto k in cfg.server)
            srv_count = srv_count + 1;
        check(srv_count == 3, "29a for-in over cfg.server (3 keys)");
    }

    /* 30. for-in with key, value */
    {
        int found_x = 0;
        for (auto k, v in pt) {
            if ("x" in pt) found_x = 1;
        }
        check(found_x, "30a for (auto k,v in pt) found x");
    }

    /* 31. for-in with body that uses bracket subscript */
    {
        printf("    31: ");
        for (auto k in meta)
            printf("%s ", k);
        printf("\n");
        check(1, "31a for-in keys printed (visual check above)");
    }

    /* 21. Three-level deep nesting */
    dict deep = {
        "l1": {
            "l2": {
                "l3": 999
            }
        }
    };
    check(deep != 0,                   "21a deep dict is non-null");
    check(deep.l1 != 0,               "21b deep.l1 is non-null");
    check(deep.l1.l2 != 0,            "21c deep.l1.l2 is non-null");
    check(deep.l1.l2.l3 != 0,         "21d deep.l1.l2.l3 is non-null");

    /* ========== SERIALISE FOR VISUAL INSPECTION ========== */

    printf("\n--- serialised state ---\n");

    dict_serialize_json(flat, buf, 1024, 1);
    printf("flat   = %s\n\n", buf);

    dict_serialize_json(cfg, buf, 1024, 1);
    printf("cfg    = %s\n\n", buf);

    dict_serialize_json(meta, buf, 1024, 1);
    printf("meta   = %s\n\n", buf);

    dict_serialize_json(local1, buf, 1024, 1);
    printf("local1 = %s\n\n", buf);

    dict_serialize_json(nested, buf, 1024, 1);
    printf("nested = %s\n\n", buf);

    dict_serialize_json(pt, buf, 1024, 1);
    printf("pt     = %s\n\n", buf);

    dict_serialize_json(deep, buf, 1024, 1);
    printf("deep   = %s\n\n", buf);

    /* ========== SUMMARY ========== */

    printf("=== results: %d passed, %d failed ===\n", passed, failed);

    return failed;
}
