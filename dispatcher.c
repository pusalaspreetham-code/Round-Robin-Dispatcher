/*
 * dispatcher.c -- the Round Robin engine
 *
 * One loop pass = one second:
 *   1. admit jobs whose arrival <= timer into the ready queue
 *   2. charge the last second to the running job
 *        - finished        -> SIGINT + waitpid
 *        - quantum expired -> SIGTSTP + waitpid, back of the ready queue
 *   3. if the CPU is free, take the front of the ready queue
 *        - first time -> fork() + exec(./sleepy)
 *        - later      -> SIGCONT
 *   4. show the state, record the Gantt column, sleep 1 second, timer++
 */
#include "dispatcher.h"
#include "monitor.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

/* ---------------- real process control ---------------- */

static void start_process(Pcb *p)
{
    char arg[16];
    sprintf(arg, "%d", p->id);

    fflush(stdout);                     /* so the child does not copy our buffer */
    pid_t pid = fork();

    if (pid == 0) {
        setpgid(0, 0);                  /* own process group (needed for SIGTSTP) */
        execl("./sleepy", "sleepy", arg, (char *)NULL);
        perror("exec ./sleepy");
        _exit(127);
    }

    setpgid(pid, pid);                  /* same call in the parent: no race */
    p->pid   = pid;
    p->state = RUNNING;
}

static void resume_process(Pcb *p)
{
    kill(p->pid, SIGCONT);
    p->state = RUNNING;
}

static void suspend_process(Pcb *p)
{
    kill(p->pid, SIGTSTP);
    waitpid(p->pid, NULL, WUNTRACED);   /* wait until it is really stopped */
    p->state = SUSPENDED;
}

static void terminate_process(Pcb *p)
{
    kill(p->pid, SIGINT);
    waitpid(p->pid, NULL, 0);           /* reap it: no zombie */
    p->pid   = 0;
    p->state = TERMINATED;
}

/* ---------------- the steps of one second ---------------- */

static void admit(Dispatcher *d)
{
    while (d->input.head && d->input.head->arrival <= d->timer) {
        Pcb *p = dequeue(&d->input);
        p->state = READY;
        enqueue(&d->ready, p);
        monitor_event(d->timer, p, "ARRIVED", "input queue -> ready queue (back)");
    }
}

static void charge(Dispatcher *d)
{
    Pcb *p = d->running;
    if (!p) return;

    p->remaining--;
    d->quantum_left--;
    d->busy_ticks++;

    if (p->remaining == 0) {                    /* job finished */
        p->finish_time = d->timer;
        monitor_event(d->timer, p, "FINISHED", "sending SIGINT, waiting for it");
        terminate_process(p);
        d->finished++;
        d->running      = NULL;
        d->quantum_left = 0;
        return;
    }

    if (d->quantum_left > 0) return;            /* quantum not over yet */

    if (!d->ready.head) {                       /* nobody waiting: keep the CPU */
        d->quantum_left = d->quantum;
        return;
    }

    monitor_event(d->timer, p, "PREEMPTED", "quantum over, sending SIGTSTP");
    suspend_process(p);
    p->state = READY;
    enqueue(&d->ready, p);
    d->running      = NULL;
    d->quantum_left = 0;
}

static void dispatch(Dispatcher *d)
{
    if (d->running || !d->ready.head) return;

    Pcb *p = dequeue(&d->ready);
    d->running      = p;
    d->quantum_left = d->quantum;
    d->context_switches++;

    if (p->start_time < 0) {                    /* first time on the CPU */
        p->start_time = d->timer;
        monitor_event(d->timer, p, "STARTED", "fork() + exec(./sleepy)");
        start_process(p);
    } else {
        monitor_event(d->timer, p, "RESUMED", "sending SIGCONT");
        resume_process(p);
    }
}

/* ---------------- public functions ---------------- */

void dispatcher_prepare(Dispatcher *d, int quantum)
{
    memset(d, 0, sizeof *d);
    d->quantum = (quantum >= 1) ? quantum : 1;
}

int dispatcher_add_job(Dispatcher *d, Pcb *p)
{
    if (d->njobs >= MAX_JOBS) {
        printf("Cannot add more than %d jobs\n", MAX_JOBS);
        return -1;
    }
    d->all[d->njobs++] = p;
    insert_by_arrival(&d->input, p);
    return 0;
}

int dispatcher_load_jobs(Dispatcher *d, const char *file)
{
    snprintf(d->jobs_file, sizeof d->jobs_file, "%s", file);

    FILE *fp = fopen(file, "r");
    if (!fp) {
        printf("Cannot open job file '%s'\n", file);
        return -1;
    }

    char line[256];
    int  line_no = 0;
    while (fgets(line, sizeof line, fp)) {
        line_no++;

        Pcb *p = process_parse_line(line, d->njobs);
        if (!p) {
            if (line[0] != '\n' && line[0] != '#')      /* not blank / comment */
                printf("jobs file: line %d is invalid -> skipped\n", line_no);
            continue;
        }
        if (dispatcher_add_job(d, p) != 0) {
            free(p);
            break;
        }
    }
    fclose(fp);

    if (d->njobs == 0) {
        printf("'%s' contains no valid job\n", file);
        return -1;
    }
    return 0;
}

void dispatcher_run(Dispatcher *d)
{
    memset(d->gantt, -1, sizeof d->gantt);

    while (d->finished < d->njobs && d->timer < MAX_GANTT) {
        admit(d);                       /* step 1 */
        charge(d);                      /* step 2 */
        if (d->finished >= d->njobs) break;
        dispatch(d);                    /* step 3 */

        /* step 4 */
        monitor_tick(d->timer, d->all, d->njobs, &d->ready, d->running);
        d->gantt[d->timer] = d->running ? d->running->id : -1;
        sleep(1);
        d->timer++;
    }
}

void dispatcher_destroy(Dispatcher *d)
{
    for (int i = 0; i < d->njobs; i++)
        free(d->all[i]);
    d->njobs = 0;
}

/* Ctrl+C: kill every child that is still alive (no orphans, no zombies). */
void dispatcher_kill_all_children(Dispatcher *d)
{
    for (int i = 0; i < d->njobs; i++) {
        Pcb *p = d->all[i];
        if (p->pid > 0) {
            kill(p->pid, SIGKILL);
            waitpid(p->pid, NULL, 0);
        }
    }
}
