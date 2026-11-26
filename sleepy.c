/* sleepy.c -- the job that runs inside every child process.
 * It never ends by itself: the dispatcher stops it (SIGTSTP), continues it
 * (SIGCONT) and terminates it (SIGINT). No handlers, so the defaults apply. */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    int job = (argc > 1) ? atoi(argv[1]) : -1;

    setvbuf(stdout, NULL, _IONBF, 0);
    printf("[sleepy] job P%d started, pid=%d, working in 1 second slices\n",
           job, (int)getpid());

    for (;;)
        sleep(1);
}
