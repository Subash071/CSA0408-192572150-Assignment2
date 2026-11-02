/* Q1: create child processes with fork() and observe them with ps and /proc */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define CHILDREN 3

static void show_proc(pid_t pid)
{
    char path[64], line[256];
    snprintf(path, sizeof path, "/proc/%d/status", (int)pid);
    FILE *f = fopen(path, "r");
    if (!f) { printf("  /proc/%d gone\n", (int)pid); return; }
    while (fgets(line, sizeof line, f))
        if (!strncmp(line, "Name:", 5)  || !strncmp(line, "State:", 6) ||
            !strncmp(line, "Pid:", 4)   || !strncmp(line, "PPid:", 5)  ||
            !strncmp(line, "VmRSS:", 6) || !strncmp(line, "voluntary_ctxt", 14))
            printf("  %s", line);
    fclose(f);
}

int main(void)
{
    pid_t kids[CHILDREN + 1];
    printf("parent pid = %d\n", (int)getpid());
    for (int i = 0; i < CHILDREN; i++) {
        pid_t pid = fork();
        if (pid < 0) { perror("fork"); exit(1); }
        if (pid == 0) {                       /* child */
            if (i == 0) {                     /* child 0: CPU work -> R */
                volatile unsigned long x = 0;
                for (unsigned long k = 0; k < 3000000000UL; k++) x += k;
            } else {                          /* child 1,2: sleep -> S  */
                sleep(4);
            }
            _exit(i);
        }
        kids[i] = pid;
    }
    /* extra child exits at once: stays a zombie (Z) until reaped */
    pid_t z = fork();
    if (z == 0) _exit(0);
    kids[CHILDREN] = z;

    sleep(1);                                 /* let the states settle */
    printf("\n--- ps view ---\n");
    char cmd[128];
    snprintf(cmd, sizeof cmd,
             "ps -o pid,ppid,stat,wchan:12,comm --ppid %d", (int)getpid());
    fflush(stdout);
    system(cmd);

    printf("\n--- /proc view of each child ---\n");
    for (int i = 0; i <= CHILDREN; i++) {
        printf("child %d (pid %d):\n", i, (int)kids[i]);
        show_proc(kids[i]);
    }
    fflush(stdout);
    for (int i = 0; i <= CHILDREN; i++) {
        int st; waitpid(kids[i], &st, 0);
        printf("reaped pid %d, exit status %d\n", (int)kids[i], WEXITSTATUS(st));
    }
    return 0;
}
