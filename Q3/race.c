/* Q3: shared counter updated by threads with and without protection */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <sched.h>

#define PER_THREAD 1000000

static volatile long unsafe_count = 0;        /* no protection            */
static volatile long window_count = 0;        /* no protection, window widened */
static long locked_count = 0;                 /* protected by a mutex     */
static atomic_long atomic_count = 0;          /* atomic increment         */
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static void *work(void *arg)
{
    (void)arg;
    for (int i = 0; i < PER_THREAD; i++) {
        unsafe_count++;                       /* load, add, store: not atomic */
        if (i % 100 == 0) {                   /* same update, but let another thread
                                                 run between the load and the store */
            long tmp = window_count; sched_yield(); window_count = tmp + 1;
        } else window_count++;
        pthread_mutex_lock(&lock); locked_count++; pthread_mutex_unlock(&lock);
        atomic_fetch_add(&atomic_count, 1);
    }
    return NULL;
}

int main(int argc, char **argv)
{
    int n = argc > 1 ? atoi(argv[1]) : 4;
    pthread_t th[64];
    for (int i = 0; i < n; i++) pthread_create(&th[i], NULL, work, NULL);
    for (int i = 0; i < n; i++) pthread_join(th[i], NULL);
    long expect = (long)n * PER_THREAD;
    printf("threads=%d expected=%ld\n  plain counter++ : %ld (lost %ld)\n  widened window  : %ld (lost %ld)\n  mutex           : %ld\n  atomic          : %ld\n",
           n, expect, unsafe_count, expect - unsafe_count,
           window_count, expect - window_count, locked_count, (long)atomic_count);
    return 0;
}
