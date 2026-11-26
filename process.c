/* process.c -- create and parse jobs */
#include "process.h"
#include <stdio.h>
#include <stdlib.h>

const char *state_name(State s)
{
    const char *names[] = { "NEW", "READY", "RUNNING", "SUSPENDED", "TERMINATED" };
    return names[s];
}

Pcb *process_create(int id, int arrival, int burst)
{
    if (arrival < 0 || burst < 1) return NULL;

    Pcb *p = calloc(1, sizeof(Pcb));
    p->id          = id;
    p->arrival     = arrival;
    p->burst       = burst;
    p->remaining   = burst;
    p->state       = NEW;
    p->start_time  = -1;
    p->finish_time = -1;
    sprintf(p->name, "P%d", id);
    return p;
}

Pcb *process_parse_line(const char *line, int id)
{
    int v[8];       /* arrival, priority, cpu, memory, 4 resources */

    if (sscanf(line, "%d , %d , %d , %d , %d , %d , %d , %d",
               &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6], &v[7]) != 8)
        return NULL;                    /* blank, comment or bad line */

    return process_create(id, v[0], v[2]);
}
