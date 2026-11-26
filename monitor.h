/* monitor.h -- live screen output + log.txt + process.log */
#ifndef MONITOR_H
#define MONITOR_H

#include <stdio.h>
#include "process.h"
#include "queue.h"

void  monitor_init(const char *log_file);   /* open log.txt and process.log  */
void  monitor_close(void);
FILE *monitor_log_stream(void);             /* log.txt (NULL if not open)    */
void  monitor_banner(const char *jobs_file, int njobs, int quantum);
void  monitor_event(int timer, const Pcb *p, const char *action, const char *detail);
void  monitor_tick(int timer, Pcb *const *all, int njobs,
                   const Queue *ready, const Pcb *running);

#endif
