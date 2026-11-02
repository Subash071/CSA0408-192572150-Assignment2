/* Q5 deployment decision: seat booking with one global lock, one lock per train,
   one lock per coach, or no lock.  A booking holds the lock while it "writes the
   ticket to the database" (usleep(50)).                                        */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>

#define TRAINS   16
#define COACHES  8
#define SEATS    50
#define AGENTS   32
#define ATTEMPTS 300

static unsigned char sold[TRAINS][COACHES][SEATS];
static atomic_int    sales[TRAINS][COACHES][SEATS];   /* how many times a seat was sold */
static pthread_mutex_t global_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t train_lock[TRAINS];
static pthread_mutex_t coach_lock[TRAINS][COACHES];
static int mode;                                      /* 0 none 1 global 2 train 3 coach */
static atomic_long ok;

static double now(void)
{
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}

static pthread_mutex_t *pick_lock(int t, int c)
{
    if (mode == 1) return &global_lock;
    if (mode == 2) return &train_lock[t];
    if (mode == 3) return &coach_lock[t][c];
    return NULL;
}

static void *agent(void *arg)
{
    unsigned seed = (unsigned)(long)arg * 7919u + 13;
    for (int k = 0; k < ATTEMPTS; k++) {
        int t = rand_r(&seed) % TRAINS, c = rand_r(&seed) % COACHES, s = rand_r(&seed) % SEATS;
        pthread_mutex_t *l = pick_lock(t, c);
        if (l) pthread_mutex_lock(l);
        if (!sold[t][c][s]) {                 /* check ...                          */
            usleep(50);                       /* ... write to the database ...       */
            sold[t][c][s] = 1;                /* ... then act (check-then-act)       */
            atomic_fetch_add(&sales[t][c][s], 1);
            atomic_fetch_add(&ok, 1);
        }
        if (l) pthread_mutex_unlock(l);
    }
    return NULL;
}

int main(int argc, char **argv)
{
    mode = atoi(argv[1]);
    for (int t = 0; t < TRAINS; t++) {
        pthread_mutex_init(&train_lock[t], NULL);
        for (int c = 0; c < COACHES; c++) pthread_mutex_init(&coach_lock[t][c], NULL);
    }
    pthread_t th[AGENTS];
    double t0 = now();
    for (long i = 0; i < AGENTS; i++) pthread_create(&th[i], NULL, agent, (void *)i);
    for (int i = 0; i < AGENTS; i++) pthread_join(th[i], NULL);
    double dt = now() - t0;
    int over = 0, extra = 0;
    for (int t = 0; t < TRAINS; t++) for (int c = 0; c < COACHES; c++) for (int s = 0; s < SEATS; s++) {
        int n = sales[t][c][s]; if (n > 1) { over++; extra += n - 1; }
    }
    static const char *nm[] = {"no lock", "one global lock", "one lock per train", "one lock per coach"};
    printf("%-20s time=%7.1f ms  throughput=%7.0f requests/s  tickets sold=%ld  seats sold twice=%d (extra tickets %d)\n",
           nm[mode], dt * 1e3, AGENTS * ATTEMPTS / dt, (long)ok, over, extra);
    return 0;
}
