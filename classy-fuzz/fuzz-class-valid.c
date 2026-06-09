/* fuzz-class-valid.c — class edge-case tests that should all compile & pass
 *
 * Exercises valid class patterns:
 *   - Class with only data members
 *   - Class with self-referential this pointer
 *   - Multiple classes in same file
 *   - Class with String member
 *   - Class with int and pointer members
 *   - Class with many members
 *   - Class instance in a loop
 *   - Multiple instances of same class
 *   - Nested class member access (class inside class)
 *   - Class member overwrite
 *
 * NEW (constructors + defaults, from classy-classes.c + classy8.c):
 *   - `new ClassName(args)` → heap allocation + constructor call
 *   - Implicit `this` in constructor body and methods (no explicit `class X* this;` member needed)
 *   - Comma-separated data members (int x, y;)
 *   - Method-call chaining directly on a new-expression
 *   - Go-style "return this;" fluent chaining
 *   - Named constructor arguments (in declared order or out-of-order)
 *   - Default member initializers (int a=42; String s="hi;")
 */

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

/* -- dict runtime helpers (must appear before any `dict` type is used in class members or dict literals) -- */
dict  dict_object_get(dict obj, char *key);
int   dict_object_set(dict obj, char *key, dict val);
dict  dict_create_int64(long n);
dict  dict_create_string(char *s);
dict  dict_create_object();

/* ======== CLASS DEFINITIONS ======== */

/* 1. Minimal class — just data fields */
class Point {
    int x;
    int y;
};

/* 2. Class with this pointer and method */
class Animal {
    class Animal *this;
    String name;
    int legs;

    int describe() {
        printf("    %s has %d legs\n", this->name, this->legs);
        return this->legs;
    }
};

/* 3. Class with many members */
class BigClass {
    int a;
    int b;
    int c;
    int d;
    int e;
    int f;
    int g;
    int h;
    String label;
};

/* 4. Class with only a String */
class Label {
    String text;
};

/* 5. Class with a pointer member */
class IntHolder {
    int value;
    int *ref;
};

/* 6. Class that holds another class (nesting) */
class Rect {
    Point origin;
    int width;
    int height;
};

/* 7. Class with multiple String members */
class Contact {
    String first_name;
    String last_name;
    String email;
    int age;
};

/* 8. New constructor support (heap + ctor body with implicit this) */
class CtorPoint {
    int x, y;  /* comma-separated data members */

    CtorPoint(int x, int y) {
        this.x = x;
        this.y = y;
    }

    int sum() { return this.x + this.y; }

    /* fluent setters return the object for chaining */
    CtorPoint* withX(int v) { this.x = v; return this; }
    CtorPoint* withY(int v) { this.y = v; return this; }
};

/* 9. Implicit this (no explicit "class X* this;" declared in class) */
class ImplicitThis {
    int val;
    String name;

    void set(int v, String n) {
        this.val = v;      /* implicit this in method */
        this.name = n;
    }
    int get() { return this->val; }
};

/* 10. Member default initializers (classy8 style) */
class WithDefaults {
    int a = 42;
    int b = -1;
    String msg = "default";
};

/* ======== MAIN ======== */

