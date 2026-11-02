/* Q3: 64 traffic cameras. Each camera delivers FRAMES frames. One frame =
   wait for the camera (I/O, io_us) + read the plate (CPU, about 1 ms).
   modes: seq | percam | pool K | proc K                                  */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>
#include <sys/wait.h>

#define CAMERAS 64
#define FRAMES  10

static long cpu_iters;
static int io_us;
static int next_cam = 0;
static pthread_mutex_t qlock = PTHREAD_MUTEX_INITIALIZER;
static atomic_ulong total_plates;

static double now(void)
{
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}

static unsigned long process_frame(int cam, int f)       /* CPU part */
{
    volatile unsigned long x = cam * 1000 + f;
    for (long i = 0; i < cpu_iters; i++) x = x * 1664525UL + 1013904223UL;
    return x & 1;                                          /* "plate found" */
}

static unsigned long handle_camera(int cam)
{
    unsigned long plates = 0;
    for (int f = 0; f < FRAMES; f++) {
        if (io_us) usleep(io_us);                          /* I/O part */
        plates += process_frame(cam, f);
    }
    return plates;
}

static void calibrate(int target_us)
{
    cpu_iters = 1000000;
    double t0 = now(); process_frame(0, 0); double t1 = now();
    cpu_iters = (long)(1000000.0 * target_us / ((t1 - t0) * 1e6));
}

static void *camera_thread(void *arg)                      /* one per camera */
{
    atomic_fetch_add(&total_plates, handle_camera((int)(long)arg));
    return NULL;
}

static void *pool_worker(void *arg)                        /* pool of K */
{
    (void)arg;
    for (;;) {
        pthread_mutex_lock(&qlock);
        int cam = next_cam < CAMERAS ? next_cam++ : -1;    /* take next job */
        pthread_mutex_unlock(&qlock);
        if (cam < 0) break;
        atomic_fetch_add(&total_plates, handle_camera(cam));
    }
    return NULL;
}

int main(int argc, char **argv)
{
    if (argc < 4) { fprintf(stderr, "usage: cams mode K io_us\n"); return 1; }
    const char *mode = argv[1]; int K = atoi(argv[2]); io_us = atoi(argv[3]);
    calibrate(1000);
    double t0 = now();
    if (!strcmp(mode, "seq")) {
        for (int c = 0; c < CAMERAS; c++) total_plates += handle_camera(c);
    } else if (!strcmp(mode, "percam")) {
        pthread_t th[CAMERAS];
        for (long c = 0; c < CAMERAS; c++) pthread_create(&th[c], NULL, camera_thread, (void *)c);
        for (int c = 0; c < CAMERAS; c++) pthread_join(th[c], NULL);
    } else if (!strcmp(mode, "pool")) {
        pthread_t th[256];
        for (int i = 0; i < K; i++) pthread_create(&th[i], NULL, pool_worker, NULL);
        for (int i = 0; i < K; i++) pthread_join(th[i], NULL);
    } else if (!strcmp(mode, "proc")) {
        int fd[2]; pipe(fd);
        for (int i = 0; i < K; i++) {
            if (fork() == 0) {
                unsigned long s = 0;
                for (int c = i; c < CAMERAS; c += K) s += handle_camera(c);
                write(fd[1], &s, sizeof s); _exit(0);
            }
        }
        close(fd[1]);
        unsigned long s;
        while (read(fd[0], &s, sizeof s) == sizeof s) total_plates += s;
        while (wait(NULL) > 0) ;
    }
    double t1 = now();
    printf("%-7s K=%-3d io=%dus  time=%8.1f ms  plates=%lu\n",
           mode, K, io_us, (t1 - t0) * 1e3, (unsigned long)total_plates);
    return 0;
}
