/* fuzz-ctor-crash-repro.c
 *
 * This is a minimized reproduction of the segfault seen in earlier
 * versions of fuzz-ctor.c when exercising new class constructors,
 * zero-arg ctors + default member initializers, String members,
 * and especially *stack-allocated* class instances of classes that
 * declare constructors.
 *
 * Symptoms seen:
 *   - ./c2m thisfile.c -el   → segfault (core dumped) at runtime
 *   - ./c2m thisfile.c -eg   → sometimes also segfaults
 *   - Minimal new-only tests (classy-classes.c style) work fine.
 *
 * Suspect areas (based on bisection in fuzz-ctor development):
 *   - class WithDefaults { int a=42; ...; WithDefaults() {} }
 *   - Point stk;  (stack, no 'new', even if members assigned manually)
 *   - Payload with String member initialized in ctor
 *
 * The rest of the suite (fuzz-class-valid etc.) passes.
 */

#include <stdio.h>
#include <string.h>

int main() {
    printf("=== ctor crash repro ===\n");

    /* 1. The Point class from classy-classes.c (known good when using only new) */
    class Point {
        int x, y;

        Point(int x, int y) {
            this.x = x;
            this.y = y;
        }

        int sum() { return this.x + this.y; }

        Point* withX(int v) { this.x = v; return this; }
        Point* withY(int v) { this.y = v; return this; }
    };

    /* Heap new works */
    Point *p = new Point(2, 5);
    printf("heap: x=%d y=%d sum=%d\n", p->x, p->y, p->sum());

    Point *fl = new Point(0, 0).withX(11).withY(22);
    printf("fluent: x=%d y=%d\n", fl->x, fl->y);

    /* 2. This class uses default member initializers + explicit zero-arg ctor.
     *    This combination, when a 'new WithDefaults()' is done, has been
     *    observed to contribute to later crashes in the same binary.
     */
    class WithDefaults {
        int a = 42;
        int b = -7;
        String label = "unset";

        WithDefaults() { }   /* explicit zero-arg ctor */
    };

    WithDefaults *d = new WithDefaults();
    printf("defaults: a=%d b=%d label=%s\n", d->a, d->b, d->label);

    /* 3. Stack-allocated instance of a class that declares a (non-default) ctor.
     *    No 'new'. Direct member writes.
     *    This pattern has repeatedly been associated with the segfault in
     *    fuzz-ctor runs (even when the stack object is used after some heap
     *    new allocations).
     */
    Point stk;
    stk.x = 1000;
    stk.y = 2000;
    printf("stack: x=%d y=%d sum=%d\n", stk.x, stk.y, stk.sum());

    /* 4. A class with a String member, initialized in a ctor that takes
     *    a String. Simple payload, no dicts.
     */
    class Payload {
        String name;
        int size;

        Payload(String n, int s) {
            this.name = n;
            this.size = s;
        }

        String describe() { return this.name; }
    };

    Payload *pay = new Payload("thing", 42);
    String desc = pay->describe();
    printf("payload: name=%s size=%d desc=%s\n", pay->name, pay->size, desc);

    /* Do some more heap activity after the stack object, to increase
     * the chance of triggering any use-after-free / uninitialized-this /
     * vtable / constructor epilogue bug.
     */
    for (int i = 0; i < 3; i++) {
        Point *lp = new Point(i, i*100);
        printf("loop[%d]: sum=%d\n", i, lp->sum());
    }

    Point *mixed = (1 ? new Point(99,1) : (Point*)0);
    printf("ternary-new sum=%d\n", mixed->sum());

    printf("=== repro finished (if you see this, it did not crash) ===\n");
    return 0;
}
