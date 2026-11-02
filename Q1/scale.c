/* Q1 evaluation: cost of 2000 processes versus 2000 threads, and the
   cost of one context switch (pipe ping-pong between two tasks).      */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/wait.h>

#define N 2000

static double now(void)
{
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}

static long read_kb(const char *file, const char *key)
{
    char path[64], line[256]; long v = -1;
    snprintf(path, sizeof path, "/proc/%s", file);
    FILE *f = fopen(path, "r"); if (!f) return -1;
    while (fgets(line, sizeof line, f))
        if (!strncmp(line, key, strlen(key))) { v = atol(line + strlen(key)); break; }
    fclose(f); return v;
}

/* ---------- 2000 processes ---------- */
static void run_procs(void)
{
    int gate[2]; pipe(gate);
    pid_t pids[N];
    double t0 = now();
    for (int i = 0; i < N; i++) {
        pids[i] = fork();
        if (pids[i] == 0) { char c; close(gate[1]); read(gate[0], &c, 1); _exit(0); }
    }
    double t1 = now();
    usleep(300000);
    long total_pss = 0;
    char f[64];
    for (int i = 0; i < N; i += 50) {                 /* sample 40 children */
        snprintf(f, sizeof f, "%d/smaps_rollup", (int)pids[i]);
        long p = read_kb(f, "Pss:"); if (p > 0) total_pss += p;
    }
    snprintf(f, sizeof f, "%d/status", (int)pids[0]);
    long rss = read_kb(f, "VmRSS:");
    long pt = 0; char buf[8];
    for (int i = 0; i < 40; i++) {
        snprintf(f, sizeof f, "%d/status", (int)pids[i * 50]);
        pt += read_kb(f, "VmPTE:");
    }
    close(gate[1]);                                    /* release everybody */
    double t2 = now();
    for (int i = 0; i < N; i++) waitpid(pids[i], NULL, 0);
    double t3 = now();
    (void)buf;
    printf("PROC   N=%d create=%.1f ms (%.1f us each)  teardown=%.1f ms\n",
           N, (t1 - t0) * 1e3, (t1 - t0) * 1e6 / N, (t3 - t2) * 1e3);
    printf("PROC   per-process VmRSS=%ld kB, avg Pss=%.0f kB, page-table=%.0f kB\n",
           rss, total_pss / 40.0, pt / 40.0);
    printf("PROC   estimated memory for %d processes (Pss) = %.1f MB\n",
           N, total_pss / 40.0 * N / 1024.0);
}

/* ---------- 2000 threads ---------- */
static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  c = PTHREAD_COND_INITIALIZER;
static int go = 0;
static void *waiter(void *a)
{
    (void)a;
    pthread_mutex_lock(&m);
    while (!go) pthread_cond_wait(&c, &m);
    pthread_mutex_unlock(&m);
    return NULL;
}
static void run_threads(void)
{
    pthread_t th[N]; pthread_attr_t at;
    pthread_attr_init(&at); pthread_attr_setstacksize(&at, 64 * 1024);
    long rss0 = read_kb("self/status", "VmRSS:");
    double t0 = now();
    for (int i = 0; i < N; i++) pthread_create(&th[i], &at, waiter, NULL);
    double t1 = now();
    usleep(300000);
    long rss1 = read_kb("self/status", "VmRSS:");
    pthread_mutex_lock(&m); go = 1; pthread_cond_broadcast(&c); pthread_mutex_unlock(&m);
    double t2 = now();
    for (int i = 0; i < N; i++) pthread_join(th[i], NULL);
    double t3 = now();
    printf("THREAD N=%d create=%.1f ms (%.1f us each)  teardown=%.1f ms\n",
           N, (t1 - t0) * 1e3, (t1 - t0) * 1e6 / N, (t3 - t2) * 1e3);
    printf("THREAD memory added by %d threads (RSS growth) = %.1f MB (%.1f kB each)\n",
           N, (rss1 - rss0) / 1024.0, (double)(rss1 - rss0) / N);
}

/* ---------- context switch cost: ping-pong ---------- */
#define ROUNDS 200000
static int a2b[2], b2a[2];
static void *pong(void *x)
{
    (void)x; char ch;
    for (int i = 0; i < ROUNDS; i++) { read(a2b[0], &ch, 1); write(b2a[1], &ch, 1); }
    return NULL;
}
static void ping(void)
{
    char ch = 'x';
    for (int i = 0; i < ROUNDS; i++) { write(a2b[1], &ch, 1); read(b2a[0], &ch, 1); }
}
static void pingpong(int use_threads)
{
    pipe(a2b); pipe(b2a);
    double t0, t1;
    if (use_threads) {
        pthread_t t; pthread_create(&t, NULL, pong, NULL);
        t0 = now(); ping(); t1 = now(); pthread_join(t, NULL);
    } else {
        pid_t p = fork();
        if (p == 0) { pong(NULL); _exit(0); }
        t0 = now(); ping(); t1 = now(); waitpid(p, NULL, 0);
    }
    /* one round trip = 2 context switches */
    printf("%s ping-pong: %.2f us per switch (incl. 2 syscalls)\n",
           use_threads ? "THREAD" : "PROCESS", (t1 - t0) * 1e6 / (2.0 * ROUNDS));
}

int main(int argc, char **argv)
{
    if (argc < 2) return 1;
    if (!strcmp(argv[1], "proc"))   run_procs();
    if (!strcmp(argv[1], "thread")) run_threads();
    if (!strcmp(argv[1], "switch")) { pingpong(0); pingpong(1); }
    return 0;
}
