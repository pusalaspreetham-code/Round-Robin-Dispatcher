/* ui.c -- interactive menu. The scheduling itself is done by dispatcher_run(). */
#include "ui.h"
#include "monitor.h"
#include "report.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_FILE "log.txt"

/* ---------------- small input helpers ---------------- */

static void clear_screen(void) { printf("\033[2J\033[H"); }

static void print_title(void)
{
    printf("========================================\n");
    printf("       ROUND ROBIN CPU DISPATCHER\n");
    printf("========================================\n");
}

/* Read one line. At end of input (Ctrl+D) the program simply exits. */
static void read_line(char *buf, int n)
{
    if (!fgets(buf, n, stdin)) {
        printf("\n");
        exit(0);
    }
    buf[strcspn(buf, "\r\n")] = '\0';
}

/* Ask for a whole number between lo and hi; ask again until it is valid. */
static int read_int(const char *prompt, int lo, int hi)
{
    char line[64], *end;

    for (;;) {
        printf("%s", prompt);
        fflush(stdout);
        read_line(line, sizeof line);

        long v = strtol(line, &end, 10);
        if (end != line && *end == '\0' && v >= lo && v <= hi)
            return (int)v;
        printf("  Invalid: enter a whole number between %d and %d.\n", lo, hi);
    }
}

/* y/n question. */
static int confirm(const char *prompt)
{
    char line[64];

    for (;;) {
        printf("%s", prompt);
        fflush(stdout);
        read_line(line, sizeof line);
        if (line[0] == 'y' || line[0] == 'Y') return 1;
        if (line[0] == 'n' || line[0] == 'N') return 0;
        printf("  Please answer 'y' or 'n'.\n");
    }
}

static void press_enter(void)
{
    char line[16];
    printf("\nPress Enter to continue...");
    fflush(stdout);
    read_line(line, sizeof line);
    printf("\n");
}

/* ---------------- tables ---------------- */

static void print_job_table(Pcb *const *jobs, int n)
{
    printf("  %-12s | %-12s | %-12s\n", "Process ID", "Arrival Time", "Burst Time");
    printf("  ------------ | ------------ | ------------\n");
    for (int i = 0; i < n; i++)
        printf("  %-12s | %-12d | %-12d\n",
               jobs[i]->name, jobs[i]->arrival, jobs[i]->burst);
}

static const char *state_colour(State s)
{
    switch (s) {
    case RUNNING:    return "\033[92m";
    case READY:      return "\033[93m";
    case SUSPENDED:  return "\033[95m";
    case TERMINATED: return "\033[90m";
    default:         return "\033[96m";
    }
}

static void print_process_table(Pcb *const *jobs, int n)
{
    printf("  %-12s | %-12s | %-12s | %-16s | %-11s\n",
           "Process ID", "Arrival Time", "Burst Time", "Remaining Burst", "State");
    printf("  ------------ | ------------ | ------------ | "
           "---------------- | -----------\n");
    for (int i = 0; i < n; i++)
        printf("  %-12s | %-12d | %-12d | %-16d | %s%-11s\033[0m\n",
               jobs[i]->name, jobs[i]->arrival, jobs[i]->burst, jobs[i]->remaining,
               state_colour(jobs[i]->state), state_name(jobs[i]->state));
}

/* ---------------- menu options ---------------- */

/* Option 3 */
static void view_details(Dispatcher *d)
{
    clear_screen();
    print_title();
    printf("\n============ PROCESS DETAILS ============\n\n");

    if (d->njobs == 0) {
        printf("  No processes are loaded yet.\n");
        printf("  Use option 1 (manual) or option 2 (from file) first.\n");
    } else {
        print_process_table(d->all, d->njobs);
        printf("\n  Total : %d process(es), quantum = %d second(s)\n", d->njobs, d->quantum);
        printf("  Source: %s\n", d->jobs_file);
    }
    press_enter();
}

