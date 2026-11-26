/* process.h -- Process Control Block (one PCB per job) */
#ifndef PROCESS_H
#define PROCESS_H

#include <sys/types.h>

#define MAX_JOBS 64

typedef enum { NEW, READY, RUNNING, SUSPENDED, TERMINATED } State;

typedef struct pcb {
    int   id;               /* P0, P1, ...                       */
    char  name[16];         /* "P0", "P1", ...                   */
    int   arrival;          /* arrival time                      */
    int   burst;            /* total CPU time needed             */
    int   remaining;        /* CPU time still needed             */
    pid_t pid;              /* real Linux PID (0 = no process)   */
    State state;
    int   start_time;       /* first time on the CPU (-1 = never)*/
    int   finish_time;      /* time it finished (-1 = never)     */
    struct pcb *next;       /* used by the queues                */
} Pcb;

const char *state_name(State s);

/* Make a PCB. Returns NULL if arrival < 0 or burst < 1. */
Pcb *process_create(int id, int arrival, int burst);

/* Read one line of the job file: "arrival, priority, cpu, memory, r1..r4".
 * Returns a PCB, or NULL for blank / comment / bad lines. */
Pcb *process_parse_line(const char *line, int id);

#endif
