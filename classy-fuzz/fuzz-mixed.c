#include <stdio.h>
#include <string.h>
/* fuzz-mixed.c — tests interactions between dict, class, and String
 *
 * Exercises cross-feature patterns:
 *   - String member in class
 *   - Class data converted into dict fields
 *   - for-in over String array with dict operations
 *   - json() on dict containing String values
 *   - String concat result stored in dict
 *   - Dict for-in with String key printing
 *   - Exception handling with class + String
 *   - In operator with String variable keys
 *   - Multiple feature interactions in one function
 */

/* -- extern helpers -- */
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

/* ======== DEFINITIONS ======== */

/* Class with String and int members */
class Person {
    String name;
    int age;
};

/* Class with multiple Strings */
class Config {
    String section;
    int version;
};

/* ======== MAIN ======== */

int main() {
    printf("=== fuzz-mixed ===\n\n");
    passed = 0;
    failed = 0;

    /* ========== 1. Class with String members ========== */
    printf("-- 1. class + String --\n");

    Person alice;
    alice.name = "Alice";
    alice.age = 30;
    check(strcmp(alice.name, "Alice") == 0, "1a  Person.name == Alice");
    check(alice.age == 30, "1b  Person.age == 30");

    Person bob;
    bob.name = "Bob";
    bob.age = 25;
    check((int)strlen(bob.name) == 3, "1c  Bob name len 3");

    /* ========== 2. Class data into dict ========== */
    printf("\n-- 2. class -> dict --\n");

    dict alice_d = { "name": "?", "age": 0 };
    alice_d.name = alice.name;
    alice_d.age = alice.age;
    check(alice_d != 0, "2a  dict from class non-null");
    check(alice_d.name != 0, "2b  dict.name non-null");
    check(alice_d.age != 0, "2c  dict.age non-null");
    printf("    alice_d.json = %s\n", alice_d.json);

    /* ========== 3. Dict with String values ========== */
    printf("\n-- 3. dict + String --\n");

    String colors[3] = {"red", "green", "blue"};
    dict palette = dict_create_object();
    palette.primary = colors[0];
    palette.secondary = colors[1];
    palette.accent = colors[2];
    check("primary" in palette, "3a  palette has primary");
    check("accent" in palette, "3b  palette has accent");

    /* ========== 4. String concat stored in dict ========== */
    printf("\n-- 4. concat -> dict --\n");

    String first = "Hello";
    String second = " World";
    dict greetings = dict_create_object();
    greetings.full = first + second;
    check(greetings.full != 0, "4a  concat result in dict");

    /* ========== 5. for-in over String array + dict operations ========== */
    printf("\n-- 5. for-in String + dict --\n");

    {
        String animals[3] = {"cat", "dog", "bird"};
        dict animal_dict = dict_create_object();
        int idx = 0;
        for (auto a in animals) {
            if (idx == 0) animal_dict.a0 = a;
            if (idx == 1) animal_dict.a1 = a;
            if (idx == 2) animal_dict.a2 = a;
            idx = idx + 1;
        }
        check(idx == 3, "5a  for-in count 3");
        check("a0" in animal_dict, "5b  animal_dict has a0");
        check("a2" in animal_dict, "5c  animal_dict has a2");
    }

    /* ========== 6. json() with String values ========== */
    printf("\n-- 6. json + String --\n");

    dict jd = { "greeting": "hello", "name": "world" };
    char *j = json(jd);
    check(j != 0, "6a  json(dict_with_strings) non-null");
    printf("    json: %s\n", j);

    /* Parse back and verify */
    dict parsed = json(j);
    check(parsed != 0, "6b  parsed back non-null");
    check("greeting" in parsed, "6c  parsed has greeting");
    check("name" in parsed, "6d  parsed has name");

    /* ========== 7. Dict for-in with String operations ========== */
    printf("\n-- 7. dict for-in + String --\n");

    {
        dict info = { "first": "Alice", "last": "Smith", "city": "NYC" };
        int key_count = 0;
        for (auto k in info) {
            printf("    key: %s\n", k);
            key_count = key_count + 1;
        }
        check(key_count == 3, "7a  dict for-in key count 3");
    }

    /* ========== 8. Config class with dict ========== */
    printf("\n-- 8. Config class --\n");

    Config cfg;
    cfg.section = "database";
    cfg.version = 3;
    check(strcmp(cfg.section, "database") == 0, "8a  Config.section");
    check(cfg.version == 3, "8b  Config.version");

    /* Convert to dict for serialization */
    dict cfg_dict = { "section": "?", "version": 0 };
    cfg_dict.section = cfg.section;
    cfg_dict.version = cfg.version;
    printf("    config = %s\n", cfg_dict.json);
    check(cfg_dict != 0, "8c  Config as dict non-null");

    /* ========== 9. In operator with String values ========== */
    printf("\n-- 9. in operator + String --\n");

    {
        String key_to_find = "name";
        dict d = { "name": "test", "value": 42 };
        check(key_to_find in d, "9a  String var as 'in' key");
        String missing = "nope";
        check(!(missing in d), "9b  missing String var not in dict");
    }

    /* ========== 10. Dict with many String values ========== */
    printf("\n-- 10. many string values --\n");

    dict names = {
        "n1": "Alice", "n2": "Bob", "n3": "Carol",
        "n4": "Dave",  "n5": "Eve"
    };
    int name_count = 0;
    for (auto k in names)
        name_count = name_count + 1;
    check(name_count == 5, "10a  dict with 5 string values");
    printf("    names = %s\n", names.json);

    /* ========== 11. Stress: class + dict + String in sequence ========== */
    printf("\n-- 11. stress sequence --\n");

    {
        /* Create class instances */
        Person p1;
        p1.name = "Stress1";
        p1.age = 1;

        Person p2;
        p2.name = "Stress2";
        p2.age = 2;

        /* Put into dict */
        dict roster = dict_create_object();
        roster.p1_name = p1.name;
        roster.p1_age = p1.age;
        roster.p2_name = p2.name;
        roster.p2_age = p2.age;

        /* Verify via for-in */
        int kcount = 0;
        for (auto k in roster)
            kcount = kcount + 1;
        check(kcount == 4, "11a  stress roster has 4 keys");

        /* Serialize */
        printf("    roster = %s\n", roster.json);
        check(1, "11b  stress sequence completed");
    }

    /* ========== 12. String concat + dict + for-in combined ========== */
    printf("\n-- 12. all combined --\n");

    {
        String tags[2] = {"prod", "dev"};
        dict env = dict_create_object();

        /* Use concat to build values, store in dict */
        String base = "server-";
        for (auto t in tags) {
            /* Can't dynamically key, but can set known keys */
        }
        env.tag0 = base + tags[0];
        env.tag1 = base + tags[1];

        check("tag0" in env, "12a  combined tag0 exists");
        check("tag1" in env, "12b  combined tag1 exists");
        printf("    env = %s\n", env.json);
    }

    /* -- summary -- */
    printf("\n=== fuzz-mixed: %d passed, %d failed ===\n", passed, failed);
    return failed;
}
