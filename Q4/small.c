/* Q4: time to transfer n small readings (one int each) between two processes */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/wait.h>

static double now(void)
{
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}

static void pipe_each(int n)                       /* one write()/read() per reading */
{
    int fd[2]; pipe(fd);
    if (fork() == 0) { close(fd[1]); int v; long s = 0; while (read(fd[0], &v, sizeof v) == sizeof v) s += v; _exit(0); }
    close(fd[0]);
    for (int i = 0; i < n; i++) write(fd[1], &i, sizeof i);
    close(fd[1]); wait(NULL);
}

struct slot  { sem_t empty, full; int value; int done; };
static void shm_slot(int n)                        /* one-slot mailbox, 2 sem ops per reading */
{
    struct slot *s = mmap(NULL, sizeof *s, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_init(&s->empty, 1, 1); sem_init(&s->full, 1, 0); s->done = 0;
    if (fork() == 0) {
        long sum = 0;
        for (;;) { sem_wait(&s->full); if (s->done) break; sum += s->value; sem_post(&s->empty); }
        _exit(0);
    }
    for (int i = 0; i < n; i++) { sem_wait(&s->empty); s->value = i; sem_post(&s->full); }
    sem_wait(&s->empty); s->done = 1; sem_post(&s->full);
    wait(NULL);
}

#define RING 1024
struct ring { sem_t empty, full; int buf[RING]; };
static void shm_ring(int n)                        /* ring buffer of 1024 readings */
{
    struct ring *r = mmap(NULL, sizeof *r, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_init(&r->empty, 1, RING); sem_init(&r->full, 1, 0);
    if (fork() == 0) {
        long sum = 0;
        for (int i = 0; i < n; i++) { sem_wait(&r->full); sum += r->buf[i % RING]; sem_post(&r->empty); }
        _exit(0);
    }
    for (int i = 0; i < n; i++) { sem_wait(&r->empty); r->buf[i % RING] = i; sem_post(&r->full); }
    wait(NULL);
}

int main(void)
{
    int sizes[] = {10000, 100000, 1000000};
    const char *name[] = {"pipe, 1 write per reading", "shm, one-slot mailbox", "shm, ring of 1024"};
    void (*fn[])(int) = {pipe_each, shm_slot, shm_ring};
    printf("%-27s %10s %12s %14s\n", "method", "readings", "time (ms)", "ns per reading");
    for (int m = 0; m < 3; m++)
        for (int s = 0; s < 3; s++) {
            double sum = 0;
            for (int k = 0; k < 5; k++) { double t0 = now(); fn[m](sizes[s]); sum += now() - t0; }
            double mean = sum / 5;
            printf("%-27s %10d %12.2f %14.0f\n", name[m], sizes[s], mean * 1e3, mean * 1e9 / sizes[s]);
        }
    return 0;
}
