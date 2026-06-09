/* crash1.c — keep updating until segfault on ctor/new/stack
 *
 * This version uses UNIQUE method names per class to avoid "repeated declaration"
 * and uses ONLY stack member init for stack objects (no ctor call syntax on stack).
 * Heavy interleaving of stack ctored-classes + new heap + method calls.
 */

#include <stdio.h>
#include <string.h>

class Point {
    int x, y;

    Point(int x, int y) {
        this.x = x;
        this.y = y;
    }

    int sum() { return this.x + this.y; }

    Point* withX(int v) { this.x = v; return this; }
};

class WithDefaults {
    int a = 42;
    int b = -7;
    String label = "unset";

    void wd_dump() {
        printf("  WD: a=%d b=%d label=%s\n", this.a, this.b, this.label);
    }

    WithDefaults() { }
};

class Payload {
    String name;
    int size;

    Payload(String n, int s) {
        this.name = n;
        this.size = s;
    }

    void pl_dump() {
        printf("  PL: %s size=%d\n", this.name, this.size);
    }
};

class Mixed {
    int val = 99;
    String tag = "mix";
    Point* ptr = 0;

    Mixed() { }

    void set(int v, Point* p) { this.val = v; this.ptr = p; }

    void mx_dump() {
        printf("  MX: val=%d tag=%s\n", this.val, this.tag);
    }
};

void recurse_stress(int d) {
    if (d < 0) return;

    WithDefaults sw;
    sw.a = 10000 + d;
    sw.wd_dump();

    Point sp;
    sp.x = d;
    sp.y = d * 10;
    printf("  recurse Point sum=%d\n", sp.sum());

    Payload spl;
    spl.name = "rec";
    spl.size = d;
    spl.pl_dump();

    Mixed sm;
    sm.set(d * 2, &sp);
    sm.mx_dump();

    /* heaps in recursion */
    for (int i = 0; i < 2; i++) {
        new WithDefaults();
        new Point(i, d);
        new Payload("hrec", i);
        new Mixed();
    }

    recurse_stress(d - 1);

    /* use again after deeper recursion */
    sw.wd_dump();
}

int main() {
    printf("=== crash1 ultimate stress ===\n");

    /* some initial heaps */
    Point* hp = new Point(1, 2);
    WithDefaults* hwd = new WithDefaults();
    Payload* hpl = new Payload("h", 1);
    Mixed* hmx = new Mixed();
    hmx->set(42, hp);

    hwd->wd_dump();
    hpl->pl_dump();
    hmx->mx_dump();

    /* heavy outer loop */
    for (int o = 0; o < 50; o++) {
        printf("outer %d\n", o);

        /* big stack arrays */
        WithDefaults swd[15];
        Point spt[15];
        Payload spl[15];
        Mixed smx[15];

        for (int i = 0; i < 15; i++) {
            swd[i].a = o * 100 + i;
            swd[i].wd_dump();

            spt[i].x = o * 20 + i;
            spt[i].y = 5;
            printf("  spt sum=%d\n", spt[i].sum());

            spl[i].name = "s";
            spl[i].size = i;
            spl[i].pl_dump();

            smx[i].set(i, &spt[i]);
            smx[i].mx_dump();
        }

        /* heaps right after */
        for (int j = 0; j < 20; j++) {
            new Point(j, o);
            new WithDefaults();
            new Payload("b", j);
            new Mixed();
        }

        /* use stacks again */
        for (int k = 0; k < 15; k += 3) {
            swd[k].a = 99999;
            swd[k].wd_dump();
            printf("  late spt sum=%d\n", spt[k].sum());
            spl[k].pl_dump();
            smx[k].mx_dump();
        }

        /* inner nested stacks + heaps */
        {
            WithDefaults w2[5];
            Point p2[5];
            Payload pl2[5];
            Mixed m2[5];

            for (int n = 0; n < 5; n++) {
                w2[n].a = 20000 + n;
                w2[n].wd_dump();

                p2[n].x = n + 100;
                p2[n].y = n;
                pl2[n].size = n;
                pl2[n].pl_dump();

                m2[n].set(n * 10, &p2[n]);
                m2[n].mx_dump();
            }

            for (int h = 0; h < 8; h++) {
                new WithDefaults();
                new Point(h, h);
                new Payload("in", h);
                new Mixed();
            }
        }
    }

    /* deep recursion with stacks */
    recurse_stress(15);

    /* final huge mix */
    for (int f = 0; f < 30; f++) {
        WithDefaults wf;
        wf.a = f * 777;
        wf.wd_dump();

        Point pf;
        pf.x = f;
        pf.y = f + 100;
        printf("final spt sum=%d\n", pf.sum());

        Payload plf;
        plf.name = "f";
        plf.size = f;
        plf.pl_dump();

        Mixed mf;
        mf.set(f * 3, &pf);
        mf.mx_dump();

        new WithDefaults();
        new Point(42, f);
        new Payload("ff", f);
        new Mixed();
    }

    printf("=== crash1 reached end (no segfault yet, updating...) ===\n");
    return 0;
}