int main() {
    printf("=== fuzz-class-valid ===\n\n");
    passed = 0;
    failed = 0;

    /* -- 1. Basic class instantiation and member access -- */
    printf("-- basic class --\n");
    Point p1;
    p1.x = 10;
    p1.y = 20;
    check(p1.x == 10, "1a  Point.x = 10");
    check(p1.y == 20, "1b  Point.y = 20");

    /* -- 2. Multiple instances of same class -- */
    Point p2;
    p2.x = 100;
    p2.y = 200;
    check(p2.x == 100, "2a  second Point.x = 100");
    check(p1.x == 10,  "2b  first Point.x unchanged after second");

    /* -- 3. Class with String member -- */
    printf("\n-- String member --\n");
    Label lbl;
    lbl.text = "Hello Label";
    check((int)strlen(lbl.text) == 11, "3a  Label.text strlen == 11");

    Label lbl2;
    lbl2.text = "";
    check((int)strlen(lbl2.text) == 0, "3b  Label.text empty string");

    Label lbl3;
    lbl3.text = "UTF-8: Ünïcödé 😊";
    check((int)strlen(lbl3.text) > 10, "3c  Label.text UTF-8 content");

    /* -- 4. Member overwrite -- */
    printf("\n-- member overwrite --\n");
    p1.x = 999;
    check(p1.x == 999, "4a  overwritten member == 999");
    p1.x = 0;
    check(p1.x == 0,   "4b  overwritten to 0");
    p1.x = -42;
    check(p1.x == -42, "4c  negative value");
    p1.x = 10;  /* restore */

    /* -- 5. Many members -- */
    /* NOTE: Tests 5a-5c reveal a compiler bug where classes with many
     * members (8+ ints + String) have incorrect memory layout.  The
     * member values read back incorrectly after assignment. */
    printf("\n-- many members --\n");
    BigClass big;
    big.a = 1;
    big.b = 2;
    big.c = 3;
    big.d = 4;
    big.e = 5;
    big.f = 6;
    big.g = 7;
    big.h = 8;
    big.label = "big one";
    check(big.a == 1, "5a  BigClass.a == 1");
    check(big.h == 8, "5b  BigClass.h == 8");
    check(big.d == 4, "5c  BigClass.d == 4 (middle member)");
    check((int)strlen(big.label) == 7, "5d  BigClass.label strlen");

    /* Overwrite a middle member */
    big.d = 44;
    check(big.d == 44, "5e  BigClass.d overwritten to 44");
    check(big.c == 3,  "5f  BigClass.c unchanged by d overwrite");
    check(big.e == 5,  "5g  BigClass.e unchanged by d overwrite");

    /* -- 6. Nested class -- */
    printf("\n-- nested class --\n");
    Rect r;
    r.origin.x = 0;
    r.origin.y = 0;
    r.width = 640;
    r.height = 480;
    check(r.origin.x == 0,  "6a  Rect.origin.x == 0");
    check(r.origin.y == 0,  "6b  Rect.origin.y == 0");
    check(r.width == 640,    "6c  Rect.width == 640");
    check(r.height == 480,   "6d  Rect.height == 480");

    /* Modify nested member */
    r.origin.x = 100;
    r.origin.y = 200;
    check(r.origin.x == 100, "6e  Rect.origin.x updated to 100");
    check(r.origin.y == 200, "6f  Rect.origin.y updated to 200");
    check(r.width == 640,    "6g  Rect.width unchanged");

    /* -- 7. Class with pointer member -- */
    printf("\n-- pointer member --\n");
    int val = 42;
    IntHolder holder;
    holder.value = 10;
    holder.ref = &val;
    check(holder.value == 10, "7a  IntHolder.value == 10");
    check(*holder.ref == 42,  "7b  IntHolder.ref dereference == 42");

    /* Modify through pointer */
    *holder.ref = 99;
    check(val == 99,          "7c  modified through ref");
    check(*holder.ref == 99,  "7d  ref reads new value");

    /* -- 8. Class with this pointer and method -- */
    printf("\n-- this pointer --\n");
    Animal cat;
    cat.this = &cat;
    cat.name = "Cat";
    cat.legs = 4;
    int legs = cat.describe();
    check(legs == 4, "8a  describe() returned legs == 4");

    Animal spider;
    spider.this = &spider;
    spider.name = "Spider";
    spider.legs = 8;
    int spider_legs = spider.describe();
    check(spider_legs == 8, "8b  spider describe() returned 8");

    /* -- 9. Class instance in loop -- */
    printf("\n-- class in loop --\n");
    {
        int sum = 0;
        int i;
        for (i = 0; i < 5; i++) {
            Point lp;
            lp.x = i;
            lp.y = i * 2;
            sum = sum + lp.x + lp.y;
        }
        /* sum = (0+0) + (1+2) + (2+4) + (3+6) + (4+8) = 30 */
        check(sum == 30, "9a  class-in-loop sum == 30");
    }

    /* -- 10. Multiple String members -- */
    printf("\n-- multiple String members --\n");
    Contact c;
    c.first_name = "Alice";
    c.last_name = "Smith";
    c.email = "alice@example.com";
    c.age = 30;
    check(strcmp(c.first_name, "Alice") == 0, "10a Contact.first_name");
    check(strcmp(c.last_name, "Smith") == 0,  "10b Contact.last_name");
    check((int)strlen(c.email) == 17,         "10c Contact.email len");
    check(c.age == 30,                         "10d Contact.age");

    /* Overwrite String members */
    c.first_name = "Bob";
    c.last_name = "Jones";
    check(strcmp(c.first_name, "Bob") == 0,   "10e overwritten first_name");
    check(strcmp(c.last_name, "Jones") == 0,  "10f overwritten last_name");

    /* -- 11. Two classes used together -- */
    printf("\n-- two classes together --\n");
    {
        Point pt;
        pt.x = 3;
        pt.y = 4;

        Label name;
        name.text = "origin";

        check(pt.x == 3, "11a  Point alongside Label");
        check(strcmp(name.text, "origin") == 0, "11b  Label alongside Point");
    }

    /* -- 12. Zero-initialized class -- */
    printf("\n-- zero init --\n");
    {
        Point zp;
        zp.x = 0;
        zp.y = 0;
        check(zp.x == 0, "12a  zero-initialized x");
        check(zp.y == 0, "12b  zero-initialized y");
    }

    /* -- 13. Large values in members -- */
    printf("\n-- large values --\n");
    {
        Point large;
        large.x = 2147483647;  /* INT_MAX */
        large.y = -2147483647; /* near INT_MIN */
        check(large.x == 2147483647,  "13a  INT_MAX in member");
        check(large.y == -2147483647, "13b  near INT_MIN in member");
    }

    /* =======================================================
     * NEW constructor / heap / fluent / named-arg / implicit-this
     * + rich dict-inside-class tests (from classy-classes.c + classy8.c + classy7.c patterns)
     * ======================================================= */
    printf("\n-- NEW constructors + heap + fluent + named args --\n");

    /* plain heap allocation + constructor call (classic classy-classes.c test 1/2) */
    CtorPoint *p = new CtorPoint(2, 5);
    check(p != 0,               "c1a  new CtorPoint returned non-null");
    check(p->x == 2,            "c1b  p->x == 2");
    check(p->y == 5,            "c1c  p->y == 5");

    CtorPoint *q = new CtorPoint(40, 2);
    check(q != p,               "c1d  distinct heap objects");
    check(q->x == 40,           "c1e  q->x == 40");
    check(q->y == 2,            "c1f  q->y == 2");

    /* method call chained directly on a new-expression */
    check( (new CtorPoint(3, 4)).sum() == 7 , "c2a  new CtorPoint(3,4).sum() == 7" );

    /* Go-style fluent chaining (return this) */
    CtorPoint *fluent = new CtorPoint(0, 0).withX(11).withY(22);
    check(fluent->x == 11,      "c3a  fluent after withX == 11");
    check(fluent->y == 22,      "c3b  fluent after withY == 22");

    /* named constructor arguments (declared order) */
    CtorPoint *n1 = new CtorPoint(x=5, y=2);
    check(n1->x == 5 && n1->y == 2, "c4a  named in-order (x=5,y=2)");

    /* named constructor arguments (out of order) */
    CtorPoint *n2 = new CtorPoint(y=99, x=7);
    check(n2->x == 7 && n2->y == 99, "c4b  named out-of-order (y=99,x=7)");

    /* stack CtorPoint should still work alongside heap ones */
    CtorPoint stk;
    stk.x = 100;
    stk.y = 200;
    check(stk.x == 100, "c5a  stack CtorPoint still assignable");
    check(stk.sum() == 300, "c5b  stack.sum() works");

	    /* -- implicit this without declaring a `class T* this` member (classy8) -- */
	    printf("\n-- implicit this --\n");
	    ImplicitThis it;
	    it.set(123, "implicit");
	    check(it.get() == 123,               "c6a  implicit-this method set/get");
	    check(strcmp(it.name, "implicit") == 0, "c6b  implicit-this String field");

	    /* -- default member initializers (classy8 style) -- */
	    printf("\n-- defaults --\n");
	    WithDefaults wd;
	    check(wd.a == 42,           "c7a  default a == 42");
	    check(wd.b == -1,           "c7b  default b == -1");
	    check(strcmp(wd.msg, "default") == 0, "c7c  default String msg");

	    wd.a = 99;  /* overwrite default */
	    wd.msg = "changed";
	    check(wd.a == 99,                     "c7d  overwrite default a");
	    check(strcmp(wd.msg, "changed") == 0, "c7e  overwrite default msg");

	    /* -- summary -- */
	    printf("\n=== fuzz-class-valid: %d passed, %d failed ===\n", passed, failed);
	    return failed;
}
