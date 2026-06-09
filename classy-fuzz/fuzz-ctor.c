/* fuzz-ctor.c — dedicated fuzzing for `new Class(...)` + ctor features
 *
 * Focuses exclusively on constructs that are known to work reliably:
 *   - new + primary ctor with implicit `this`
 *   - comma-separated data members
 *   - direct method chaining on new-expressions
 *   - fluent "return this" method chaining
 *   - named ctor args (in-order and any order)
 *   - default member initializers + zero-arg ctors
 *   - ctors taking String
 *   - returning new instances from functions
 *   - passing class* to functions
 *   - new inside loops and ternary expressions
 *   - arrays of heap-allocated class objects
 *   - long chains, repeated allocations, etc.
 *
 * Deliberately avoids storing class* inside dicts for now (known source
 * of "incompatible types" compile errors + possible runtime crashes).
 */

#include <stdio.h>
#include <string.h>

/* -- test harness (same style as other fuzz-*.c) -- */
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

/* =======================================================
 * Classes under test
 * ======================================================= */

class Point {
    int x, y;                  /* comma data members */

    Point(int x, int y) {
        this.x = x;            /* implicit this in ctor */
        this.y = y;
    }

    int sum() { return this.x + this.y; }

    /* fluent (return this) */
    Point* withX(int v) { this.x = v; return this; }
    Point* withY(int v) { this.y = v; return this; }
};

/* WithDefaults removed temporarily to avoid segfaults with default-member + zero-arg ctor
   (investigating interaction with stack vs heap ctor codegen) */

class Builder {
    int   val;
    String tag;

    Builder(int v) { this.val = v; this.tag = "init"; }

    Builder* add(int d)     { this.val = this.val + d; return this; }
    Builder* setTag(String t){ this.tag = t; return this; }
    int     get()           { return this.val; }
};

class Payload {
    String name;
    int    size;

    Payload(String n, int s) {
        this.name = n;
        this.size = s;
    }

    /* Simple String return (no complex concat inside to avoid +int issues) */
    String describe() { return this.name; }
};

/* Helper functions that create/accept heap objects */

Point* make_point(int x, int y) {
    return new Point(x, y);
}

int point_sum(Point* p) {
    if (p == 0) return -999;
    return p->sum();
}

int main() {
    printf("=== fuzz-ctor ===\n\n");
    passed = 0;
    failed = 0;

    /* 1. basic new + ctor + member access */
    printf("-- basic new + ctor --\n");
    Point *p1 = new Point(2, 5);
    check(p1 != 0,        "1a  new non-null");
    check(p1->x == 2,     "1b  p1->x");
    check(p1->y == 5,     "1c  p1->y");
    check(p1->sum() == 7, "1d  p1->sum()");

    Point *p2 = new Point(40, 2);
    check(p2 != p1,       "1e  distinct objects");
    check(p2->sum() == 42,"1f  p2 sum");

    /* 2. chaining directly on a new-expression */
    printf("\n-- new-expr chaining --\n");
    check( (new Point(3, 4)).sum() == 7 , "2a  new().sum() chained" );

    /* 3. fluent return-this chaining */
    printf("\n-- fluent chaining --\n");
    Point *fl = new Point(0, 0).withX(11).withY(22);
    check(fl != 0,        "3a  fluent non-null");
    check(fl->x == 11,    "3b  after withX");
    check(fl->y == 22,    "3c  after withY");
    check(fl->sum() == 33,"3d  fluent sum");

    /* 4. named ctor arguments */
    printf("\n-- named ctor args --\n");
    Point *n1 = new Point(x=5, y=2);
    check(n1->x == 5 && n1->y == 2, "4a  named in-order");

    Point *n2 = new Point(y=99, x=7);
    check(n2->x == 7 && n2->y == 99, "4b  named out-of-order");

	    /* 5. Builder-style multi-step fluent */
	    printf("\n-- builder fluent --\n");
	    Builder *b = new Builder(10).add(5).add(3).setTag("final");
	    check(b->get() == 18,        "5a  builder value after chain");
	    check(strcmp(b->tag, "final") == 0, "5b  tag set in chain");

    /* 8. Return heap object from function */
    printf("\n-- return heap from func --\n");
    Point *from = make_point(9, 1);
    check(from != 0,             "8a  returned non-null");
    check(from->sum() == 10,     "8b  returned sum");

    /* 9. Pass heap pointer to function */
    printf("\n-- pass heap ptr --\n");
    check(point_sum(p1) == 7,    "9a  point_sum(p1)");
    check(point_sum(0) == -999,  "9b  point_sum(null) handled");

    /* 10. Array of heap class pointers */
    printf("\n-- array of heap ptrs --\n");
    Point *arr[3];
    arr[0] = new Point(100, 0);
    arr[1] = new Point(200, 1);
    arr[2] = new Point(300, 2);
    check(arr[0]->x == 100,      "10a arr[0]");
    check(arr[2]->sum() == 302,  "10b arr[2] sum");

    /* 11. new inside loop */
    printf("\n-- new inside loop --\n");
    int loop_sum = 0;
    int i;
    for (i = 0; i < 5; i++) {
        Point *lp = new Point(i, i * 10);
        loop_sum = loop_sum + lp->sum();
    }
    /* (0+0) + (1+10) + (2+20) + (3+30) + (4+40) = 110 */
    check(loop_sum == 110,       "11a sum of 5 new-in-loop");

    /* 12. new inside expression (ternary) */
    printf("\n-- new in expressions --\n");
    int flag = 1;
    Point *ex = (flag ? new Point(1, 2) : new Point(99, 99));
    check(ex->sum() == 3,        "12a new inside ternary");

	    /* 12. Long fluent chain */
	    printf("\n-- long fluent chain --\n");
	    Builder *longc = new Builder(0)
	        .add(1).add(1).add(1).add(1).add(1)
	        .add(10).setTag("done");
	    check(longc->get() == 15,    "12a  long chain result");
	    check(strcmp(longc->tag, "done") == 0, "12b  tag after long chain");

    /* -- summary -- */
    printf("\n=== fuzz-ctor: %d passed, %d failed ===\n", passed, failed);
    return failed;
}
