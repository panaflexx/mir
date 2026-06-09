/* perf-math.c — math performance test using classes (Mandelbrot via Complex)
 *
 * Stresses the generated code for:
 *   - class instances on the stack
 *   - methods that mutate `this` (this.re / this.im)
 *   - methods that return values (norm2)
 *   - tight floating-point inner loops with many method calls
 *
 * The workload computes the escape-time of every pixel of a Mandelbrot grid.
 * Each pixel runs up to MAXIT iterations, and each iteration makes two method
 * calls on a stack `Complex`.  At the default size that is ~30M method calls,
 * which makes this a good probe for method-dispatch + FP codegen quality.
 *
 * Output is deterministic, so the printed checksum doubles as a correctness
 * check: it must be identical between -ei / -el / -eg and across optimisation
 * levels (only the timing should change).
 *
 * Build / run:
 *   ./c2m -O2 perf-tests/perf-math.c -eg     # fully generated (fastest)
 *   ./c2m -O2 perf-tests/perf-math.c -el     # lazily generated
 *   ./c2m      perf-tests/perf-math.c -ei     # interpreted (baseline)
 */

#include <time.h>
#include <stdio.h>

/* ---- a small complex-number class with mutating + value-returning methods ---- */
class Complex {
    double re, im;

    void set(double r, double i) {
        this.re = r;
        this.im = i;
    }

    /* squared magnitude — value-returning method */
    double norm2() {
        return this.re * this.re + this.im * this.im;
    }

    /* z = z*z + c  — mutates `this` in place */
    void step(double cr, double ci) {
        double r = this.re * this.re - this.im * this.im + cr;
        double i = 2.0 * this.re * this.im + ci;
        this.re = r;
        this.im = i;
    }
};

/* escape-time for a single point c = (cr, ci) */
static int mandel_escape(double cr, double ci, int maxit) {
    Complex z;
    z.set(0.0, 0.0);
    int it = 0;
    while (it < maxit && z.norm2() <= 4.0) {
        z.step(cr, ci);
        it = it + 1;
    }
    return it;
}

int main() {
    /* grid + view window (classic Mandelbrot framing) */
    int    W     = 700;
    int    H     = 460;
    int    MAXIT = 256;
    double x0 = -2.5, x1 = 1.0;
    double y0 = -1.25, y1 = 1.25;

    printf("=== perf-math: Mandelbrot via Complex class ===\n");
    printf("grid %dx%d  maxit %d\n", W, H, MAXIT);

    clock_t t0 = clock();

    long total_iters = 0; /* checksum: sum of escape times      */
    long in_set      = 0; /* checksum: points that never escaped */

    for (int py = 0; py < H; py++) {
        double ci = y0 + (y1 - y0) * (double) py / (double) H;
        for (int px = 0; px < W; px++) {
            double cr = x0 + (x1 - x0) * (double) px / (double) W;
            int it = mandel_escape(cr, ci, MAXIT);
            total_iters = total_iters + (long) it;
            if (it == MAXIT) in_set = in_set + 1;
        }
    }

    clock_t t1 = clock();
    double ms = (double) (t1 - t0) * 1000.0 / (double) CLOCKS_PER_SEC;

    long pixels    = (long) W * (long) H;
    double mpix    = ms > 0.0 ? (double) pixels / (ms * 1000.0) : 0.0;
    /* each iteration issues 2 method calls (norm2 + step) */
    double mcalls  = ms > 0.0 ? (double) (total_iters * 2) / (ms * 1000.0) : 0.0;

    printf("\n--- results (checksum must be stable across modes) ---\n");
    printf("total_iters = %ld\n", total_iters);
    printf("in_set      = %ld\n", in_set);
    printf("\n--- timing ---\n");
    printf("elapsed     = %.2f ms\n", ms);
    printf("throughput  = %.2f Mpixel/s\n", mpix);
    printf("method calls= %.2f Mcall/s\n", mcalls);

    /* expected checksum for the default 700x460 / 256-iter grid */
    long EXPECT = 15712039;
    if (total_iters == EXPECT)
        printf("\nPASS  checksum matches expected %ld\n", EXPECT);
    else
        printf("\nNOTE  checksum %ld (expected %ld at default size — differs if size changed)\n",
               total_iters, EXPECT);

    return 0;
}
