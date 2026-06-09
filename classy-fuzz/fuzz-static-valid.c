/* fuzz-static-valid.c — tests for static class methods
 *
 * Exercises:
 *   - static method with no args
 *   - static method with args
 *   - static factory (returns heap pointer)
 *   - mix of static and non-static methods
 *   - calling static method on class name (ClassName.method())
 *   - calling static method on an instance (obj.method())
 */
#include <stdio.h>
#include <string.h>

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

/* ===== CLASS DEFINITIONS ===== */

class Counter {
    int value;

    Counter(int v) { this.value = v; }

    static int add(int a, int b) { return a + b; }
    static int max(int a, int b) { return a > b ? a : b; }

    void set(int v) { this.value = v; }
    int get() { return this.value; }
};

class Point {
    int x;
    int y;

    Point(int x, int y) {
        this.x = x;
        this.y = y;
    }
    ~Point() {}

    static Point* create(int x, int y) { return new Point(x, y); }
    static int distance_sq(int x1, int y1, int x2, int y2) {
        int dx = x2 - x1;
        int dy = y2 - y1;
        return dx*dx + dy*dy;
    }

    int sum() { return this.x + this.y; }
};

class Math {
    static int square(int n) { return n * n; }
    static int abs_val(int n) { return n < 0 ? -n : n; }
    static int clamp(int v, int lo, int hi) {
        if (v < lo) return lo;
        if (v > hi) return hi;
        return v;
    }
};

/* ===== MAIN ===== */

int main() {
    printf("=== fuzz-static-valid ===\n\n");
    passed = 0;
    failed = 0;

    printf("-- Counter static methods --\n");
    check(Counter.add(3, 4) == 7,   "s1a  Counter.add(3,4) == 7");
    check(Counter.add(0, 0) == 0,   "s1b  Counter.add(0,0) == 0");
    check(Counter.max(5, 3) == 5,   "s1c  Counter.max(5,3) == 5");
    check(Counter.max(2, 9) == 9,   "s1d  Counter.max(2,9) == 9");

    printf("\n-- Counter instance methods alongside static --\n");
    Counter c;
    c.set(10);
    check(c.get() == 10,            "s2a  instance.get() == 10");
    c.set(Counter.add(c.get(), 5));
    check(c.get() == 15,            "s2b  Counter.add used in instance set");

    printf("\n-- Point static factory --\n");
    Point *p = Point.create(3, 4);
    check(p != 0,                   "s3a  Point.create() non-null");
    check(p->x == 3,                "s3b  p->x == 3");
    check(p->y == 4,                "s3c  p->y == 4");
    check(p->sum() == 7,            "s3d  p->sum() == 7");
    delete p;

    printf("\n-- Point static distance_sq --\n");
    check(Point.distance_sq(0,0,3,4) == 25, "s4a  distance_sq(0,0,3,4)==25");
    check(Point.distance_sq(1,1,1,1) == 0,  "s4b  distance_sq same point == 0");

    printf("\n-- Math static methods --\n");
    check(Math.square(5) == 25,             "s5a  Math.square(5) == 25");
    check(Math.square(0) == 0,              "s5b  Math.square(0) == 0");
    check(Math.abs_val(-7) == 7,            "s5c  Math.abs_val(-7) == 7");
    check(Math.abs_val(3) == 3,             "s5d  Math.abs_val(3) == 3");
    check(Math.clamp(5, 0, 10) == 5,        "s5e  Math.clamp(5,0,10)==5");
    check(Math.clamp(-3, 0, 10) == 0,       "s5f  Math.clamp(-3,0,10)==0");
    check(Math.clamp(15, 0, 10) == 10,      "s5g  Math.clamp(15,0,10)==10");

    printf("\n-- Heap allocated with static factory --\n");
    Point *q = Point.create(10, 20);
    check(q != 0,                           "s6a  Point.create(10,20) non-null");
    check(q->x == 10,                       "s6b  q->x == 10");
    check(q->y == 20,                       "s6c  q->y == 20");
    check(Point.distance_sq(0, 0, q->x, q->y) == 500, "s6d  dsq(0,0,10,20)==500");
    delete q;

    printf("\n=== fuzz-static-valid: %d passed, %d failed ===\n", passed, failed);
    return failed;
}
