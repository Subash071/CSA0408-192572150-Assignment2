/* Q6 D/E: running time of the Banker's safety algorithm against n and m.
   worst case: processes can only finish in reverse order (n passes of n checks)
   best case : processes finish in index order (one pass)                        */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static double now(void)
{
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}

static int n, m;
static int *avail, *maxm, *alloc;

static int is_safe(int *seq)
{
    int *work = malloc(m * sizeof(int)); char *finish = calloc(n, 1); int count = 0;
    memcpy(work, avail, m * sizeof(int));
    while (count < n) {
        int found = 0;
        for (int p = 0; p < n; p++) {
            if (finish[p]) continue;
            int ok = 1;
            for (int r = 0; r < m; r++)
                if (maxm[p * m + r] - alloc[p * m + r] > work[r]) { ok = 0; break; }
            if (ok) {
                for (int r = 0; r < m; r++) work[r] += alloc[p * m + r];
                finish[p] = 1; seq[count++] = p; found = 1;
            }
        }
        if (!found) { free(work); free(finish); return 0; }
    }
    free(work); free(finish); return 1;
}

static void build(int worst)
{
    avail = calloc(m, sizeof(int)); maxm = calloc((size_t)n * m, sizeof(int)); alloc = calloc((size_t)n * m, sizeof(int));
    int last = m - 1;                         /* binding resource = last column */
    avail[last] = 1;
    for (int i = 0; i < n; i++) {
        alloc[i * m + last] = 1;
        int need = worst ? n - i : i + 1;     /* worst: P(n-1) needs 1, P(n-2) needs 2 ... */
        maxm[i * m + last] = 1 + need;
    }
}

int main(void)
{
    int ns[] = {10, 100, 1000}, ms[] = {3, 6, 12};
    printf("%-6s %6s %6s %14s %16s %s\n", "case", "n", "m", "time (us)", "time/(m*n^2) ns", "safe?");
    for (int w = 1; w >= 0; w--)
        for (int a = 0; a < 3; a++)
            for (int b = 0; b < (a == 2 ? 3 : 1); b++) {
                n = ns[a]; m = ms[b];
                build(w);
                int *seq = malloc(n * sizeof(int));
                long reps = 1; double t = 0;
                while (1) {
                    double t0 = now(); int ok = 0;
                    for (long r = 0; r < reps; r++) ok = is_safe(seq);
                    t = now() - t0;
                    if (t > 0.2) { printf("%-6s %6d %6d %14.2f %16.3f %s\n", w ? "worst" : "best", n, m,
                                          t / reps * 1e6, t / reps * 1e9 / ((double)m * n * n), ok ? "yes" : "NO"); break; }
                    reps *= 4;
                }
                free(seq); free(avail); free(maxm); free(alloc);
            }
    return 0;
}
