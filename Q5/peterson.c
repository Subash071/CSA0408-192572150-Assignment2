/* Q5: Peterson's solution for two threads (ids 0 and 1) protecting a counter.
   -DPLAIN  : ordinary int variables (the textbook version)
   default  : C11 sequentially consistent atomics                          */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <pthread.h>
#include <stdatomic.h>
#include <sched.h>

#ifndef LOOPS
#define LOOPS 1000000
#endif

#ifdef PLAIN
static int flag[2] = {0, 0};
static int turn = 0;
#define SET_FLAG(i, v) (flag[i] = (v))
#define GET_FLAG(i)    (flag[i])
#define SET_TURN(v)    (turn = (v))
#define GET_TURN()     (turn)
#else
static atomic_int flag[2];
static atomic_int turn;
#define SET_FLAG(i, v) atomic_store(&flag[i], (v))
#define GET_FLAG(i)    atomic_load(&flag[i])
#define SET_TURN(v)    atomic_store(&turn, (v))
#define GET_TURN()     atomic_load(&turn)
#endif

static long counter = 0;

static void *worker(void *arg)
{
    int i = (int)(long)arg, j = 1 - i;
    for (int k = 0; k < LOOPS; k++) {
        SET_FLAG(i, 1);                         /* I want to enter            */
        SET_TURN(j);                            /* but you go first if you want */
        while (GET_FLAG(j) && GET_TURN() == j)  /* busy wait                  */
#ifdef YIELD
            sched_yield();                      /* give the CPU to the other thread */
#else
            ;
#endif
        counter++;                              /* critical section           */
        SET_FLAG(i, 0);                         /* exit                       */
    }
    return NULL;
}

int main(void)
{
    pthread_t t0, t1;
    pthread_create(&t0, NULL, worker, (void *)0L);
    pthread_create(&t1, NULL, worker, (void *)1L);
    pthread_join(t0, NULL); pthread_join(t1, NULL);
    printf("counter = %ld (expected %d)\n", counter, 2 * LOOPS);
    return 0;
}
