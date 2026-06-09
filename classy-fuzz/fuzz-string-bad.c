/* fuzz-string-bad.c — intentionally incorrect String code
 *
 * The compiler should reject this file (produce errors).
 */

/* ======== BAD STRING OPERATIONS ======== */

/* B1. String subtraction (only + is overloaded) */
void bad_string_sub() {
    String a = "hello";
    String b = "world";
    String c = a - b;    /* no - operator on String */
}

/* B2. String multiplication */
void bad_string_mul() {
    String a = "abc";
    String b = a * 3;    /* no * operator on String */
}

/* B3. String division */
void bad_string_div() {
    String a = "abc";
    String b = a / 2;    /* no / operator on String */
}

/* B4. String modulo */
void bad_string_mod() {
    String a = "abc";
    int b = a % 2;    /* no % operator on String */
}

/* B5. Bitwise operations on String */
void bad_string_bitwise() {
    String a = "abc";
    String b = a & "def";    /* no & operator */
    String c = a | "ghi";    /* no | operator */
    String d = a ^ "jkl";    /* no ^ operator */
}

/* B6. Increment/decrement String */
void bad_string_incdec() {
    String a = "abc";
    a++;        /* no ++ on String */
    a--;        /* no -- on String */
}

/* B7. String comparison with < > (might or might not be an error) */
void bad_string_ordering() {
    String a = "abc";
    String b = "def";
    int r = a < b;    /* may not be supported */
}

/* ======== BAD STRING ARRAY ======== */

/* B8. String array with negative size */
void bad_string_arr_neg() {
    String arr[-1] = {"oops"};
}

/* B9. for-in over non-array, non-dict String */
void bad_forin_string() {
    String s = "hello";
    for (auto c in s)    /* String is not an array — s is char* */
        printf("%c", c);
}

/* ======== BAD STRING DECLARATIONS ======== */

/* B10. String initialized with integer */
void bad_string_from_int() {
    String s = 42;    /* type mismatch */
}

/* B11. String initialized with float */
void bad_string_from_float() {
    String s = 3.14;    /* type mismatch */
}

/* B12. Assign integer to String variable */
void bad_string_int_assign() {
    String s = "hello";
    s = 100;    /* type mismatch on reassignment */
}

/* ======== BAD STRING CONCATENATION ======== */

/* B13. Concat String with struct */
struct Blob {
    int x;
};

void bad_concat_struct() {
    String s = "hello";
    struct Blob b;
    b.x = 1;
    String r = s + b;    /* can't concat String with struct */
}

/* B14. Concat String with void* */
void bad_concat_void() {
    String s = "hello";
    void *p = 0;
    String r = s + p;    /* can't concat with void* */
}

/* ======== MAIN ======== */

int main() {
    printf("fuzz-string-bad: this file should NOT compile successfully\n");
    return 1;
}
