/* Q5: bounded buffer with NP producers and NC consumers (same design as bb.c).
   Checks that every item 1..TOTAL is consumed exactly once.
   Compile with -DNOMUTEX to remove the mutex, -DWIDEN to add a yield inside the
   critical section (makes the lost-update interleaving easy to hit).          */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <sched.h>
#include <stdatomic.h>

#define SIZE  5
#define TOTAL 2000

static int buffer[SIZE], in = 0, out = 0;
static sem_t empty, full;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static atomic_int seen[TOTAL + 1];
static atomic_long consumed_sum;
static int np, nc;

#ifdef NOMUTEX
#define LOCK()
#define UNLOCK()
#else
#define LOCK()   pthread_mutex_lock(&lock)
#define UNLOCK() pthread_mutex_unlock(&lock)
#endif
#ifdef WIDEN
#define WINDOW() sched_yield()
#else
#define WINDOW()
#endif

static void *producer(void *arg)
{
    int id = (int)(long)arg, share = TOTAL / np;
    for (int k = 0; k < share; k++) {
        int item = id * share + k + 1;
        sem_wait(&empty);
        LOCK();
        buffer[in] = item;
        WINDOW();
        in = (in + 1) % SIZE;
        UNLOCK();
        sem_post(&full);
    }
    return NULL;
}

static void *consumer(void *arg)
{
    (void)arg;
    int share = TOTAL / nc;
    for (int k = 0; k < share; k++) {
        sem_wait(&full);
        LOCK();
        int item = buffer[out];
        WINDOW();
        out = (out + 1) % SIZE;
        UNLOCK();
        sem_post(&empty);
        atomic_fetch_add(&seen[item], 1);
        atomic_fetch_add(&consumed_sum, item);
    }
    return NULL;
}

int main(int argc, char **argv)
{
    np = nc = argc > 1 ? atoi(argv[1]) : 1;
    sem_init(&empty, 0, SIZE); sem_init(&full, 0, 0);
    pthread_t p[16], c[16];
    for (long i = 0; i < np; i++) pthread_create(&p[i], NULL, producer, (void *)i);
    for (long i = 0; i < nc; i++) pthread_create(&c[i], NULL, consumer, NULL);
    for (int i = 0; i < np; i++) pthread_join(p[i], NULL);
    for (int i = 0; i < nc; i++) pthread_join(c[i], NULL);
    int once = 0, lost = 0, dup = 0;
    for (int i = 1; i <= TOTAL; i++) { int s = seen[i]; if (s == 1) once++; else if (s == 0) lost++; else dup++; }
    long expect = (long)TOTAL * (TOTAL + 1) / 2;
    printf("P=%d C=%d: consumed exactly once=%d  never=%d  more than once=%d  sum=%ld (expected %ld) %s\n",
           np, nc, once, lost, dup, (long)consumed_sum, expect,
           once == TOTAL && consumed_sum == expect ? "OK" : "WRONG");
    return 0;
}
