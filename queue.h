/* queue.h -- simple FIFO queue of PCBs (used for input queue and ready queue) */
#ifndef QUEUE_H
#define QUEUE_H

#include <stddef.h>
#include "process.h"

typedef struct {
    Pcb *head;
    Pcb *tail;
} Queue;

void  enqueue(Queue *q, Pcb *p);                /* add at the back          */
Pcb  *dequeue(Queue *q);                        /* remove from the front    */
void  insert_by_arrival(Queue *q, Pcb *p);      /* keep sorted by arrival   */
char *queue_describe(const Queue *q, char *buf, size_t len);  /* "P1(6s) -> P2(4s)" */

#endif
