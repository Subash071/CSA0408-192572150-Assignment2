/* Q4 alternative: same weather exchange, using shared memory + semaphores */
#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <semaphore.h>

struct shared {
    sem_t empty, full;       /* empty: slot may be written, full: slot holds a reading */
    int value;               /* the one-slot "mailbox" */
    int done;                /* set by the writer after the last reading */
};

int main(void)
{
    struct shared *s = mmap(NULL, sizeof *s, PROT_READ | PROT_WRITE,
                            MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_init(&s->empty, 1, 1);          /* pshared = 1: shared between processes */
    sem_init(&s->full, 1, 0);
    s->done = 0;
    pid_t pid = fork();
    if (pid == 0) {                                  /* analyser */
        int count = 0, sum = 0, max = 0;
        for (;;) {
            sem_wait(&s->full);
            if (s->done) break;
            int r = s->value;
            count++; sum += r; if (count == 1 || r > max) max = r;
            sem_post(&s->empty);
        }
        printf("analyser: %d readings, average %.1f, max %d\n", count,
               count ? (double)sum / count : 0.0, max);
        fflush(stdout);
        _exit(0);
    }
    int readings[] = {31, 33, 36, 34, 41, 38};       /* sensor reader */
    for (int i = 0; i < 6; i++) {
        sem_wait(&s->empty);
        s->value = readings[i];
        sem_post(&s->full);
    }
    sem_wait(&s->empty);
    s->done = 1;
    sem_post(&s->full);
    waitpid(pid, NULL, 0);
    printf("reader: done\n");
    return 0;
}
