/* fuzz-dict-bad.c — intentionally incorrect dict code
 *
 * Each block below is an independent error case.
 * The compiler should reject this file (produce errors).
 * Lines are annotated with what error we expect.
 *
 * NOTE: because all errors are in one file the compiler may stop
 * after the first few.  If testing individual cases, copy them
 * into separate files.
 */

/* -- extern helpers -- */
dict  dict_object_get(dict obj, char *key);
int   dict_object_set(dict obj, char *key, dict val);
dict  dict_create_object();

/* ======== BAD INITIALIZER SYNTAX ======== */

/* B1. Missing colon in initializer (should be "key": value) */
dict bad_init1 = { "key" 42 };

/* B2. Missing value after colon */
dict bad_init2 = { "key": };

/* B3. Trailing comma then close brace (may or may not be accepted) */
/* dict warn_trailing = { "a": 1, }; */

/* B4. Numeric literal as key (keys must be strings) */
dict bad_init4 = { 42: "value" };

/* B5. Nested initializer with missing brace */
dict bad_init5 = { "a": { "b": 1 };

/* ======== BAD DOT ACCESS ======== */

/* B6. Dot access on non-dict type */
int bad_dot_base = 42;
int bad_dot_result;

void bad_dot_access() {
    bad_dot_result = bad_dot_base.field;   /* int has no fields as dict */
}

/* B7. Chained dot on integer result */
dict chained_base = { "x": 1 };
void bad_chained_dot() {
    int y;
    /* x is an int value, not a dict — second dot should fail */
    /* This tests whether the compiler validates intermediate types */
}

/* ======== BAD BRACKET ACCESS ======== */

/* B8. Bracket subscript with integer index on dict (dict expects string key) */
dict bad_bracket = { "a": 1 };
void bad_bracket_int() {
    dict val = bad_bracket[0];    /* integer index on dict */
}

/* B9. Bracket subscript with no argument */
/* dict bad_bracket_empty = bad_bracket[]; */  /* uncomment to test parser */

/* ======== BAD IN OPERATOR ======== */

/* B10. "in" operator with wrong operand types */
void bad_in_ops() {
    int x = 5;
    int y = 10;
    int result = x in y;    /* "in" needs string-in-dict */
}

/* B11. "in" with dict on wrong side */
void bad_in_reversed() {
    dict d = { "a": 1 };
    int result = d in "a";    /* reversed operands */
}

/* ======== BAD FOR-IN ======== */

/* B12. for-in over an integer (not dict or array) */
void bad_forin_int() {
    int x = 42;
    for (auto k in x)       /* x is not iterable */
        printf("%s", k);
}

/* B13. for-in with too many variables */
void bad_forin_3vars() {
    dict d = { "a": 1 };
    for (auto a, b, c in d)   /* only 1 or 2 loop vars allowed */
        printf("%s", a);
}

/* B14. for-in without auto keyword */
void bad_forin_no_auto() {
    dict d = { "a": 1 };
    int k;
    for (k in d)              /* missing auto declaration */
        printf("%d", k);
}

/* ======== BAD JSON ======== */

/* B15. json() with integer argument (not string or dict) */
void bad_json_int() {
    dict result = json(42);   /* wrong argument type */
}

/* B16. json() with no arguments */
void bad_json_noargs() {
    dict result = json();     /* missing argument */
}

/* ======== BAD ASSIGNMENT ======== */

/* B17. Assign integer to dict variable */
void bad_dict_assign() {
    dict d = 42;              /* dict initialized with plain int */
}

/* B18. Assign dict to integer variable */
void bad_int_from_dict() {
    dict d = { "a": 1 };
    int x = d;                /* type mismatch */
}

/* ======== BAD ARITHMETIC ======== */

/* B19. Arithmetic on dict (dict + dict) */
void bad_dict_add() {
    dict a = { "x": 1 };
    dict b = { "y": 2 };
    dict c = a + b;           /* dicts don't support + */
}

/* B20. Comparison between dicts (not just with 0) */
void bad_dict_cmp() {
    dict a = { "x": 1 };
    dict b = { "y": 2 };
    int result = a < b;       /* no ordering on dicts */
}

/* ======== MAIN ======== */

int main() {
    printf("fuzz-dict-bad: this file should NOT compile successfully\n");
    return 1;
}
