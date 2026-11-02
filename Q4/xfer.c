/* Q4 growth investigation: time to move 1, 10 and 100 MB between two processes
   with (1) a pipe, (2) shared memory + semaphores, (3) a POSIX message queue. */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <semaphore.h>
#include <mqueue.h>
#include <sys/mman.h>
#include <sys/wait.h>

#define CHUNK  (64 * 1024)      /* pipe and shared-memory chunk */
#define MSGSZ  8192             /* default POSIX mq message size limit */
#define SLOTS  8

static double now(void)
{
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}
static unsigned long sum_bytes(const unsigned char *p, size_t n)   /* use the data */
{
    unsigned long s = 0; const unsigned long *w = (const unsigned long *)p;
    for (size_t i = 0; i < n / 8; i++) s += w[i];
    return s;
}

static void by_pipe(size_t total)
{
    int fd[2]; pipe(fd);
    pid_t pid = fork();
    if (pid == 0) {
        close(fd[1]);
        unsigned char *buf = malloc(CHUNK); size_t got = 0; unsigned long s = 0; ssize_t r;
        while (got < total && (r = read(fd[0], buf, CHUNK)) > 0) { s += sum_bytes(buf, r); got += r; }
        _exit(got == total ? 0 : 1);
    }
    close(fd[0]);
    unsigned char *buf = malloc(CHUNK);
    for (size_t sent = 0; sent < total; sent += CHUNK) {
        memset(buf, 7, CHUNK);                       /* produce the data */
        size_t off = 0; while (off < CHUNK) off += write(fd[1], buf + off, CHUNK - off);
    }
    close(fd[1]);
    int st; waitpid(pid, &st, 0);
    if (WEXITSTATUS(st)) fprintf(stderr, "pipe: size mismatch\n");
}

struct ring { sem_t empty, full; unsigned char slot[SLOTS][CHUNK]; };
static void by_shm(size_t total)
{
    struct ring *r = mmap(NULL, sizeof *r, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_init(&r->empty, 1, SLOTS); sem_init(&r->full, 1, 0);
    size_t chunks = total / CHUNK;
    pid_t pid = fork();
    if (pid == 0) {
        unsigned long s = 0;
        for (size_t i = 0; i < chunks; i++) {
            sem_wait(&r->full);
            s += sum_bytes(r->slot[i % SLOTS], CHUNK);     /* read in place, no copy */
            sem_post(&r->empty);
        }
        _exit(0);
    }
    for (size_t i = 0; i < chunks; i++) {
        sem_wait(&r->empty);
        memset(r->slot[i % SLOTS], 7, CHUNK);              /* produce directly in shared memory */
        sem_post(&r->full);
    }
    waitpid(pid, NULL, 0);
    munmap(r, sizeof *r);
}

static void by_mq(size_t total)
{
    struct mq_attr at = {0}; at.mq_maxmsg = 10; at.mq_msgsize = MSGSZ;
    mq_unlink("/xfer_q");
    mqd_t q = mq_open("/xfer_q", O_CREAT | O_RDWR, 0600, &at);
    if (q == (mqd_t)-1) { perror("mq_open"); exit(1); }
    size_t msgs = total / MSGSZ;
    pid_t pid = fork();
    if (pid == 0) {
        unsigned char *buf = malloc(MSGSZ); unsigned long s = 0;
        for (size_t i = 0; i < msgs; i++) { mq_receive(q, (char *)buf, MSGSZ, NULL); s += sum_bytes(buf, MSGSZ); }
        _exit(0);
    }
    unsigned char *buf = malloc(MSGSZ);
    for (size_t i = 0; i < msgs; i++) { memset(buf, 7, MSGSZ); mq_send(q, (char *)buf, MSGSZ, 0); }
    waitpid(pid, NULL, 0);
    mq_close(q); mq_unlink("/xfer_q");
}

int main(int argc, char **argv)
{
    int reps = argc > 1 ? atoi(argv[1]) : 5;
    size_t sizes[] = {1u << 20, 10u << 20, 100u << 20};
    const char *names[] = {"pipe", "shared memory", "message queue"};
    void (*fn[])(size_t) = {by_pipe, by_shm, by_mq};
    printf("%-14s %8s %12s %10s\n", "method", "size", "time (ms)", "MB/s");
    for (int m = 0; m < 3; m++)
        for (int s = 0; s < 3; s++) {
            double best = 1e9, sum = 0;
            for (int k = 0; k < reps; k++) {
                double t0 = now(); fn[m](sizes[s]); double t = now() - t0;
                sum += t; if (t < best) best = t;
            }
            double mean = sum / reps;
            printf("%-14s %6zuMB %12.2f %10.0f   (best %.2f ms)\n", names[m], sizes[s] >> 20,
                   mean * 1e3, (sizes[s] >> 20) / mean, best * 1e3);
        }
    return 0;
}
