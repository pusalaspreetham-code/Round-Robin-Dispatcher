/*
 * main.c -- Round Robin Dispatcher
 *
 *   ./dispatcher               interactive menu
 *   ./dispatcher jobs.txt 1    batch mode: jobs file + time quantum
 */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "dispatcher.h"
#include "monitor.h"
#include "report.h"
#include "ui.h"

static Dispatcher disp;

/* Ctrl+C: kill every child so nothing is left running. */
static void on_sigint(int sig)
{
    (void)sig;
    dispatcher_kill_all_children(&disp);
    _exit(130);
}

/* Batch mode: run once, print the report, exit. */
static int run_batch(const char *jobs_file, int quantum)
{
    dispatcher_prepare(&disp, quantum);
    if (dispatcher_load_jobs(&disp, jobs_file) != 0)
        return 1;

    monitor_init("log.txt");
    monitor_banner(jobs_file, disp.njobs, disp.quantum);

    dispatcher_run(&disp);

    report_gantt(stdout, disp.gantt, disp.timer);
    report_performance(stdout, disp.all, disp.njobs, disp.timer,
                       disp.busy_ticks, disp.context_switches);

    FILE *log = monitor_log_stream();
    if (log) {
        report_gantt(log, disp.gantt, disp.timer);
        report_performance(log, disp.all, disp.njobs, disp.timer,
                           disp.busy_ticks, disp.context_switches);
        fprintf(log, "\nEnd of simulation: %d job(s), %d context switch(es).\n",
                disp.njobs, disp.context_switches);
    }

    printf("\nSimulation finished: %d job(s) completed in %d second(s) "
           "with %d context switch(es).\n",
           disp.njobs, disp.timer, disp.context_switches);
    if (log) printf("Full execution log written to log.txt\n");

    monitor_close();
    dispatcher_destroy(&disp);
    return 0;
}

int main(int argc, char *argv[])
{
    setvbuf(stdout, NULL, _IOLBF, 0);
    signal(SIGINT, on_sigint);

    if (argc == 1) {                    /* no arguments: interactive menu */
        ui_run(&disp);
        return 0;
    }

    const char *jobs_file = argv[1];
    int quantum = (argc > 2) ? atoi(argv[2]) : 1;   /* bad value -> 0 -> becomes 1 */

    return run_batch(jobs_file, quantum);
}
