/* Q5: dining philosophers with semaphores.
   naive  : pick left fork, pause, pick right fork        -> can deadlock
   room   : at most 4 philosophers may sit (semaphore = 4) -> no deadlock
   ordered: always pick the lower-numbered fork first      -> no deadlock   */
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define N 5
#define MEALS 200

static sem_t fork_[N], room;
static int meals[N];
static int mode;                       /* 0 naive, 1 room, 2 ordered */

static void *philosopher(void *arg)
{
    int i = (int)(long)arg, l = i, r = (i + 1) % N;
    for (int m = 0; m < MEALS; m++) {
        if (mode == 1) sem_wait(&room);
        int first = l, second = r;
        if (mode == 2 && l > r) { first = r; second = l; }
        sem_wait(&fork_[first]);
        usleep(100);                     /* think a little: makes the deadlock likely */
        sem_wait(&fork_[second]);
        meals[i]++; usleep(50);          /* eat */
        sem_post(&fork_[second]);
        sem_post(&fork_[first]);
        if (mode == 1) sem_post(&room);
    }
    return NULL;
}

int main(int argc, char **argv)
{
    const char *names[] = {"naive", "room", "ordered"};
    mode = !strcmp(argv[1], "room") ? 1 : !strcmp(argv[1], "ordered") ? 2 : 0;
    for (int i = 0; i < N; i++) sem_init(&fork_[i], 0, 1);
    sem_init(&room, 0, N - 1);
    pthread_t t[N];
    for (long i = 0; i < N; i++) pthread_create(&t[i], NULL, philosopher, (void *)i);
    for (int i = 0; i < N; i++) pthread_join(t[i], NULL);
    printf("%s: meals eaten =", names[mode]);
    for (int i = 0; i < N; i++) printf(" %d", meals[i]);
    printf("  (all %d philosophers finished)\n", N);
    return 0;
}
