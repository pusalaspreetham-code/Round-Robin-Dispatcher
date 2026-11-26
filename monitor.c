/* monitor.c -- prints the live view and writes log.txt / process.log */
#include "monitor.h"

#include <stdarg.h>
#include <string.h>

#define BOLD  "\033[1m"
#define RESET "\033[0m"

static const char *COLOUR[] = { "\033[91m", "\033[92m", "\033[93m",
                                "\033[94m", "\033[95m", "\033[96m" };
#define COLOUR_OF(id) COLOUR[(id) % 6]

static FILE *log_fp  = NULL;        /* log.txt     (plain copy of the screen)   */
static FILE *live_fp = NULL;        /* process.log (for: tail -f process.log)   */

FILE *monitor_log_stream(void) { return log_fp; }

void monitor_init(const char *log_file)
{
    if (log_fp) return;             /* already open */
    log_fp  = fopen(log_file, "w");
    live_fp = fopen("process.log", "w");
}

void monitor_close(void)
{
    if (log_fp)  fclose(log_fp);
    if (live_fp) fclose(live_fp);
    log_fp = live_fp = NULL;
}

/* Print the same text on the screen and into log.txt. */
static void emit(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);

    if (log_fp) {
        va_start(ap, fmt);
        vfprintf(log_fp, fmt, ap);
        va_end(ap);
        fflush(log_fp);
    }
    fflush(stdout);
}

/* The word shown in process.log for each action. */
static const char *live_action(const char *action)
{
    if (!strcmp(action, "ARRIVED"))   return "READY";
    if (!strcmp(action, "STARTED"))   return "CREATED";
    if (!strcmp(action, "PREEMPTED")) return "SUSPENDED";
    if (!strcmp(action, "FINISHED"))  return "COMPLETED";
    return action;
}

static void pid_text(const Pcb *p, char *buf)
{
    if (p->pid > 0) sprintf(buf, "%d", (int)p->pid);
    else            strcpy(buf, "-");
}

void monitor_banner(const char *jobs_file, int njobs, int quantum)
{
    printf(BOLD "=== ROUND ROBIN DISPATCHER (real Linux processes) ===" RESET "\n");
    if (log_fp) fprintf(log_fp, "=== ROUND ROBIN DISPATCHER (real Linux processes) ===\n");

    emit("  jobs file : %s\n", jobs_file);
    emit("  jobs      : %d\n", njobs);
    emit("  quantum   : %d second(s)\n", quantum);
    emit("  children  : fork() + exec(./sleepy), driven by signals\n");
    emit("  signals   : SIGTSTP = suspend, SIGCONT = resume, SIGINT = terminate\n");
    emit("  notation  : rem = CPU seconds still needed, rem(s) in the queue view\n\n");
}

void monitor_event(int timer, const Pcb *p, const char *action, const char *detail)
{
    char pid[16];
    pid_text(p, pid);

    /* screen (coloured) */
    printf("[T=%2d] %s%-4s" RESET " %-11s pid=%-6s %s\n",
           timer, COLOUR_OF(p->id), p->name, action, pid, detail);
    fflush(stdout);

    /* log.txt (plain) */
    if (log_fp) {
        fprintf(log_fp, "[T=%2d] %-4s %-11s pid=%-6s %s\n",
                timer, p->name, action, pid, detail);
        fflush(log_fp);
    }

    /* process.log (second terminal) */
    if (live_fp) {
        fprintf(live_fp, "[T=%2d] PROCESS %-4s %-10s pid=%-6s rem=%d\n",
                timer, p->name, live_action(action), pid, p->remaining);
        fflush(live_fp);
    }
}

void monitor_tick(int timer, Pcb *const *all, int njobs,
                  const Queue *ready, const Pcb *running)
{
    char queue_view[512];
    queue_describe(ready, queue_view, sizeof queue_view);

    /* header: which job is on the CPU */
    if (running) {
        printf("\n" BOLD "---- T=%d ----" RESET " CPU: %s%s" RESET " (pid=%d, rem=%d)\n",
               timer, COLOUR_OF(running->id), running->name,
               (int)running->pid, running->remaining);
        if (log_fp)
            fprintf(log_fp, "\n---- T=%d ---- CPU: %s (pid=%d, rem=%d)\n",
                    timer, running->name, (int)running->pid, running->remaining);
        if (live_fp)
            fprintf(live_fp, "[T=%2d] CPU RUNNING %-4s pid=%-6d rem=%d | ready: %s\n",
                    timer, running->name, (int)running->pid,
                    running->remaining, queue_view);
    } else {
        printf("\n" BOLD "---- T=%d ----" RESET " CPU: (idle)\n", timer);
        if (log_fp)  fprintf(log_fp, "\n---- T=%d ---- CPU: (idle)\n", timer);
        if (live_fp) fprintf(live_fp, "[T=%2d] CPU IDLE | ready: %s\n", timer, queue_view);
    }
    if (live_fp) fflush(live_fp);

    /* ready queue + state table of every process */
    emit("  ready queue : %s\n", queue_view);
    emit("  %-4s %-7s %-11s %-8s %-6s %-7s %-6s %-7s\n",
         "ID", "PID", "STATE", "ARRIVAL", "BURST", "REMAIN", "START", "FINISH");

    for (int i = 0; i < njobs; i++) {
        Pcb *p = all[i];
        char pid[16], start[16] = "-", finish[16] = "-";

        pid_text(p, pid);
        if (p->start_time  >= 0) sprintf(start,  "%d", p->start_time);
        if (p->finish_time >= 0) sprintf(finish, "%d", p->finish_time);

        emit("  %-4s %-7s %-11s %-8d %-6d %-7d %-6s %-7s\n",
             p->name, pid, state_name(p->state),
             p->arrival, p->burst, p->remaining, start, finish);
    }
}
