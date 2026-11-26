/* dispatcher.h -- the Round Robin scheduling engine */
#ifndef DISPATCHER_H
#define DISPATCHER_H

#include "process.h"
#include "queue.h"

#define MAX_GANTT 10000

typedef struct {
    int   quantum;                  /* time slice in seconds            */
    char  jobs_file[256];           /* where the jobs came from         */
    Queue input;                    /* jobs that have not arrived yet   */
    Queue ready;                    /* Round Robin ready queue          */
    Pcb  *all[MAX_JOBS];            /* every job (for reports)          */
    int   njobs;
    Pcb  *running;                  /* job on the CPU (NULL = idle)     */
    int   timer;                    /* dispatcher clock                 */
    int   quantum_left;
    int   finished;                 /* number of finished jobs          */
    int   busy_ticks;               /* seconds the CPU was used         */
    int   context_switches;
    int   gantt[MAX_GANTT];         /* gantt[t] = job id at second t, -1 = idle */
} Dispatcher;

void dispatcher_prepare(Dispatcher *d, int quantum);            /* empty simulation   */
int  dispatcher_add_job(Dispatcher *d, Pcb *p);                 /* 0 ok, -1 table full*/
int  dispatcher_load_jobs(Dispatcher *d, const char *file);     /* 0 ok, -1 error     */
void dispatcher_run(Dispatcher *d);                             /* run to the end     */
void dispatcher_destroy(Dispatcher *d);                         /* free all jobs      */
void dispatcher_kill_all_children(Dispatcher *d);               /* used by Ctrl+C     */

#endif
