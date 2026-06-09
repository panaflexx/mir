/* fuzz-class-bad.c — intentionally incorrect class code
 *
 * The compiler should reject this file (produce errors).
 */

#include <inc/exception.h>

/* ======== BAD CLASS DEFINITIONS ======== */

/* B1. Class with duplicate member names */
class DupMembers {
    int x;
    int x;    /* duplicate member name */
};

/* B2. Class method accessing undeclared member */
class BadMethod {
    class BadMethod *this;
    int a;

    int broken() {
        return this->nonexistent;    /* no such member */
    }
};

/* B3. Recursive class without pointer (infinite size) */
class InfiniteClass {
    int val;
    InfiniteClass child;    /* non-pointer self-reference => infinite */
};

/* B4. Class with void member */
class VoidMember {
    void x;    /* void is not a valid member type */
    int y;
};

/* ======== BAD CLASS USAGE ======== */

/* B5. Access member of non-class type */
void bad_member_access() {
    int x = 42;
    int y = x.field;    /* int has no members */
}

/* B6. Access nonexistent member
 * NOTE: This test case can trigger a compiler SEGFAULT after reporting
 * the error.  This is a real compiler bug found by this test suite.
 */
class Simple {
    int val;
};

void bad_field() {
    Simple s;
    s.val = 1;
    int x = s.nonexistent;    /* no such member */
}

/* B7. Method call on class without this pointer */
class NoThis {
    int data;

    int method() {
        return this->data;    /* this was never declared */
    }
};

/* B8. Assign wrong type to class member */
class TypeMismatch {
    int count;
};

void bad_type_assign() {
    TypeMismatch t;
    /* Potentially: assigning struct to int field */
    /* t.count = "string";  — whether this errors depends on implicit conversion */
}

/* B9. Class used as if it were a type for arithmetic */
void bad_class_arithmetic() {
    Simple a;
    Simple b;
    a.val = 1;
    b.val = 2;
    Simple c = a + b;    /* no + operator on class */
}

/* B10. Declare class inside function (may or may not be supported) */
void class_in_function() {
    class LocalClass {
        int x;
    };
    LocalClass lc;
    lc.x = 1;
}

/* B11. Class array with negative size */
void bad_class_array() {
    Simple arr[-1];    /* negative array size */
}

/* B12. Missing semicolon after class definition */
/* Uncomment to test:
class NoSemicolon {
    int x;
}
*/

/* B13. Using class name as a variable */
void class_name_as_var() {
    int Simple = 5;    /* shadows class name with variable */
}

/* ======== MAIN ======== */

int main() {
    printf("fuzz-class-bad: this file should NOT compile successfully\n");
    return 1;
}
