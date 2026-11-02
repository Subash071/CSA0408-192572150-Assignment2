/* Q5: bounded buffer as a MONITOR (one mutex + two condition variables).
   The mutex is taken on entry to put()/get(); no semaphore counting is needed. */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>

#define SIZE 5
#define TOTAL 2000

static struct {
    int buf[SIZE], in, out, count;
    pthread_mutex_t m;
    pthread_cond_t not_full, not_empty;
} mon = {.m = PTHREAD_MUTEX_INITIALIZER, .not_full = PTHREAD_COND_INITIALIZER,
         .not_empty = PTHREAD_COND_INITIALIZER};

static void put(int item)
{
    pthread_mutex_lock(&mon.m);
    while (mon.count == SIZE) pthread_cond_wait(&mon.not_full, &mon.m);
    mon.buf[mon.in] = item; mon.in = (mon.in + 1) % SIZE; mon.count++;
    pthread_cond_signal(&mon.not_empty);
    pthread_mutex_unlock(&mon.m);
}
static int get(void)
{
    pthread_mutex_lock(&mon.m);
    while (mon.count == 0) pthread_cond_wait(&mon.not_empty, &mon.m);
    int item = mon.buf[mon.out]; mon.out = (mon.out + 1) % SIZE; mon.count--;
    pthread_cond_signal(&mon.not_full);
    pthread_mutex_unlock(&mon.m);
    return item;
}

static atomic_int seen[TOTAL + 1];
static int np;
static void *producer(void *a) { int id = (int)(long)a, share = TOTAL / np; for (int k = 0; k < share; k++) put(id * share + k + 1); return NULL; }
static void *consumer(void *a) { (void)a; for (int k = 0; k < TOTAL / np; k++) atomic_fetch_add(&seen[get()], 1); return NULL; }

int main(int argc, char **argv)
{
    np = argc > 1 ? atoi(argv[1]) : 1;
    pthread_t p[16], c[16];
    for (long i = 0; i < np; i++) { pthread_create(&p[i], NULL, producer, (void *)i); pthread_create(&c[i], NULL, consumer, NULL); }
    for (int i = 0; i < np; i++) { pthread_join(p[i], NULL); pthread_join(c[i], NULL); }
    int once = 0; for (int i = 1; i <= TOTAL; i++) if (seen[i] == 1) once++;
    printf("monitor, P=%d C=%d: %d of %d items consumed exactly once %s\n", np, np, once, TOTAL, once == TOTAL ? "OK" : "WRONG");
    return 0;
}