/* Option 4 */
static void start_scheduling(Dispatcher *d, int *has_run)
{
    clear_screen();
    print_title();

    if (d->njobs == 0) {
        printf("\nNo processes are loaded. Use option 1 or option 2 first.\n");
        press_enter();
        return;
    }
    if (*has_run) {
        printf("\nThis session has already been scheduled.\n");
        printf("Choose option 7 (Run New Scheduling) to start a fresh run.\n");
        press_enter();
        return;
    }

    int last_arrival = 0, total_burst = 0;
    for (int i = 0; i < d->njobs; i++) {
        if (d->all[i]->arrival > last_arrival) last_arrival = d->all[i]->arrival;
        total_burst += d->all[i]->burst;
    }

    printf("\nStarting Round Robin scheduling on %d process(es) with quantum %d second(s).\n",
           d->njobs, d->quantum);
    printf("This takes about %d second(s) of real time.\n\n", last_arrival + total_burst);
    printf("Tip: in a second terminal run  tail -F process.log  to watch the "
           "processes live.\n\n");

    monitor_init(LOG_FILE);
    monitor_banner(d->jobs_file, d->njobs, d->quantum);

    dispatcher_run(d);

    /* report on the screen and the same report in log.txt */
    report_gantt(stdout, d->gantt, d->timer);
    report_performance(stdout, d->all, d->njobs, d->timer,
                       d->busy_ticks, d->context_switches);

    FILE *log = monitor_log_stream();
    if (log) {
        report_gantt(log, d->gantt, d->timer);
        report_performance(log, d->all, d->njobs, d->timer,
                           d->busy_ticks, d->context_switches);
        fprintf(log, "\nEnd of simulation: %d process(es), %d context switch(es).\n",
                d->njobs, d->context_switches);
    }
    monitor_close();

    *has_run = 1;
    printf("\nScheduling finished. Use option 5 for the report, option 6 for the log.\n");
    press_enter();
}

/* Option 5 */
static void view_report(Dispatcher *d, int has_run)
{
    clear_screen();
    print_title();

    if (!has_run) {
        printf("\nPlease enter processes and start scheduling first.\n");
    } else {
        report_gantt(stdout, d->gantt, d->timer);
        report_performance(stdout, d->all, d->njobs, d->timer,
                           d->busy_ticks, d->context_switches);
    }
    press_enter();
}

/* Option 6 */
static void view_log(int has_run)
{
    clear_screen();
    print_title();

    FILE *fp = has_run ? fopen(LOG_FILE, "r") : NULL;
    if (!fp) {
        printf("\nPlease enter processes and start scheduling first.\n");
        press_enter();
        return;
    }

    printf("\n============ EXECUTION LOG (%s) ============\n\n", LOG_FILE);
    char line[512];
    while (fgets(line, sizeof line, fp))
        fputs(line, stdout);
    fclose(fp);
    press_enter();
}

/* Option 1: ask for the processes, show a confirmation table, allow edits. */
static void collect_processes(Dispatcher *d)
{
    Pcb *jobs[MAX_JOBS];
    int n, q;

    for (;;) {
        clear_screen();
        print_title();
        printf("\n------------ ENTER PROCESS DETAILS ------------\n\n");

        n = read_int("Enter the total number of processes (1..64): ", 1, MAX_JOBS);
        q = read_int("Enter the time quantum in seconds (1..3600): ", 1, 3600);

        for (int i = 0; i < n; i++) {
            printf("\nProcess P%d:\n", i + 1);
            int arrival = read_int("  Enter Arrival Time (0 or greater): ", 0, 1000000);
            int burst   = read_int("  Enter Burst Time (greater than 0): ", 1, 1000000);
            jobs[i] = process_create(i + 1, arrival, burst);
        }

        /* confirmation table */
        int again = 0;
        for (;;) {
            printf("\n================= CONFIRMATION TABLE =================\n");
            print_job_table(jobs, n);
            printf("======================================================\n");

            if (confirm("\nIs this correct? (y = start, n = edit): "))
                break;

            printf("\nEnter the number of the process to edit (1..%d),\n", n);
            printf("or 0 to enter all processes again.\n");
            int edit = read_int("Choice: ", 0, n);

            if (edit == 0) { again = 1; break; }

            printf("\nEditing %s:\n", jobs[edit - 1]->name);
            jobs[edit - 1]->arrival   = read_int("  New Arrival Time (0 or greater): ", 0, 1000000);
            jobs[edit - 1]->burst     = read_int("  New Burst Time (greater than 0): ", 1, 1000000);
            jobs[edit - 1]->remaining = jobs[edit - 1]->burst;
        }

        if (!again) break;
        for (int i = 0; i < n; i++) free(jobs[i]);      /* enter everything again */
    }

    dispatcher_destroy(d);
    dispatcher_prepare(d, q);
    strcpy(d->jobs_file, "manual entry");
    for (int i = 0; i < n; i++)
        dispatcher_add_job(d, jobs[i]);

    printf("\nSaved %d process(es). Quantum = %d second(s).\n", n, q);
    press_enter();
}

