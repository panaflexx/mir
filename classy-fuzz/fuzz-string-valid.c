#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* fuzz-string-valid.c — String edge-case tests that should all compile & pass
 *
 * Exercises unusual but valid String patterns:
 *   - Empty string
 *   - String with only whitespace
 *   - String with escape sequences
 *   - String concatenation (variable + variable)
 *   - String concatenation with array elements
 *   - String arrays — single, many
 *   - strlen on various String forms
 *   - strcmp on various String forms
 *   - String to printf with multiple format specifiers
 *   - for-in over String arrays
 *   - Indexed for-in over String arrays
 *   - UTF-8 strings
 *   - String value in dict
 *   - Reassigning String variables
 *   - String in conditional expressions
 *   - Mixed String and char* in same function
 */

/* -- globals -- */
String g_hello = "global hello";
String g_empty = "";
String g_utf8  = "Schöne Grüße 😊";
String g_arr[3] = {"first", "second", "third"};

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

void print_greeting(String name) {
    String hi = "Hi, ";
    printf("    %s\n", hi + name);
}

int string_notempty(String s) {
    return (int)strlen(s) > 0;
}

/* -- main -- */

int main() {
    printf("=== fuzz-string-valid ===\n\n");
    passed = 0;
    failed = 0;

    /* ========== 1. Basic declaration ========== */
    printf("-- 1. declaration --\n");

    String local = "local string";
    check((int)strlen(local) == 12,      "1a  local String len 12");
    check((int)strlen(g_hello) == 12,    "1b  global String len 12");
    check((int)strlen(g_empty) == 0,     "1c  empty String len 0");
    check((int)strlen(g_utf8) > 10,      "1d  UTF-8 String len > 10");

    /* ========== 2. Empty and whitespace strings ========== */
    printf("\n-- 2. empty/whitespace --\n");

    String ws = "   ";
    check((int)strlen(ws) == 3,     "2a  whitespace String len 3");
    String tab_str = "\t\t";
    check((int)strlen(tab_str) == 2, "2b  tab String len 2");
    String nl_str = "\n\n\n";
    check((int)strlen(nl_str) == 3,  "2c  newline String len 3");

    /* ========== 3. Escape sequences ========== */
    printf("\n-- 3. escape sequences --\n");

    String esc1 = "tab\there";
    String esc2 = "newline\nhere";
    String esc3 = "quote\"here";
    String esc4 = "backslash\\here";
    String esc5 = "null\0hidden";
    check((int)strlen(esc1) > 0, "3a  tab escape");
    check((int)strlen(esc2) > 0, "3b  newline escape");
    check((int)strlen(esc3) > 0, "3c  quote escape");
    check((int)strlen(esc4) > 0, "3d  backslash escape");
    /* esc5 has embedded null — strlen truncates */
    check((int)strlen(esc5) == 4, "3e  null truncates strlen to 4");

    /* ========== 4. Concatenation ========== */
    printf("\n-- 4. concatenation --\n");

    String a = "Hello";
    String b = " World";
    printf("    concat: %s\n", a + b);
    check((int)strlen(a + b) == 11, "4a  basic concat len 11");

    /* Concat with global array element */
    String prefix = "my_";
    printf("    prefix+arr: %s\n", prefix + g_arr[0]);
    check((int)strlen(prefix + g_arr[0]) == 8, "4b  prefix+array[0] len 8");

    /* Multiple concat in same scope */
    String x = "one";
    String y = "two";
    String z = "three";
    printf("    x+y: %s\n", x + y);
    printf("    y+z: %s\n", y + z);
    check((int)strlen(x + y) == 6, "4c  x+y len 6");
    check((int)strlen(y + z) == 8, "4d  y+z len 8");

    /* ========== 5. String arrays ========== */
    printf("\n-- 5. arrays --\n");

    /* Global array */
    check((int)strlen(g_arr[0]) == 5, "5a  global array[0] len 5 (first)");
    check((int)strlen(g_arr[2]) == 5, "5b  global array[2] len 5 (third)");

    /* Local array */
    String local_arr[4] = {"alpha", "beta", "gamma", "delta"};
    check((int)strlen(local_arr[0]) == 5, "5c  local array[0] len 5");
    check((int)strlen(local_arr[3]) == 5, "5d  local array[3] len 5");

    /* Single-element array */
    String single[1] = {"only"};
    check((int)strlen(single[0]) == 4, "5e  single-elem array len 4");

    /* ========== 6. strcmp ========== */
    printf("\n-- 6. strcmp --\n");

    String sa = "apple";
    String sb = "banana";
    String sc = "apple";
    check(strcmp(sa, sc) == 0,   "6a  equal strings");
    check(strcmp(sa, sb) != 0,   "6b  unequal strings");
    check(strcmp(sa, sb) < 0,    "6c  apple < banana");
    check(strcmp(sb, sa) > 0,    "6d  banana > apple");
    check(strcmp(g_empty, "") == 0, "6e  empty == empty");

    /* ========== 7. printf ========== */
    printf("\n-- 7. printf --\n");

    printf("    single: %s\n", local);
    check(1, "7a  printf with String (visual)");
    printf("    multi: %s | %s | %s\n", g_arr[0], g_arr[1], g_arr[2]);
    check(1, "7b  printf with multiple Strings (visual)");
    printf("    mixed: %s has %d chars\n", local, (int)strlen(local));
    check(1, "7c  printf String + int (visual)");

    /* ========== 8. Function parameter ========== */
    printf("\n-- 8. functions --\n");

    printf("    greeting: ");
    print_greeting("Bob");
    check(1, "8a  print_greeting(\"Bob\") visual");

    printf("    greeting2: ");
    print_greeting(g_arr[1]);
    check(1, "8b  print_greeting with global array elem");

    check(string_notempty(local),    "8c  string_notempty(local)");
    check(!string_notempty(g_empty), "8d  !string_notempty(empty)");

    /* ========== 9. for-in over String array ========== */
    printf("\n-- 9. for-in --\n");

    {
        int count = 0;
        for (auto s in g_arr)
            count = count + 1;
        check(count == 3, "9a  for-in global String[3] count");
    }

    {
        int count = 0;
        for (auto s in local_arr)
            count = count + 1;
        check(count == 4, "9b  for-in local String[4] count");
    }

    /* Print all */
    {
        printf("    g_arr:");
        for (auto s in g_arr)
            printf(" [%s]", s);
        printf("\n");
        check(1, "9c  for-in print (visual)");
    }

    /* ========== 10. Indexed for-in ========== */
    printf("\n-- 10. indexed for-in --\n");

    {
        int last_i = -1;
        for (auto i, s in g_arr)
            last_i = i;
        check(last_i == 2, "10a  indexed for-in last idx == 2");
    }

    {
        printf("    indexed:");
        for (auto i, s in local_arr)
            printf(" [%d]=%s", i, s);
        printf("\n");
        check(1, "10b  indexed for-in print (visual)");
    }

    /* ========== 11. UTF-8 content ========== */
    printf("\n-- 11. UTF-8 --\n");

    printf("    utf8: %s\n", g_utf8);
    check((int)strlen(g_utf8) > 0, "11a  UTF-8 global non-empty");

    String emoji = "🎉🎊🎈🎆";
    printf("    emoji: %s\n", emoji);
    check((int)strlen(emoji) > 0, "11b  emoji String non-empty");

    String jp = "日本語テスト";
    printf("    japanese: %s\n", jp);
    check((int)strlen(jp) > 0, "11c  Japanese non-empty");

    /* ========== 12. Reassignment ========== */
    printf("\n-- 12. reassignment --\n");

    String mut = "original";
    check((int)strlen(mut) == 8, "12a  original len 8");
    mut = "changed";
    check((int)strlen(mut) == 7, "12b  changed len 7");
    mut = "";
    check((int)strlen(mut) == 0, "12c  empty after reassign len 0");
    mut = "final value";
    check((int)strlen(mut) == 11, "12d  final len 11");

    /* ========== 13. String in dict ========== */
    printf("\n-- 13. dict interaction --\n");

    dict d = { "name": "alice", "city": "paris" };
    check(d.name != 0, "13a  dict with String val");
    d.country = "france";
    check(d.country != 0, "13b  dict String assign");
    d.animal = g_arr[0];
    check("animal" in d, "13c  dict from String array");

    /* ========== 14. Mixed String and char* ========== */
    printf("\n-- 14. mixed types --\n");

    char *cstr = "c-string";
    String sstr = "String-type";
    printf("    cstr: %s  sstr: %s\n", cstr, sstr);
    check(1, "14a  mixed char* and String printf (visual)");

    /* strlen works on both */
    check((int)strlen(cstr) == 8, "14b  strlen on char*");
    check((int)strlen(sstr) == 11, "14c  strlen on String");

    /* ========== 15. String in for-in alongside int array ========== */
    printf("\n-- 15. mixed for-in --\n");
    {
        int str_count = 0;
        for (auto s in g_arr)
            str_count = str_count + 1;

        int nums[] = {10, 20, 30};
        int num_sum = 0;
        for (auto n in nums)
            num_sum = num_sum + n;

        check(str_count == 3, "15a  String for-in count");
        check(num_sum == 60,  "15b  int array for-in sum alongside");
    }

    /* ========== 16. String longer than typical buffer ========== */
    printf("\n-- 16. long string --\n");
    String long_s = "This is a somewhat long string value that should work fine even if it is quite lengthy and takes up more space than the typical short identifier might need.";
    check((int)strlen(long_s) > 100, "16a  long string len > 100");

    /* -- summary -- */
    printf("\n=== fuzz-string-valid: %d passed, %d failed ===\n", passed, failed);
    return failed;
}
