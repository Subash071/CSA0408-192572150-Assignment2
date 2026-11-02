/* Q6 alternative concept: deadlock DETECTION and RECOVERY for the laboratory cluster.
   Resources: GPU (2 units), LIC = simulation licence (1), STO = high-speed storage (1).
   The detection algorithm is the safety algorithm, but it uses the CURRENT REQUESTS
   instead of the maximum claims, so no advance knowledge is needed.            */
#include <stdio.h>
#include <string.h>

#define J 4
#define R 3
static const char *jname[J] = {"J1", "J2", "J3", "J4"};
static int total[R] = {2, 1, 1};
static int alloc[J][R]   = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 0}};
static int request[J][R] = {{0, 1, 0}, {0, 0, 1}, {1, 0, 0}, {0, 1, 0}};
static double hours_run[J] = {5.0, 1.0, 3.0, 0.5};          /* work done so far */

/* returns number of deadlocked jobs; marks them in dead[] */
static int detect(int dead[J])
{
    int work[R], finish[J] = {0};
    for (int r = 0; r < R; r++) { work[r] = total[r]; for (int j = 0; j < J; j++) work[r] -= alloc[j][r]; }
    for (int j = 0; j < J; j++) {                            /* a job holding nothing is not blocking anyone */
        int holds = 0; for (int r = 0; r < R; r++) holds |= alloc[j][r];
        if (!holds) finish[j] = 1;
    }
    for (int changed = 1; changed; ) {
        changed = 0;
        for (int j = 0; j < J; j++) {
            if (finish[j]) continue;
            int ok = 1; for (int r = 0; r < R; r++) if (request[j][r] > work[r]) ok = 0;
            if (ok) { for (int r = 0; r < R; r++) work[r] += alloc[j][r]; finish[j] = 1; changed = 1; }
        }
    }
    int n = 0; for (int j = 0; j < J; j++) { dead[j] = !finish[j]; n += dead[j]; }
    return n;
}

int main(void)
{
    int dead[J];
    int n = detect(dead);
    printf("detection: %d deadlocked jobs:", n);
    for (int j = 0; j < J; j++) if (dead[j]) printf(" %s", jname[j]);
    printf("\n");
    while (n > 0) {                                           /* recovery: abort cheapest victim */
        int v = -1;
        for (int j = 0; j < J; j++) if (dead[j] && (v < 0 || hours_run[j] < hours_run[v])) v = j;
        printf("recovery : abort %s (loses %.1f h of work), releases its resources\n", jname[v], hours_run[v]);
        for (int r = 0; r < R; r++) { alloc[v][r] = 0; request[v][r] = 0; }
        n = detect(dead);
        printf("detection: %d deadlocked jobs left\n", n);
    }
    return 0;
}