/* Option 2: load the processes from a file. */
static int load_from_file(Dispatcher *d)
{
    char path[256];

    clear_screen();
    print_title();
    printf("\n------------ LOAD PROCESSES FROM FILE ------------\n\n");
    printf("Enter the job file path (press Enter for the default 'jobs.txt').\n");
    printf("File: ");
    fflush(stdout);
    read_line(path, sizeof path);
    if (path[0] == '\0') strcpy(path, "jobs.txt");

    int q = read_int("Enter the time quantum in seconds (1..3600): ", 1, 3600);

    dispatcher_destroy(d);
    dispatcher_prepare(d, q);

    if (dispatcher_load_jobs(d, path) != 0) {
        printf("\nCould not load '%s'. See the message above.\n", path);
        dispatcher_destroy(d);
        press_enter();
        return -1;
    }

    printf("\nLoaded %d process(es) from '%s'. Quantum = %d second(s).\n",
           d->njobs, path, d->quantum);
    press_enter();
    return 0;
}

/* ---------------- main menu loop ---------------- */

void ui_run(Dispatcher *d)
{
    int has_run = 0;        /* 1 after a scheduling run has finished */

    monitor_init(LOG_FILE); /* create the logs now so "tail -F process.log" works at once */

    for (;;) {
        clear_screen();
        print_title();
        printf("\n1. Enter Processes Manually\n");
        printf("2. Load Processes From File\n");
        printf("3. View Process Details\n");
        printf("4. Start Scheduling\n");
        printf("5. View Performance Report\n");
        printf("6. View Execution Log\n");
        printf("7. Run New Scheduling\n");
        printf("8. Exit\n\n");

        if (d->njobs > 0)
            printf("[ loaded: %d process(es), quantum = %d s%s ]\n\n",
                   d->njobs, d->quantum, has_run ? ", scheduled" : "");
        else
            printf("[ no processes loaded yet ]\n\n");

        int choice = read_int("Enter your choice: ", 1, 8);

        if (choice == 1) {
            collect_processes(d);
            has_run = 0;
        } else if (choice == 2) {
            if (load_from_file(d) == 0) has_run = 0;
        } else if (choice == 3) {
            view_details(d);
        } else if (choice == 4) {
            start_scheduling(d, &has_run);
        } else if (choice == 5) {
            view_report(d, has_run);
        } else if (choice == 6) {
            view_log(has_run);
        } else if (choice == 7) {
            clear_screen();
            print_title();
            printf("\n============ RUN NEW SCHEDULING ============\n\n");
            printf("  This clears the current results and starts a fresh session.\n\n");
            printf("    1. Enter Processes Manually\n");
            printf("    2. Load Processes From File\n");
            printf("    0. Cancel (keep the current session)\n\n");

            int sub = read_int("Enter your choice: ", 0, 2);
            if (sub == 1) {
                collect_processes(d);
                has_run = 0;
            } else if (sub == 2) {
                dispatcher_destroy(d);
                has_run = 0;
                load_from_file(d);
            }
        } else {
            if (confirm("\nAre you sure you want to exit? (y/n): ")) {
                monitor_close();
                dispatcher_destroy(d);
                printf("\nSession closed. Goodbye.\n");
                return;
            }
        }
    }
}
