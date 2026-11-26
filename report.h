/* report.h -- Gantt chart and performance report (written to any stream) */
#ifndef REPORT_H
#define REPORT_H

#include <stdio.h>
#include "process.h"

void report_gantt(FILE *out, const int *gantt, int total_ticks);
void report_performance(FILE *out, Pcb *const *all, int njobs,
                        int total_ticks, int busy_ticks, int context_switches);

#endif
