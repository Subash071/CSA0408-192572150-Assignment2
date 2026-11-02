#define _POSIX_C_SOURCE 200809L
/* Q2 alternative: cost of ONE scheduling decision.
   SJF  : scan all n ready jobs for the smallest burst   -> O(n)
   RR / MLFQ queue : take the job at the head of a queue  -> O(1)      */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static double now(void)
{
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}

int main(void)
{
    int sizes[] = {100, 1000, 10000, 100000};
    printf("%8s %18s %18s\n", "n", "SJF scan (ns)", "queue pop (ns)");
    for (int s = 0; s < 4; s++) {
        int n = sizes[s];
        int *burst = malloc(n * sizeof(int));
        int *queue = malloc(n * sizeof(int));
        for (int i = 0; i < n; i++) { burst[i] = rand() % 1000 + 1; queue[i] = i; }
        int reps = 20000000 / n; if (reps < 50) reps = 50;
        volatile int sink = 0;
        double t0 = now();
        for (int r = 0; r < reps; r++) {            /* SJF: find min burst */
            int best = 0;
            for (int i = 1; i < n; i++) if (burst[i] < burst[best]) best = i;
            sink += best; burst[best]++;
        }
        double t1 = now();
        int head = 0;
        long ops = 20000000;
        for (long r = 0; r < ops; r++) {            /* queue: pop head, push back */
            int p = queue[head]; head = (head + 1) % n; sink += p;
        }
        double t2 = now();
        printf("%8d %18.0f %18.2f\n", n, (t1 - t0) * 1e9 / reps, (t2 - t1) * 1e9 / ops);
        free(burst); free(queue);
    }
    return 0;
}
