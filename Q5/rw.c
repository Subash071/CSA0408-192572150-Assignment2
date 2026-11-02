#define _DEFAULT_SOURCE
/* Q5: readers-writers (readers preference) with semaphores.
   A checker verifies: never a writer together with a reader or another writer. */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

static sem_t rw_mutex;                 /* writers (and the first/last reader) hold this */
static sem_t mutex;                    /* protects read_count                           */
static int read_count = 0, active_readers = 0, active_writers = 0, violations = 0;
static int shared_data = 0, reads_done = 0, writes_done = 0;
static pthread_mutex_t chk = PTHREAD_MUTEX_INITIALIZER;

static void *writer(void *a)
{
    (void)a;
    for (int i = 0; i < 200; i++) {
        sem_wait(&rw_mutex);
        pthread_mutex_lock(&chk); active_writers++;
        if (active_writers > 1 || active_readers > 0) violations++;
        pthread_mutex_unlock(&chk);
        shared_data++; usleep(100);
        pthread_mutex_lock(&chk); active_writers--; writes_done++; pthread_mutex_unlock(&chk);
        sem_post(&rw_mutex);
        usleep(200);
    }
    return NULL;
}
static void *reader(void *a)
{
    (void)a;
    for (int i = 0; i < 400; i++) {
        sem_wait(&mutex);
        if (++read_count == 1) sem_wait(&rw_mutex);     /* first reader locks writers out */
        sem_post(&mutex);
        pthread_mutex_lock(&chk); active_readers++;
        if (active_writers > 0) violations++;
        pthread_mutex_unlock(&chk);
        volatile int v = shared_data; (void)v; usleep(50);
        pthread_mutex_lock(&chk); active_readers--; reads_done++; pthread_mutex_unlock(&chk);
        sem_wait(&mutex);
        if (--read_count == 0) sem_post(&rw_mutex);     /* last reader lets writers in */
        sem_post(&mutex);
        usleep(100);
    }
    return NULL;
}

int main(void)
{
    sem_init(&rw_mutex, 0, 1); sem_init(&mutex, 0, 1);
    pthread_t r[5], w[2];
    for (int i = 0; i < 5; i++) pthread_create(&r[i], NULL, reader, NULL);
    for (int i = 0; i < 2; i++) pthread_create(&w[i], NULL, writer, NULL);
    for (int i = 0; i < 5; i++) pthread_join(r[i], NULL);
    for (int i = 0; i < 2; i++) pthread_join(w[i], NULL);
    printf("readers-writers: reads=%d writes=%d shared_data=%d (expected 400) violations=%d\n",
           reads_done, writes_done, shared_data, violations);
    return 0;
}
