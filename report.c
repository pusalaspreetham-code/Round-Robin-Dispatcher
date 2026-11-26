/*
 * report.c -- Gantt chart and performance report
 *   turnaround = finish - arrival      waiting  = turnaround - burst
 *   response   = start  - arrival      CPU use  = busy / total * 100
 */
#include "report.h"

void report_gantt(FILE *out, const int *gantt, int total_ticks)
{
    fprintf(out, "\n===================== GANTT CHART =====================\n");

    if (total_ticks <= 0) {
        fprintf(out, "  (no CPU activity was recorded)\n");
        return;
    }

    /* 1. execution intervals */
    fprintf(out, "Execution intervals (1 column = 1 second):\n");

    int t = 0;
    while (t < total_ticks) {
        int id = gantt[t], start = t;
        while (t < total_ticks && gantt[t] == id)
            t++;

        if (id < 0) fprintf(out, "  %-6s %3d - %-3d  (%d second(s))\n", "IDLE", start, t, t - start);
        else        fprintf(out, "  P%-5d %3d - %-3d  (%d second(s))\n", id, start, t, t - start);
    }

    /* 2. CPU timeline, 20 seconds per row */
    fprintf(out, "\nCPU timeline (-- = idle):\n");

    for (int start = 0; start < total_ticks; start += 20) {
        int end = (start + 20 < total_ticks) ? start + 20 : total_ticks;

        fprintf(out, "  CPU |");
        for (int i = start; i < end; i++) {
            if (gantt[i] < 0) fprintf(out, " -- |");
            else              fprintf(out, " P%-2d|", gantt[i]);
        }
        fprintf(out, "\n  Time|");
        for (int i = start; i < end; i++)
            fprintf(out, "%-4d|", i);
        fprintf(out, "\n\n");
    }
}

void report_performance(FILE *out, Pcb *const *all, int njobs,
                        int total_ticks, int busy_ticks, int context_switches)
{
    fprintf(out, "================== PERFORMANCE REPORT ==================\n");
    fprintf(out, "%-5s %-8s %-6s %-7s %-11s %-8s %-9s\n",
            "Job", "Arrival", "Burst", "Finish", "Turnaround", "Waiting", "Response");

    double sum_turn = 0, sum_wait = 0, sum_resp = 0;

    for (int i = 0; i < njobs; i++) {
        Pcb *p = all[i];
        int turn = p->finish_time - p->arrival;
        int wait = turn - p->burst;
        int resp = p->start_time - p->arrival;

        fprintf(out, "%-5s %-8d %-6d %-7d %-11d %-8d %-9d\n",
                p->name, p->arrival, p->burst, p->finish_time, turn, wait, resp);

        sum_turn += turn;
        sum_wait += wait;
        sum_resp += resp;
    }

    fprintf(out, "--------------------------------------------------------\n");
    fprintf(out, "Average turnaround time : %.2f s\n", sum_turn / njobs);
    fprintf(out, "Average waiting time    : %.2f s\n", sum_wait / njobs);
    fprintf(out, "Average response time   : %.2f s\n", sum_resp / njobs);
    fprintf(out, "CPU utilisation         : %.1f %%  (%d busy / %d total second(s), %d idle)\n",
            100.0 * busy_ticks / total_ticks, busy_ticks, total_ticks, total_ticks - busy_ticks);
    fprintf(out, "Throughput              : %.3f job(s) per second (%d job(s) in %d second(s))\n",
            (double)njobs / total_ticks, njobs, total_ticks);
    fprintf(out, "Context switches        : %d\n", context_switches);
    fprintf(out, "========================================================\n");
}
