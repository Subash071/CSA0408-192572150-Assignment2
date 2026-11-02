/* Q5 alternative concept: cost of protecting one shared counter with
   mutex | spin lock | atomic_fetch_add | binary semaphore                       */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <sys/resource.h>
#include <time.h>

static double now(void)
{
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}
static double cpu_time(void)
{
    struct rusage u; getrusage(RUSAGE_SELF, &u);
    return u.ru_utime.tv_sec + u.ru_stime.tv_sec + (u.ru_utime.tv_usec + u.ru_stime.tv_usec) / 1e6;
}

static int method, cs_us;
static long per_thread;
static long counter = 0;
static atomic_long acounter, spin_fail;
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static atomic_flag spin = ATOMIC_FLAG_INIT;
static sem_t sem;

static void *worker(void *a)
{
    (void)a;
    long fails = 0;
    for (long i = 0; i < per_thread; i++) {
        switch (method) {
        case 0: pthread_mutex_lock(&mtx); counter++; if (cs_us) usleep(cs_us); pthread_mutex_unlock(&mtx); break;
        case 1: while (atomic_flag_test_and_set_explicit(&spin, memory_order_acquire)) fails++;
                counter++; if (cs_us) usleep(cs_us);
                atomic_flag_clear_explicit(&spin, memory_order_release); break;
        case 2: atomic_fetch_add(&acounter, 1); break;
        case 3: sem_wait(&sem); counter++; if (cs_us) usleep(cs_us); sem_post(&sem); break;
        }
    }
    atomic_fetch_add(&spin_fail, fails);
    return NULL;
}

int main(int argc, char **argv)
{
    method = atoi(argv[1]); int th = atoi(argv[2]); cs_us = atoi(argv[3]); long total = atol(argv[4]);
    per_thread = total / th; sem_init(&sem, 0, 1);
    pthread_t t[64];
    double w0 = now(), c0 = cpu_time();
    for (int i = 0; i < th; i++) pthread_create(&t[i], NULL, worker, NULL);
    for (int i = 0; i < th; i++) pthread_join(t[i], NULL);
    double w = now() - w0, c = cpu_time() - c0;
    static const char *nm[] = {"mutex", "spin lock", "atomic add", "semaphore"};
    long done = per_thread * th, val = method == 2 ? (long)acounter : counter;
    printf("%-10s threads=%-2d cs=%3dus  wall=%9.2f ms  ns/update=%9.0f  cpu=%9.2f ms (%3.0f%% of wall)  spins/update=%8.2f  %s\n",
           nm[method], th, cs_us, w * 1e3, w * 1e9 / done, c * 1e3, 100 * c / w, (double)spin_fail / done,
           val == done ? "correct" : "WRONG");
    return 0;
}
