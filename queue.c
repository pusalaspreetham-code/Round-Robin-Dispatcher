/* queue.c -- simple FIFO queue */
#include "queue.h"
#include <stdio.h>

void enqueue(Queue *q, Pcb *p)
{
    p->next = NULL;
    if (q->tail) q->tail->next = p;
    else         q->head = p;
    q->tail = p;
}

Pcb *dequeue(Queue *q)
{
    Pcb *p = q->head;
    if (!p) return NULL;

    q->head = p->next;
    if (!q->head) q->tail = NULL;
    p->next = NULL;
    return p;
}

/* Sorted insert, so a job file that is not in arrival order still works. */
void insert_by_arrival(Queue *q, Pcb *p)
{
    if (!q->head || q->head->arrival > p->arrival) {
        p->next = q->head;
        q->head = p;
        if (!q->tail) q->tail = p;
        return;
    }

    Pcb *cur = q->head;
    while (cur->next && cur->next->arrival <= p->arrival)
        cur = cur->next;

    p->next   = cur->next;
    cur->next = p;
    if (q->tail == cur) q->tail = p;
}

char *queue_describe(const Queue *q, char *buf, size_t len)
{
    if (!q->head) {
        snprintf(buf, len, "(empty)");
        return buf;
    }

    size_t used = 0;
    buf[0] = '\0';
    for (Pcb *p = q->head; p && used < len; p = p->next)
        used += snprintf(buf + used, len - used, "%s%s(%ds)",
                         p == q->head ? "" : " -> ", p->name, p->remaining);
    return buf;
}
